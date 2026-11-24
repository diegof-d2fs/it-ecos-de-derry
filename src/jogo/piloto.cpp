#include "jogo/piloto.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "jogo/armadilhas.h"
#include "mundo/colisao.h"

namespace {

constexpr float ALCANCE_TACO = 34.0f;          // do corpo do Bill ao corpo do alvo
constexpr float MEMORIA_ALVO = 1.5f;           // s indo ao último lugar onde viu o alvo
constexpr float TROCA_ALVO = 0.6f;             // troca de alvo só se o novo estiver a < 60% da distância
constexpr float INTERVALO_REPLANEJAR = 0.3f;
constexpr float TEMPO_TRAVADO = 1.2f;
constexpr int DIRECOES_FUGA = 16;

// ---------- mapa como o jogador o conhece ----------
// Célula nunca vista: "provavelmente livre" (uma pessoa também supõe isso). Vista: o que ela é.
bool passavelConhecido(const EstadoPartida& p, int c, int l) {
    if (!dentroDaSala(p.sala, c, l)) return false;
    if (!visaoEm(p.visao, c, l).explorada) return true;
    return !tileEm(p.sala, c, l).bloqueiaMovimento;
}

// Armadilha que o piloto CONHECE (o que a pessoa também vê): pregos sempre; o piso cedendo só se
// revelado (Stan acompanhando ou Óculos do Richie) — escondido, ele cai nela como qualquer um.
bool armadilhaConhecida(const EstadoPartida& p, int c, int l) {
    if (!visaoEm(p.visao, c, l).explorada) return false;
    for (const Brasa& b : p.brasas) {   // brasas do Espectro (visíveis, 6/s): contorna
        if (comprimento(subtrair(centroCelula({c, l}), b.pos)) <= RAIO_BRASA + 10.0f) return true;
    }
    for (const NuvemPutrida& n : p.nuvens) {   // nuvem pútrida (visível): contorna
        if (comprimento(subtrair(centroCelula({c, l}), n.centro)) <= n.raio) return true;
    }
    const TipoTile t = tileEm(p.sala, c, l).tipo;
    if (t == TipoTile::Espinhos || t == TipoTile::PocaSangue || t == TipoTile::Fogo || t == TipoTile::Labareda) return true;   // a grelha se vê
    return t == TipoTile::PisoCedendo && (revelaArmadilhas(p) || p.bill.oculos > 0.0f);
}

// BFS a partir do Bill pelo mapa conhecido: distância e de onde se veio (para montar o caminho).
struct Busca {
    std::array<short, SALA_MAX_CELULAS> dist;
    std::array<short, SALA_MAX_CELULAS> veioDe;
};

// Por padrão, evita as armadilhas conhecidas: TODA camada do piloto planeja do mesmo jeito que o
// seguir replaneja (camadas com regras diferentes brigavam a cada quadro e o piloto oscilava — 11.1).
// Só planejar e explorar tentam de novo passando por elas, quando não há outro jeito.
void buscarAPartirDe(const EstadoPartida& p, Celula origem, Busca& b, bool evitarArmadilhas = true) {
    b.dist.fill(-1);
    b.veioDe.fill(-1);
    const int cols = p.sala.cols;
    std::array<short, SALA_MAX_CELULAS> fila{};
    int ini = 0, fim = 0;
    const int o = origem.lin * cols + origem.col;
    if (!dentroDaSala(p.sala, origem.col, origem.lin)) return;
    b.dist[static_cast<std::size_t>(o)] = 0;
    fila[static_cast<std::size_t>(fim++)] = static_cast<short>(o);
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    while (ini < fim) {
        const int atual = fila[static_cast<std::size_t>(ini++)];
        const int c = atual % cols, l = atual / cols;
        for (int k = 0; k < 4; ++k) {
            const int nc = c + dc[k], nl = l + dl[k];
            if (!passavelConhecido(p, nc, nl)) continue;
            if (evitarArmadilhas && armadilhaConhecida(p, nc, nl)) continue;
            const int i = nl * cols + nc;
            if (b.dist[static_cast<std::size_t>(i)] >= 0) continue;
            b.dist[static_cast<std::size_t>(i)] = static_cast<short>(b.dist[static_cast<std::size_t>(atual)] + 1);
            b.veioDe[static_cast<std::size_t>(i)] = static_cast<short>(atual);
            fila[static_cast<std::size_t>(fim++)] = static_cast<short>(i);
        }
    }
}

std::vector<Celula> montarCaminho(const EstadoPartida& p, const Busca& b, Celula destino) {
    std::vector<Celula> inverso;
    const int cols = p.sala.cols;
    int i = destino.lin * cols + destino.col;
    if (!dentroDaSala(p.sala, destino.col, destino.lin) || b.dist[static_cast<std::size_t>(i)] < 0) return {};
    while (b.dist[static_cast<std::size_t>(i)] > 0) {
        inverso.push_back({i % cols, i / cols});
        i = b.veioDe[static_cast<std::size_t>(i)];
    }
    return std::vector<Celula>(inverso.rbegin(), inverso.rend());
}

bool planejar(const EstadoPartida& p, MemoriaPiloto& m, Celula destino);

// O segmento reto a→b não passa por armadilha conhecida (amostra a cada 8 px).
bool segmentoSemArmadilha(const EstadoPartida& p, Vec2 a, Vec2 b) {
    const Vec2 d = subtrair(b, a);
    const int passos = std::max(1, static_cast<int>(comprimento(d) / 8.0f));
    for (int i = 0; i <= passos; ++i) {
        const Celula c = celulaDe(somar(a, escalar(d, static_cast<float>(i) / static_cast<float>(passos))));
        if (armadilhaConhecida(p, c.col, c.lin)) return false;
    }
    return true;
}

// Segue o caminho: mira o centro da próxima célula; ao chegar perto, passa para a seguinte.
// Caminho feito antes de ver uma armadilha (célula inexplorada conta como livre): se o próximo
// trecho entra numa armadilha que agora ele CONHECE, replaneja (Etapa 11.1).
Vec2 seguir(const EstadoPartida& p, MemoriaPiloto& m, Vec2 alvoFinal) {
    for (std::size_t k = 0; k < m.caminho.size() && k < 3; ++k) {
        if (armadilhaConhecida(p, m.caminho[k].col, m.caminho[k].lin)) {
            planejar(p, m, m.destino);
            break;
        }
    }
    const Vec2 pes = p.bill.pes;
    while (!m.caminho.empty()) {
        const Celula c = m.caminho.front();
        const Celula aqui = celulaDe(pes);
        if ((aqui.col == c.col && aqui.lin == c.lin) && m.caminho.size() > 1) {
            m.caminho.erase(m.caminho.begin());
            continue;
        }
        break;
    }
    const Vec2 alvo = m.caminho.size() > 1 ? centroCelula(m.caminho.front()) : alvoFinal;
    const Vec2 d = subtrair(alvo, pes);
    return comprimento(d) < 2.0f ? Vec2{} : normalizar(d);
}

bool planejar(const EstadoPartida& p, MemoriaPiloto& m, Celula destino) {
    Busca b;
    buscarAPartirDe(p, celulaDe(p.bill.pes), b, true);   // primeiro, sem pisar em armadilha conhecida
    m.caminho = montarCaminho(p, b, destino);
    if (m.caminho.empty()) {                              // não há outro jeito: passa por ela
        buscarAPartirDe(p, celulaDe(p.bill.pes), b, false);
        m.caminho = montarCaminho(p, b, destino);
    }
    m.destino = destino;
    return !m.caminho.empty() || (destino.col == celulaDe(p.bill.pes).col && destino.lin == celulaDe(p.bill.pes).lin);
}

// ---------- percepção ----------
// Visível na luz, ou revelado pelos Óculos do Richie (o jogador também os vê: é justo).
bool inimigoPercebido(const EstadoPartida& p, const Inimigo& i) {
    return i.vivo && i.surgindo <= 0.0f && (estaVisivel(p.visao, i.pes) || p.bill.oculos > 0.0f);
}

Vec2 centroDoAlvo(const Inimigo& i) { return {i.pes.x, i.pes.y - parametrosDe(i.tipo).alturaCorpo / 2}; }

Retangulo inflar(Retangulo r, float m) { return {r.x - m, r.y - m, r.w + 2 * m, r.h + 2 * m}; }

bool avisoPercebido(const Aviso& a, float reacao) { return a.tempoTotal - a.tempoRestante >= reacao; }

// O Bill (com folga) está dentro de algum aviso percebido? Devolve o menor tempo restante.
// Esse aviso passou despercebido? Sorteio fixo pelo id do golpe (determinístico e igual a cada quadro).
bool distraido(const Aviso& a, int pct) {
    return pct > 0 && static_cast<int>(((a.golpe.id * 2654435761u) >> 16) % 100u) < pct;
}

float perigoEm(const EstadoPartida& p, Vec2 pes, float folga, bool soPercebidos, float reacao = ATRASO_REACAO, int distracao = 0) {
    Jogador sombra = p.bill;
    sombra.pes = pes;
    const Retangulo caixa = inflar(hurtboxJogador(sombra), folga);
    float menor = -1.0f;
    for (const Aviso& a : p.avisos) {
        if (soPercebidos && (!avisoPercebido(a, reacao) || distraido(a, distracao))) continue;
        if (!areaToca(a.golpe.area, caixa)) continue;
        if (menor < 0.0f || a.tempoRestante < menor) menor = a.tempoRestante;
    }
    return menor;
}

// ---------- camada 1: desviar ----------
bool desviar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    const float restante = perigoEm(p, p.bill.pes, 4.0f, true, m.atrasoReacao, m.distracaoPct);
    if (restante < 0.0f) return false;
    const Jogador& b = p.bill;
    const bool podeDash = b.dashRestante <= 0.0f && b.recargaDash <= 0.0f;
    const float alcance = podeDash ? DASH_DISTANCIA : velocidadeMovimento(b.attr.agilidade) * 0.35f;
    // Testa 16 direções: onde termina, fora de TODOS os avisos e longe dos inimigos.
    float melhorNota = -1e9f;
    Vec2 melhor;
    for (int k = 0; k < DIRECOES_FUGA; ++k) {
        const float ang = 6.2831853f * static_cast<float>(k) / DIRECOES_FUGA;
        const Vec2 dir{std::cos(ang), std::sin(ang)};
        const Vec2 fim = moverComColisao(p.sala, b.pes, CAIXA_BILL, escalar(dir, alcance), false);
        const float andou = comprimento(subtrair(fim, b.pes));
        if (perigoEm(p, fim, 6.0f, false) >= 0.0f) continue;
        float nota = andou;
        for (const Inimigo& i : p.inimigos) {
            if (!inimigoPercebido(p, i)) continue;
            const float longe = 0.25f * std::fmin(comprimento(subtrair(fim, i.pes)), 120.0f);
            // Contra quem ataca de LONGE (o Boneco), desviar para trás só dá a ele o próximo pino: o
            // desvio bom é de lado ou para a frente. Recuando sempre, o piloto nunca chegava (11.3).
            nota += i.tipo == TipoInimigo::Boneco ? -longe : longe;
        }
        if (nota > melhorNota) {
            melhorNota = nota;
            melhor = dir;
        }
    }
    if (melhorNota < -1e8f) return false;   // nenhuma saída: segue a vida (combate)
    m.objetivo = ObjetivoPiloto::Desviar;
    m.descricao = "Desviar do aviso";
    e.eixo = melhor;
    if (podeDash && restante < 0.35f) e.pressionou[static_cast<int>(Comando::Dash)] = true;
    return true;
}

// ---------- camada 1a: o Covil (Etapa 15, piloto v5) ----------
// Georgie: a ilusão fere ao toque e a voz puxa; perto dela, o piloto se afasta (o que uma pessoa faz ao ver
// a capa amarela vindo). Luzes Mortas: fica DENTRO de um círculo (acompanhando o giro); só sai para bater
// quando o Pennywise está na recuperação, perto.
bool covil(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    const Jogador& b = p.bill;
    if (p.georgie.ativo) {
        const Vec2 d = subtrair(b.pes, p.georgie.pes);
        const float dist = comprimento(d);
        if (dist < 80.0f) {
            m.objetivo = ObjetivoPiloto::Desviar;
            m.descricao = "Longe do Georgie";
            e.eixo = normalizar(dist > 0.1f ? d : Vec2{1.0f, 0.0f});
            if (dist < 36.0f && b.dashRestante <= 0.0f) e.pressionou[static_cast<int>(Comando::Dash)] = true;
            return true;
        }
    }
    if (!p.luzesMortas || p.quantasLuzes <= 0) return false;
    const Inimigo* ch = chefeNaSala(p);
    if (ch && ch->estado == EstadoIA::Recuperando && comprimento(subtrair(ch->pes, b.pes)) < 90.0f) return false;   // a janela: bater
    int melhor = 0;
    float menor = 1e9f;
    for (int i = 0; i < p.quantasLuzes; ++i) {
        const float d = comprimento(subtrair(centroLuzMorta(p, i), b.pes));
        if (d < menor) { menor = d; melhor = i; }
    }
    const Vec2 alvo = centroLuzMorta(p, melhor);
    m.objetivo = ObjetivoPiloto::Desviar;
    m.descricao = "Ficar na luz";
    const Vec2 d = subtrair(alvo, b.pes);
    e.eixo = comprimento(d) > 12.0f ? normalizar(d) : escalar(normalizar(d), 0.3f);
    // Já dentro e com o Pennywise colado (sem janela): bate mesmo assim se ele estiver vulnerável.
    return true;
}

// ---------- camada 1b: cuidar (kit, barquinho, power-ups) ----------
int inimigosPercebidos(const EstadoPartida& p, float raio) {
    int n = 0;
    for (const Inimigo& i : p.inimigos) {
        if (inimigoPercebido(p, i) && comprimento(subtrair(i.pes, p.bill.pes)) <= raio) ++n;
    }
    return n;
}

int quantos(const EstadoPartida& p, TipoItem t) { return p.inventario.quantidades[static_cast<std::size_t>(t)]; }

// Pede o uso de um item pelo inventário (o resolverMenu faz o resto nos quadros seguintes).
bool usarPeloMenu(MemoriaPiloto& m, EstadoEntrada& e, TipoItem t) {
    m.itemMenu = t;
    m.confirmouMenu = false;
    m.quadrosMenu = 0;
    m.objetivo = ObjetivoPiloto::Pendencia;
    m.descricao = std::string("Abrir o inventário: ") + definicaoDe(t).nome;
    e.pressionou[static_cast<int>(Comando::Inventario)] = true;
    return true;
}

bool cuidar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    const Jogador& b = p.bill;
    const int pct = b.hp * 100 / std::max(1, b.stats.hpMax);
    // Kit no atalho (Q): vida abaixo de 35%.
    if (pct < 35 && quantos(p, TipoItem::Kit) > 0 && m.esperaKit <= 0.0f) {
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = "Usar kit (Q)";
        e.pressionou[static_cast<int>(Comando::UsarKit)] = true;
        m.esperaKit = 0.6f;
        return false;   // não ocupa o quadro: o resto (desviar/combater) continua
    }
    if (m.itemMenu != TipoItem::Total) return false;   // já está a caminho do menu
    const int perto = inimigosPercebidos(p, 6.0f * TILE);
    if (pct < 55 && quantos(p, TipoItem::Barquinho) > 0 && inimigosPercebidos(p, 2.5f * TILE) == 0) {
        return usarPeloMenu(m, e, TipoItem::Barquinho);
    }
    if (perto >= 2 && b.coragem <= 0.0f && quantos(p, TipoItem::FichaCoragem) > 0) return usarPeloMenu(m, e, TipoItem::FichaCoragem);
    if (perto >= 3 && b.tonico <= 0.0f && quantos(p, TipoItem::Tonico) > 0) return usarPeloMenu(m, e, TipoItem::Tonico);
    // Sala ainda em combate e nenhum inimigo à vista (está rondando no escuro): os Óculos do
    // Richie revelam onde eles estão. A pessoa tem a mesma pista ("Onda 1 de 2" no HUD).
    if (!p.salaConcluida && hostisVivos(p) > 0 && inimigosPercebidos(p, 1e6f) == 0 && b.oculos <= 0.0f &&
        quantos(p, TipoItem::Oculos) > 0) {
        return usarPeloMenu(m, e, TipoItem::Oculos);
    }
    return false;
}

// ---------- camada 2: combater ----------
// Alvo sumiu de vista: vai até onde o viu pela última vez (memória curta e justa).
bool irAteUltimaPosicao(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e, float dt) {
    if (m.alvo == ID_NULO) return false;
    m.tempoSemVerAlvo += dt;
    const Inimigo* ainda = buscarInimigo(p, m.alvo);   // só para saber se MORREU (o HUD mostra o abate)
    if (!ainda || !ainda->vivo || m.tempoSemVerAlvo > MEMORIA_ALVO ||
        comprimento(subtrair(m.ultimaPosicaoAlvo, p.bill.pes)) < TILE * 0.5f) {
        m.alvo = ID_NULO;
        return false;
    }
    m.objetivo = ObjetivoPiloto::Combater;
    m.descricao = "Procurar onde o inimigo sumiu";
    m.aproximando = true;
    const Celula destino = celulaDe(m.ultimaPosicaoAlvo);
    if (m.destino.col != destino.col || m.destino.lin != destino.lin) planejar(p, m, destino);
    e.eixo = seguir(p, m, m.ultimaPosicaoAlvo);
    return true;
}

bool combater(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e, float dt) {
    const Jogador& b = p.bill;
    const Vec2 corpo = corpoJogador(b);
    const Inimigo* alvo = nullptr;
    const Inimigo* atual = nullptr;   // o alvo que já vinha sendo combatido, se ainda à vista
    float melhor = 1e9f, distAtual = 1e9f;
    int perto = 0;
    for (const Inimigo& i : p.inimigos) {
        if (!inimigoPercebido(p, i)) continue;
        float d = comprimento(subtrair(centroDoAlvo(i), corpo));
        if (d < 50.0f) ++perto;
        // Prioridade: quem está se recuperando (janela de punição) fica "mais perto".
        if (i.estado == EstadoIA::Recuperando) d *= 0.6f;
        if (i.itemCarregado >= 0) d *= 0.5f;   // ladrão com item: persegue antes que fuja
        if (i.id == m.alvo) { atual = &i; distAtual = d; }
        if (d < melhor) { melhor = d; alvo = &i; }
    }
    if (!alvo) return irAteUltimaPosicao(p, m, e, dt);
    // O alvo de agora acabou de sumir atrás de um pilar e o outro visível está mais longe: vai
    // até onde o viu, por pouco tempo, ANTES de trocar. Sem isso, um passo para cada lado faz o
    // alvo aparecer e sumir, e o piloto alterna entre os dois para sempre (Etapa 8.4, seed 18).
    if (!atual && m.alvo != ID_NULO && m.tempoSemVerAlvo < MEMORIA_ALVO) {
        const float dLembrado = comprimento(subtrair(m.ultimaPosicaoAlvo, corpo));
        if (melhor > dLembrado * TROCA_ALVO && irAteUltimaPosicao(p, m, e, dt)) return true;
    }
    // Histerese: só troca de alvo se o outro estiver BEM mais perto (evita ir e voltar).
    if (atual && melhor > distAtual * TROCA_ALVO) alvo = atual;
    m.alvo = alvo->id;
    m.ultimaPosicaoAlvo = alvo->pes;
    m.tempoSemVerAlvo = 0.0f;
    m.objetivo = ObjetivoPiloto::Combater;
    m.descricao = std::string("Combater: ") + nomeDoChefe(*alvo);   // o chefe muda de nome na fase 2 (os outros: o nome do tipo)
    const Vec2 paraAlvo = subtrair(centroDoAlvo(*alvo), corpo);
    // Distância até o ponto mais perto da CAIXA de dano do alvo (+ meia caixa de um inimigo comum):
    // igual ao centro para os pequenos, mas o chefe largo era "longe" com Bill encostado nele, e
    // Bill andava contra a Múmia sem nunca bater (impasse achado na 11.1).
    const Retangulo hb = hurtboxInimigo(*alvo);
    const Vec2 maisPerto{std::clamp(corpo.x, hb.x, hb.x + hb.w), std::clamp(corpo.y, hb.y, hb.y + hb.h)};
    const float dist = std::min(comprimento(paraAlvo), comprimento(subtrair(maisPerto, corpo)) + 11.0f);
    const Vec2 dir = normalizar(paraAlvo);

    m.aproximando = dist > ALCANCE_TACO;
    if (dist <= ALCANCE_TACO) {
        e.eixo = escalar(dir, 0.2f);   // mira sem avançar demais
        // Chefe na pose de preparar (visível para qualquer pessoa): não começa golpe — no meio do
        // golpe Bill anda a 30% e não sai da linha anunciada (Etapa 11.1). Bate na recuperação.
        const bool segurar = parametrosDe(alvo->tipo).chefe && alvo->estado == EstadoIA::Preparando;
        if (!segurar && m.quadro % 6 == 0) e.pressionou[static_cast<int>(Comando::Atacar)] = true;
        if (perto >= 2 && b.recargaEspecial <= 0.0f && b.acao == AcaoJogador::Livre) {
            e.pressionou[static_cast<int>(Comando::Especial)] = true;
        }
        return true;
    }
    // Clarão na Sombra (a mais perigosa) quando sobra carga e ela não está atordoada.
    if (alvo->tipo == TipoInimigo::Sombra && b.cargasLanterna >= 2 && dist < 110.0f && alvo->atordoado <= 0.0f &&
        alvo->estado != EstadoIA::Recuperando && b.acao == AcaoJogador::Livre) {
        e.eixo = dir;
        e.pressionou[static_cast<int>(Comando::Clarao)] = true;
        return true;
    }
    // Aproximar: direto se perto e com o caminho reto livre; senão, pelo caminho (BFS). Sem essa
    // checagem, Bill empurrava a quina de um pilar a 59 px do chefe para sempre (8.5).
    Celula destino = celulaDe(alvo->pes);
    // Cabelo do ralo: a cruz cobre a linha e a coluna dele; o lugar seguro é a DIAGONAL. Fora dela,
    // vai primeiro até a célula diagonal vizinha mais perto (andar pela linha era entrar na cruz).
    if (alvo->tipo == TipoInimigo::Cabelo) {
        const Celula aqui = celulaDe(b.pes), ralo = celulaDe(alvo->pes);
        const bool naDiagonal = aqui.col != ralo.col && aqui.lin != ralo.lin;
        if (!naDiagonal || std::max(std::abs(aqui.col - ralo.col), std::abs(aqui.lin - ralo.lin)) > 1) {
            float menor = 1e9f;
            for (int dc : {-1, 1}) {
                for (int dl : {-1, 1}) {
                    const Celula d{ralo.col + dc, ralo.lin + dl};
                    if (tileEm(p.sala, d.col, d.lin).bloqueiaMovimento) continue;
                    const float custo = comprimento(subtrair(centroCelula(d), b.pes));
                    if (custo < menor) { menor = custo; destino = d; }
                }
            }
            if (menor < 1e8f) {
                if (m.destino.col != destino.col || m.destino.lin != destino.lin || m.quadro % 18 == 0) planejar(p, m, destino);
                e.eixo = seguir(p, m, centroCelula(destino));
                return true;
            }
        }
    }
    if (dist < 2.5f * TILE && segmentoLivre(p.sala, b.pes, alvo->pes, true) && segmentoSemArmadilha(p, b.pes, alvo->pes)) {
        e.eixo = dir;
    } else {
        if (m.destino.col != destino.col || m.destino.lin != destino.lin || m.quadro % 18 == 0) planejar(p, m, destino);
        e.eixo = seguir(p, m, alvo->pes);
    }
    return true;
}

// ---------- camada 0: pendência (menu aberto) ----------
// Só comandos: move o cursor com Baixo até o atributo desejado e confirma.
bool resolverMenu(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (p.pendencia == Pendencia::Inventario && m.itemMenu != TipoItem::Total) {
        // O PILOTO abriu o inventário para usar um item: navega como uma pessoa.
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = std::string("Inventário: usar ") + definicaoDe(m.itemMenu).nome;
        if (m.quadro % 2 == 0) return true;   // um toque a cada 2 quadros
        int alvo = 0;
        for (int i = 0; i < TOTAL_ITENS_USAVEIS; ++i) alvo = ITENS_USAVEIS[i] == m.itemMenu ? i : alvo;
        Comando c = Comando::Inventario;   // padrão: fechar
        if (p.aba != AbaInventario::Itens) c = Comando::AbaDireita;
        else if (!m.confirmouMenu && p.cursor != alvo) c = Comando::Baixo;
        else if (!m.confirmouMenu) { c = Comando::Confirmar; m.confirmouMenu = true; }
        else { m.itemMenu = TipoItem::Total; m.confirmouMenu = false; }
        e.pressionou[static_cast<int>(c)] = true;
        return true;
    }
    if (p.pendencia == Pendencia::Loja) {   // compra a preferida; sem nada que valha, fecha
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = "Loja do Elias";
        if (m.quadro % 2 == 0) return true;
        const int alvo = ofertaPreferidaDoPiloto(p);
        Comando c = Comando::Voltar;
        if (alvo < 0) m.lojaFeita = true;
        else c = p.cursor == alvo ? Comando::Confirmar : Comando::Baixo;
        e.pressionou[static_cast<int>(c)] = true;
        return true;
    }
    if (p.pendencia == Pendencia::EscolherEco) {   // anda pelas cartas até a preferida e confirma
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = "Escolher um Eco";
        if (m.quadro % 2 == 0) return true;
        const bool certo = p.cursor == ecoPreferidoDoPiloto(p.ofertasEco);
        e.pressionou[static_cast<int>(certo ? Comando::Confirmar : Comando::Direita)] = true;
        return true;
    }
    if (p.pendencia == Pendencia::Dialogo) {   // lê e avança (nas escolhas, a primeira)
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = "Lendo o diálogo";
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Confirmar)] = true;
        return true;
    }
    if (p.pendencia == Pendencia::Alcapao) {   // escolhe a dificuldade pedida e confirma
        m.objetivo = ObjetivoPiloto::Clube;
        m.descricao = std::string("Alçapão: ") + nomeDificuldade(m.dificuldadeDesejada);
        if (m.quadro % 2 == 0) return true;
        const bool certo = p.cursor == static_cast<int>(m.dificuldadeDesejada);
        e.pressionou[static_cast<int>(certo ? Comando::Confirmar : Comando::Baixo)] = true;
        return true;
    }
    if (p.pendencia == Pendencia::Pausa || p.pendencia == Pendencia::Inventario || p.pendencia == Pendencia::Mural ||
        p.pendencia == Pendencia::Quadro) {
        // Quem abriu foi a PESSOA (ou o controle caiu): o piloto espera, sem apertar nada.
        m.objetivo = ObjetivoPiloto::Pendencia;
        m.descricao = "Jogo pausado";
        return true;
    }
    if (p.pendencia != Pendencia::SubirNivel) return false;
    m.objetivo = ObjetivoPiloto::Pendencia;
    m.descricao = "Distribuir pontos";
    const int alvo = atributoPreferidoDoPiloto(p.bill);
    if (m.quadro % 2 == 0) return true;   // um toque a cada 2 quadros (como um dedo, sem "segurar")
    e.pressionou[static_cast<int>(p.cursor == alvo ? Comando::Confirmar : Comando::Baixo)] = true;
    return true;
}

bool explorar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e);

// Vizinha livre de um objeto, a mais perto de Bill pelo mapa conhecido ({-1,-1} = nenhuma).
Celula vizinhaMaisPerto(const EstadoPartida& p, const Busca& b, Celula alvo) {
    Celula melhor{-1, -1};
    int menor = 1 << 20;
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    for (int k = 0; k < 4; ++k) {
        const int nc = alvo.col + dc[k], nl = alvo.lin + dl[k];
        if (!dentroDaSala(p.sala, nc, nl)) continue;
        const short d = b.dist[static_cast<std::size_t>(nl * p.sala.cols + nc)];
        if (d >= 0 && d < menor) { menor = d; melhor = {nc, nl}; }
    }
    return melhor;
}

// Quem é cada NPC: amigo para resgatar (os seis), o Elias ou qualquer um (prólogo).
enum class QualNpc : std::uint8_t { Qualquer, Amigo, Elias };
bool npcServe(const EstadoPartida& p, const ObjetoSala& o, QualNpc qual) {
    const std::string& nome = nomeDoNpc(p, o);
    if (qual == QualNpc::Amigo) return amigoDoCodigo(nome) != Amigo::Total;
    if (qual == QualNpc::Elias) return nome == "elias";
    return true;
}

// Vai até ao lado do NPC (1..9 ainda no lugar) e aperta Interagir.
bool falarComAmigo(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e, bool perto, const char* descricao,
                   QualNpc qual = QualNpc::Qualquer) {
    if (perto) {
        m.descricao = descricao;
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Interagir)] = true;
        return true;
    }
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo < '1' || o.codigo > '9' || o.usado || !visaoEm(p.visao, o.celula.col, o.celula.lin).explorada) continue;
        if (!npcServe(p, o, qual)) continue;
        Busca b;
        buscarAPartirDe(p, celulaDe(p.bill.pes), b);
        const Celula ao = vizinhaMaisPerto(p, b, o.celula);
        if (ao.col < 0) continue;
        m.descricao = descricao;
        m.caminho = montarCaminho(p, b, ao);
        m.destino = ao;
        e.eixo = seguir(p, m, centroCelula(ao));
        return true;
    }
    return false;
}

// Parede rachada já vista (rachaduras discretas: uma pessoa atenta também nota): vai até ao
// lado dela e usa o Especial (o giro alcança a parede sem precisar mirar).
bool derrubarParedeRachada(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    Busca b;
    bool buscou = false;
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            if (tileEm(p.sala, c, l).tipo != TipoTile::ParedeRachada || !visaoEm(p.visao, c, l).explorada) continue;
            m.objetivo = ObjetivoPiloto::Missao;
            if (comprimento(subtrair(centroCelula({c, l}), p.bill.pes)) < 1.3f * TILE) {
                m.descricao = "Derrubar a parede rachada";
                if (p.bill.recargaEspecial <= 0.0f && p.bill.acao == AcaoJogador::Livre) {
                    e.pressionou[static_cast<int>(Comando::Especial)] = true;
                }
                return true;
            }
            if (!buscou) { buscarAPartirDe(p, celulaDe(p.bill.pes), b); buscou = true; }
            const Celula ao = vizinhaMaisPerto(p, b, {c, l});
            if (ao.col < 0) continue;
            m.descricao = "Ir até a parede rachada";
            m.caminho = montarCaminho(p, b, ao);
            m.destino = ao;
            e.eixo = seguir(p, m, centroCelula(ao));
            return true;
        }
    }
    return false;
}

// Altar de Eco aceso e já visto: vai até ele e aperta Interagir.
bool tocarAltar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (altarProximo(p)) {
        m.descricao = "Tocar o altar de Eco";
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Interagir)] = true;
        return true;
    }
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo != '*' || o.usado || !visaoEm(p.visao, o.celula.col, o.celula.lin).explorada) continue;
        Busca b;
        buscarAPartirDe(p, celulaDe(p.bill.pes), b);
        const Celula ao = vizinhaMaisPerto(p, b, o.celula);
        if (ao.col < 0) continue;
        m.descricao = "Ir até o altar de Eco";
        m.caminho = montarCaminho(p, b, ao);
        m.destino = ao;
        e.eixo = seguir(p, m, centroCelula(ao));
        return true;
    }
    return false;
}

// ---------- prólogo: falar com o Mike e seguir pela rua ----------
bool prologo(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (!p.noPrologo) return false;
    m.objetivo = ObjetivoPiloto::Clube;
    if (!p.salaConcluida) {
        const ObjetoSala* perto = objetoDoClubeProximo(p);
        if (falarComAmigo(p, m, e, perto != nullptr, "Falar com o Mike")) return true;
        return explorar(p, m, e);
    }
    if (p.sala.portas.empty()) return false;
    const Celula porta = p.sala.portas.front().celula;
    m.descricao = "Seguir para a galeria";
    if (m.caminho.empty() || m.destino.col != porta.col || m.destino.lin != porta.lin) planejar(p, m, porta);
    const Vec2 alvo = somar(centroCelula(porta), escalar(vetorDaDirecao(p.sala.portas.front().lado), 12.0f));
    e.eixo = seguir(p, m, alvo);
    return true;
}

// ---------- Festa dos Balões: estourar os coloridos, fugir dos vermelhos ----------
bool festa(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (!p.festaAtiva) return false;
    m.objetivo = ObjetivoPiloto::Combater;
    // Vermelho perto: se afasta dele primeiro (é armadilha).
    for (const BalaoFesta& b : p.baloes) {
        const Vec2 d = subtrair(p.bill.pes, b.pos);
        if (b.vermelho && comprimento(d) < 56.0f) {
            m.descricao = "Fugir do balão vermelho";
            e.eixo = normalizar(comprimento(d) > 0.1f ? d : Vec2{1.0f, 0.0f});
            return true;
        }
    }
    const BalaoFesta* alvo = nullptr;
    float menor = 1e9f;
    for (const BalaoFesta& b : p.baloes) {
        if (b.vermelho) continue;
        const float d = comprimento(subtrair(b.pos, p.bill.pes));
        if (d < menor) { menor = d; alvo = &b; }
    }
    if (!alvo) { m.descricao = "Esperar balões"; return true; }
    m.descricao = "Estourar balões";
    const Vec2 debaixo{alvo->pos.x, alvo->pos.y + 6.0f};   // ficar logo abaixo da corda
    const Vec2 d = subtrair(debaixo, p.bill.pes);
    if (comprimento(d) > 14.0f) e.eixo = normalizar(d);
    if (menor < 40.0f && p.bill.acao == AcaoJogador::Livre && m.quadro % 4 == 0) {
        e.pressionou[static_cast<int>(Comando::Atacar)] = true;
    }
    return true;
}

// ---------- Clube: até o alçapão ----------
bool clube(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (!p.noClube) return false;
    m.objetivo = ObjetivoPiloto::Clube;
    const ObjetoSala* perto = objetoDoClubeProximo(p);
    if (perto && perto->codigo == 'H') {
        m.descricao = "Descer pelo alçapão";
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Interagir)] = true;
        return true;
    }
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo != 'H') continue;
        m.descricao = "Ir até o alçapão";
        if (m.caminho.empty() || m.destino.col != o.celula.col || m.destino.lin != o.celula.lin) planejar(p, m, o.celula);
        e.eixo = seguir(p, m, centroCelula(o.celula));
        return true;
    }
    m.descricao = "Clube sem alçapão";
    return true;
}

// ---------- missão: porta do Medo e resgate ----------
// Com a Chave do Medo, vai até a porta T já vista e aperta Interagir. Aberta, resgata o amigo.
bool missao(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (p.salaConcluida && p.emTentativa) {
        m.objetivo = ObjetivoPiloto::Missao;
        if (falarComAmigo(p, m, e, amigoParaResgatar(p) != nullptr, "Resgatar o amigo", QualNpc::Amigo)) return true;
        if (tocarAltar(p, m, e)) return true;
        if (derrubarParedeRachada(p, m, e)) return true;
        // Loja: entregar a manivela (a grade abre) e comprar o que vale a pena.
        const bool manivela = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] > 0 && !p.manivelaEntregue;
        if (manivela || (!m.lojaFeita && ofertaPreferidaDoPiloto(p) >= 0)) {
            if (falarComAmigo(p, m, e, eliasProximo(p) != nullptr, manivela ? "Entregar a manivela" : "Comprar com o Elias",
                              QualNpc::Elias)) {
                return true;
            }
        }
    }
    if (!p.salaConcluida || p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] <= 0) return false;
    if (portaDoMedoProxima(p).col >= 0) {
        m.objetivo = ObjetivoPiloto::Missao;
        m.descricao = "Abrir a porta do Medo";
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Interagir)] = true;
        return true;
    }
    Busca b;
    buscarAPartirDe(p, celulaDe(p.bill.pes), b);
    Celula melhor{-1, -1};
    int menor = 1 << 20;
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            if (tileEm(p.sala, c, l).tipo != TipoTile::PortaTrancada || !visaoEm(p.visao, c, l).explorada) continue;
            for (int k = 0; k < 4; ++k) {   // a célula livre vizinha mais perto do Bill
                const int nc = c + dc[k], nl = l + dl[k];
                if (!dentroDaSala(p.sala, nc, nl)) continue;
                const short d = b.dist[static_cast<std::size_t>(nl * p.sala.cols + nc)];
                if (d >= 0 && d < menor) { menor = d; melhor = {nc, nl}; }
            }
        }
    }
    if (melhor.col < 0) return false;
    m.objetivo = ObjetivoPiloto::Missao;
    m.descricao = "Levar a Chave do Medo à porta";
    m.caminho = montarCaminho(p, b, melhor);
    m.destino = melhor;
    e.eixo = seguir(p, m, centroCelula(melhor));
    return true;
}

// ---------- coletar: baús já vistos que dá para abrir (trancado só com chave) ----------
bool abrirBaus(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (!p.salaConcluida) return false;
    const bool temChave = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveEnferrujada)] > 0;
    if (bauTrancado(p) && !temChave) return false;
    if (bauProximo(p)) {
        m.objetivo = ObjetivoPiloto::Coletar;
        m.descricao = "Abrir o baú";
        if (m.quadro % 2 == 1) e.pressionou[static_cast<int>(Comando::Interagir)] = true;
        return true;
    }
    Busca b;
    buscarAPartirDe(p, celulaDe(p.bill.pes), b);
    Celula melhor{-1, -1};
    int menor = 1 << 20;
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo != 'B' || o.usado || !visaoEm(p.visao, o.celula.col, o.celula.lin).explorada) continue;
        for (int k = 0; k < 4; ++k) {   // a célula livre vizinha mais perto do Bill
            const int nc = o.celula.col + dc[k], nl = o.celula.lin + dl[k];
            if (!dentroDaSala(p.sala, nc, nl)) continue;
            const short d = b.dist[static_cast<std::size_t>(nl * p.sala.cols + nc)];
            if (d >= 0 && d < menor) { menor = d; melhor = {nc, nl}; }
        }
    }
    if (melhor.col < 0) return false;
    m.objetivo = ObjetivoPiloto::Coletar;
    m.descricao = "Ir até o baú";
    m.caminho = montarCaminho(p, b, melhor);
    m.destino = melhor;
    e.eixo = seguir(p, m, centroCelula(melhor));
    return true;
}

// ---------- coletar: itens visíveis que cabem ----------
bool coletar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    Busca b;
    bool buscou = false;
    const ItemNoChao* alvo = nullptr;
    int menor = 1 << 20;
    for (const ItemNoChao& it : p.itensChao) {
        const Celula c = celulaDe(it.pos);
        if (!visaoEm(p.visao, c.col, c.lin).explorada || espacoPara(p.bill, p.inventario, it.tipo) <= 0) continue;
        if (!buscou) { buscarAPartirDe(p, celulaDe(p.bill.pes), b); buscou = true; }
        const short d = b.dist[static_cast<std::size_t>(c.lin * p.sala.cols + c.col)];
        if (d >= 0 && d < menor) { menor = d; alvo = &it; }
    }
    if (!alvo) return abrirBaus(p, m, e);
    m.objetivo = ObjetivoPiloto::Coletar;
    m.descricao = std::string("Pegar: ") + definicaoDe(alvo->tipo).nome;
    m.caminho = montarCaminho(p, b, celulaDe(alvo->pos));
    m.destino = celulaDe(alvo->pos);
    e.eixo = seguir(p, m, alvo->pos);
    return true;
}

// ---------- camadas 3 e 4: sair e explorar ----------
bool explorar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e);

constexpr float TEMPO_MAXIMO_OLHANDO = 8.0f;   // s explorando a sala concluída antes de escolher

bool sair(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    if (!p.salaConcluida) return false;
    // Antes de escolher, OLHA a sala (como uma pessoa): só escolhe entre portas que já viu, então
    // sem isto ele sairia pela primeira porta que a lanterna revelasse.
    if (p.emTentativa && m.tempoOlhando < TEMPO_MAXIMO_OLHANDO) {
        m.tempoOlhando += DT;
        if (explorar(p, m, e)) {
            m.descricao = "Olhar a sala antes de escolher a porta";
            return true;
        }
        m.tempoOlhando = TEMPO_MAXIMO_OLHANDO;   // já viu tudo: decide agora
    }
    // Portas vistas e abertas, da melhor nota para a pior (empate: ordem da sala).
    std::vector<const PortaSala*> ordem;
    for (const PortaSala& porta : p.sala.portas) {
        if (porta.aberta && visaoEm(p.visao, porta.celula.col, porta.celula.lin).explorada) ordem.push_back(&porta);
    }
    std::stable_sort(ordem.begin(), ordem.end(),
                     [&](const PortaSala* a, const PortaSala* b) { return notaDaPorta(p, *a) > notaDaPorta(p, *b); });
    for (const PortaSala* pp : ordem) {
        const PortaSala& porta = *pp;
        if (m.destino.col != porta.celula.col || m.destino.lin != porta.celula.lin || m.quadro % 20 == 0) {
            if (!planejar(p, m, porta.celula)) continue;
        }
        m.objetivo = ObjetivoPiloto::Sair;
        bool descida = false;
        const OpcaoPorta* o = destinoDaPorta(p, porta, descida);
        m.descricao = descida ? "Descer" : (o ? std::string("Porta: ") + (o->recompensa != Recompensa::Nenhuma
                                                                              ? definicaoRecompensa(o->recompensa).nome
                                                                              : "sala especial")
                                              : "Sair pela porta");
        // Passa do centro da porta, em direção à borda: é assim que se atravessa.
        const Vec2 alem = somar(centroCelula(porta.celula), escalar(vetorDaDirecao(porta.lado), 12.0f));
        e.eixo = seguir(p, m, alem);
        return true;
    }
    return false;
}

bool explorar(const EstadoPartida& p, MemoriaPiloto& m, EstadoEntrada& e) {
    // A mesma política do planejar: o mapa SEM as armadilhas conhecidas; só se isso isolar o Bill
    // de tudo, com elas. (Antes, este BFS passava pelos espinhos e o seguir replanejava desviando:
    // os dois brigavam a cada quadro e o piloto oscilava — Etapa 11.1.)
    Busca b;
    buscarAPartirDe(p, celulaDe(p.bill.pes), b, true);
    bool algumAlcance = false;
    for (int i = 0; i < p.sala.cols * p.sala.lins && !algumAlcance; ++i) algumAlcance = b.dist[static_cast<std::size_t>(i)] > 0;
    if (!algumAlcance) buscarAPartirDe(p, celulaDe(p.bill.pes), b, false);
    // A célula NÃO explorada mais próxima que dá para alcançar pelo que se conhece.
    Celula alvo{-1, -1};
    int melhor = 1 << 20;
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            if (visaoEm(p.visao, c, l).explorada) continue;
            if (m.tempoEvitar > 0.0f && c == m.evitarExplorar.col && l == m.evitarExplorar.lin) continue;
            const int d = b.dist[static_cast<std::size_t>(l * p.sala.cols + c)];
            if (d > 0 && d < melhor) { melhor = d; alvo = {c, l}; }
        }
    }
    if (alvo.col < 0) {
        // Tudo explorado e ainda há hostis no escuro: RONDAR. Vai até o canto escuro mais
        // distante (pelo caminho) e, ao chegar ou em 6 s, escolhe outro: varre a sala de ponta
        // a ponta, como uma pessoa procurando o último inimigo.
        if (hostisVivos(p) == 0) return false;
        // Os últimos (≤ 2) CAÇAM Bill e o Cabelo sobe por um ralo perto dele: a jogada certa é ESPERAR.
        // Rondando, o piloto corria para o canto oposto e o caçador nunca o alcançava (12.2). A ronda
        // continua, alternada, para quem não tem caminho até ele.
        if (hostisVivos(p) <= LIMITE_HOSTIS_CACA) {
            m.cicloEspera += DT;
            if (std::fmod(m.cicloEspera, 16.0f) < 8.0f) {
                m.objetivo = ObjetivoPiloto::Explorar;
                m.descricao = "Esperar (os últimos vêm atrás de Bill)";
                e.eixo = Vec2{};
                return true;
            }
        }
        m.tempoRonda -= DT;
        const Celula aqui = celulaDe(p.bill.pes);
        const bool chegou = m.ronda.col == aqui.col && m.ronda.lin == aqui.lin;
        if (m.ronda.col < 0 || chegou || m.tempoRonda <= 0.0f) {
            int maisLonge = -1;
            m.ronda = {-1, -1};
            for (int l = 0; l < p.sala.lins; ++l) {
                for (int c = 0; c < p.sala.cols; ++c) {
                    const int d = b.dist[static_cast<std::size_t>(l * p.sala.cols + c)];
                    if (d > maisLonge && visaoEm(p.visao, c, l).luz == 0) { maisLonge = d; m.ronda = {c, l}; }
                }
            }
            // Tempo para CHEGAR lá (≥ 6 s): numa sala grande com um lago no meio, 6 s fixos faziam
            // ele desistir antes de chegar à outra margem, e o último inimigo nunca era achado (11.1).
            m.tempoRonda = std::max(6.0f, 0.5f * static_cast<float>(maisLonge));
        }
        if (m.ronda.col < 0) return false;
        alvo = m.ronda;
        m.descricao = "Rondar (inimigos fora da luz)";
    } else {
        m.descricao = "Explorar";
    }
    m.objetivo = ObjetivoPiloto::Explorar;
    m.caminho = montarCaminho(p, b, alvo);
    m.destino = alvo;
    e.eixo = seguir(p, m, centroCelula(alvo));
    return true;
}

// Travou (quer andar e não sai do lugar)? Conta, evita o alvo de exploração e replaneja.
void vigiarTravamento(const EstadoPartida& p, MemoriaPiloto& m, const EstadoEntrada& e, float dt) {
    if (m.tempoEvitar > 0.0f) m.tempoEvitar -= dt;
    // Em combate, parado batendo é normal; travado é querer CHEGAR ao alvo e não sair do lugar.
    const bool querAndar = comprimento(e.eixo) > 0.5f && (m.objetivo != ObjetivoPiloto::Combater || m.aproximando);
    if (!querAndar || comprimento(subtrair(p.bill.pes, m.posicaoReferencia)) > 8.0f) {
        m.posicaoReferencia = p.bill.pes;
        m.tempoSemProgresso = 0.0f;
        return;
    }
    m.tempoSemProgresso += dt;
    if (m.tempoSemProgresso < TEMPO_TRAVADO) return;
    ++m.travamentos;
    m.tempoSemProgresso = 0.0f;
    m.posicaoReferencia = p.bill.pes;
    m.evitarExplorar = m.destino;
    m.tempoEvitar = 5.0f;
    m.alvo = ID_NULO;   // em combate: esquece o alvo e escolhe de novo
    m.caminho.clear();
    m.destino = {-1, -1};
}

}  // namespace

const char* nomeObjetivo(ObjetivoPiloto o) {
    switch (o) {
        case ObjetivoPiloto::Desviar: return "Desviar";
        case ObjetivoPiloto::Combater: return "Combater";
        case ObjetivoPiloto::Explorar: return "Explorar";
        case ObjetivoPiloto::Sair: return "Sair";
        case ObjetivoPiloto::Pendencia: return "Menu";
        case ObjetivoPiloto::Missao: return "Missão";
        case ObjetivoPiloto::Coletar: return "Coletar";
        case ObjetivoPiloto::Clube: return "Clube";
        default: return "Parado";
    }
}

int notaDaPorta(const EstadoPartida& p, const PortaSala& porta) {
    bool descida = false;
    const OpcaoPorta* o = destinoDaPorta(p, porta, descida);
    if (descida) return 100;
    if (!o) return 0;   // sala avulsa: qualquer porta
    const Jogador& b = p.bill;
    const int pct = b.hp * 100 / std::max(1, b.stats.hpMax);
    switch (o->tipo) {
        case TipoSala::Resgate: case TipoSala::Chefe: return 100;   // caminho único
        case TipoSala::Refugio: return pct < 60 ? 85 : 15;
        case TipoSala::Tesouro: return p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveEnferrujada)] > 0 ? 55 : 5;
        case TipoSala::Bonus: return 45;   // Festa dos Balões: moedas e XP sem risco de morrer
        case TipoSala::Eco: return 50;     // um Eco sempre ajuda
        case TipoSala::Loja: {   // vale com moedas para algo bom, ou para entregar a manivela
            const int moedas = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)];
            const bool manivela = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] > 0;
            if (pct < 45) return 30;   // ferido: o refúgio vale mais
            return manivela ? 70 : (moedas >= 60 ? 55 : 10);
        }
        default: break;
    }
    switch (o->recompensa) {
        case Recompensa::ChaveDoMedo: return 100;
        case Recompensa::Kit: return pct < 50 ? 90 : 40;
        case Recompensa::PontoAtributo: return 60;
        case Recompensa::Melhoria: return 50;
        case Recompensa::Bateria: return b.cargasLanterna < 2 ? 45 : 20;
        case Recompensa::ChaveEnferrujada: return 25;
        case Recompensa::Moedas: return 15;
        default: return 10;
    }
}

int ofertaPreferidaDoPiloto(const EstadoPartida& p) {
    const int moedas = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)];
    const int kits = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)];
    auto nota = [&](const OfertaLoja& o) {
        switch (o.item) {
            case TipoItem::Total: return 60;          // Eco
            case TipoItem::MelhoriaArma: return 50;
            case TipoItem::Escudo: return 45;
            case TipoItem::Kit: return kits < 2 ? 40 : 0;
            case TipoItem::Bateria: return p.bill.cargasLanterna < 2 ? 20 : 0;
            case TipoItem::Barquinho: return kits == 0 ? 15 : 0;
            default: return 0;                        // chave enferrujada: só se sobrar (não compra)
        }
    };
    int melhor = -1, melhorNota = 0;
    for (int i = 0; i < static_cast<int>(p.loja.size()); ++i) {
        const OfertaLoja& o = p.loja[static_cast<std::size_t>(i)];
        if (o.vendida || o.preco > moedas) continue;
        if (o.item != TipoItem::Total && espacoPara(p.bill, p.inventario, o.item) <= 0) continue;
        const int n = nota(o);
        if (n > melhorNota) { melhorNota = n; melhor = i; }
    }
    return melhor;
}

int ecoPreferidoDoPiloto(const std::vector<OfertaEco>& ofertas) {
    // Utilidade para quem joga como o piloto (sem mirar paredes de propósito): vida e armadura primeiro.
    auto utilidade = [](const OfertaEco& o) {
        switch (definicaoEco(o.definicao).efeito) {
            case EfeitoEco::HpMax: return 4;
            case EfeitoEco::ArmaduraFixa: return 3;
            case EfeitoEco::MarcaTerceiroGolpe: return 3;   // o piloto bate muito no mesmo alvo
            case EfeitoEco::InvulnerabilidadeDash: return 2;
            case EfeitoEco::GessoUmGolpe: return 3;
            case EfeitoEco::KitMaisForte: return 2;
            case EfeitoEco::VelocidadePct: return 2;
            case EfeitoEco::BarreiraDash: return 2;
            case EfeitoEco::Prata: return 3;               // os chefes são a parte longa da tentativa
            case EfeitoEco::MiraFirme: return 2;
            case EfeitoEco::EspecialEstilingue: return 2;  // o piloto usa o especial com 2+ perto
            case EfeitoEco::Resistencia: return 3;         // armadura + o fogo do chão dos biomas 5–6 (Etapa 13.4)
            case EfeitoEco::Marreta: return 2;             // o piloto fecha o combo de 3
            case EfeitoEco::Cronista: return 1;
            case EfeitoEco::PiadaRuim: return 3;           // menos dano tomado: vale como armadura (Etapa 14.3)
            case EfeitoEco::VozLocutor: return 2;          // o especial com 2+ perto vira controle de grupo
            case EfeitoEco::BipBip: return 2;              // o piloto desvia com dash o tempo todo
            case EfeitoEco::OculosRichie: return 1;
            default: return 1;
        }
    };
    int melhor = 0;
    for (int i = 1; i < static_cast<int>(ofertas.size()); ++i) {
        const OfertaEco& a = ofertas[static_cast<std::size_t>(i)];
        const OfertaEco& b = ofertas[static_cast<std::size_t>(melhor)];
        if (a.raridade > b.raridade || (a.raridade == b.raridade && utilidade(a) > utilidade(b))) melhor = i;
    }
    return melhor;
}

int atributoPreferidoDoPiloto(const Jogador& j) {
    // 0 Força, 1 Foco, 2 Agilidade, 3 Coragem (ordem da tela).
    if (std::min(j.attr.forca, j.attr.agilidade) < 4) return j.attr.forca <= j.attr.agilidade ? 0 : 2;
    if (std::min(j.attr.coragem, j.attr.foco) < ATRIBUTO_MAXIMO) return j.attr.coragem <= j.attr.foco ? 3 : 1;
    return j.attr.forca < ATRIBUTO_MAXIMO ? 0 : 2;
}

// Em cima da esteira, faz força CONTRA ela (Etapa 13.1): escolhe a direção u (unitária) tal que
// velocidade·u + esteira aponte exatamente para onde ele quer ir. Sem isso, andando na diagonal a
// componente contra a esteira perdia para os 60 px/s e Bill era levado contra a parede para sempre.
void compensarEsteira(const EstadoPartida& p, EstadoEntrada& e) {
    if (comprimento(e.eixo) < 0.1f || p.bill.dashRestante > 0.0f) return;
    const Tile& t = tileEm(p.sala, celulaDe(p.bill.pes).col, celulaDe(p.bill.pes).lin);
    if (t.tipo != TipoTile::Esteira) return;
    e.eixo = contraEsteira(t, e.eixo, p.bill.stats.velocidade);
}

EstadoEntrada pilotar(const EstadoPartida& p, MemoriaPiloto& m, float dt) {
    EstadoEntrada e;
    e.ultimoUsado = Dispositivo::Teclado;
    ++m.quadro;
    if (m.salaAtual != p.sala.modelo || p.trocouDeSala) {   // sala nova: plano novo
        m.salaAtual = p.sala.modelo;
        m.caminho.clear();
        m.destino = {-1, -1};
        m.tempoEvitar = 0.0f;
        m.alvo = ID_NULO;
        m.tempoOlhando = 0.0f;
        m.lojaFeita = false;
    }
    m.aproximando = false;
    if (p.terminou || !estaVivo(p.bill)) {
        m.objetivo = ObjetivoPiloto::Parado;
        m.descricao = "Fim";
        return e;
    }
    if (m.esperaKit > 0.0f) m.esperaKit -= dt;
    if (m.itemMenu != TipoItem::Total && p.pendencia != Pendencia::Inventario && ++m.quadrosMenu > 12) {
        m.itemMenu = TipoItem::Total;   // o menu não abriu (ex.: a partida acabou): desiste
    }
    if (!resolverMenu(p, m, e) && !prologo(p, m, e) && !clube(p, m, e) && !festa(p, m, e) && !desviar(p, m, e) && !covil(p, m, e) && !cuidar(p, m, e) && !combater(p, m, e, dt) && !missao(p, m, e) &&
        !coletar(p, m, e) && !sair(p, m, e) && !explorar(p, m, e)) {
        m.objetivo = ObjetivoPiloto::Parado;
        m.descricao = "Esperando";
    }
    compensarEsteira(p, e);
    vigiarTravamento(p, m, e, dt);
    // Comandos "segurados" acompanham os pressionados (como o teclado faria).
    for (int i = 0; i < TOTAL_COMANDOS; ++i) e.segurando[static_cast<std::size_t>(i)] = e.pressionou[static_cast<std::size_t>(i)];
    return e;
}
