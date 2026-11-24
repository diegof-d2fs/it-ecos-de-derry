#include "jogo/partida.h"
#include "jogo/clube.h"
#include "jogo/armadilhas.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "combate/atributos.h"
#include "mundo/colisao.h"

namespace {

constexpr int MAXIMO_HOSTIS = 12;              // PRD §9
constexpr int ORCAMENTO_PADRAO = 6;            // sala com ondas que não definiu "orcamento="
constexpr float ATRASO_PRIMEIRA_ONDA = 1.0f;   // respiro ao entrar
constexpr float ATRASO_ENTRE_ONDAS = 1.2f;
constexpr int XP_SALA_CONCLUIDA = 5;           // PRD §7.2
constexpr float HITSTOP_ACERTO = 0.04f;
constexpr float HITSTOP_DANO_BILL = 0.06f;
// O recuo decai com fator 0,0005 por segundo; para percorrer `d` px, a velocidade inicial é d × 7,6.
constexpr float FATOR_RECUO = 7.6f;
constexpr int ITENS_POR_BAU = 2;               // baú trancado (tesouro): 3
constexpr int BONUS_DROP_CAIXA = 15;           // a caixa solta item com chanceDrop + 15%
constexpr int ESCALADA_ETAPA_PCT = 10;         // +10% de orçamento por etapa do bioma (1ª = base)
constexpr int ESCALADA_ONDA_PCT = 15;          // +15% por onda dentro da sala (a última aperta)
constexpr int ESCALA_INIMIGO_POR_BIOMA_PCT = 15;   // DANO dos comuns por bioma depois do 1º
constexpr int ESCALA_VIDA_POR_BIOMA_PCT = 15;      // VIDA dos comuns por bioma depois do 1º (25 foi medido: não ajudou os Porões e matou mais nos biomas 2–3)
constexpr float RAIO_NEVOA_INALADOR = 56.0f;   // Eco Inalador (Eddie)
constexpr int ELITE_VIDA_EXTRA_PCT = 60, ELITE_DANO_EXTRA_PCT = 25;   // PRD §9 (XP ×2)
constexpr int MOEDAS_ABATE_MIN = 1, MOEDAS_ABATE_MAX = 3;   // por inimigo comum (com chanceMoedaAbate)

// ---------- quem aparece em cada bioma (peso de sorteio) ----------
struct EntradaSorteio {
    TipoInimigo tipo;
    int peso;
};
struct ListaBioma {
    EntradaSorteio itens[4];
    int quantidade;
};
// Os inimigos 4–8 entram com os biomas 2–6 (Etapas 10–14).
const ListaBioma INIMIGOS_POR_BIOMA[8] = {
    {{{TipoInimigo::Balao, 1}}, 1},                                                          // 0 prólogo
    {{{TipoInimigo::Balao, 3}, {TipoInimigo::Rato, 3}, {TipoInimigo::Sombra, 2}}, 3},          // 1 galeria
    {{{TipoInimigo::Rato, 3}, {TipoInimigo::Sanguessuga, 3}, {TipoInimigo::Afogado, 2}}, 3},   // 2 barrens (PRD §9)
    {{{TipoInimigo::Boneco, 3}, {TipoInimigo::Rato, 3}, {TipoInimigo::Sanguessuga, 2}}, 3},   // 3 neibolt (PRD §9)
    {{{TipoInimigo::Cabelo, 3}, {TipoInimigo::Sombra, 2}, {TipoInimigo::Rato, 3}, {TipoInimigo::Afogado, 2}}, 4},   // 4 porões (PRD §9 + ratos: a pressão rápida, 12.2)
    {{{TipoInimigo::Espectro, 3}, {TipoInimigo::Sanguessuga, 2}, {TipoInimigo::Rato, 2}, {TipoInimigo::Sombra, 2}}, 4},  // 5 siderúrgica (PRD §9, 13.2)
    // 6 cisterna (PRD §9, Etapa 14.1): o Espectro some e reaparece entre as colunas; o Boneco atira de longe
    // por cima da água funda; a Sombra quebra o entulho; o balão estoura no meio do grupo.
    {{{TipoInimigo::Espectro, 3}, {TipoInimigo::Boneco, 3}, {TipoInimigo::Sombra, 2}, {TipoInimigo::Balao, 2}}, 4},
    {{{TipoInimigo::Balao, 3}, {TipoInimigo::Sombra, 2}, {TipoInimigo::Rato, 1}}, 3},          // 7 covil
};

Direcao ladoOposto(Direcao d) {
    switch (d) {
        case Direcao::Cima: return Direcao::Baixo;
        case Direcao::Baixo: return Direcao::Cima;
        case Direcao::Esquerda: return Direcao::Direita;
        default: return Direcao::Esquerda;
    }
}

void texto(EstadoPartida& p, Vec2 pos, const std::string& t, TipoTexto tipo) {
    TextoFlutuante tf;
    tf.pos = pos;
    tf.texto = t;
    tf.tipo = tipo;
    p.textos.push_back(tf);
}

// Centro de uma célula caminhável perto de `pos` (para o item não cair dentro de parede).
// `n` escolhe entre as vizinhas: itens do mesmo baú se espalham em vez de empilhar.
Vec2 celulaLivrePerto(const EstadoPartida& p, Vec2 pos, int n) {
    const Celula base = celulaDe(pos);
    const int dc[] = {0, -1, 1, 0, -1, 1, 0, -1, 1}, dl[] = {1, 1, 1, 0, 0, 0, -1, -1, -1};
    int achadas = 0;
    Vec2 primeira = pos;
    bool temPrimeira = false;
    for (int k = 0; k < 9; ++k) {
        const int c = base.col + dc[k], l = base.lin + dl[k];
        if (tileEm(p.sala, c, l).bloqueiaMovimento) continue;
        const Vec2 centro = centroCelula({c, l});
        if (!temPrimeira) { primeira = centro; temPrimeira = true; }
        if (achadas++ == n % 9) return centro;
    }
    // Poucas vizinhas livres: a primeira delas (ou a própria posição, se nada estiver livre).
    return primeira;
}

Inimigo* buscarVivo(EstadoPartida& p, IdEntidade id) {
    for (Inimigo& i : p.inimigos) {
        if (i.id == id && i.vivo) return &i;
    }
    return nullptr;
}

// ---------- portas ----------
void definirPortas(EstadoPartida& p, bool abertas) {
    for (PortaSala& porta : p.sala.portas) {
        porta.aberta = abertas && !porta.selada;   // selada nunca abre
        Tile& t = p.sala.tiles[static_cast<std::size_t>(porta.celula.lin * p.sala.cols + porta.celula.col)];
        t.bloqueiaMovimento = !porta.aberta;
        t.bloqueiaVoo = !porta.aberta;
    }
    ++p.sala.versaoTerreno;   // o render refaz o cache (porta aberta × fechada)
}

// ---------- ondas ----------
// "Forte" = custa 4 ou mais pontos de onda (hoje, a Sombra). A dificuldade muda o peso deles.
constexpr int CUSTO_INIMIGO_FORTE = 4;

int pesoNaDificuldade(const EstadoPartida& p, const EntradaSorteio& e) {
    // Peso × 100 para a porcentagem não se perder no arredondamento (peso 2 × 50% = 1, não 0).
    const int base = e.peso * 100;
    return parametrosDe(e.tipo).custoOnda >= CUSTO_INIMIGO_FORTE ? base * p.ajustes.pesoFortes / 100 : base;
}

TipoInimigo sortearTipo(EstadoPartida& p, int bioma, int orcamento) {
    const ListaBioma& lista = INIMIGOS_POR_BIOMA[std::clamp(bioma, 0, 7)];
    int total = 0;
    for (int i = 0; i < lista.quantidade; ++i) {
        if (parametrosDe(lista.itens[i].tipo).custoOnda <= orcamento) total += pesoNaDificuldade(p, lista.itens[i]);
    }
    if (total == 0) return TipoInimigo::Total;
    int r = intervalo(p.rng, 1, total);
    for (int i = 0; i < lista.quantidade; ++i) {
        if (parametrosDe(lista.itens[i].tipo).custoOnda > orcamento) continue;
        r -= pesoNaDificuldade(p, lista.itens[i]);
        if (r <= 0) return lista.itens[i].tipo;
    }
    return lista.itens[0].tipo;
}

// Água rasa ou lama mais perto de `c` (Chebyshev ≤ raio, ordem fixa: determinístico); sem água, `c`.
Celula aguaPerto(const SalaViva& sala, Celula c, int raio) {
    for (int r = 0; r <= raio; ++r) {
        for (int dl = -r; dl <= r; ++dl) {
            for (int dc = -r; dc <= r; ++dc) {
                if (std::max(std::abs(dc), std::abs(dl)) != r) continue;
                const TipoTile t = tileEm(sala, c.col + dc, c.lin + dl).tipo;
                if (t == TipoTile::AguaRasa || t == TipoTile::Lama) return {c.col + dc, c.lin + dl};
            }
        }
    }
    return c;
}

void iniciarOnda(EstadoPartida& p) {
    const SalaModelo& m = *p.sala.modelo;
    ++p.ondaAtual;
    // Arena: a 1ª onda é o chefe-medo do bioma (PRD §10), surgindo devagar no '@'.
    const TipoInimigo chefe = m.tipo == TipoSala::Chefe ? chefeDoBioma(m.bioma) : TipoInimigo::Total;
    if (chefe != TipoInimigo::Total && p.ondaAtual == 1) {
        const Celula c = m.posChefe.col >= 0 ? m.posChefe : Celula{m.cols / 2, m.lins / 2};
        Inimigo ini = criarInimigo(chefe, p.proximoId++, centroCelula(c));
        ini.surgindo = 1.6f;
        // Dificuldade (Fácil: chefes 12% mais fracos): vida e dano. As fases viram nas mesmas porcentagens.
        ini.hp = ini.hpMax = aplicarPorcentagem(ini.hp, p.ajustes.forcaChefes);
        ini.forcaPct = p.ajustes.forcaChefes;
        if (chefe == TipoInimigo::PennywiseIV) {   // o Covil no Difícil, um pouco mais justo (Diego, 24/11)
            ini.hp = ini.hpMax = aplicarPorcentagem(ini.hp, p.ajustes.forcaCovil);
            ini.forcaPct = aplicarPorcentagem(ini.forcaPct, p.ajustes.forcaCovil);
        }
        // Modo Defesa (--pennywise): a luta começa na fase 2 — o medo surge e já se rasga (a mesma
        // transição de quando cai a 50%), com metade da vida. O Pennywise IV JÁ é o Pennywise: a opção não vale no
        // Covil (24/11: ela o fazia começar com meia vida e já na fase do Georgie).
        if (p.chefeNaFase2 && chefe != TipoInimigo::PennywiseIV) {
            ini.fase = 2;
            ini.hp = ini.hpMax / 2;
            ini.transicao = DURACAO_TRANSICAO_CHEFE;
            ini.estado = EstadoIA::Recuperando;
            ini.timer = 0.3f;
            if (chefe == TipoInimigo::RaloSangue) {   // o ralo fica sangrando, como na troca normal
                p.sala.tiles[static_cast<std::size_t>(c.lin * p.sala.cols + c.col)] = criarTile(TipoTile::PocaSangue);
                ++p.sala.versaoTerreno;
            }
        }
        p.inimigos.push_back(ini);
        p.sons.push_back(IdSom::MorteInimigo);
        texto(p, {centroCelula(c).x, centroCelula(c).y - 70.0f}, std::string(nomeDoChefe(ini)) + " desperta", TipoTexto::Critico);
        return;
    }
    int orcamento = orcamentoDaOnda(p.ajustes, m.orcamentoOnda);
    // Escalada (Etapa 8.5, como em Hades): cada etapa do bioma e cada onda da sala pesam mais.
    if (p.emTentativa && m.tipo == TipoSala::Combate) {
        const int pct = 100 + ESCALADA_ETAPA_PCT * std::max(0, p.etapa - 1) + ESCALADA_ONDA_PCT * (p.ondaAtual - 1);
        orcamento = aplicarPorcentagem(orcamento, pct);
    }
    // Spawns longe do Bill primeiro (≥ 4 células); se não houver, qualquer um.
    std::vector<Celula> longe, todos;
    for (const PontoSpawn& s : m.spawns) {
        todos.push_back(s.celula);
        const Celula b = celulaDe(p.bill.pes);
        if (std::max(std::abs(s.celula.col - b.col), std::abs(s.celula.lin - b.lin)) >= 4) longe.push_back(s.celula);
    }
    const std::vector<Celula>& pontos = longe.empty() ? todos : longe;
    if (pontos.empty()) return;
    while (orcamento > 0 && hostisVivos(p) < MAXIMO_HOSTIS) {
        const TipoInimigo tipo = sortearTipo(p, m.bioma, orcamento);
        if (tipo == TipoInimigo::Total) break;
        orcamento -= parametrosDe(tipo).custoOnda;
        Celula c = pontos[static_cast<std::size_t>(intervalo(p.rng, 0, static_cast<int>(pontos.size()) - 1))];
        if (tipo == TipoInimigo::Cabelo) {   // torreta fixa: nasce no spawn MAIS PERTO de Bill (onde a luta está)
            float menor = 1e9f;
            for (const Celula& s : pontos) {
                // Fixos nunca dividem o mesmo ralo (dois alternando a cruz não deixavam ninguém chegar).
                const bool ocupado = std::any_of(p.inimigos.begin(), p.inimigos.end(), [&](const Inimigo& o) {
                    return o.vivo && o.tipo == TipoInimigo::Cabelo && celulaDe(o.pes).col == s.col && celulaDe(o.pes).lin == s.lin;
                });
                const float d = comprimento(subtrair(centroCelula(s), p.bill.pes));
                if (!ocupado && d < menor) { menor = d; c = s; }
            }
        }
        if (tipo == TipoInimigo::Afogado) {   // PRD §9: surge da água (a mais perto do ponto, até 4 células)
            c = aguaPerto(p.sala, c, 4);
            p.pocas.push_back({centroCelula(c), VIDA_POCA});
        }
        // Pequeno deslocamento dentro da célula para não nascerem empilhados.
        const Vec2 pos = somar(centroCelula(c), Vec2{static_cast<float>(intervalo(p.rng, -6, 6)),
                                                      static_cast<float>(intervalo(p.rng, -4, 4))});
        Inimigo novo = criarInimigo(tipo, p.proximoId++, pos);
        // Escala por bioma (Etapa 11.5): Bill cresce de bioma em bioma (nível, Ecos, melhorias); os
        // inimigos comuns também, para a pressão não cair (a Neibolt tirava menos vida que o Barrens).
        novo.forcaPct = 100 + ESCALA_INIMIGO_POR_BIOMA_PCT * std::max(0, m.bioma - 1);
        // A vida escala mais que o dano: o dano de Bill quase triplica do bioma 1 ao 4 (Força no máximo,
        // melhorias); com a vida só +15%/bioma, ele matava antes de os inimigos atacarem (12.2).
        novo.hp = aplicarPorcentagem(novo.hp, 100 + ESCALA_VIDA_POR_BIOMA_PCT * std::max(0, m.bioma - 1));
        // Dificuldade (Fácil: 10% mais fracos e 5% mais lentos).
        novo.hp = aplicarPorcentagem(novo.hp, p.ajustes.forcaInimigos);
        novo.forcaPct = aplicarPorcentagem(novo.forcaPct, p.ajustes.forcaInimigos);
        novo.velocidadePct = p.ajustes.velocidadeInimigos;
        if (p.eliteNaSala && p.ondaAtual == std::min(2, m.ondas)) {   // a elite vem na onda do meio
            novo.elite = true;
            novo.hp = aplicarPorcentagem(novo.hp, 100 + ELITE_VIDA_EXTRA_PCT);
            p.eliteNaSala = false;
            texto(p, {pos.x, pos.y - 40.0f}, "Assombrado!", TipoTexto::Critico);
        }
        novo.hpMax = novo.hp;
        p.inimigos.push_back(novo);
    }
}

// O que a porta prometeu cai no chão perto da entrada da sala (lugar sempre caminhável).
void entregarRecompensa(EstadoPartida& p) {
    if (!p.emTentativa || p.salaPrometida.recompensa == Recompensa::Nenhuma) return;
    const DefinicaoRecompensa& d = definicaoRecompensa(p.salaPrometida.recompensa);
    if (p.salaPrometida.recompensa == Recompensa::Eco) {
        if (!abrirEscolhaDeEco(p)) texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Nenhum Eco novo", TipoTexto::Info);
        return;
    }
    if (p.salaPrometida.recompensa == Recompensa::PontoAtributo) {
        ++p.bill.progresso.pontosPendentes;
        texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "+1 ponto de atributo", TipoTexto::Info);
        return;
    }
    const Vec2 centro = centroCelula(p.sala.modelo->entradaJogador);
    soltarItem(p, d.item, intervalo(p.rng, d.qMin, d.qMax), p.bill.pes, celulaLivrePerto(p, centro, 4));
    // Bioma 1: a manivela do Elias estava junto da Chave do Medo (chega sempre antes da loja).
    const bool temManivela = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] > 0;
    if (p.salaPrometida.recompensa == Recompensa::ChaveDoMedo && p.plano.bioma == BIOMA_DA_MANIVELA && !temManivela &&
        !p.manivelaEntregue) {
        soltarItem(p, TipoItem::Manivela, 1, p.bill.pes, celulaLivrePerto(p, centro, 6));
    }
}

void abrirPendenciaDeNivel(EstadoPartida& p) {
    if (p.bill.progresso.pontosPendentes <= 0) return;
    p.pendencia = Pendencia::SubirNivel;
    p.cursor = 0;
}

void concluirSala(EstadoPartida& p) {
    p.salaConcluida = true;
    ++p.stats.C;
    definirPortas(p, true);
    texto(p, corpoJogador(p.bill), "Sala concluída", TipoTexto::Info);
    // Eco "Escoteiro" (Stan): combate sem levar dano rende moedas.
    if (p.sala.modelo->tipo == TipoSala::Combate && !p.danoNaSala) {
        if (const int moedas = valorEco(p, EfeitoEco::MoedasSalaSemDano); moedas > 0) {
            soltarItem(p, TipoItem::Moeda, moedas, p.bill.pes, celulaLivrePerto(p, p.bill.pes, 0));
            texto(p, {p.bill.pes.x, p.bill.pes.y - 88.0f}, "Escoteiro: sala sem dano", TipoTexto::Info);
        }
    }
    if (concederXp(p.bill.progresso, XP_SALA_CONCLUIDA) > 0) p.sons.push_back(IdSom::Nivel);
    entregarRecompensa(p);
    abrirPendenciaDeNivel(p);   // pontos para distribuir: a ficha abre ao fim do combate
}

// ---------- dano ----------
void ganharXp(EstadoPartida& p, int xp) {
    if (const int cronista = valorEco(p, EfeitoEco::Cronista); cronista > 0) xp = xp * (100 + cronista) / 100;   // Eco do Mike
    const int niveis = concederXp(p.bill.progresso, xp);
    if (niveis <= 0) return;
    p.bill.hp = std::min(p.bill.stats.hpMax, p.bill.hp + CURA_POR_NIVEL * niveis);
    p.sons.push_back(IdSom::Nivel);
    char t[32];
    std::snprintf(t, sizeof t, "Nível %d!", p.bill.progresso.nivel);
    texto(p, {p.bill.pes.x, p.bill.pes.y - 52.0f}, t, TipoTexto::Info);
}

// Inimigo encostado numa parede: a célula vizinha (4 direções) é parede e os pés estão a até
// 3/4 de célula da FACE dela. É o Eco "Planta baixa" (prensar contra a parede).
bool encostadoNaParede(const EstadoPartida& p, const Inimigo& ini) {
    const Celula c = celulaDe(ini.pes);
    const float x0 = static_cast<float>(c.col * TILE), y0 = static_cast<float>(c.lin * TILE);
    const float distancia[4] = {x0 + TILE - ini.pes.x, ini.pes.x - x0, y0 + TILE - ini.pes.y, ini.pes.y - y0};
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    for (int k = 0; k < 4; ++k) {
        if (tileEm(p.sala, c.col + dc[k], c.lin + dl[k]).tipo == TipoTile::Parede && distancia[k] <= 0.75f * TILE) return true;
    }
    return false;
}

// Eco "Ricochete" (Beverly): o clarão que acertou `de` salta para os N inimigos mais perto dele (até
// 4 células, à vista) que ele ainda não atingiu. Cada salto é um golpe novo no fim da resolução.
constexpr float ALCANCE_RICOCHETE = 4.0f * TILE;
void ricochetear(EstadoPartida& p, const Golpe& g, const Inimigo& de) {
    const int saltos = valorEco(p, EfeitoEco::RicocheteClarao);
    if (saltos <= 0) return;
    const Retangulo h = hurtboxInimigo(de);
    const Vec2 origem{h.x + h.w / 2.0f, h.y + h.h / 2.0f};
    std::vector<std::pair<float, const Inimigo*>> perto;
    for (const Inimigo& o : p.inimigos) {
        if (o.id == de.id || !podeSerAtingido(o)) continue;
        if (std::find(g.atingidos.begin(), g.atingidos.end(), o.id) != g.atingidos.end()) continue;
        const float d = comprimento(subtrair(o.pes, de.pes));
        if (d <= ALCANCE_RICOCHETE && segmentoLivre(p.sala, de.pes, o.pes, true)) perto.push_back({d, &o});
    }
    std::sort(perto.begin(), perto.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (int k = 0; k < saltos && k < static_cast<int>(perto.size()); ++k) {
        const Inimigo& alvo = *perto[static_cast<std::size_t>(k)].second;
        const Retangulo ha = hurtboxInimigo(alvo);
        const Vec2 centro{ha.x + ha.w / 2.0f, ha.y + ha.h / 2.0f};
        Golpe s = g;
        s.id = p.proximoGolpe++;
        s.area = areaCirculo(centro, 10.0f);
        s.ricochete = true;
        s.derrubaParede = false;
        s.atingidos = g.atingidos;   // quem o clarão já pegou não leva de novo
        s.atingidos.push_back(de.id);
        p.golpesNovos.push_back(s);
        p.tracos.push_back({origem, centro, 0.18f, false});
    }
}

// Eco "Estilingue" (Beverly): junto do especial sai um tiro de prata na mira, até 7 células; para no
// 1º inimigo (ou na parede). Rápido demais para voar quadro a quadro: o traço mostra o caminho.
constexpr float ALCANCE_ESTILINGUE = 7.0f * TILE;
void dispararEstilingue(EstadoPartida& p, int dano) {
    const Vec2 de = corpoJogador(p.bill);
    const Vec2 dir = normalizar(p.bill.mira);
    Vec2 fim = de;
    const Inimigo* alvo = nullptr;
    for (float d = 0.0f; d <= ALCANCE_ESTILINGUE && !alvo; d += 4.0f) {
        const Vec2 ponto = somar(de, escalar(dir, d));
        const Celula c = celulaDe(ponto);
        if (tileEm(p.sala, c.col, c.lin).bloqueiaVoo) break;
        fim = ponto;
        for (const Inimigo& o : p.inimigos) {
            const Retangulo h = hurtboxInimigo(o);
            if (podeSerAtingido(o) && ponto.x >= h.x && ponto.x <= h.x + h.w && ponto.y >= h.y && ponto.y <= h.y + h.h) {
                alvo = &o;
                break;
            }
        }
    }
    p.tracos.push_back({de, fim, 0.15f, true});
    if (!alvo) return;
    Golpe g = golpeDoBill(p.bill, p.proximoGolpe, areaCirculo(fim, 8.0f), dano);
    g.recuo = 30.0f;
    p.golpes.push_back(g);
}

// Covil (Etapa 15): o ponto de spawn mais longe de Bill (onde a ilusão de Georgie aparece).
Vec2 spawnMaisLonge(const EstadoPartida& p) {
    Vec2 melhor = centroCelula({p.sala.cols / 2, 3});
    float maior = -1.0f;
    for (const PontoSpawn& s : p.sala.modelo->spawns) {
        const float d = comprimento(subtrair(centroCelula(s.celula), p.bill.pes));
        if (d > maior) { maior = d; melhor = centroCelula(s.celula); }
    }
    return melhor;
}

// O Pennywise IV troca de fase (PRD §4.5): 2 = Georgie (a sala escurece, a ilusão chama Bill); 3 = Luzes Mortas.
void entrarFaseDoCovil(EstadoPartida& p, Inimigo& ini, int fase) {
    ini.fase = fase;
    ini.transicao = DURACAO_TRANSICAO_CHEFE;
    ini.estado = EstadoIA::Recuperando;
    ini.timer = 0.3f;
    p.avisos.erase(std::remove_if(p.avisos.begin(), p.avisos.end(), [&](const Aviso& a) { return a.golpe.origem == ini.id; }),
                   p.avisos.end());
    p.sons.push_back(IdSom::MorteInimigo);
    if (fase == 2) {
        p.georgie = GeorgieIlusao{};
        p.georgie.ativo = true;
        p.georgie.pes = spawnMaisLonge(p);
        p.georgie.chamado = 5.0f;
        texto(p, {p.georgie.pes.x, p.georgie.pes.y - 60.0f}, "Billy, me ajuda a pegar o barquinho.", TipoTexto::Critico);
        texto(p, {ini.pes.x, ini.pes.y - 90.0f}, "A sala escurece…", TipoTexto::Critico);
    } else {
        p.georgie.ativo = false;
        p.luzesMortas = true;
        // Uma por amigo resgatado; na arena avulsa (--sala b7_covil, testes), sem tentativa, as seis.
        p.quantasLuzes = p.emTentativa ? std::max(1, static_cast<int>(p.resgatados.size())) : 6;
        p.anguloLuzes = 0.0f;
        p.tickEscuridao = INTERVALO_ESCURIDAO;
        texto(p, {p.bill.pes.x, p.bill.pes.y - 64.0f}, "Você não é ele. Você nunca foi ele.", TipoTexto::Info);
        texto(p, {ini.pes.x, ini.pes.y - 90.0f}, "As Luzes Mortas!", TipoTexto::Critico);
    }
}

void golpeEmInimigo(EstadoPartida& p, Golpe& g, Inimigo& ini) {
    const ParametrosInimigo& par = parametrosDe(ini.tipo);
    g.atingidos.push_back(ini.id);
    const Vec2 acima{ini.pes.x, ini.pes.y - par.alturaCorpo - 4.0f};
    // Eco "Mira firme" (Beverly): pontos a mais na chance de acerto dos golpes de Bill.
    const float mira = g.origem == ID_JOGADOR ? static_cast<float>(valorEco(p, EfeitoEco::MiraFirme)) : 0.0f;
    if (g.rolaAcerto && !chance(p.rng, std::min(100.0f, chanceAcerto(g.focoAtacante, par.agilidade) + mira) / 100.0f)) {
        texto(p, acima, "Errou", TipoTexto::Errou);
        return;
    }
    if (!g.terreno && !g.queima && chance(p.rng, chanceEsquiva(par.agilidade) / 100.0f)) {
        texto(p, acima, "Esquivou", TipoTexto::Esquivou);
        return;
    }
    const bool deBill = g.origem == ID_JOGADOR;
    const int marreta = deBill && g.passoCombo == 3 ? valorEco(p, EfeitoEco::Marreta) : 0;   // Eco do Mike
    // Eco "Ordem" (Stan): crítico a mais em alvo atordoado.
    const float bonusOrdem = deBill && ini.atordoado > 0.0f ? static_cast<float>(valorEco(p, EfeitoEco::CriticoAtordoado)) : 0.0f;
    const bool critico = g.podeCritico && chance(p.rng, (g.critico + bonusOrdem) / 100.0f);
    int bruto = critico ? danoCritico(g.dano) : g.dano;
    if (deBill && ++ini.golpesDeBill % 3 == 0) bruto += valorEco(p, EfeitoEco::MarcaTerceiroGolpe);   // Livro de pássaros
    const int planta = deBill ? valorEco(p, EfeitoEco::DanoContraParede) : 0;
    if (planta > 0 && encostadoNaParede(p, ini)) bruto = bruto * planta / 100;   // Eco "Planta baixa"
    if (deBill && par.chefe) bruto = bruto * (100 + valorEco(p, EfeitoEco::Prata)) / 100;   // Eco "Prata"
    if (marreta > 0) bruto = bruto * (100 + marreta) / 100;   // Eco "Marreta": o 3º golpe do combo
    const int dano = g.queima ? bruto : danoEfetivo(bruto, par.armadura);   // a queimadura atravessa a armadura
    ini.hp -= dano;
    // O amigo ajuda mas não vence sozinho (Diego, 24/11): o golpe dele não derruba o último inimigo nem o chefe.
    if (g.doAmigo && ini.hp <= 0 && !amigoPodeDerrubar(p, ini)) ini.hp = 1;
    ini.flash = 0.1f;
    if (g.atordoa > 0.0f) ini.atordoado = g.doAmigo ? std::max(ini.atordoado, par.chefe ? g.atordoa * 0.5f : g.atordoa) : g.atordoa;
    if (g.doAmigo && g.lentidao > 0.0f) ini.lento = std::max(ini.lento, g.lentidao);
    if (deBill && g.especial) {
        // Eco "Voz do locutor" (Richie): o especial atordoa (o chefe, pela metade).
        if (const int ms = valorEco(p, EfeitoEco::VozLocutor); ms > 0)
            ini.atordoado = std::max(ini.atordoado, static_cast<float>(ms) / 1000.0f * (par.chefe ? 0.5f : 1.0f));
        // Eco lendário "Imitação" (Richie): 1× por sala, o 1º inimigo comum que o especial acerta (e que
        // sobrevive) passa a lutar do lado de Bill por alguns segundos.
        if (const int s = valorEco(p, EfeitoEco::Imitacao); s > 0 && !p.imitacaoUsada && !par.chefe && ini.hp > 0) {
            ini.aliado = static_cast<float>(s);
            p.imitacaoUsada = true;
            texto(p, {ini.pes.x, ini.pes.y - par.alturaCorpo - 18.0f}, "Imitação: agora luta do seu lado!", TipoTexto::Info);
        }
    }
    // Eco "Fogo controlado" (Mike): o clarão de Bill deixa o alvo queimando por 3 s.
    if (deBill && g.atordoa > 0.0f) {
        if (const int fogo = valorEco(p, EfeitoEco::FogoControlado); fogo > 0) {
            ini.queimando = DURACAO_QUEIMA;
            ini.queimaPorSegundo = fogo;
            ini.tickQueima = 1.0f;
        }
    }
    // Eco lendário "Tiro certeiro" (Beverly): crítico atordoa (o chefe, pela metade).
    if (deBill && critico) {
        if (const int ms = valorEco(p, EfeitoEco::TiroCerteiro); ms > 0)
            ini.atordoado = std::max(ini.atordoado, static_cast<float>(ms) / 1000.0f * (par.chefe ? 0.5f : 1.0f));
    }
    if (deBill && g.atordoa > 0.0f && !g.ricochete) ricochetear(p, g, ini);
    const Vec2 dir = normalizar(subtrair(ini.pes, g.area.centro));
    const float recuo = g.recuo * (marreta > 0 ? 1.6f : 1.0f);   // a Marreta empurra mais
    if (!inimigoFixo(ini)) ini.empurrao = escalar(dir, recuo * FATOR_RECUO);   // o Cabelo e o Ralo são fixos
    texto(p, acima, std::to_string(dano) + (critico ? "!" : ""), critico ? TipoTexto::Critico : TipoTexto::DanoInimigo);
    if (g.equipe == Equipe::Jogador && !g.queima) {
        // No chefe, um congelamento bem mais curto: ~45 golpes por luta, cada um parando o jogo 40 ms,
        // pareciam "lagadinhas" (Diego, 17/11). O acerto se lê pelo impacto e pelo tremor do corpo.
        p.hitStop = std::max(p.hitStop, par.chefe ? HITSTOP_ACERTO * 0.35f : HITSTOP_ACERTO);
        p.sons.push_back(IdSom::AcertoInimigo);
        const Retangulo h = hurtboxInimigo(ini);
        EfeitoAtaque e;   // o impacto no corpo de quem apanhou (fx_impacto)
        e.area = areaCirculo({h.x + h.w / 2.0f, h.y + h.h * 0.45f}, 4.0f);
        e.vida = DURACAO_EFEITO_ATAQUE * 0.7f;
        p.efeitosAtaque.push_back(e);
    }
    // Chefe: a ≤ 50% o medo se rasga e o Pennywise sai (fase 2). Os avisos em curso somem.
    // Pennywise IV (Etapa 15): três fases pelas marcas de 60% e 25% da vida (PRD §4.5).
    if (ini.tipo == TipoInimigo::PennywiseIV && ini.hp > 0) {
        const int pct = ini.hp * 100 / std::max(1, ini.hpMax);
        if ((ini.fase == 1 && pct <= 60) || (ini.fase == 2 && pct <= 25)) entrarFaseDoCovil(p, ini, ini.fase + 1);
    }
    if (par.chefe && ini.tipo != TipoInimigo::PennywiseIV && ini.fase == 1 && ini.hp > 0 && ini.hp * 2 <= ini.hpMax) {
        ini.fase = 2;
        ini.transicao = DURACAO_TRANSICAO_CHEFE;
        ini.estado = EstadoIA::Recuperando;
        ini.timer = 0.3f;
        p.avisos.erase(std::remove_if(p.avisos.begin(), p.avisos.end(), [&](const Aviso& a) { return a.golpe.origem == ini.id; }),
                       p.avisos.end());
        texto(p, {ini.pes.x, ini.pes.y - 80.0f}, "O medo se rasga…", TipoTexto::Critico);
        if (ini.tipo == TipoInimigo::RaloSangue) {   // o Pennywise sai e o ralo fica sangrando (poça permanente)
            const Celula c = celulaDe(ini.origem);
            p.sala.tiles[static_cast<std::size_t>(c.lin * p.sala.cols + c.col)] = criarTile(TipoTile::PocaSangue);
            ++p.sala.versaoTerreno;
        }
        p.sons.push_back(IdSom::MorteInimigo);
    }
    if (ini.hp <= 0 && par.chefe) {   // "Manifestação dispersa" (PRD §10.1): não é vitória, é a descida
        ini.vivo = false;
        ++p.stats.B;
        ganharXp(p, par.xp);
        p.dispersoes.push_back({ini.pes, DURACAO_DISPERSAO, ini.tipo});
        if (ini.tipo == TipoInimigo::PennywiseIV) {   // o Covil: agora sim, a VITÓRIA (PRD §4.5)
            p.stats.V = 1;
            p.timerVitoria = DURACAO_VITORIA;
            p.georgie.ativo = false;
            p.luzesMortas = false;
            texto(p, {ini.pes.x, ini.pes.y - 96.0f}, "Vocês vão esquecer.", TipoTexto::Critico);
            texto(p, {p.bill.pes.x, p.bill.pes.y - 64.0f}, "Hoje, ninguém ficou para trás.", TipoTexto::Info);
        } else {
            texto(p, {ini.pes.x, ini.pes.y - 70.0f}, "Manifestação dispersa", TipoTexto::Critico);
        }
        p.sons.push_back(IdSom::MorteInimigo);
        p.avisos.clear();
        for (Inimigo& outro : p.inimigos) {   // o que ele chamou se desfaz junto (sem XP)
            if (outro.id != ini.id && outro.vivo) outro.vivo = false;
        }
        return;
    }
    if (ini.hp <= 0) {
        ini.vivo = false;   // a morte acontece UMA vez: o golpe nunca atinge um morto
        ++p.stats.K;
        p.sons.push_back(IdSom::MorteInimigo);
        ganharXp(p, ini.elite ? par.xp * 2 : par.xp);
        if (ini.itemCarregado >= 0) {   // o ladrão caiu: o item volta ao chão onde ele estava
            soltarItem(p, static_cast<TipoItem>(ini.itemCarregado), ini.qtdCarregada, ini.pes, celulaLivrePerto(p, ini.pes, 0));
            texto(p, {ini.pes.x, ini.pes.y - 40.0f}, "Recuperado!", TipoTexto::Info);
            ini.itemCarregado = -1;
        }
        // Drop: a chance vem da dificuldade (SDD §24.2); o item pula para perto de onde caiu.
        if (sortearPorcentagem(p.rng, p.ajustes.chanceDrop)) {
            const ItemSorteado s = sortearDrop(p.rng, p.ajustes.pesoKit);
            soltarItem(p, s.tipo, s.quantidade, ini.pes, celulaLivrePerto(p, ini.pes, 0));
        }
        // Moedas: rolagem À PARTE do drop (a Loja do Elias precisa de uma economia; Etapa 8.5).
        if (sortearPorcentagem(p.rng, p.ajustes.chanceMoedaAbate)) {
            soltarItem(p, TipoItem::Moeda, intervalo(p.rng, MOEDAS_ABATE_MIN, MOEDAS_ABATE_MAX), ini.pes, celulaLivrePerto(p, ini.pes, 0));
        }
    }
}

void golpeNoBill(EstadoPartida& p, Golpe& g) {
    if (g.chama && valorEco(p, EfeitoEco::Resistencia) > 0) return;   // Eco "Resistência" (Mike): o fogo do chão não queima
    Jogador& b = p.bill;
    if (estaInvulneravel(b) || !estaVivo(b)) return;
    g.atingidos.push_back(ID_JOGADOR);
    const bool atrasDaBarreira = std::any_of(p.barreiras.begin(), p.barreiras.end(), [&](const BarreiraTijolos& t) {
        return comprimento(subtrair(t.centro, b.pes)) <= RAIO_BARREIRA_DASH;
    });
    if (!g.terreno && (barreiraAtiva(p) || atrasDaBarreira)) {   // Ben (companheiro) ou Eco "Ponte firme": os tijolos seguram
        texto(p, {b.pes.x, b.pes.y - 52.0f}, "Bloqueado", TipoTexto::Esquivou);
        return;
    }
    const Vec2 acima{b.pes.x, b.pes.y - 52.0f};
    // Eco "Piada ruim" (Richie): os inimigos erram mais (o golpe de terreno não rola acerto).
    const float piada = static_cast<float>(valorEco(p, EfeitoEco::PiadaRuim));
    if (g.rolaAcerto && !chance(p.rng, std::max(0.0f, chanceAcerto(g.focoAtacante, b.attr.agilidade) - piada) / 100.0f)) {
        texto(p, acima, "Errou", TipoTexto::Errou);
        return;
    }
    if (!g.terreno && chance(p.rng, b.stats.esquiva / 100.0f)) {   // armadilha de terreno não se esquiva (PRD §8.1)
        texto(p, acima, "Esquivou!", TipoTexto::Esquivou);
        b.invulneravel = 0.3f;
        return;
    }
    // Dificuldade: crítico dos inimigos e multiplicador do dano recebido (mínimo 1).
    const bool critico = !g.terreno && sortearPorcentagem(p.rng, p.ajustes.criticoInimigo);
    const int bruto = danoEfetivo(critico ? danoCritico(g.dano) : g.dano, b.stats.armadura);
    // Assombrado (elite): +25% de dano.
    const Inimigo* agressor = buscarInimigo(p, g.origem);
    int brutoElite = agressor && agressor->elite ? aplicarPorcentagem(bruto, 100 + ELITE_DANO_EXTRA_PCT) : bruto;
    if (agressor) brutoElite = aplicarPorcentagem(brutoElite, agressor->forcaPct);   // escala do bioma
    int dano = std::max(1, aplicarPorcentagem(brutoElite, p.ajustes.danoRecebido));
    if (!p.gessoUsado) {   // Eco "Gesso" (Eddie): 1× por sala, este golpe perde parte do dano
        if (const int gesso = valorEco(p, EfeitoEco::GessoUmGolpe); gesso > 0) {
            dano = std::max(1, dano * (100 - gesso) / 100);
            p.gessoUsado = true;
            texto(p, {b.pes.x, b.pes.y - 88.0f}, "Gesso!", TipoTexto::Info);
        }
    }
    b.hp -= dano;
    p.danoNaSala = true;
    // Causa da morte (telemetria e falas do Clube): armadilha e fogo do chão não são de ninguém. Sem isto, uma
    // morte num jato da Cisterna saía como "morreu para o Pássaro", o chefe da sala anterior (simulação, 14.4).
    if (const Inimigo* quem = buscarInimigo(p, g.origem)) p.ultimoAgressor = static_cast<int>(quem->tipo);
    else if (g.terreno || g.chama) p.ultimoAgressor = -1;
    b.invulneravel = INVULNERAVEL_APOS_DANO;
    b.flash = 1.0f / 60.0f;   // 1 quadro (sem piscar contínuo: PRD §8.2)
    const Vec2 dir = normalizar(subtrair(b.pes, g.area.centro));
    b.pes = moverComColisao(p.sala, b.pes, CAIXA_BILL, escalar(dir, g.recuo * 0.4f), false);
    texto(p, acima, std::to_string(dano) + (critico ? "!" : ""), TipoTexto::DanoBill);
    if (g.prende > 0.0f) {   // a faixa da Múmia: preso, sem andar nem dar dash (ainda ataca)
        b.preso = g.prende;
        texto(p, {b.pes.x, b.pes.y - 70.0f}, "Preso!", TipoTexto::DanoBill);
    }
    if (g.lentidao > 0.0f) {   // o agarrão do Afogado: Bill a 50% (o dash continua valendo)
        b.lento = std::max(b.lento, g.lentidao);
        texto(p, {b.pes.x, b.pes.y - 70.0f}, "Agarrado!", TipoTexto::DanoBill);
    }
    if (g.drena) {   // a Sanguessuga se cura com o que tirou
        for (Inimigo& quem : p.inimigos) {
            if (quem.id != g.origem || !quem.vivo) continue;
            const int antes = quem.hp;
            quem.hp = std::min(parametrosDe(quem.tipo).hp, quem.hp + dano);
            if (quem.hp > antes) texto(p, {quem.pes.x, quem.pes.y - 30.0f}, "+" + std::to_string(quem.hp - antes), TipoTexto::Info);
        }
    }
    p.hitStop = std::max(p.hitStop, HITSTOP_DANO_BILL);
    p.sons.push_back(IdSom::BillFerido);
}

// Bill sem vida: o Pacto de Sangue o levanta com 50% (e afasta quem está perto); sem pacto, fim.
void verificarMorteDoBill(EstadoPartida& p) {
    Jogador& b = p.bill;
    if (b.hp > 0) return;
    if (p.festaAtiva) {   // fase bônus: ninguém morre (SDD §24.5)
        b.hp = 1;
        return;
    }
    if (!p.poemaUsado && valorEco(p, EfeitoEco::Poema) > 0) {   // Eco lendário do Ben: 1× por bioma
        p.poemaUsado = true;
        b.hp = 1;
        b.invulneravel = 1.5f;
        texto(p, {b.pes.x, b.pes.y - 60.0f}, "Poema: ainda não!", TipoTexto::Info);
        return;
    }
    ++p.stats.U;
    if (b.pactos > 0) {
        --b.pactos;
        b.pactoUsado = true;
        b.hp = b.stats.hpMax / 2;
        b.invulneravel = 1.5f;
        for (Inimigo& i : p.inimigos) {
            const Vec2 d = subtrair(i.pes, b.pes);
            if (comprimento(d) < 96.0f) i.empurrao = escalar(normalizar(d), 80.0f * FATOR_RECUO);
        }
        texto(p, {b.pes.x, b.pes.y - 60.0f}, "Pacto de Sangue!", TipoTexto::Info);
        return;
    }
    b.hp = 0;
    p.terminou = true;
    p.vitoria = false;
    p.sons.push_back(IdSom::BillCaiu);
}

// Lascas: direções e forças vêm de um hash do golpe e do índice (não do Rng da partida: um efeito
// visual não pode mudar os sorteios do jogo, nem o piloto determinístico).
void soltarLascas(EstadoPartida& p, Vec2 de, int quantas, float forca, std::uint32_t semente) {
    for (int k = 0; k < quantas; ++k) {
        std::uint32_t h = semente * 2654435761u + static_cast<std::uint32_t>(k) * 40503u;
        h ^= h >> 13;
        h *= 0x5bd1e995u;
        h ^= h >> 15;
        const float a = static_cast<float>(h % 628u) / 100.0f;
        const float v = forca * (0.5f + static_cast<float>((h >> 10) % 100u) / 200.0f);
        Estilhaco e;
        e.pos = {de.x, de.y};
        e.vel = {std::cos(a) * v, std::sin(a) * v * 0.6f};
        e.altura = 14.0f;
        e.vz = 60.0f + static_cast<float>((h >> 4) % 60u);
        e.vida = 0.5f + static_cast<float>((h >> 7) % 30u) / 100.0f;
        e.tom = static_cast<int>((h >> 3) % 3u);
        p.estilhacos.push_back(e);
    }
}

// Eco "Fogo controlado": quem está queimando leva o dano 1× por segundo (um golpe de queimadura, que
// passa pela mesma resolução: morte, XP, fase do chefe). Só no próprio alvo (área pequena no corpo).
void atualizarQueimaduras(EstadoPartida& p, float dt) {
    for (Inimigo& i : p.inimigos) {
        if (!i.vivo || i.queimando <= 0.0f) continue;
        i.queimando -= dt;
        i.tickQueima -= dt;
        if (i.tickQueima > 0.0f) continue;
        i.tickQueima += 1.0f;
        Golpe g;
        g.id = p.proximoGolpe++;
        g.equipe = Equipe::Jogador;
        g.origem = ID_NULO;   // não é golpe "de Bill": não conta para Livro de pássaros, Marreta etc.
        const Retangulo h = hurtboxInimigo(i);
        g.area = areaCirculo({h.x + h.w / 2.0f, h.y + h.h / 2.0f}, 3.0f);
        g.dano = i.queimaPorSegundo;
        g.rolaAcerto = false;
        g.queima = true;
        g.duracao = 0.02f;
        p.golpes.push_back(g);
    }
}

void atualizarEstilhacos(EstadoPartida& p, float dt) {
    for (Estilhaco& e : p.estilhacos) {
        e.vida -= dt;
        e.pos = somar(e.pos, escalar(e.vel, dt));
        e.vz -= 420.0f * dt;
        e.altura += e.vz * dt;
        if (e.altura < 0.0f) {   // bate no chão: quica baixo e perde velocidade
            e.altura = 0.0f;
            e.vz = -e.vz * 0.35f;
            e.vel = escalar(e.vel, 0.5f);
        }
    }
    p.estilhacos.erase(std::remove_if(p.estilhacos.begin(), p.estilhacos.end(), [](const Estilhaco& e) { return e.vida <= 0.0f; }),
                       p.estilhacos.end());
    for (ObjetoSala& o : p.sala.objetos) {
        if (o.tremor > 0.0f) o.tremor -= dt;
    }
}

// Golpe do Bill numa caixa (17/11): cada golpe tira 1 de vida (o Especial e o Clarão, que derrubam
// parede, quebram de uma vez), a caixa treme e solta lascas; com 0 ela estoura, o caminho abre e às
// vezes solta um item. Cada golpe atinge cada caixa uma vez (a área dura vários quadros).
void golpeEmCaixas(EstadoPartida& p, Golpe& g) {
    for (std::size_t i = 0; i < p.sala.objetos.size(); ++i) {
        ObjetoSala& o = p.sala.objetos[i];
        if (o.codigo != 'c' || o.usado) continue;
        const Retangulo r{static_cast<float>(o.celula.col * TILE), static_cast<float>(o.celula.lin * TILE),
                          static_cast<float>(TILE), static_cast<float>(TILE)};
        if (!areaToca(g.area, r)) continue;
        const IdEntidade idCaixa = 0xC0000000u + static_cast<IdEntidade>(i);
        if (std::find(g.atingidos.begin(), g.atingidos.end(), idCaixa) != g.atingidos.end()) continue;
        g.atingidos.push_back(idCaixa);
        const Vec2 centro = centroCelula(o.celula);
        o.vida -= g.derrubaParede ? VIDA_CAIXA : 1;
        o.tremor = TREMOR_CAIXA;
        p.sons.push_back(IdSom::AcertoInimigo);
        if (o.vida > 0) {
            soltarLascas(p, {centro.x, centro.y + 4.0f}, 4, 70.0f, g.id * 31u + static_cast<std::uint32_t>(i));
            p.hitStop = std::max(p.hitStop, HITSTOP_ACERTO * 0.5f);
            continue;
        }
        o.usado = true;
        p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)].bloqueiaMovimento = false;
        soltarLascas(p, {centro.x, centro.y + 4.0f}, 14, 120.0f, g.id * 37u + static_cast<std::uint32_t>(i));
        p.hitStop = std::max(p.hitStop, HITSTOP_ACERTO);
        p.sons.push_back(IdSom::MorteInimigo);
        if (sortearPorcentagem(p.rng, p.ajustes.chanceDrop + BONUS_DROP_CAIXA)) {
            const ItemSorteado s = sortearDrop(p.rng, p.ajustes.pesoKit);
            soltarItem(p, s.tipo, s.quantidade, centro, centro);
        }
    }
}

// Especial ou Clarão numa parede rachada: ela cai (vira chão), a visão se refaz e o segredo
// rende XP. É uma alteração de mapa pelo jogador (F19).
constexpr int XP_SEGREDO = 15;

void derrubarParedes(EstadoPartida& p, const Golpe& g) {
    if (!g.derrubaParede) return;
    const Retangulo env = envolvente(g.area);
    const Celula a = celulaDe({env.x, env.y}), b = celulaDe({env.x + env.w, env.y + env.h});
    int caiu = 0;
    for (int l = a.lin; l <= b.lin; ++l) {
        for (int c = a.col; c <= b.col; ++c) {
            if (!dentroDaSala(p.sala, c, l) || tileEm(p.sala, c, l).tipo != TipoTile::ParedeRachada) continue;
            const Retangulo celula{static_cast<float>(c * TILE), static_cast<float>(l * TILE), static_cast<float>(TILE),
                                   static_cast<float>(TILE)};
            if (!areaToca(g.area, celula)) continue;
            p.sala.tiles[static_cast<std::size_t>(l * p.sala.cols + c)] = criarTile(TipoTile::Chao);
            ++caiu;
        }
    }
    if (caiu == 0) return;
    ++p.sala.versaoTerreno;
    p.sons.push_back(IdSom::Bau);
    texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "A parede cedeu: um segredo!", TipoTexto::Critico);
    ganharXp(p, XP_SEGREDO);
}

// Cada golpe hostil que acaba de sair ganha o seu efeito visual (garra, fuligem, cabelo…).
void criarEfeitosDeAtaque(EstadoPartida& p) {
    for (Golpe& g : p.golpes) {
        if (g.efeitoCriado || g.equipe == Equipe::Jogador || g.terreno || g.chama) continue;
        g.efeitoCriado = true;
        EfeitoAtaque e;
        e.area = g.area;
        e.padrao = g.padraoChefe;
        for (const Inimigo& i : p.inimigos) {   // vivo ou não (o balão estoura junto com o golpe)
            if (i.id != g.origem) continue;
            e.atacante = i.tipo;
            e.fase = i.fase;
        }
        e.fogo = g.fogo;
        e.nuvem = g.nuvem > 0.0f;
        e.poca = g.poca > 0.0f && !g.fogo;
        e.vida = DURACAO_EFEITO_ATAQUE;
        p.efeitosAtaque.push_back(e);
    }
}

void resolverGolpes(EstadoPartida& p) {
    p.golpesNovos.clear();
    criarEfeitosDeAtaque(p);
    for (Golpe& g : p.golpes) {
        if (g.equipe == Equipe::Jogador) {
            golpeEmCaixas(p, g);
            derrubarParedes(p, g);
        }
        auto jaAtingiu = [&](IdEntidade id) { return std::find(g.atingidos.begin(), g.atingidos.end(), id) != g.atingidos.end(); };
        if (g.equipe != Equipe::Hostil) {
            for (Inimigo& ini : p.inimigos) {
                if (!podeSerAtingido(ini) || ini.id == g.origem || jaAtingiu(ini.id)) continue;
                if (ini.aliado > 0.0f && g.equipe == Equipe::Jogador) continue;   // Imitação: está do lado de Bill
                if (g.terreno) {   // armadilha: só terrestres comuns, pelos pés
                    const ParametrosInimigo& par = parametrosDe(ini.tipo);
                    if (!par.voa && !par.chefe && pontoNaArea(g.area, ini.pes)) golpeEmInimigo(p, g, ini);
                    continue;
                }
                if (areaToca(g.area, hurtboxInimigo(ini))) golpeEmInimigo(p, g, ini);
            }
        }
        const bool tocaBill = g.terreno ? pontoNaArea(g.area, p.bill.pes) : areaToca(g.area, hurtboxJogador(p.bill));
        if (g.equipe != Equipe::Jogador && !jaAtingiu(ID_JOGADOR) && tocaBill) {
            golpeNoBill(p, g);
        }
    }
    p.golpes.insert(p.golpes.end(), p.golpesNovos.begin(), p.golpesNovos.end());   // os saltos: no próximo quadro
    p.golpesNovos.clear();
}

// O chão que pode virar poça: piso comum (água, buraco, portas e armadilhas ficam como estão).
bool viraPoca(const Tile& t) {
    return t.tipo == TipoTile::Chao || t.tipo == TipoTile::ChaoVariante || t.tipo == TipoTile::Detrito || t.tipo == TipoTile::Lama;
}

void secarPocas(EstadoPartida& p) {
    for (const PocaTemporaria& o : p.pocasDeSangue)
        p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)] = o.original;
    if (!p.pocasDeSangue.empty()) ++p.sala.versaoTerreno;
    p.pocasDeSangue.clear();
}

// O golpe do Ralo caiu: as células cujo centro está na área viram poça de sangue por golpe.poca s.
// A chuva de brasas do Pássaro (13.3) é o mesmo mecanismo com outro tile: fogo no piso (Labareda).
void espalharPocas(EstadoPartida& p, const Golpe& g) {
    if (g.inunda) secarPocas(p);
    const Retangulo r = envolvente(g.area);
    const Celula a = celulaDe({r.x, r.y}), b = celulaDe({r.x + r.w, r.y + r.h});
    for (int lin = std::max(0, a.lin); lin <= std::min(p.sala.lins - 1, b.lin); ++lin) {
        for (int col = std::max(0, a.col); col <= std::min(p.sala.cols - 1, b.col); ++col) {
            Tile& t = p.sala.tiles[static_cast<std::size_t>(lin * p.sala.cols + col)];
            if (!viraPoca(t) || !pontoNaArea(g.area, centroCelula({col, lin}))) continue;
            p.pocasDeSangue.push_back({{col, lin}, t, g.poca});
            t = criarTile(g.fogo ? TipoTile::Labareda : TipoTile::PocaSangue);
        }
    }
    ++p.sala.versaoTerreno;
}

void atualizarPocasDeSangue(EstadoPartida& p, float dt) {
    bool mudou = false;
    for (PocaTemporaria& o : p.pocasDeSangue) {
        o.vida -= dt;
        if (o.vida > 0.0f) continue;
        p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)] = o.original;
        mudou = true;
    }
    if (!mudou) return;
    p.pocasDeSangue.erase(std::remove_if(p.pocasDeSangue.begin(), p.pocasDeSangue.end(),
                                         [](const PocaTemporaria& o) { return o.vida <= 0.0f; }),
                          p.pocasDeSangue.end());
    ++p.sala.versaoTerreno;
}

void atualizarAvisos(EstadoPartida& p, float dt) {
    for (Aviso& a : p.avisos) {
        a.tempoRestante -= dt;
        if (a.tempoRestante > 0.0f) continue;
        Inimigo* origem = buscarVivo(p, a.golpe.origem);
        if (a.golpe.terreno) {   // o piso que cede não tem dono: acontece sempre
            p.golpes.push_back(a.golpe);
            continue;
        }
        if (!origem) continue;   // quem avisou morreu antes: o ataque não acontece
        if (a.consomeOrigem) {
            // O balão sai da lista de inimigos neste mesmo passo. Se o efeito for criado
            // no próximo quadro, já não existe atacante para escolher fx_estouro.
            EfeitoAtaque e;
            e.area = a.golpe.area;
            e.atacante = origem->tipo;
            e.vida = DURACAO_EFEITO_ATAQUE;
            p.efeitosAtaque.push_back(e);
            a.golpe.efeitoCriado = true;
        }
        p.golpes.push_back(a.golpe);
        if (a.golpe.nuvem > 0.0f) p.nuvens.push_back({a.golpe.area.centro, a.golpe.area.raio, a.golpe.nuvem});
        if (a.golpe.poca > 0.0f) espalharPocas(p, a.golpe);
        if (a.consomeOrigem) {   // o balão estoura junto (sem XP: ninguém o derrotou)
            origem->vivo = false;
            p.sons.push_back(IdSom::MorteInimigo);
        }
    }
    p.avisos.erase(std::remove_if(p.avisos.begin(), p.avisos.end(), [](const Aviso& a) { return a.tempoRestante <= 0.0f; }),
                   p.avisos.end());
}

// Inimigos não se sobrepõem e não entram no Bill (o dash atravessa; ele está invulnerável).
void separarAtores(EstadoPartida& p) {
    constexpr float DISTANCIA_MINIMA = 18.0f;
    for (std::size_t i = 0; i < p.inimigos.size(); ++i) {
        Inimigo& a = p.inimigos[i];
        if (!a.vivo) continue;
        for (std::size_t j = i + 1; j < p.inimigos.size(); ++j) {
            Inimigo& b = p.inimigos[j];
            if (!b.vivo || b.submerso > 0.0f || a.submerso > 0.0f) continue;
            Vec2 d = subtrair(b.pes, a.pes);
            const float dist = comprimento(d);
            if (dist >= DISTANCIA_MINIMA) continue;
            d = dist > 0.01f ? escalar(d, 1.0f / dist) : Vec2{1.0f, 0.0f};
            // Quem é fixo (Cabelo do ralo) não sai do lugar: o outro anda a distância toda.
            const bool aFixo = inimigoFixo(a), bFixo = inimigoFixo(b);
            const float falta = DISTANCIA_MINIMA - dist;
            const float paraA = aFixo ? 0.0f : (bFixo ? falta : falta / 2.0f);
            const float paraB = bFixo ? 0.0f : (aFixo ? falta : falta / 2.0f);
            a.pes = moverComColisao(p.sala, a.pes, CAIXA_INIMIGO, escalar(d, -paraA), parametrosDe(a.tipo).voa);
            b.pes = moverComColisao(p.sala, b.pes, CAIXA_INIMIGO, escalar(d, paraB), parametrosDe(b.tipo).voa);
        }
        if (p.bill.dashRestante > 0.0f || inimigoFixo(a)) continue;
        const Vec2 d = subtrair(a.pes, p.bill.pes);
        const float dist = comprimento(d);
        if (dist < 14.0f) {
            const Vec2 dir = dist > 0.01f ? escalar(d, 1.0f / dist) : Vec2{0.0f, 1.0f};
            a.pes = moverComColisao(p.sala, a.pes, CAIXA_INIMIGO, escalar(dir, 14.0f - dist), parametrosDe(a.tipo).voa);
        }
    }
}

void atualizarTextos(EstadoPartida& p, float dt) {
    for (TextoFlutuante& t : p.textos) {
        t.vida -= dt;
        t.pos.y -= 22.0f * dt;
    }
    p.textos.erase(std::remove_if(p.textos.begin(), p.textos.end(), [](const TextoFlutuante& t) { return t.vida <= 0.0f; }),
                   p.textos.end());
}

void atualizarOndas(EstadoPartida& p, float dt) {
    if (p.salaConcluida || p.festaAtiva || hostisVivos(p) > 0) return;   // a festa termina pelo relógio dela
    p.timerOnda -= dt;
    if (p.timerOnda > 0.0f) return;
    if (p.ondaAtual < p.sala.modelo->ondas) {
        iniciarOnda(p);
        p.timerOnda = ATRASO_ENTRE_ONDAS;
    } else {
        concluirSala(p);
    }
}

// ---------- inimigos × itens (SDD §24.5) ----------
// Spawn mais longe do Bill (para onde o ladrão corre); sem spawns, a entrada da sala.
Vec2 refugioDoLadrao(const EstadoPartida& p) {
    Vec2 melhor = centroCelula(p.sala.modelo->entradaJogador);
    float maior = -1.0f;
    for (const PontoSpawn& s : p.sala.modelo->spawns) {
        const float d = comprimento(subtrair(centroCelula(s.celula), p.bill.pes));
        if (d > maior) { maior = d; melhor = centroCelula(s.celula); }
    }
    return melhor;
}

void atualizarLadroes(EstadoPartida& p, float dt) {
    for (Inimigo& rato : p.inimigos) {
        if (rato.tipo != TipoInimigo::Rato || !rato.vivo || rato.surgindo > 0.0f) continue;
        if (rato.itemCarregado >= 0) {
            // Fugindo com o item: fora da luz por 8 s, some com ele (sem XP: ninguém o venceu).
            rato.tempoSemSerVisto = estaVisivel(p.visao, rato.pes) ? 0.0f : rato.tempoSemSerVisto + dt;
            if (rato.tempoSemSerVisto >= TEMPO_LADRAO_SUMIR) {
                rato.vivo = false;
                texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f},
                      std::string("Um rato levou: ") + definicaoDe(static_cast<TipoItem>(rato.itemCarregado)).nome, TipoTexto::Info);
            }
            continue;
        }
        if (rato.recargaRoubo > 0.0f) { rato.recargaRoubo -= dt; continue; }
        for (std::size_t i = 0; i < p.itensChao.size(); ++i) {
            ItemNoChao& it = p.itensChao[i];
            if (it.tempo < DURACAO_PULO_ITEM || ehItemDeMissao(it.tipo)) continue;   // missão: nunca
            if (comprimento(subtrair(it.pos, rato.pes)) > RAIO_ROUBO) continue;
            // Uma tentativa por aproximação (a chance é da dificuldade), não uma por quadro.
            rato.recargaRoubo = 2.0f;
            if (!sortearPorcentagem(p.rng, p.ajustes.chanceRoubo)) break;
            rato.itemCarregado = static_cast<int>(it.tipo);
            rato.qtdCarregada = it.quantidade;
            rato.estado = EstadoIA::Fugindo;
            rato.destinoFuga = refugioDoLadrao(p);
            rato.tempoSemSerVisto = 0.0f;
            texto(p, {rato.pes.x, rato.pes.y - 36.0f}, std::string("Pegou: ") + definicaoDe(it.tipo).nome, TipoTexto::Info);
            p.itensChao.erase(p.itensChao.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
}

// Espectro: solta brasas pelo caminho; brasa que encosta em item comum o queima.
void atualizarBrasas(EstadoPartida& p, float dt) {
    for (Inimigo& e : p.inimigos) {
        if (e.tipo != TipoInimigo::Espectro || !e.vivo || e.surgindo > 0.0f || e.submerso > 0.0f) continue;
        e.timerRastro -= dt;
        if (e.timerRastro <= 0.0f) {
            p.brasas.push_back({e.pes, VIDA_BRASA, e.id});
            e.timerRastro = INTERVALO_BRASA;
        }
    }
    for (Brasa& b : p.brasas) {
        b.vida -= dt;
        for (ItemNoChao& it : p.itensChao) {
            // Moeda não queima (17/11): sumir moeda debaixo do pé de Bill parecia bug de coleta.
            if (it.quantidade <= 0 || ehItemDeMissao(it.tipo) || it.tipo == TipoItem::Moeda || it.tempo < DURACAO_PULO_ITEM) continue;
            if (comprimento(subtrair(it.pos, b.pos)) > RAIO_BRASA) continue;
            texto(p, {it.pos.x, it.pos.y - 20.0f}, std::string("Queimou: ") + definicaoDe(it.tipo).nome, TipoTexto::DanoBill);
            it.quantidade = 0;   // removido abaixo
        }
    }
    p.itensChao.erase(std::remove_if(p.itensChao.begin(), p.itensChao.end(), [](const ItemNoChao& i) { return i.quantidade <= 0; }),
                      p.itensChao.end());
    // E ferem Bill (PRD §14.2: 6/s): pelos pés, sem esquiva, no máximo 1× por segundo; o dash passa.
    if (p.recargaBrasaBill > 0.0f) p.recargaBrasaBill -= dt;
    if (p.recargaBrasaBill <= 0.0f && estaVivo(p.bill) && p.bill.dashRestante <= 0.0f) {
        for (const Brasa& b : p.brasas) {
            if (b.vida <= 0.0f || comprimento(subtrair(p.bill.pes, b.pos)) > RAIO_BRASA) continue;
            Golpe g;
            g.id = p.proximoGolpe++;
            g.equipe = Equipe::Hostil;   // só Bill (PRD §14.2)
            g.origem = b.dono;
            g.area = areaCirculo(p.bill.pes, 4.0f);
            g.dano = DANO_BRASA;
            g.chama = true;
            g.rolaAcerto = false;
            g.terreno = true;
            p.golpes.push_back(g);
            p.recargaBrasaBill = 1.0f;
            break;
        }
    }
    p.brasas.erase(std::remove_if(p.brasas.begin(), p.brasas.end(), [](const Brasa& b) { return b.vida <= 0.0f; }), p.brasas.end());
}

// Afogado deixa poças por onde ANDA (uma a cada 0,5 s, sem empilhar); somem em 6 s.
void atualizarPocas(EstadoPartida& p, float dt) {
    for (Inimigo& e : p.inimigos) {
        if (e.tipo != TipoInimigo::Afogado || !e.vivo || e.surgindo > 0.0f || !e.andando) continue;
        e.timerRastro -= dt;
        if (e.timerRastro > 0.0f) continue;
        e.timerRastro = INTERVALO_POCA;
        const bool perto = std::any_of(p.pocas.begin(), p.pocas.end(),
                                       [&](const Poca& o) { return comprimento(subtrair(o.pos, e.pes)) < RAIO_POCA; });
        if (!perto) p.pocas.push_back({e.pes, VIDA_POCA});
    }
    for (Poca& o : p.pocas) o.vida -= dt;
    p.pocas.erase(std::remove_if(p.pocas.begin(), p.pocas.end(), [](const Poca& o) { return o.vida <= 0.0f; }), p.pocas.end());
    p.bill.naPoca = std::any_of(p.pocas.begin(), p.pocas.end(),
                                [&](const Poca& o) { return comprimento(subtrair(o.pos, p.bill.pes)) <= RAIO_POCA; });
}

// Sombra esmaga caixas no caminho: o conteúdo se perde (não sorteia item).
void sombrasEsmagamCaixas(EstadoPartida& p) {
    for (const Inimigo& s : p.inimigos) {
        if (s.tipo != TipoInimigo::Sombra || !s.vivo || s.surgindo > 0.0f) continue;
        for (ObjetoSala& o : p.sala.objetos) {
            if (o.codigo != 'c' || o.usado) continue;
            if (comprimento(subtrair(centroCelula(o.celula), s.pes)) > TILE + 4.0f) continue;   // encostou
            o.usado = true;
            p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)].bloqueiaMovimento = false;
            p.sons.push_back(IdSom::AcertoInimigo);
            if (estaVisivel(p.visao, centroCelula(o.celula))) {
                texto(p, centroCelula(o.celula), "A sombra esmagou a caixa", TipoTexto::Info);
            }
        }
    }
}

// ---------- itens ----------
void notificar(EstadoPartida& p, TipoItem tipo, int quantidade) {
    // Mesmo item seguido: soma na mesma linha ("+ 7 Moedas" em vez de várias).
    if (!p.notificacoes.empty() && p.notificacoes.back().tipo == tipo && p.notificacoes.back().vida > 1.0f) {
        p.notificacoes.back().quantidade += quantidade;
        p.notificacoes.back().vida = 2.0f;
        return;
    }
    p.notificacoes.push_back({tipo, quantidade, 2.0f});
    if (p.notificacoes.size() > 3) p.notificacoes.erase(p.notificacoes.begin());
}

void coletarItens(EstadoPartida& p, float dt) {
    if (p.esperaAvisoCheio > 0.0f) p.esperaAvisoCheio -= dt;
    for (ItemNoChao& it : p.itensChao) {
        it.tempo += dt;
        if (it.tempo < DURACAO_PULO_ITEM || it.quantidade <= 0) continue;
        // Ímã: perto, à vista e com espaço, o item desliza até Bill (o ícone segue a posição).
        const Vec2 falta = subtrair(p.bill.pes, it.pos);
        const float dist = comprimento(falta);
        if (estaVivo(p.bill) && dist > 1.0f && dist <= RAIO_IMA && espacoPara(p.bill, p.inventario, it.tipo) > 0 &&
            segmentoLivre(p.sala, it.pos, p.bill.pes, true)) {
            const float passo = std::min(dist, VELOCIDADE_IMA * dt);
            it.pos = somar(it.pos, escalar(falta, passo / dist));
            it.origem = it.pos;   // o desenho interpola origem→pos no pulo; depois dele, segue a posição
        }
        if (!itemEncostaNoBill(p.bill.pes, it.pos)) continue;
        const int entrou = coletarNoInventario(p.bill, p.inventario, it.tipo, it.quantidade);
        if (entrou <= 0) {   // cheio: o item fica no chão (PRD §11.1)
            if (p.esperaAvisoCheio <= 0.0f) {
                const DefinicaoItem& d = definicaoDe(it.tipo);
                texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f},
                      std::string("Sem espaço: ") + d.nome + " (" + std::to_string(d.limite) + "/" + std::to_string(d.limite) + ")",
                      TipoTexto::Info);
                p.esperaAvisoCheio = 1.5f;
            }
            continue;
        }
        it.quantidade -= entrou;
        ++p.stats.I;   // cada coleta conta 1× (PRD §16)
        if (it.tipo == TipoItem::Lembrancinha) {   // vai para a estante do Clube no fim da tentativa
            const int bioma = p.emTentativa ? p.plano.bioma : p.sala.modelo->bioma;
            p.lembrancinhas |= 1u << std::clamp(bioma, 0, 7);
            texto(p, {p.bill.pes.x, p.bill.pes.y - 70.0f}, "Uma Lembrancinha dos Perdedores!", TipoTexto::Critico);
        }
        notificar(p, it.tipo, entrou);
        p.sons.push_back(IdSom::Moeda);
    }
    p.itensChao.erase(std::remove_if(p.itensChao.begin(), p.itensChao.end(), [](const ItemNoChao& i) { return i.quantidade <= 0; }),
                      p.itensChao.end());
    for (Notificacao& n : p.notificacoes) n.vida -= dt;
    p.notificacoes.erase(std::remove_if(p.notificacoes.begin(), p.notificacoes.end(), [](const Notificacao& n) { return n.vida <= 0.0f; }),
                         p.notificacoes.end());
}

void abrirBau(EstadoPartida& p, ObjetoSala& bau) {
    const bool trancado = bauTrancado(p);
    if (trancado) {
        int& chaves = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveEnferrujada)];
        if (chaves <= 0) {
            texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Trancado: precisa de uma chave enferrujada", TipoTexto::Info);
            return;
        }
        --chaves;
    }
    bau.usado = true;
    p.sons.push_back(IdSom::Bau);
    const Vec2 centro = centroCelula(bau.celula);
    const int quantos = trancado ? ITENS_POR_BAU + 1 : ITENS_POR_BAU;
    for (int i = 0; i < quantos; ++i) {
        const ItemSorteado s = sortearItemDeBau(p.rng, p.ajustes.pesoKit);
        soltarItem(p, s.tipo, s.quantidade, centro, celulaLivrePerto(p, centro, i));
    }
}

void usarKitRapido(EstadoPartida& p) {
    const Vec2 acima{p.bill.pes.x, p.bill.pes.y - 60.0f};
    switch (usarItem(p.bill, p.inventario, TipoItem::Kit)) {
        case ResultadoUso::Usou:
            p.sons.push_back(IdSom::Kit);
            ++p.kitsUsados;
            break;
        case ResultadoUso::NaoPrecisa: texto(p, acima, "Vida cheia", TipoTexto::Info); break;
        default: texto(p, acima, "Sem kits", TipoTexto::Info); break;
    }
}

// Porta do Medo (T): a Chave do Medo abre (e é consumida). Sem a chave, um aviso.
bool abrirPortaDoMedo(EstadoPartida& p) {
    const Celula t = portaDoMedoProxima(p);
    if (t.col < 0) return false;
    int& chaves = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)];
    if (chaves <= 0) {
        texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Trancada: precisa da Chave do Medo", TipoTexto::Info);
        return true;
    }
    --chaves;
    p.sala.tiles[static_cast<std::size_t>(t.lin * p.sala.cols + t.col)] = criarTile(TipoTile::Chao);
    ++p.sala.versaoTerreno;
    p.sons.push_back(IdSom::Bau);
    texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "A porta do medo se abriu", TipoTexto::Info);
    return true;
}

// Resgate (SDD §13.1, parte desta etapa): o amigo preso conta o resgate, dá XP, conversa e
// passa a acompanhar Bill; o companheiro anterior volta ao Clube. A cena (ponte, Eco
// lendário) entra com o bioma 1 completo (Etapa 8).
constexpr int XP_RESGATE = 25;

void concluirResgate(EstadoPartida& p, const Conteudo& c, ObjetoSala& o) {
    const std::string& codigo = p.sala.modelo->npcs[static_cast<std::size_t>(o.codigo - '0')];
    const Amigo a = amigoDoCodigo(codigo);
    o.usado = true;
    p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)].bloqueiaMovimento = false;
    ++p.stats.R;
    // O amigo muda o mapa (F19): o Ben baixa a ponte, o Stan fecha a comporta… (ao_resgatar= na sala).
    for (const SalaModelo::Alteracao& alt : p.sala.modelo->aoResgatar) {
        p.sala.tiles[static_cast<std::size_t>(alt.celula.lin * p.sala.cols + alt.celula.col)] = tileDoSimbolo(alt.simbolo);
    }
    for (const SalaModelo::TrocaEnfeite& t : p.sala.modelo->enfeitesAoResgatar) {   // a comporta fecha
        for (ObjetoSala& e : p.sala.objetos) {
            if (e.codigo != 'e' || e.celula.col != t.celula.col || e.celula.lin != t.celula.lin) continue;
            e.quadro = t.quadro;
            if (t.passa) p.sala.tiles[static_cast<std::size_t>(e.celula.lin * p.sala.cols + e.celula.col)].bloqueiaMovimento = false;
        }
    }
    if (!p.sala.modelo->aoResgatar.empty()) {
        ++p.sala.versaoTerreno;
        const char* fala = a == Amigo::Ben       ? "O Ben baixou a ponte"
                           : a == Amigo::Stan    ? "O Stan fechou a comporta"
                           : a == Amigo::Beverly ? "A Beverly derrubou a passarela"
                                                 : "O caminho mudou";
        texto(p, {p.bill.pes.x, p.bill.pes.y - 106.0f}, fala, TipoTexto::Info);
    }
    // Mike liga o gerador (PRD §14.3, Etapa 13.4): as luzes revelam a sala inteira (F19 por NPC).
    if (a == Amigo::Mike) {
        p.salaIluminada = true;
        revelarTudo(p.visao);
        texto(p, {p.bill.pes.x, p.bill.pes.y - 106.0f}, "O Mike ligou o gerador", TipoTexto::Info);
    }
    ganharXp(p, XP_RESGATE);
    p.sons.push_back(IdSom::Nivel);
    const Amigo anterior = p.companheiro.ativo ? p.companheiro.quem : Amigo::Total;
    if (a != Amigo::Total) {
        p.resgatados.push_back(a);
        entrarCompanheiro(p, a, centroCelula(o.celula));
        texto(p, {p.bill.pes.x, p.bill.pes.y - 70.0f}, std::string(nomeAmigo(a)) + " resgatado!", TipoTexto::Info);
        if (const int lendario = ecoLendarioDe(a); lendario >= 0) {   // PRD §12: o resgate dá o Lendário
            aplicarEco(p, lendario, Raridade::Lendario);
            texto(p, {p.bill.pes.x, p.bill.pes.y - 88.0f}, std::string("Eco lendário: ") + definicaoEco(lendario).nome,
                  TipoTexto::Critico);
        }
        if (anterior != Amigo::Total && anterior != a) {
            texto(p, {p.bill.pes.x, p.bill.pes.y - 52.0f}, std::string(nomeAmigo(anterior)) + " volta ao Clube pela rota segura",
                  TipoTexto::Info);
        }
    }
    abrirDialogo(p, buscarDialogo(c.dialogos, "resgate_" + codigo));
}

// Elias: com a manivela, ele abre a grade (alteração de mapa por NPC, F19); senão, a loja.
void atenderElias(EstadoPartida& p, const Conteudo& c) {
    int& manivelas = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)];
    if (manivelas > 0 && !p.manivelaEntregue) {
        --manivelas;
        p.manivelaEntregue = true;
        int abertas = 0;
        for (Tile& t : p.sala.tiles) {
            if (t.tipo != TipoTile::Grade) continue;
            t = criarTile(TipoTile::Chao);
            ++abertas;
        }
        if (abertas > 0) ++p.sala.versaoTerreno;
        p.sons.push_back(IdSom::Bau);
        texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "O Elias abriu a grade", TipoTexto::Info);
        abrirDialogo(p, buscarDialogo(c.dialogos, "elias_manivela"));
        return;
    }
    if (p.loja.empty()) {
        texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Elias: Hoje não tenho nada pra você.", TipoTexto::Info);
        return;
    }
    p.pendencia = Pendencia::Loja;
    p.cursor = 0;
    ++p.conversasElias;
}

// Clube: Mural, quadro e alçapão abrem a sua pendência; um amigo fala (balão).
// Prólogo: o Mike conversa e passa a acompanhar Bill; as portas da rua abrem.
void interagirNoClube(EstadoPartida& p) {
    const ObjetoSala* o = objetoDoClubeProximo(p);
    if (!o) return;
    switch (o->codigo) {
        case 'M': p.pendencia = Pendencia::Mural; p.cursor = 0; break;
        case 'Q': p.pendencia = Pendencia::Quadro; break;
        case 'H': p.pendencia = Pendencia::Alcapao; p.cursor = static_cast<int>(p.dificuldade); break;
        default: {   // amigo: 1..9. Fala nova desta visita (caixa de diálogo) ou, se já falou, o balão.
            const std::size_t digito = static_cast<std::size_t>(o->codigo - '0');
            if (const Dialogo* nova = p.falaNova[digito]) {
                p.falaNova[digito] = nullptr;
                p.falasVistasAgora.push_back(nova->id);
                p.balao.vida = 0.0f;
                abrirDialogo(p, nova);
                if (p.noPrologo) {   // "Eu vou com você até achar o Ben"
                    ObjetoSala& npc = p.sala.objetos[static_cast<std::size_t>(o - p.sala.objetos.data())];
                    npc.usado = true;
                    p.sala.tiles[static_cast<std::size_t>(npc.celula.lin * p.sala.cols + npc.celula.col)].bloqueiaMovimento = false;
                    entrarCompanheiro(p, amigoDoCodigo(p.sala.modelo->npcs[digito]), centroCelula(npc.celula));
                    p.salaConcluida = true;
                    definirPortas(p, true);
                }
                break;
            }
            const std::string& nome = p.sala.modelo->npcs[digito];
            p.balao.objeto = static_cast<int>(o - p.sala.objetos.data());
            p.balao.texto = falaRapida(nome, p.conversas++);
            p.balao.vida = DURACAO_BALAO;
            break;
        }
    }
}

void interagir(EstadoPartida& p, const Conteudo& c) {
    if (p.noClube || p.noPrologo) {
        interagirNoClube(p);
        return;
    }
    if (abrirPortaDoMedo(p)) return;
    if (eliasProximo(p)) {
        atenderElias(p, c);
        return;
    }
    if (const ObjetoSala* amigo = amigoParaResgatar(p)) {
        concluirResgate(p, c, p.sala.objetos[static_cast<std::size_t>(amigo - p.sala.objetos.data())]);
        return;
    }
    if (const ObjetoSala* altar = altarProximo(p)) {   // altar de Eco: 1 de 3, uma vez
        if (abrirEscolhaDeEco(p)) p.sala.objetos[static_cast<std::size_t>(altar - p.sala.objetos.data())].usado = true;
        else texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Nenhum Eco novo", TipoTexto::Info);
        return;
    }
    for (ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == 'B' && !o.usado && comprimento(subtrair(centroCelula(o.celula), p.bill.pes)) <= RAIO_INTERAGIR) {
            abrirBau(p, o);
            return;
        }
    }
}

// Objetos ao entrar: chaves do mapa viram itens no chão; baú e caixa ocupam a célula.
void prepararObjetos(EstadoPartida& p) {
    for (ObjetoSala& o : p.sala.objetos) {
        const Vec2 centro = centroCelula(o.celula);
        Tile& t = p.sala.tiles[static_cast<std::size_t>(o.celula.lin * p.sala.cols + o.celula.col)];
        switch (o.codigo) {
            case 'k': soltarItem(p, TipoItem::ChaveEnferrujada, 1, centro, centro); o.usado = true; break;
            case 'K': soltarItem(p, TipoItem::ChaveDoMedo, 1, centro, centro); o.usado = true; break;
            case 'l':   // a Lembrancinha só existe se esta sala guarda o segredo do bioma
                if (p.salaTemSegredo) soltarItem(p, TipoItem::Lembrancinha, 1, centro, centro);
                o.usado = true;
                break;
            case 'B': case 'c': case 'M': case 'Q': case 'e': case '*':   // bloqueiam passos, não a visão
                t.bloqueiaMovimento = true;
                break;
            default:   // amigos (1..9) também ocupam a célula; o alçapão (H) é piso
                if (o.codigo >= '1' && o.codigo <= '9') t.bloqueiaMovimento = true;
                break;
        }
    }
    // Itens do mapa já estão "pousados" (sem pulo).
    for (ItemNoChao& it : p.itensChao) it.tempo = DURACAO_PULO_ITEM;
}

// ---------- pendência (mundo congelado) ----------
bool confirmou(const EstadoEntrada& e) { return pressionou(e, Comando::Confirmar) || pressionou(e, Comando::Atacar); }

void moverCursor(EstadoPartida& p, const EstadoEntrada& e, int total) {
    if (pressionou(e, Comando::Cima)) p.cursor = (p.cursor + total - 1) % total;
    if (pressionou(e, Comando::Baixo)) p.cursor = (p.cursor + 1) % total;
}

void abrirInventario(EstadoPartida& p, bool daPausa) {
    p.pendencia = Pendencia::Inventario;
    p.aba = AbaInventario::Itens;
    p.cursor = 0;
    p.inventarioVeioDaPausa = daPausa;
}

void atualizarPausa(EstadoPartida& p, const EstadoEntrada& e) {
    if (p.confirmando != OpcaoPausa::Total) {   // "Tem certeza?" — 0 = Sim, 1 = Não
        moverCursor(p, e, 2);
        // Sim e Não ficam LADO A LADO: ← → também trocam (só ↑↓ funcionava; o Diego apertava ← → e
        // achava que o menu estava com atraso).
        if (pressionou(e, Comando::Esquerda) || pressionou(e, Comando::Direita)) p.cursor = 1 - p.cursor;
        if (confirmou(e)) {
            if (p.cursor == 0 && p.confirmando == OpcaoPausa::Abandonar) {
                p.abandonou = true;
                p.terminou = true;
                fecharPendencia(p);
            } else if (p.cursor == 0 && p.confirmando == OpcaoPausa::Sair) {
                p.pedidoSair = true;
            }
            p.cursor = static_cast<int>(p.confirmando);
            p.confirmando = OpcaoPausa::Total;
        } else if (pressionou(e, Comando::Voltar)) {
            p.cursor = static_cast<int>(p.confirmando);
            p.confirmando = OpcaoPausa::Total;
        }
        return;
    }
    if (pressionou(e, Comando::Pausa) || pressionou(e, Comando::Voltar)) {   // Esc de novo: continua
        fecharPendencia(p);
        return;
    }
    moverCursor(p, e, TOTAL_OPCOES_PAUSA);
    if (!confirmou(e)) return;
    switch (static_cast<OpcaoPausa>(p.cursor)) {
        case OpcaoPausa::Continuar: fecharPendencia(p); break;
        case OpcaoPausa::Inventario: abrirInventario(p, true); break;
        case OpcaoPausa::Ficha:
            if (p.bill.progresso.pontosPendentes > 0) {
                abrirPendenciaDeNivel(p);   // há pontos: vai direto distribuir
            } else {
                abrirInventario(p, true);   // sem pontos: mostra a ficha (só leitura)
                p.aba = AbaInventario::Ficha;
            }
            break;
        case OpcaoPausa::Salvar:
            // A cena abre o cartão de memória (arquivos e telas não moram nas regras). No Clube e no prólogo
            // não há tentativa: um aviso diz por quê.
            if (p.emTentativa && p.pontoDaSala.valido) p.pedidoSalvar = true;
            else {
                p.avisoMenu = "Só dá para salvar durante a noite, nos subterrâneos.";
                p.tempoAvisoMenu = 2.5f;
            }
            break;
        case OpcaoPausa::Abandonar:
            if (p.noClube || p.noPrologo) {   // Clube/prólogo: não há tentativa a perder; volta ao menu
                p.pedidoMenu = true;
                fecharPendencia(p);
                break;
            }
            p.confirmando = OpcaoPausa::Abandonar;
            p.cursor = 1;
            break;
        case OpcaoPausa::Sair:
            p.confirmando = static_cast<OpcaoPausa>(p.cursor);
            p.cursor = 1;   // começa no "Não": um Enter a mais não joga a tentativa fora
            break;
        default: break;
    }
}

void atualizarInventario(EstadoPartida& p, const EstadoEntrada& e) {
    if (pressionou(e, Comando::Voltar) || pressionou(e, Comando::Inventario) || pressionou(e, Comando::Pausa)) {
        const bool volta = p.inventarioVeioDaPausa;
        fecharPendencia(p);
        if (volta) {
            p.pendencia = Pendencia::Pausa;
            p.cursor = static_cast<int>(OpcaoPausa::Inventario);
        }
        return;
    }
    // Abas: < > (vírgula/ponto), PageUp/PageDown, LB/RB, ou ← → no teclado.
    int a = static_cast<int>(p.aba);
    if (pressionou(e, Comando::AbaDireita) || pressionou(e, Comando::Direita)) a = (a + 1) % TOTAL_ABAS;
    if (pressionou(e, Comando::AbaEsquerda) || pressionou(e, Comando::Esquerda)) a = (a + TOTAL_ABAS - 1) % TOTAL_ABAS;
    if (a != static_cast<int>(p.aba)) {
        p.aba = static_cast<AbaInventario>(a);
        p.cursor = 0;
    }
    if (p.aba == AbaInventario::Itens) {
        moverCursor(p, e, TOTAL_ITENS_USAVEIS);
        if (confirmou(e)) {
            const TipoItem t = ITENS_USAVEIS[p.cursor];
            const std::string nome = definicaoDe(t).nome;
            switch (usarItem(p.bill, p.inventario, t)) {
                case ResultadoUso::Usou:
                    avisarNoMenu(p, "Usou: " + nome);
                    p.sons.push_back(t == TipoItem::Kit || t == TipoItem::Barquinho ? IdSom::Kit : IdSom::Moeda);
                    break;
                case ResultadoUso::NaoPrecisa: avisarNoMenu(p, "Vida cheia: o item não foi gasto"); break;
                case ResultadoUso::NaoTem: avisarNoMenu(p, "Você não tem " + nome); break;
                default: break;
            }
        }
    } else if (p.aba == AbaInventario::Ficha && confirmou(e) && p.bill.progresso.pontosPendentes > 0) {
        abrirPendenciaDeNivel(p);
    }
}

// Caixa de diálogo: as letras aparecem aos poucos; Confirmar completa a linha e, completa,
// avança. Voltar pula para o fim (as escolhas, se houver). Escolha leva ao bloco de destino.
void atualizarDialogo(EstadoPartida& p, const Conteudo& c, const EstadoEntrada& e, float dt) {
    EstadoDialogo& d = p.dialogo;
    if (!d.atual || d.atual->linhas.empty()) {
        fecharPendencia(p);
        return;
    }
    const Dialogo& dl = *d.atual;
    if (d.escolhendo) {
        moverCursor(p, e, static_cast<int>(dl.escolhas.size()));
        if (!confirmou(e)) return;
        const Dialogo* proximo = buscarDialogo(c.dialogos, dl.escolhas[static_cast<std::size_t>(p.cursor)].destino);
        if (proximo) p.falasVistasAgora.push_back(proximo->id);
        if (!abrirDialogo(p, proximo)) fecharPendencia(p);
        return;
    }
    const float total = static_cast<float>(contarCaracteres(dl.linhas[static_cast<std::size_t>(d.linha)].texto));
    d.letras = std::min(total, d.letras + dt * LETRAS_POR_SEGUNDO);
    const bool pular = pressionou(e, Comando::Voltar);
    if (!pular && !confirmou(e)) return;
    if (pular) {   // vai direto ao fim da conversa
        d.linha = static_cast<int>(dl.linhas.size()) - 1;
        d.letras = static_cast<float>(contarCaracteres(dl.linhas.back().texto));
    } else if (d.letras < total) {
        d.letras = total;   // 1º toque: completa a linha
        return;
    } else if (d.linha + 1 < static_cast<int>(dl.linhas.size())) {
        ++d.linha;
        d.letras = 0.0f;
        return;
    }
    if (!dl.escolhas.empty()) {
        d.escolhendo = true;
        p.cursor = 0;
        return;
    }
    d = EstadoDialogo{};   // fim da conversa
    fecharPendencia(p);
}

void atualizarPendencia(EstadoPartida& p, const Conteudo& c, const EstadoEntrada& e, float dt) {
    switch (p.pendencia) {
        case Pendencia::Dialogo: atualizarDialogo(p, c, e, dt); break;
        case Pendencia::Loja: {
            const int n = static_cast<int>(p.loja.size());
            if (n == 0 || pressionou(e, Comando::Voltar) || pressionou(e, Comando::Pausa)) {
                fecharPendencia(p);
                break;
            }
            moverCursor(p, e, n);
            if (!confirmou(e)) break;
            const OfertaLoja oferta = p.loja[static_cast<std::size_t>(p.cursor)];
            switch (comprar(p, p.cursor)) {
                case ResultadoCompra::Comprou:
                    avisarNoMenu(p, "Comprou: " + nomeDaOferta(p, oferta));
                    p.sons.push_back(IdSom::Moeda);
                    break;
                case ResultadoCompra::SemMoedas: avisarNoMenu(p, "Moedas de prata insuficientes"); break;
                case ResultadoCompra::SemEspaco: avisarNoMenu(p, "Não cabe mais: deixe para depois"); break;
                case ResultadoCompra::Vendida: avisarNoMenu(p, "Já vendido"); break;
                default: break;
            }
            break;
        }
        case Pendencia::EscolherEco: {   // 3 cartas lado a lado: ← → (ou ↑↓) e Confirmar
            const int n = static_cast<int>(p.ofertasEco.size());
            if (n == 0) { fecharPendencia(p); break; }
            if (pressionou(e, Comando::Direita) || pressionou(e, Comando::Baixo)) p.cursor = (p.cursor + 1) % n;
            if (pressionou(e, Comando::Esquerda) || pressionou(e, Comando::Cima)) p.cursor = (p.cursor + n - 1) % n;
            if (!confirmou(e)) break;
            const OfertaEco escolhida = p.ofertasEco[static_cast<std::size_t>(p.cursor)];
            aplicarEco(p, escolhida.definicao, escolhida.raridade);
            p.ofertasEco.clear();
            p.sons.push_back(IdSom::Nivel);
            texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f},
                  std::string("Eco: ") + definicaoEco(escolhida.definicao).nome + " (" + nomeRaridade(escolhida.raridade) + ")",
                  TipoTexto::Info);
            fecharPendencia(p);
            break;
        }
        case Pendencia::SubirNivel:
            moverCursor(p, e, TOTAL_ATRIBUTOS);
            if (confirmou(e)) resolverPendencia(p, p.cursor);
            else if (pressionou(e, Comando::Voltar) || pressionou(e, Comando::Ficha)) fecharPendencia(p);
            break;
        case Pendencia::Pausa: atualizarPausa(p, e); break;
        case Pendencia::Inventario: atualizarInventario(p, e); break;
        case Pendencia::Mural:
            moverCursor(p, e, TOTAL_TALENTOS);
            // O Mural virou um quadro em 3 colunas de 3 (24/11): ← → trocam de coluna, na mesma altura.
            if (pressionou(e, Comando::Direita)) p.cursor = (p.cursor + 3) % TOTAL_TALENTOS;
            if (pressionou(e, Comando::Esquerda)) p.cursor = (p.cursor + TOTAL_TALENTOS - 3) % TOTAL_TALENTOS;
            if (confirmou(e)) p.pedidoCompra = p.cursor;   // jogo/clube atende (quem tem o perfil)
            else if (pressionou(e, Comando::Voltar) || pressionou(e, Comando::Pausa)) fecharPendencia(p);
            break;
        case Pendencia::Quadro: {
            int f = p.filtroQuadro;
            if (pressionou(e, Comando::Direita) || pressionou(e, Comando::AbaDireita)) f = (f + 1) % 3;
            if (pressionou(e, Comando::Esquerda) || pressionou(e, Comando::AbaEsquerda)) f = (f + 2) % 3;
            p.filtroQuadro = f;
            if (confirmou(e) || pressionou(e, Comando::Voltar) || pressionou(e, Comando::Pausa)) fecharPendencia(p);
            break;
        }
        case Pendencia::Alcapao:
            moverCursor(p, e, TOTAL_DIFICULDADES);
            if (confirmou(e)) {
                p.dificuldade = static_cast<Dificuldade>(p.cursor);
                p.pedidoTentativa = true;   // jogo/clube começa a tentativa (e aplica o Mural)
                fecharPendencia(p);
            } else if (pressionou(e, Comando::Voltar) || pressionou(e, Comando::Pausa)) {
                fecharPendencia(p);
            }
            break;
        default: break;
    }
}

// ---------- Festa dos Balões ----------
Vec2 cabecaDoBalao(const BalaoFesta& b) { return {b.pos.x + 3.0f * std::sin(b.fase), b.pos.y - 22.0f}; }

void estourar(EstadoPartida& p, BalaoFesta& b) {
    const Vec2 c = cabecaDoBalao(b);
    if (b.vermelho) {   // armadilha: estoura ferindo quem está perto (a vida para em 1 na festa)
        if (comprimento(subtrair(p.bill.pes, b.pos)) < 48.0f && !estaInvulneravel(p.bill)) {
            p.bill.hp -= aplicarPorcentagem(DANO_BALAO_VERMELHO, p.ajustes.danoRecebido);
            p.bill.invulneravel = INVULNERAVEL_APOS_DANO;
            p.bill.flash = 1.0f / 60.0f;
            texto(p, {p.bill.pes.x, p.bill.pes.y - 52.0f}, "Balão vermelho!", TipoTexto::DanoBill);
            p.sons.push_back(IdSom::BillFerido);
        }
    } else {
        ++p.baloesEstourados;
        soltarItem(p, TipoItem::Moeda, MOEDAS_POR_BALAO, c, celulaLivrePerto(p, b.pos, 0));
        ganharXp(p, XP_POR_BALAO);
    }
    p.sons.push_back(IdSom::MorteInimigo);
    b.id = ID_NULO;   // marcado para sair
}

void atualizarFesta(EstadoPartida& p, float dt) {
    if (!p.festaAtiva) return;
    p.tempoFesta -= dt;
    if (p.tempoFesta <= 0.0f) {   // fim: os que sobraram voam embora; as portas abrem
        p.festaAtiva = false;
        p.baloes.clear();
        texto(p, {p.bill.pes.x, p.bill.pes.y - 70.0f}, "Fim da festa: " + std::to_string(p.baloesEstourados) + " balões",
              TipoTexto::Critico);
        concluirSala(p);
        return;
    }
    p.proximoBalao -= dt;
    if (p.proximoBalao <= 0.0f && !p.sala.modelo->spawns.empty()) {   // nascem nos 's' (embaixo)
        p.proximoBalao = INTERVALO_BALAO;
        const std::vector<PontoSpawn>& sp = p.sala.modelo->spawns;
        const Celula c = sp[static_cast<std::size_t>(intervalo(p.rng, 0, static_cast<int>(sp.size()) - 1))].celula;
        BalaoFesta b;
        b.id = p.proximoId++;
        b.pos = somar(centroCelula(c), Vec2{static_cast<float>(intervalo(p.rng, -10, 10)), 0.0f});
        b.vermelho = sortearPorcentagem(p.rng, CHANCE_BALAO_VERMELHO_PCT);
        b.cor = intervalo(p.rng, 0, 2);
        b.fase = static_cast<float>(intervalo(p.rng, 0, 628)) / 100.0f;
        p.baloes.push_back(b);
    }
    for (BalaoFesta& b : p.baloes) {   // sobem devagar, balançando
        b.pos.y -= 34.0f * dt;
        b.fase += 2.0f * dt;
        if (b.pos.y < 2.5f * TILE) b.id = ID_NULO;   // escapou pelo teto
    }
    // Golpes do Bill estouram (o taco, o giro e o clarão).
    for (const Golpe& g : p.golpes) {
        if (g.equipe != Equipe::Jogador) continue;
        for (BalaoFesta& b : p.baloes) {
            if (b.id == ID_NULO) continue;
            const Vec2 c = cabecaDoBalao(b);
            if (areaToca(g.area, {c.x - 8.0f, c.y - 9.0f, 16.0f, 18.0f})) estourar(p, b);
        }
    }
    p.baloes.erase(std::remove_if(p.baloes.begin(), p.baloes.end(), [](const BalaoFesta& b) { return b.id == ID_NULO; }),
                   p.baloes.end());
}

constexpr float RAIO_BIP_BIP = 26.0f;   // px entre os pés: Bill "passou por cima" do inimigo

// Eco "Ponte firme": quando o dash TERMINA, fica uma barreira de tijolos onde Bill parou.
void atualizarBarreirasDoDash(EstadoPartida& p, float dt) {
    const bool emDash = p.bill.dashRestante > 0.0f;
    // Eco "Bip-bip" (Richie): quem Bill atravessa no dash leva dano (1× por inimigo em cada dash).
    if (emDash && !p.estavaEmDash) p.bipBipAtingidos.clear();
    if (const int bip = valorEco(p, EfeitoEco::BipBip); bip > 0 && emDash) {
        for (const Inimigo& ini : p.inimigos) {
            if (!ini.vivo || !podeSerAtingido(ini) || ini.aliado > 0.0f) continue;
            if (std::find(p.bipBipAtingidos.begin(), p.bipBipAtingidos.end(), ini.id) != p.bipBipAtingidos.end()) continue;
            if (comprimento(subtrair(ini.pes, p.bill.pes)) > RAIO_BIP_BIP) continue;
            Golpe g = golpeDoBill(p.bill, p.proximoGolpe, areaCirculo({ini.pes.x, ini.pes.y - 8.0f}, 10.0f), bip);
            g.rolaAcerto = false;
            g.podeCritico = false;
            g.recuo = 20.0f;
            g.duracao = 0.05f;
            p.golpes.push_back(g);
            p.bipBipAtingidos.push_back(ini.id);
        }
    }
    const int ms = valorEco(p, EfeitoEco::BarreiraDash);
    if (p.estavaEmDash && !emDash && ms > 0) p.barreiras.push_back({p.bill.pes, static_cast<float>(ms) / 1000.0f});
    // Eco "Calma" (Stan): um respiro de invulnerabilidade quando o dash termina.
    if (p.estavaEmDash && !emDash) {
        if (const int calma = valorEco(p, EfeitoEco::InvulnerabilidadeDash); calma > 0)
            p.bill.invulneravel = std::max(p.bill.invulneravel, static_cast<float>(calma) / 1000.0f);
        // Eco "Inalador" (Eddie): uma névoa onde o dash terminou deixa lentos os inimigos perto.
        if (const int nevoa = valorEco(p, EfeitoEco::NevoaDash); nevoa > 0) {
            for (Inimigo& i : p.inimigos) {
                if (i.vivo && comprimento(subtrair(i.pes, p.bill.pes)) <= RAIO_NEVOA_INALADOR)
                    i.lento = std::max(i.lento, static_cast<float>(nevoa) / 1000.0f);
            }
            p.nevoasDash.push_back(p.bill.pes);
            p.vidaNevoas.push_back(0.6f);
        }
    }
    for (float& v : p.vidaNevoas) v -= dt;
    while (!p.vidaNevoas.empty() && p.vidaNevoas.front() <= 0.0f) {
        p.vidaNevoas.erase(p.vidaNevoas.begin());
        p.nevoasDash.erase(p.nevoasDash.begin());
    }
    p.estavaEmDash = emDash;
    for (BarreiraTijolos& t : p.barreiras) t.vida -= dt;
    p.barreiras.erase(std::remove_if(p.barreiras.begin(), p.barreiras.end(), [](const BarreiraTijolos& t) { return t.vida <= 0.0f; }),
                      p.barreiras.end());
}

// ---------- tutorial: gatilhos (PRD §15.4) ----------
// Olha o estado do quadro e pede as dicas cujo momento chegou. pedirDica ignora as já vistas,
// então o custo é só o de olhar. D10 (Ecos) e D20 (mapa) entram com os seus sistemas.
bool armadilhaVisivel(const EstadoPartida& p) {
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int col = 0; col < p.sala.cols; ++col) {
            const TipoTile t = tileEm(p.sala, col, l).tipo;
            const bool armadilha = t == TipoTile::Espinhos || t == TipoTile::Jato || t == TipoTile::PisoCedendo || t == TipoTile::Fogo;
            if (armadilha && visaoEm(p.visao, col, l).luz > 0) return true;
        }
    }
    return false;
}

void verificarDicas(EstadoPartida& p) {
    if (!p.dicasLigadas) return;
    if (p.noClube || p.noPrologo) {   // D02: perto de um amigo pela 1ª vez
        const ObjetoSala* o = objetoDoClubeProximo(p);
        if (o && o->codigo >= '1' && o->codigo <= '9') pedirDica(p, 2);
        return;
    }
    if (!p.emTentativa) return;
    pedirDica(p, 3);   // a 1ª sala da tentativa já é escura
    pedirDica(p, 6);
    for (const Inimigo& i : p.inimigos) {
        if (i.vivo && i.surgindo <= 0.0f && estaVisivel(p.visao, i.pes)) pedirDica(p, 4);
    }
    for (const Aviso& a : p.avisos) {
        if (a.golpe.equipe != Equipe::Jogador) pedirDica(p, 5);
    }
    if (p.stats.I > 0) pedirDica(p, 7);
    if (estaVivo(p.bill) && p.bill.hp * 2 < p.bill.stats.hpMax) pedirDica(p, 8);
    if (p.salaConcluida && p.sala.modelo->ondas > 0) pedirDica(p, 9);
    if (p.bill.progresso.pontosPendentes > 0) pedirDica(p, 11);
    if (!dicaVista(p.dicasVistas, 12) && armadilhaVisivel(p)) pedirDica(p, 12);
    if (portaDoMedoProxima(p).col >= 0 || (bauTrancado(p) && bauProximo(p))) pedirDica(p, 13);
    switch (p.sala.modelo->tipo) {
        case TipoSala::Loja: pedirDica(p, 14); break;
        case TipoSala::Refugio: pedirDica(p, 15); break;
        case TipoSala::Chefe: pedirDica(p, 16); break;
        case TipoSala::Resgate: pedirDica(p, 17); break;
        default: break;
    }
    if (p.stats.U > 0) pedirDica(p, 18);
    if (p.sala.modelo->tipo == TipoSala::Eco || p.salaPrometida.recompensa == Recompensa::Eco) pedirDica(p, 10);
}



// ---------- portas da tentativa ----------
// Quantas opções a próxima etapa oferece (a saída do chefe é 1 "descida").
int opcoesSeguintes(const EstadoPartida& p) {
    const int proxima = p.etapa + 1;
    if (proxima < static_cast<int>(p.plano.etapas.size())) {
        return static_cast<int>(p.plano.etapas[static_cast<std::size_t>(proxima)].opcoes.size());
    }
    return 1;
}

// Sela a porta de entrada; as outras recebem as opções da próxima etapa, e as que sobram são seladas.
void configurarPortas(EstadoPartida& p, int portaEntrada) {
    if (!p.emTentativa) return;
    const int quantas = opcoesSeguintes(p);
    int dadas = 0;
    for (int i = 0; i < static_cast<int>(p.sala.portas.size()); ++i) {
        PortaSala& porta = p.sala.portas[static_cast<std::size_t>(i)];
        if (i == portaEntrada || dadas >= quantas) {
            porta.selada = true;
            continue;
        }
        porta.opcao = dadas++;
    }
}

// Depois do chefe: o próximo bioma. Se ainda não existe (salas futuras), a tentativa termina
// como "fim do conteúdo" (não é derrota nem vitória).
void descerBioma(EstadoPartida& p, const Conteudo& c) {
    std::string erro;
    PlanoBioma proximo;
    if (!gerarBioma(c.salas, p.plano.bioma + 1, p.rngGerador, p.ajustes.pesoKit, proximo, erro)) {
        p.terminou = true;
        p.fimDoConteudo = true;
        return;
    }
    p.plano = proximo;
    p.etapa = 0;
    p.poemaUsado = false;   // Poema: 1× por bioma
    p.segredoAtivo = sortearPorcentagem(p.rngGerador, p.ajustes.chanceSecreta);   // SDD §24.5
    p.segredoUsado = false;
    p.salaPrometida = p.plano.etapas[0].opcoes[0];
    entrarNaSala(p, c, p.salaPrometida.sala, nullptr);
    p.sons.push_back(IdSom::TrocaSala);
}

// Item de missão esquecido no chão vai junto (PRD §5.3: nunca travar a tentativa; sem a Chave
// do Medo, a porta do resgate nunca abriria e a tentativa ficaria presa).
void guardarItensDeMissao(EstadoPartida& p) {
    for (ItemNoChao& it : p.itensChao) {
        if (!ehItemDeMissao(it.tipo) || it.quantidade <= 0) continue;
        it.quantidade -= coletarNoInventario(p.bill, p.inventario, it.tipo, it.quantidade);
        notificar(p, it.tipo, 1);
    }
}

void seguirPorta(EstadoPartida& p, const Conteudo& c, const PortaSala& porta) {
    guardarItensDeMissao(p);
    bool descida = false;
    const OpcaoPorta* destino = destinoDaPorta(p, porta, descida);
    if (descida) {
        descerBioma(p, c);
        return;
    }
    if (!destino) return;
    const OpcaoPorta escolhida = *destino;   // a sala vai ser trocada: copiar antes
    ++p.etapa;
    p.salaPrometida = escolhida;
    entrarNaSala(p, c, escolhida.sala, &porta);
}

void prepararPartida(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade dificuldade) {
    p = EstadoPartida{};
    p.seed = seed;
    semear(p.rng, seed);
    semear(p.rngGerador, seed ^ 0x9E3779B97F4A7C15ULL);
    p.dificuldade = dificuldade;
    p.ajustes = parametros(c.dificuldades, dificuldade);
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    p.bill.pactos += p.ajustes.pactosExtras;
}

}  // namespace

const char* nomeAtributo(int i) {
    static const char* const NOMES[TOTAL_ATRIBUTOS] = {"Força", "Foco", "Agilidade", "Coragem"};
    return (i >= 0 && i < TOTAL_ATRIBUTOS) ? NOMES[i] : "?";
}

int atributoDoBill(const Jogador& j, int i) {
    switch (i) {
        case 0: return j.attr.forca;
        case 1: return j.attr.foco;
        case 2: return j.attr.agilidade;
        default: return j.attr.coragem;
    }
}

bool resolverPendencia(EstadoPartida& p, int escolha) {
    if (p.pendencia != Pendencia::SubirNivel || escolha < 0 || escolha >= TOTAL_ATRIBUTOS) return false;
    int* atributos[TOTAL_ATRIBUTOS] = {&p.bill.attr.forca, &p.bill.attr.foco, &p.bill.attr.agilidade, &p.bill.attr.coragem};
    if (!aplicarPonto(p.bill.progresso, *atributos[escolha])) return false;
    recalcularJogador(p.bill);
    if (p.bill.progresso.pontosPendentes <= 0) fecharPendencia(p);
    return true;
}

void fecharPendencia(EstadoPartida& p) {
    p.pendencia = Pendencia::Nenhuma;
    p.confirmando = OpcaoPausa::Total;
    p.inventarioVeioDaPausa = false;
}

void abrirPausa(EstadoPartida& p) {
    if (p.terminou || p.pendencia != Pendencia::Nenhuma) return;
    p.pendencia = Pendencia::Pausa;
    p.cursor = 0;
    p.confirmando = OpcaoPausa::Total;
}

const char* nomeOpcaoPausa(OpcaoPausa o, bool noClube) {
    static const char* const NOMES[TOTAL_OPCOES_PAUSA] = {"Continuar", "Inventário", "Ficha de atributos", "Salvar jogo",
                                                          "Abandonar a tentativa", "Sair do jogo"};
    if (noClube && o == OpcaoPausa::Abandonar) return "Voltar ao menu principal";
    const int i = static_cast<int>(o);
    return (i >= 0 && i < TOTAL_OPCOES_PAUSA) ? NOMES[i] : "?";
}

const char* nomeAba(AbaInventario a) {
    static const char* const NOMES[TOTAL_ABAS] = {"Itens", "Equipamento", "Missão", "Ecos", "Ficha"};
    const int i = static_cast<int>(a);
    return (i >= 0 && i < TOTAL_ABAS) ? NOMES[i] : "?";
}

std::string objetivoAtual(const EstadoPartida& p) {
    if (!p.emTentativa) return "Sala avulsa (teste): limpe a sala e saia pela porta.";
    const bool temChave = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] > 0;
    const int etapaResgate = 5, etapaChefe = 6;
    if (p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] > 0 && !p.manivelaEntregue &&
        p.etapa >= 3 && p.etapa <= 4) {
        return "A manivela é do Elias: entregue na loja (ele abre a grade do depósito).";
    }
    if (p.etapa < 3) return "Atravesse a galeria e encontre a Chave do Medo.";
    if (p.etapa == 3 && !temChave) return "Vença esta sala: a Chave do Medo está aqui.";
    if (p.etapa < etapaResgate || (p.etapa == etapaResgate && temChave)) return "Leve a Chave do Medo até a porta do resgate.";
    if (p.etapa == etapaResgate) return "A porta do Medo está aberta: siga em frente.";
    if (p.etapa == etapaChefe) return "Enfrente o medo que guarda a descida.";
    return "Desça para o próximo bioma.";
}

const OpcaoPorta* destinoDaPorta(const EstadoPartida& p, const PortaSala& porta, bool& descida) {
    descida = false;
    if (!p.emTentativa || porta.opcao < 0) return nullptr;
    const int proxima = p.etapa + 1;
    if (proxima >= static_cast<int>(p.plano.etapas.size())) {
        descida = true;
        return nullptr;
    }
    const std::vector<OpcaoPorta>& ops = p.plano.etapas[static_cast<std::size_t>(proxima)].opcoes;
    return porta.opcao < static_cast<int>(ops.size()) ? &ops[static_cast<std::size_t>(porta.opcao)] : nullptr;
}

Celula portaDoMedoProxima(const EstadoPartida& p) {
    const Celula b = celulaDe(p.bill.pes);
    for (int l = b.lin - 2; l <= b.lin + 2; ++l) {
        for (int c = b.col - 2; c <= b.col + 2; ++c) {
            if (!dentroDaSala(p.sala, c, l) || tileEm(p.sala, c, l).tipo != TipoTile::PortaTrancada) continue;
            if (comprimento(subtrair(centroCelula({c, l}), p.bill.pes)) <= RAIO_INTERAGIR) return {c, l};
        }
    }
    return {-1, -1};
}

bool abrirDialogo(EstadoPartida& p, const Dialogo* d) {
    if (!d || d->linhas.empty()) return false;
    p.dialogo = EstadoDialogo{};
    p.dialogo.atual = d;
    p.pendencia = Pendencia::Dialogo;
    p.cursor = 0;
    return true;
}

void pedirDica(EstadoPartida& p, int numero) {
    if (!p.dicasLigadas || numero < 1 || numero > TOTAL_DICAS || dicaVista(p.dicasVistas, numero)) return;
    p.dicasVistas |= 1u << (numero - 1);
    p.dicasNaFila.push_back(numero);
}

// Raio da luz: a lanterna na tentativa (+1 com o Mike); o Clube é iluminado, a sala toda aparece.
float raioDaLuz(const EstadoPartida& p) {
    if (p.noClube || p.salaIluminada) return 40.0f;   // o Clube; a sala do gerador que o Mike ligou
    const float r = RAIO_LANTERNA + static_cast<float>(bonusDeLuz(p) + valorEco(p, EfeitoEco::LuzGerador));
    // Covil (PRD §4.5): na fase de Georgie a sala escurece; nas Luzes Mortas a tela fica quase sem luz.
    if (p.luzesMortas) return std::min(r, 2.5f);
    if (p.georgie.ativo) return std::max(2.5f, r - 1.5f);
    return r;
}

const ObjetoSala* altarProximo(const EstadoPartida& p) {
    if (p.noClube || p.noPrologo) return nullptr;
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == '*' && !o.usado && comprimento(subtrair(centroCelula(o.celula), p.bill.pes)) <= RAIO_INTERAGIR) return &o;
    }
    return nullptr;
}

bool abrirEscolhaDeEco(EstadoPartida& p) {
    const int bioma = p.emTentativa ? p.plano.bioma : p.sala.modelo->bioma;
    p.ofertasEco = sortearEcos(p.ecos, amigoDoBioma(bioma), p.rng);
    if (p.ofertasEco.empty()) return false;
    p.pendencia = Pendencia::EscolherEco;
    p.cursor = 0;
    return true;
}

const std::string& nomeDoNpc(const EstadoPartida& p, const ObjetoSala& o) {
    static const std::string NENHUM;
    if (o.codigo < '1' || o.codigo > '9') return NENHUM;
    return p.sala.modelo->npcs[static_cast<std::size_t>(o.codigo - '0')];
}

const ObjetoSala* eliasProximo(const EstadoPartida& p) {
    if (p.noClube || p.noPrologo) return nullptr;
    for (const ObjetoSala& o : p.sala.objetos) {
        if (nomeDoNpc(p, o) != "elias") continue;
        if (comprimento(subtrair(centroCelula(o.celula), p.bill.pes)) <= RAIO_INTERAGIR + 8.0f) return &o;
    }
    return nullptr;
}

const ObjetoSala* amigoParaResgatar(const EstadoPartida& p) {
    if (!p.emTentativa) return nullptr;
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo < '1' || o.codigo > '9' || o.usado) continue;
        if (amigoDoCodigo(nomeDoNpc(p, o)) == Amigo::Total) continue;   // Elias e outros NPCs não são resgate
        if (comprimento(subtrair(centroCelula(o.celula), p.bill.pes)) <= RAIO_INTERAGIR) return &o;
    }
    return nullptr;
}

static int indiceDaSala(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

bool iniciarPrologo(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade ultima) {
    const int indice = indiceDaSala(c, ID_SALA_PROLOGO);
    if (indice < 0) return false;
    prepararPartida(p, c, seed, ultima);
    p.noPrologo = true;
    entrarNaSala(p, c, indice, nullptr);
    p.salaConcluida = false;   // a rua só abre depois de falar com o Mike
    definirPortas(p, false);
    for (const ObjetoSala& o : p.sala.objetos) {   // o "!" sobre quem tem a fala do prólogo
        if (o.codigo < '1' || o.codigo > '9') continue;
        const std::string& nome = p.sala.modelo->npcs[static_cast<std::size_t>(o.codigo - '0')];
        p.falaNova[static_cast<std::size_t>(o.codigo - '0')] = buscarDialogo(c.dialogos, "prologo_" + nome);
    }
    return true;
}

void avisarNoMenu(EstadoPartida& p, const std::string& t) {
    p.avisoMenu = t;
    p.tempoAvisoMenu = 1.6f;
}

int indiceDoClube(const Conteudo& c) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].tipo == TipoSala::Clube) return i;
    }
    return -1;
}

bool iniciarClube(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade ultima) {
    const int indice = indiceDoClube(c);
    if (indice < 0) return false;
    prepararPartida(p, c, seed, ultima);
    p.noClube = true;   // antes de entrar: a luz do Clube vale já no primeiro quadro
    entrarNaSala(p, c, indice, nullptr);
    return true;
}

const ObjetoSala* objetoDoClubeProximo(const EstadoPartida& p) {
    if (!p.noClube && !p.noPrologo) return nullptr;
    const ObjetoSala* melhor = nullptr;
    float menor = RAIO_INTERAGIR;
    for (const ObjetoSala& o : p.sala.objetos) {
        const bool interage = o.codigo == 'M' || o.codigo == 'Q' || o.codigo == 'H' || (o.codigo >= '1' && o.codigo <= '9' && !o.usado);
        if (!interage) continue;
        const float d = comprimento(subtrair(centroCelula(o.celula), p.bill.pes));
        if (d <= menor) {
            menor = d;
            melhor = &o;
        }
    }
    return melhor;
}

Amigo companheiroAoEntrar(int bioma) {
    // Resgates (data/salas/bN_resgate): Ben no 1, Stan no 2, Eddie no 3, Beverly no 4, Mike no 5, Richie no 6.
    static const Amigo AO_ENTRAR[8] = {Amigo::Mike, Amigo::Mike, Amigo::Ben, Amigo::Stan, Amigo::Eddie, Amigo::Beverly,
                                       Amigo::Mike, Amigo::Richie};
    return AO_ENTRAR[std::clamp(bioma, 0, 7)];
}

bool iniciarTentativa(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade dificuldade,
                      std::string& erro, int biomaInicial) {
    prepararPartida(p, c, seed, dificuldade);
    p.emTentativa = true;
    for (int bioma = std::clamp(biomaInicial, 1, 7); bioma <= 7; ++bioma) {
        if (!biomaJogavel(c.salas, bioma)) continue;
        if (!gerarBioma(c.salas, bioma, p.rngGerador, p.ajustes.pesoKit, p.plano, erro)) return false;
        p.etapa = 0;
        p.salaPrometida = p.plano.etapas[0].opcoes[0];
        p.segredoAtivo = sortearPorcentagem(p.rngGerador, p.ajustes.chanceSecreta);   // SDD §24.5
        entrarNaSala(p, c, p.salaPrometida.sala, nullptr);
        entrarCompanheiro(p, companheiroAoEntrar(bioma));   // bioma 1: o Mike vai junto (SDD §24.6)
        if (bioma > 1) chegarComoNaTentativa(p);       // começando mais adiante: Bill chega como chegaria ali
        p.pontoDaSala = capturarPonto(p, p.indiceSala);   // a 1ª sala já com o Mike (o ponto da entrada vinha sem ele)
        return true;
    }
    erro = "nenhum bioma tem sala de entrada e de combate";
    return false;
}

// Na arena AVULSA, Bill entra como chega na tentativa (telemetria: nível ~3 no chefe 1, ~5 no
// chefe 2), com os pontos onde o piloto os põe (Força e Agilidade). O PRD §10.2 dimensiona cada
// chefe para o nível de chegada; no nível 1 ele pode, e deve, perder às vezes.
// Salas comuns a partir do bioma 2 também: os inimigos escalam por bioma (Etapa 11.5), então Bill
// no nível 1 dentro dos Porões não é uma condição que exista no jogo (nível de entrada 3, 5, 7…).
void chegarComoNaTentativa(EstadoPartida& p) {
    const int bioma = p.sala.modelo->bioma;
    const bool chefe = p.sala.modelo->tipo == TipoSala::Chefe;
    if (!chefe && bioma < 2) return;
    const int nivel = chefe ? (bioma <= 1 ? 3 : 1 + 2 * bioma) : 1 + 2 * (bioma - 1);
    const int pontos = (nivel - 1) * PONTOS_POR_NIVEL;
    p.bill.progresso.xp = LIMIARES_XP[std::min(nivel, NIVEL_MAXIMO)];
    p.bill.progresso.nivel = std::min(nivel, NIVEL_MAXIMO);
    p.bill.attr.forca += pontos / 2;
    p.bill.attr.agilidade += pontos - pontos / 2;
    // A partir do bioma 2 ele chega com o kit que a telemetria mede (~1,3 usados por tentativa).
    if (p.sala.modelo->bioma >= 2) p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)] = 1;
    recalcularJogador(p.bill);
    // E com a VIDA MÁXIMA que a tentativa dá (Ecos do Ben, compras, melhorias): só com os pontos em
    // Força/Agilidade, Bill tinha 46 no bioma 4, contra ~77 medidos na tentativa (12.5). Medido na
    // simulação de 50 no Normal (hp_max ao entrar): salas comuns e arena de cada bioma.
    // Bioma 5 medido na 13.3 (simulação de 50 no Normal): salas 82, arena 84.
    constexpr int VIDA_SALA[6] = {0, 47, 52, 64, 74, 82}, VIDA_CHEFE[6] = {0, 47, 57, 69, 77, 84};
    const int b = std::clamp(bioma, 0, 5);
    const int alvo = chefe ? VIDA_CHEFE[b] : VIDA_SALA[b];
    if (p.bill.stats.hpMax < alvo) {
        p.bill.equip.ecoHp += alvo - p.bill.stats.hpMax;
        recalcularJogador(p.bill);
    }
    p.bill.hp = p.bill.stats.hpMax;
}

bool itemEncostaNoBill(Vec2 pesBill, Vec2 item) {
    const float dx = item.x - pesBill.x, dy = item.y - pesBill.y;
    if (dx * dx + dy * dy <= RAIO_COLETA * RAIO_COLETA) return true;
    return std::fabs(dx) <= COLETA_LARGURA && dy >= -COLETA_ACIMA && dy <= COLETA_ABAIXO;
}

void soltarItem(EstadoPartida& p, TipoItem tipo, int quantidade, Vec2 origem, Vec2 destino) {
    if (tipo == TipoItem::Total || quantidade <= 0) return;
    if (tipo == TipoItem::Moeda && p.bonusMoedasPct > 0) quantidade = aplicarPorcentagem(quantidade, 100 + p.bonusMoedasPct);
    ItemNoChao it;
    it.id = p.proximoId++;
    it.tipo = tipo;
    it.quantidade = quantidade;
    it.origem = origem;
    it.pos = destino;
    p.itensChao.push_back(it);
}

bool bauTrancado(const EstadoPartida& p) { return p.sala.modelo && p.sala.modelo->tipo == TipoSala::Tesouro; }

const ObjetoSala* bauProximo(const EstadoPartida& p) {
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == 'B' && !o.usado && comprimento(subtrair(centroCelula(o.celula), p.bill.pes)) <= RAIO_INTERAGIR) return &o;
    }
    return nullptr;
}

int orcamentoDaOnda(const ParametrosDificuldade& d, int orcamentoSala) {
    const int base = orcamentoSala > 0 ? orcamentoSala : ORCAMENTO_PADRAO;
    return std::max(1, aplicarPorcentagem(base, d.orcamentoOnda));
}

TipoInimigo chefeDoBioma(int bioma) {
    switch (bioma) {
        case 1: return TipoInimigo::Mumia;
        case 2: return TipoInimigo::MedoAfogado;   // Etapa 10.3
        case 3: return TipoInimigo::Leproso;       // Etapa 11.4
        case 4: return TipoInimigo::RaloSangue;    // Etapa 12.3
        case 5: return TipoInimigo::Passaro;       // Etapa 13.3
        case 6: return TipoInimigo::Lobisomem;     // Etapa 14.2
        case 7: return TipoInimigo::PennywiseIV;   // Etapa 15: o Covil
        default: return TipoInimigo::Total;
    }
}

const Inimigo* chefeNaSala(const EstadoPartida& p) {
    for (const Inimigo& i : p.inimigos) {
        if (i.vivo && parametrosDe(i.tipo).chefe) return &i;
    }
    return nullptr;
}

long long pontuacaoAtual(const EstadoPartida& p) {
    return calcularPontuacao(p.stats, p.ajustes.multiplicadorPontos);
}

int hostisVivos(const EstadoPartida& p) {
    int n = 0;
    for (const Inimigo& i : p.inimigos) n += i.vivo ? 1 : 0;
    return n;
}

const Inimigo* buscarInimigo(const EstadoPartida& p, IdEntidade id) {
    for (const Inimigo& i : p.inimigos) {
        if (i.id == id) return &i;
    }
    return nullptr;
}

void iniciarPartida(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, int salaInicial, Dificuldade dificuldade) {
    prepararPartida(p, c, seed, dificuldade);
    entrarNaSala(p, c, salaInicial, nullptr);
}

void entrarNaSala(EstadoPartida& p, const Conteudo& c, int indice, const PortaSala* vindoPor) {
    if (p.emTentativa) p.pontoDaSala = capturarPonto(p, indice);   // o que "Salvar jogo" grava (23/11)
    const int total = static_cast<int>(c.salas.size());
    p.indiceSala = ((indice % total) + total) % total;
    p.sala = criarSalaViva(c.salas[static_cast<std::size_t>(p.indiceSala)]);
    p.inimigos.clear();
    p.golpes.clear();
    p.avisos.clear();
    p.textos.clear();
    p.itensChao.clear();   // o que ficou no chão da sala anterior se perde
    p.brasas.clear();
    p.pocas.clear();
    p.nuvens.clear();
    p.pocasDeSangue.clear();   // a sala nova é outra cópia do mapa: nada a restaurar
    p.tracos.clear();
    p.estilhacos.clear();
    p.efeitosAtaque.clear();
    p.salaIluminada = false;
    p.recargaBrasaBill = 0.0f;
    p.danoNaSala = false;
    p.gessoUsado = false;
    p.nevoasDash.clear();
    p.vidaNevoas.clear();
    prepararArmadilhas(p);   // as opcionais ficam ou não (dificuldade)
    // Assombrado (PRD §9): a partir do bioma 2, no máximo 1 por sala de combate, pela dificuldade.
    p.eliteNaSala = p.sala.modelo->tipo == TipoSala::Combate && p.sala.modelo->bioma >= 2 &&
                    sortearPorcentagem(p.rng, p.ajustes.chanceElite);
    // Segredo: só a 1ª sala com parede rachada do bioma o guarda (e só se a chance sorteou).
    // Fora da tentativa (sala avulsa, testes), a parede rachada sempre vale.
    bool temRachada = false;
    for (const Tile& t : p.sala.tiles) temRachada |= t.tipo == TipoTile::ParedeRachada;
    p.salaTemSegredo = temRachada && (!p.emTentativa || (p.segredoAtivo && !p.segredoUsado));
    if (p.salaTemSegredo && p.emTentativa) p.segredoUsado = true;
    if (temRachada && !p.salaTemSegredo) {
        for (Tile& t : p.sala.tiles) {
            if (t.tipo == TipoTile::ParedeRachada) t = criarTile(TipoTile::Parede);
        }
    }
    prepararObjetos(p);
    Vec2 destino = centroCelula(p.sala.modelo->entradaJogador);
    int portaEntrada = -1;
    if (vindoPor && p.sala.modelo->entradaFixa) {   // mão única: entra pelo 'E'; a porta dele fica selada
        float menor = 1e9f;
        for (int i = 0; i < static_cast<int>(p.sala.portas.size()); ++i) {
            const float d = comprimento(subtrair(centroCelula(p.sala.portas[static_cast<std::size_t>(i)].celula), destino));
            if (d < menor) { menor = d; portaEntrada = i; }
        }
        p.sons.push_back(IdSom::TrocaSala);
    } else if (vindoPor) {
        const Direcao lado = ladoOposto(vindoPor->lado);
        for (int i = 0; i < static_cast<int>(p.sala.portas.size()); ++i) {
            const PortaSala& porta = p.sala.portas[static_cast<std::size_t>(i)];
            if (porta.lado != lado) continue;
            destino = somar(centroCelula(porta.celula), escalar(vetorDaDirecao(vindoPor->lado), TILE * 1.5f));
            portaEntrada = i;
            break;
        }
        p.sons.push_back(IdSom::TrocaSala);
    }
    configurarPortas(p, portaEntrada);
    posicionarJogador(p.bill, destino);
    p.bill.direcao = vindoPor ? vindoPor->lado : Direcao::Baixo;
    companheiroEntrouNaSala(p);
    iniciarVisao(p.visao, p.sala);
    atualizarVisao(p.visao, p.sala, p.bill.pes, raioDaLuz(p));
    // Sala com ondas: as portas fecham até limpar (combate obrigatório, como em Hades).
    p.ondaAtual = 0;
    p.tentativasPorta = 0;   // a insistência e o aviso são da sala anterior
    p.imitacaoUsada = false; // Eco "Imitação" (Richie): 1× por sala
    p.georgie = GeorgieIlusao{};   // o Covil (Etapa 15) só vale na arena dele
    p.luzesMortas = false;
    p.timerVitoria = 0.0f;
    p.empurrandoPorta = false;
    p.tempoAvisoPorta = 0.0f;
    p.salaConcluida = p.sala.modelo->ondas <= 0 || p.sala.modelo->spawns.empty();
    if (!p.salaConcluida) p.timerOnda = ATRASO_PRIMEIRA_ONDA;
    definirPortas(p, p.salaConcluida);   // fecha no combate; seladas ficam fechadas sempre
    // Festa dos Balões: portas fechadas por 30 s (SDD §24.5).
    p.festaAtiva = p.sala.modelo->tipo == TipoSala::Bonus;
    p.baloes.clear();
    if (p.festaAtiva) {
        p.tempoFesta = DURACAO_FESTA;
        p.proximoBalao = 1.0f;
        p.baloesEstourados = 0;
        p.salaConcluida = false;
        definirPortas(p, false);
        texto(p, {p.bill.pes.x, p.bill.pes.y - 70.0f}, "Festa dos Balões! Estoure os coloridos; fuja dos vermelhos", TipoTexto::Critico);
    }
    // Loja do Elias: as ofertas desta visita (PRD §11.3).
    p.loja.clear();
    if (p.sala.modelo->tipo == TipoSala::Loja) p.loja = montarLoja(p, p.rngGerador);
    // Refúgio (área segura): cura ao entrar (PRD §5.2).
    if (p.sala.modelo->tipo == TipoSala::Refugio) {
        const int cura = p.bill.stats.hpMax * CURA_REFUGIO_PCT / 100;
        p.bill.hp = std::min(p.bill.stats.hpMax, p.bill.hp + cura);
        p.bill.cura = 0.4f;
        texto(p, {p.bill.pes.x, p.bill.pes.y - 60.0f}, "Refúgio: +" + std::to_string(cura) + " de vida", TipoTexto::Info);
    }
    p.trocouDeSala = true;
}

// ---------- Covil (Etapa 15, PRD §4.5) ----------
Vec2 centroLuzMorta(const EstadoPartida& p, int i) {
    const Vec2 centro{static_cast<float>(p.sala.cols * TILE) / 2.0f, static_cast<float>(p.sala.lins * TILE) / 2.0f};
    const float a = p.anguloLuzes + 6.2831853f * static_cast<float>(i) / static_cast<float>(std::max(1, p.quantasLuzes));
    return {centro.x + ORBITA_LUZES * std::cos(a), centro.y + ORBITA_LUZES * std::sin(a)};
}

bool dentroDeLuzMorta(const EstadoPartida& p, Vec2 pos) {
    for (int i = 0; i < p.quantasLuzes; ++i) {
        if (comprimento(subtrair(pos, centroLuzMorta(p, i))) <= RAIO_LUZ_MORTA) return true;
    }
    return false;
}

// Georgie chama e puxa; as Luzes Mortas giram e a escuridão fere; a vitória chega depois da Coisa se quebrar.
void atualizarCovil(EstadoPartida& p, float dt) {
    if (p.timerVitoria > 0.0f) {
        p.timerVitoria -= dt;
        if (p.timerVitoria <= 0.0f) {
            p.vitoria = true;
            p.terminou = true;
        }
        return;
    }
    Jogador& b = p.bill;
    if (p.georgie.ativo && estaVivo(b)) {
        GeorgieIlusao& g = p.georgie;
        g.anim += dt;
        if (g.recarga > 0.0f) g.recarga -= dt;
        const Vec2 d = subtrair(b.pes, g.pes);
        const float dist = comprimento(d);
        if (dist > 2.0f) {
            g.pes = moverComColisao(p.sala, g.pes, CAIXA_INIMIGO, escalar(normalizar(d), VELOCIDADE_GEORGIE * dt), false);
            g.direcao = direcaoDominante(d, g.direcao);
            // A voz puxa Bill de leve (PRD §4.5); o dash escapa.
            if (b.dashRestante <= 0.0f)
                posicionarJogador(b, moverComColisao(p.sala, b.pes, CAIXA_BILL, escalar(normalizar(d), -PUXAO_GEORGIE * dt), false));
        }
        if ((g.chamado -= dt) <= 0.0f) {
            g.chamado = 6.0f;
            texto(p, {g.pes.x, g.pes.y - 60.0f}, "Billy…", TipoTexto::Critico);
        }
        if (dist <= RAIO_TOQUE_GEORGIE && g.recarga <= 0.0f) {   // tocar a ilusão fere; ela some e volta longe
            Golpe golpe;
            golpe.id = p.proximoGolpe++;
            golpe.equipe = Equipe::Hostil;
            golpe.area = areaCirculo(b.pes, 14.0f);
            golpe.dano = DANO_GEORGIE;
            golpe.rolaAcerto = false;
            golpe.recuo = 60.0f;
            golpe.duracao = 0.05f;
            p.golpes.push_back(golpe);
            g.recarga = 1.5f;
            g.pes = spawnMaisLonge(p);
            texto(p, {b.pes.x, b.pes.y - 70.0f}, "Não é ele!", TipoTexto::Info);
        }
    }
    if (p.luzesMortas) {
        p.anguloLuzes += VELOCIDADE_LUZES * dt;
        if ((p.tickEscuridao -= dt) <= 0.0f) {
            p.tickEscuridao = INTERVALO_ESCURIDAO;
            if (estaVivo(b) && !dentroDeLuzMorta(p, b.pes)) {   // fora da luz dos amigos: a escuridão fere
                Golpe golpe;
                golpe.id = p.proximoGolpe++;
                golpe.equipe = Equipe::Hostil;
                golpe.terreno = true;
                golpe.area = areaCirculo(b.pes, 8.0f);
                golpe.dano = DANO_ESCURIDAO;
                golpe.rolaAcerto = false;
                golpe.duracao = 0.05f;
                p.golpes.push_back(golpe);
            }
        }
    }
}

// ---------- porta fechada: por que não abre? (pedido do Diego, 23/11) ----------
constexpr float SONDA_PORTA = 14.0f;            // px à frente dos pés, na direção do comando
constexpr float NOVA_TENTATIVA_SEGURANDO = 1.2f; // segurar contra a porta conta como outra tentativa
constexpr float ESQUECER_TENTATIVAS = 5.0f;      // sem insistir por 5 s, a contagem zera
constexpr int TENTATIVAS_PARA_AVISO = 2;
constexpr float DURACAO_AVISO_PORTA = 5.0f;
constexpr float RECARGA_AVISO_PORTA = 6.0f;      // depois de avisar, só avisa de novo passado este tempo

// A porta fechada para onde Bill está empurrando agora (nullptr = nenhuma).
const PortaSala* portaFechadaEmpurrada(const EstadoPartida& p, const EstadoEntrada& entrada) {
    if (comprimento(entrada.eixo) < 0.3f) return nullptr;
    const Celula alvo = celulaDe(somar(p.bill.pes, escalar(normalizar(entrada.eixo), SONDA_PORTA)));
    for (const PortaSala& porta : p.sala.portas) {
        if (!porta.aberta && porta.celula.col == alvo.col && porta.celula.lin == alvo.lin) return &porta;
    }
    return nullptr;
}

std::string motivoDaPortaFechada(const EstadoPartida& p, const PortaSala& porta) {
    const std::string moldura = " Quando a moldura da porta ficar dourada, ela abre.";
    if (p.noPrologo) return "A rua ainda está fechada. Antes de seguir, fale com o Mike.";
    if (porta.selada && p.salaConcluida) return "Esta porta ficou selada. Siga por uma das portas abertas, de moldura dourada.";
    if (p.festaAtiva) return "Festa dos Balões: as portas só abrem quando o tempo da festa acabar.";
    if (p.sala.modelo->tipo == TipoSala::Chefe) return "Porta trancada. Derrote o chefe para abrir a saída." + moldura;
    if (!p.salaConcluida && p.sala.modelo->ondas > 0) {
        const int vivos = hostisVivos(p);
        const int onda = std::max(1, p.ondaAtual), total = p.sala.modelo->ondas;
        std::string t = "Porta trancada. Derrote TODOS os inimigos da sala";
        if (total > 1) t += " (onda " + std::to_string(onda) + " de " + std::to_string(total) + ")";
        if (vivos > 0) t += ": faltam " + std::to_string(vivos) + " nesta onda.";
        else t += ": a próxima onda está chegando.";
        return t + moldura;
    }
    return "Esta porta ainda está fechada.";
}

// Conta as tentativas de passar por uma porta fechada; na 2ª seguida, explica por que ela não abre.
void insistirNaPorta(EstadoPartida& p, const EstadoEntrada& entrada, float dt) {
    if (p.tempoAvisoPorta > 0.0f) p.tempoAvisoPorta -= dt;
    if (p.recargaAvisoPorta > 0.0f) p.recargaAvisoPorta -= dt;
    const PortaSala* porta = portaFechadaEmpurrada(p, entrada);
    if (!porta) {
        p.empurrandoPorta = false;
        p.tempoEmpurrando = 0.0f;
        p.desdeTentativa += dt;
        if (p.desdeTentativa > ESQUECER_TENTATIVAS) p.tentativasPorta = 0;
        return;
    }
    p.tempoEmpurrando += dt;
    if (!p.empurrandoPorta || p.tempoEmpurrando >= NOVA_TENTATIVA_SEGURANDO) {   // um empurrão novo
        ++p.tentativasPorta;
        p.tempoEmpurrando = 0.0f;
        p.desdeTentativa = 0.0f;
    }
    p.empurrandoPorta = true;
    if (p.tentativasPorta >= TENTATIVAS_PARA_AVISO && p.recargaAvisoPorta <= 0.0f) {
        p.avisoPorta = motivoDaPortaFechada(p, *porta);
        p.tempoAvisoPorta = DURACAO_AVISO_PORTA;
        p.recargaAvisoPorta = RECARGA_AVISO_PORTA;
        p.tentativasPorta = 0;
    }
}

void atualizarPartida(EstadoPartida& p, const Conteudo& c, const EstadoEntrada& entrada, float dt) {
    p.trocouDeSala = false;
    atualizarTextos(p, dt);
    for (Dispersao& d : p.dispersoes) d.vida -= dt;
    p.dispersoes.erase(std::remove_if(p.dispersoes.begin(), p.dispersoes.end(), [](const Dispersao& d) { return d.vida <= 0.0f; }),
                       p.dispersoes.end());
    if (p.terminou) return;
    // Pendência aberta: o mundo congela e o cronômetro NÃO conta (SDD §24.3).
    if (p.tempoAvisoMenu > 0.0f) p.tempoAvisoMenu -= dt;
    if (p.balao.vida > 0.0f) p.balao.vida -= dt;
    if (p.pendencia != Pendencia::Nenhuma) {
        atualizarPendencia(p, c, entrada, dt);
        return;
    }
    // Tutorial: a próxima dica da fila abre quando não há outro menu (uma de cada vez).
    while (!p.dicasNaFila.empty()) {
        const int n = p.dicasNaFila.front();
        p.dicasNaFila.erase(p.dicasNaFila.begin());
        if (abrirDialogo(p, buscarDialogo(c.dialogos, idDaDica(n)))) return;
    }
    if (pressionou(entrada, Comando::Pausa)) {
        abrirPausa(p);
        return;
    }
    if (pressionou(entrada, Comando::Inventario)) {
        abrirInventario(p, false);
        return;
    }
    if (pressionou(entrada, Comando::Ficha) && p.bill.progresso.pontosPendentes > 0) {
        abrirPendenciaDeNivel(p);
        return;
    }
    if (p.noClube || p.noPrologo) {   // Clube e prólogo: andar e interagir, sem cronômetro nem inimigos
        if (pressionou(entrada, Comando::Interagir)) interagir(p, c);
        if (p.pendencia != Pendencia::Nenhuma) return;
        atualizarJogador(p.bill, p.sala, entrada, dt);
        insistirNaPorta(p, entrada, dt);
        atualizarCompanheiro(p, dt);
        atualizarVisao(p.visao, p.sala, p.bill.pes, raioDaLuz(p));
        verificarDicas(p);
        // Prólogo: a rua termina na entrada da galeria (a porta pede a tentativa).
        if (p.noPrologo && p.salaConcluida && portaTocada(p.sala, p.bill.pes, CAIXA_BILL)) p.pedidoTentativa = true;
        return;
    }
    p.stats.tempo += dt;
    // 1. Hit-stop: um instante congelado dá peso ao impacto. O toque de Atacar NÃO se perde aqui
    // (buffer): no combo, o 2º toque caía quase sempre no congelamento do 1º acerto.
    if (p.hitStop > 0.0f) {
        if (pressionou(entrada, Comando::Atacar)) p.bill.bufferAtaque = BUFFER_ATAQUE;
        if (pressionou(entrada, Comando::Especial)) p.bill.bufferEspecial = BUFFER_ATAQUE;
        if (pressionou(entrada, Comando::Clarao)) p.bill.bufferClarao = BUFFER_ATAQUE;
        p.hitStop -= dt;
        return;
    }
    // 2. Bill: ações (golpes) e movimento.
    if (pressionou(entrada, Comando::UsarKit)) usarKitRapido(p);
    if (pressionou(entrada, Comando::Interagir)) interagir(p, c);
    const AcaoJogador acaoAntes = p.bill.acao;
    // Por que o Especial/Clarão não saiu: dito na tela (antes, o toque era ignorado e parecia atraso).
    if (pressionou(entrada, Comando::Especial) && p.bill.recargaEspecial > 0.35f) {
        char t[48];
        std::snprintf(t, sizeof t, "Especial em %.1f s", static_cast<double>(p.bill.recargaEspecial));
        texto(p, {p.bill.pes.x, p.bill.pes.y - 58.0f}, t, TipoTexto::Errou);
    }
    if (pressionou(entrada, Comando::Clarao) && p.bill.cargasLanterna <= 0)
        texto(p, {p.bill.pes.x, p.bill.pes.y - 58.0f}, "Lanterna sem carga", TipoTexto::Errou);
    acoesDoJogador(p.bill, entrada, dt, p.golpes, p.proximoGolpe, p.sons);
    if (p.bill.acao == AcaoJogador::Especial && acaoAntes != AcaoJogador::Especial) {
        if (const int prata = valorEco(p, EfeitoEco::EspecialEstilingue); prata > 0) dispararEstilingue(p, prata);
    }
    atualizarEstilhacos(p, dt);
    for (EfeitoAtaque& e : p.efeitosAtaque) e.vida -= dt;
    p.efeitosAtaque.erase(std::remove_if(p.efeitosAtaque.begin(), p.efeitosAtaque.end(), [](const EfeitoAtaque& e) { return e.vida <= 0.0f; }),
                          p.efeitosAtaque.end());
    atualizarQueimaduras(p, dt);
    for (Traco& t : p.tracos) t.vida -= dt;
    p.tracos.erase(std::remove_if(p.tracos.begin(), p.tracos.end(), [](const Traco& t) { return t.vida <= 0.0f; }), p.tracos.end());
    atualizarJogador(p.bill, p.sala, entrada, dt);
    insistirNaPorta(p, entrada, dt);
    atualizarPowerUps(p.bill, dt);
    atualizarBarreirasDoDash(p, dt);
    atualizarCompanheiro(p, dt);
    coletarItens(p, dt);
    p.stats.M = p.bill.movimentos;
    atualizarVisao(p.visao, p.sala, p.bill.pes, raioDaLuz(p));
    // 3. Golpes ativos contra os alvos (Bill → inimigos; hostis → Bill; balão → todos).
    resolverGolpes(p);
    atualizarFesta(p, dt);
    // 5. Morte do Bill ANTES de qualquer outra coisa (pacto ou fim).
    verificarMorteDoBill(p);
    if (p.terminou) return;
    // 7. IA dos inimigos (criam avisos e golpes).
    std::vector<Inimigo> nascidos;   // ratos que o chefe invocou: entram depois do laço
    const int hostis = hostisVivos(p);
    for (Inimigo& ini : p.inimigos) {   // o provocado pelo Richie mira nele (o companheiro é invulnerável)
        Vec2 alvo = provocado(p, ini.id) ? p.companheiro.pes : p.bill.pes;
        // Eco "Imitação" (Richie): o aliado mira no inimigo mais perto, e o que ele soltar fere inimigos.
        const bool imitando = ini.aliado > 0.0f;
        if (imitando) {
            ini.aliado -= dt;
            alvo = ini.pes;   // ninguém por perto: fica parado
            float melhor = 1e9f;
            for (const Inimigo& o : p.inimigos) {
                if (&o == &ini || !o.vivo || o.aliado > 0.0f || o.surgindo > 0.0f) continue;
                const float d = comprimento(subtrair(o.pes, ini.pes));
                if (d < melhor) { melhor = d; alvo = o.pes; }
            }
            if (ini.aliado <= 0.0f) texto(p, {ini.pes.x, ini.pes.y - 50.0f}, "A imitação acabou", TipoTexto::Errou);
        }
        const std::size_t avisosAntes = p.avisos.size(), golpesAntes = p.golpes.size();
        ContextoIA ctx{p.sala, alvo, estaVivo(p.bill), p.rng, p.avisos, p.golpes, p.proximoGolpe, &nascidos, &p.proximoId, hostis};
        ctx.vivos = &p.inimigos;
        ctx.cacar = hostis <= LIMITE_HOSTIS_CACA;
        atualizarInimigo(ini, ctx, dt);
        if (imitando) {   // o que o aliado soltou neste passo é do lado de Bill
            for (std::size_t a = avisosAntes; a < p.avisos.size(); ++a) p.avisos[a].golpe.equipe = Equipe::Jogador;
            for (std::size_t g = golpesAntes; g < p.golpes.size(); ++g) p.golpes[g].equipe = Equipe::Jogador;
        }
    }
    p.inimigos.insert(p.inimigos.end(), nascidos.begin(), nascidos.end());
    separarAtores(p);
    // Inimigos × itens: o rato rouba, o espectro queima, a sombra esmaga caixas.
    atualizarLadroes(p, dt);
    atualizarBrasas(p, dt);
    atualizarPocas(p, dt);
    atualizarPocasDeSangue(p, dt);
    atualizarArmadilhas(p, dt);   // F14: pregos e piso cedendo (Bill e inimigos terrestres)
    // Nuvens pútridas: Bill dentro anda a 50% enquanto estiver lá (o dash continua: é a saída).
    for (NuvemPutrida& n : p.nuvens) {
        n.vida -= dt;
        if (comprimento(subtrair(p.bill.pes, n.centro)) <= n.raio) p.bill.lento = std::max(p.bill.lento, 0.15f);
    }
    p.nuvens.erase(std::remove_if(p.nuvens.begin(), p.nuvens.end(), [](const NuvemPutrida& n) { return n.vida <= 0.0f; }),
                   p.nuvens.end());
    sombrasEsmagamCaixas(p);
    // 8. Avisos que chegaram a zero viram golpes.
    atualizarAvisos(p, dt);
    // Os golpes duram pouco; os que acabaram saem.
    for (Golpe& g : p.golpes) g.duracao -= dt;
    p.golpes.erase(std::remove_if(p.golpes.begin(), p.golpes.end(), [](const Golpe& g) { return g.duracao <= 0.0f; }),
                   p.golpes.end());
    // 10. Mortos saem da lista (nenhum ponteiro é guardado entre quadros).
    p.inimigos.erase(std::remove_if(p.inimigos.begin(), p.inimigos.end(), [](const Inimigo& i) { return !i.vivo; }),
                     p.inimigos.end());
    // 11. Ondas e conclusão da sala. O Covil (Georgie, Luzes Mortas, vitória) antes.
    atualizarCovil(p, dt);
    if (p.terminou) return;
    atualizarOndas(p, dt);
    verificarDicas(p);
    // 12. Porta aberta: na tentativa, o plano decide o destino; avulsa, a próxima do arquivo.
    if (p.salaConcluida && p.timerVitoria <= 0.0f) {   // depois do Pennywise IV não há porta: vem a vitória
        if (const PortaSala* porta = portaTocada(p.sala, p.bill.pes, CAIXA_BILL)) {
            const PortaSala copia = *porta;   // a sala vai ser trocada: não guardar o ponteiro
            if (p.emTentativa) seguirPorta(p, c, copia);
            else entrarNaSala(p, c, p.indiceSala + 1, &copia);
        }
    }
}
