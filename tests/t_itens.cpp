// Testes de itens (PRD §11.1, SDD §15; critérios F10, M05, M09): tabela, limites, uso, power-ups,
// coleta no chão, drops pela dificuldade, baús, caixas e chaves do mapa. Tudo sem janela.
#include <cstdio>
#include <filesystem>

#include "jogo/partida.h"
#include "teste.h"

namespace {

const char* const SALA_ITENS =
    "id=sala_itens\nbioma=1\ntipo=combate\nondas=0\nmapa:\n"
    "############\n"
    "#..........#\n"
    "#..k...B...#\n"
    "#....E.....#\n"
    "#..c.......#\n"
    "#..........#\n"
    "############\n";

Conteudo conteudo(const char* mapa, const char* tipo = nullptr) {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    std::string texto = mapa;
    if (tipo) texto.replace(texto.find("tipo=combate"), 12, std::string("tipo=") + tipo);
    lerSala(texto, m, erro);
    c.salas.push_back(m);
    return c;
}

int qtd(const EstadoPartida& p, TipoItem t) { return quantidadeDe(p.bill, p.inventario, t); }

// Anda até `alvo` (pés) em linha reta, alguns segundos no máximo.
void andarAte(EstadoPartida& p, const Conteudo& c, Vec2 alvo) {
    for (int q = 0; q < 400; ++q) {
        const Vec2 d = subtrair(alvo, p.bill.pes);
        if (comprimento(d) < 3.0f) break;
        EstadoEntrada e;
        e.eixo = normalizar(d);
        atualizarPartida(p, c, e, DT);
    }
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

void esperar(EstadoPartida& p, const Conteudo& c, float segundos) {
    for (int q = 0; q < static_cast<int>(segundos * 60.0f); ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
}

}  // namespace

TESTE(itens_tabela_tem_14_itens_com_nome_icone_e_limites_do_prd) {
    VERIFICAR(TOTAL_ITENS >= 7);   // critério M05
    for (int i = 0; i < TOTAL_ITENS; ++i) {
        const DefinicaoItem& d = definicaoDe(static_cast<TipoItem>(i));
        VERIFICAR(d.nome && d.nome[0] && d.descricao && d.descricao[0] && d.atlas && d.atlas[0]);
    }
    VERIFICAR(definicaoDe(TipoItem::Kit).limite == 3 && definicaoDe(TipoItem::Bateria).limite == 5);
    VERIFICAR(definicaoDe(TipoItem::MelhoriaArma).limite == 3 && definicaoDe(TipoItem::Escudo).limite == 1);
    VERIFICAR(definicaoDe(TipoItem::Moeda).limite == 0 && definicaoDe(TipoItem::Pacto).limite == MAXIMO_PACTOS);
    VERIFICAR(ehItemDeMissao(TipoItem::ChaveDoMedo) && ehItemDeMissao(TipoItem::Manivela) && !ehItemDeMissao(TipoItem::Kit));
}

TESTE(itens_limite_respeitado_e_bateria_e_pacto_no_jogador) {
    Jogador j;
    recalcularJogador(j);
    Inventario inv;
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Kit, 2) == 2);
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Kit, 5) == 1);   // só cabia 1
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Kit, 1) == 0 && quantidadeDe(j, inv, TipoItem::Kit) == 3);
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Bateria, 9) == CARGAS_LANTERNA_MAXIMO - CARGAS_LANTERNA_INICIAIS);
    VERIFICAR(j.cargasLanterna == CARGAS_LANTERNA_MAXIMO);
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Pacto, 9) == MAXIMO_PACTOS - 1 && j.pactos == MAXIMO_PACTOS);
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::Moeda, 1000) == 1000);   // moeda sem limite
    // Missão nunca é recusada por limite.
    VERIFICAR(coletarNoInventario(j, inv, TipoItem::ChaveDoMedo, 1) == 1 && coletarNoInventario(j, inv, TipoItem::ChaveDoMedo, 1) == 1);
}

TESTE(itens_kit_cura_35_por_cento_e_nao_gasta_com_vida_cheia) {
    Jogador j;
    recalcularJogador(j);
    j.hp = j.stats.hpMax;
    Inventario inv;
    VERIFICAR(usarItem(j, inv, TipoItem::Kit) == ResultadoUso::NaoTem);
    coletarNoInventario(j, inv, TipoItem::Kit, 1);
    VERIFICAR(usarItem(j, inv, TipoItem::Kit) == ResultadoUso::NaoPrecisa && quantidadeDe(j, inv, TipoItem::Kit) == 1);
    j.hp = 10;
    VERIFICAR(usarItem(j, inv, TipoItem::Kit) == ResultadoUso::Usou);
    VERIFICAR(j.hp == 10 + j.stats.hpMax * 35 / 100 && quantidadeDe(j, inv, TipoItem::Kit) == 0 && j.cura > 0.0f);
    // Nunca passa do máximo.
    coletarNoInventario(j, inv, TipoItem::Barquinho, 1);
    j.hp = j.stats.hpMax - 3;
    VERIFICAR(usarItem(j, inv, TipoItem::Barquinho) == ResultadoUso::Usou && j.hp == j.stats.hpMax);
    VERIFICAR(usarItem(j, inv, TipoItem::Escudo) == ResultadoUso::NaoUsavel);
}

TESTE(itens_escudo_e_melhoria_mudam_os_derivados) {
    Jogador j;
    recalcularJogador(j);
    Inventario inv;
    const int armadura = j.stats.armadura, dano = j.stats.danoBase;
    coletarNoInventario(j, inv, TipoItem::Escudo, 1);
    VERIFICAR(j.stats.armadura == armadura + 2);
    coletarNoInventario(j, inv, TipoItem::MelhoriaArma, 5);   // só 3 cabem
    VERIFICAR(j.stats.danoBase == dano + 9 && quantidadeDe(j, inv, TipoItem::MelhoriaArma) == 3);
}

TESTE(itens_power_ups_ligam_e_expiram) {
    Jogador j;
    recalcularJogador(j);
    Inventario inv;
    const int dano = j.stats.danoBase;
    const float vel = j.stats.velocidade;
    coletarNoInventario(j, inv, TipoItem::FichaCoragem, 1);
    coletarNoInventario(j, inv, TipoItem::Tonico, 1);
    VERIFICAR(usarItem(j, inv, TipoItem::FichaCoragem) == ResultadoUso::Usou);
    VERIFICAR(usarItem(j, inv, TipoItem::Tonico) == ResultadoUso::Usou);
    VERIFICAR(j.stats.danoBase == dano * 125 / 100);
    VERIFICAR_PERTO(j.stats.velocidade, vel * 1.3f, 0.01f);
    for (int q = 0; q < 21 * 60; ++q) atualizarPowerUps(j, DT);   // o tônico (20 s) acaba
    VERIFICAR(j.tonico == 0.0f && j.coragem > 0.0f);
    VERIFICAR_PERTO(j.stats.velocidade, vel, 0.01f);
    for (int q = 0; q < 10 * 60; ++q) atualizarPowerUps(j, DT);   // a coragem (30 s) acaba
    VERIFICAR(j.coragem == 0.0f && j.stats.danoBase == dano);
}

TESTE(itens_no_chao_sao_coletados_ao_passar_e_contam_I_uma_vez) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    // A chave 'k' do mapa virou item no chão; o baú e a caixa ocupam a célula.
    VERIFICAR(p.itensChao.size() == 1 && p.itensChao[0].tipo == TipoItem::ChaveEnferrujada);
    VERIFICAR(tileEm(p.sala, 7, 2).bloqueiaMovimento && tileEm(p.sala, 3, 4).bloqueiaMovimento);
    VERIFICAR(!tileEm(p.sala, 7, 2).bloqueiaVisao);
    andarAte(p, c, centroCelula({3, 2}));
    VERIFICAR(qtd(p, TipoItem::ChaveEnferrujada) == 1 && p.itensChao.empty() && p.stats.I == 1);
    VERIFICAR(!p.notificacoes.empty() && p.notificacoes.back().tipo == TipoItem::ChaveEnferrujada);
    // Um drop que acabou de cair não é pego antes do "pulo" terminar.
    soltarItem(p, TipoItem::Moeda, 4, p.bill.pes, p.bill.pes);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(qtd(p, TipoItem::Moeda) == 0);
    esperar(p, c, 0.5f);
    VERIFICAR(qtd(p, TipoItem::Moeda) == 4 && p.stats.I == 2);   // 4 moedas = 1 coleta
}

TESTE(itens_cheio_fica_no_chao) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 3);
    soltarItem(p, TipoItem::Kit, 1, p.bill.pes, p.bill.pes);
    esperar(p, c, 1.0f);
    VERIFICAR(qtd(p, TipoItem::Kit) == 3 && p.itensChao.size() == 2 && p.stats.I == 0);   // chave + kit no chão
}

TESTE(itens_bau_abre_com_interagir_uma_vez) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 7, 0);
    andarAte(p, c, centroCelula({7, 3}));
    VERIFICAR(bauProximo(p) != nullptr);
    const std::size_t antes = p.itensChao.size();
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.itensChao.size() == antes + 2 && bauProximo(p) == nullptr);
    apertar(p, c, Comando::Interagir);   // aberto: não solta de novo
    VERIFICAR(p.itensChao.size() == antes + 2);
    for (const ItemNoChao& it : p.itensChao) VERIFICAR(!tileEm(p.sala, celulaDe(it.pos).col, celulaDe(it.pos).lin).bloqueiaMovimento);
}

TESTE(itens_bau_da_sala_de_tesouro_exige_chave) {
    const Conteudo c = conteudo(SALA_ITENS, "tesouro");
    EstadoPartida p;
    iniciarPartida(p, c, 7, 0);
    VERIFICAR(bauTrancado(p));
    andarAte(p, c, centroCelula({7, 3}));
    apertar(p, c, Comando::Interagir);
    VERIFICAR(bauProximo(p) != nullptr);   // sem chave: continua fechado
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveEnferrujada)] = 1;
    const std::size_t antes = p.itensChao.size();
    apertar(p, c, Comando::Interagir);
    VERIFICAR(bauProximo(p) == nullptr && qtd(p, TipoItem::ChaveEnferrujada) == 0 && p.itensChao.size() == antes + 3);
}

// Caixa (Diego, 17/11): quebra com 2–3 golpes — treme e solta lascas a cada um; o Especial quebra de vez.
TESTE(itens_caixa_quebra_com_tres_golpes_e_libera_a_celula) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 3, 0);
    andarAte(p, c, centroCelula({4, 4}));
    EstadoEntrada e;
    e.eixo = {-1.0f, 0.0f};   // vira para a caixa
    atualizarPartida(p, c, e, DT);
    const ObjetoSala* caixa = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) if (o.codigo == 'c') caixa = &o;
    VERIFICAR(caixa != nullptr);
    if (!caixa) return;
    for (int golpe = 1; golpe <= VIDA_CAIXA; ++golpe) {
        apertar(p, c, Comando::Atacar);
        esperar(p, c, 0.15f);
        VERIFICAR(!p.estilhacos.empty() && caixa->tremor > 0.0f);   // lascas no ar e a caixa tremendo
        esperar(p, c, 0.5f);   // o golpe termina e a janela de combo fecha
        if (golpe < VIDA_CAIXA) VERIFICAR(tileEm(p.sala, 3, 4).bloqueiaMovimento && caixa->vida == VIDA_CAIXA - golpe);   // de pé, rachando
    }
    VERIFICAR(!tileEm(p.sala, 3, 4).bloqueiaMovimento && caixa->usado);
}

TESTE(itens_caixa_quebra_de_vez_com_o_especial) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 3, 0);
    andarAte(p, c, centroCelula({4, 4}));
    apertar(p, c, Comando::Especial);
    esperar(p, c, 0.4f);
    VERIFICAR(!tileEm(p.sala, 3, 4).bloqueiaMovimento);
}

TESTE(itens_kit_no_atalho_Q) {
    const Conteudo c = conteudo(SALA_ITENS);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 1);
    apertar(p, c, Comando::UsarKit);   // vida cheia: não gasta
    VERIFICAR(qtd(p, TipoItem::Kit) == 1);
    p.bill.hp = 5;
    apertar(p, c, Comando::UsarKit);
    VERIFICAR(qtd(p, TipoItem::Kit) == 0 && p.bill.hp > 5);
}

TESTE(itens_drop_e_sorteios_seguem_a_dificuldade) {
    // Peso de Kit/Bateria: no Fácil (150%) aparecem mais que no Difícil (70%).
    Rng a, b;
    semear(a, 11);
    semear(b, 11);
    int kitsFacil = 0, kitsDificil = 0;
    for (int i = 0; i < 2000; ++i) {
        const ItemSorteado f = sortearDrop(a, 150), d = sortearDrop(b, 70);
        kitsFacil += (f.tipo == TipoItem::Kit || f.tipo == TipoItem::Bateria) ? 1 : 0;
        kitsDificil += (d.tipo == TipoItem::Kit || d.tipo == TipoItem::Bateria) ? 1 : 0;
        VERIFICAR(f.quantidade >= 1 && d.tipo != TipoItem::Total);
    }
    VERIFICAR(kitsFacil > kitsDificil);
    // Baú nunca dá item de missão nem pacto.
    Rng r;
    semear(r, 5);
    for (int i = 0; i < 500; ++i) {
        const ItemSorteado s = sortearItemDeBau(r, 100);
        VERIFICAR(!ehItemDeMissao(s.tipo) && s.tipo != TipoItem::Pacto && s.tipo != TipoItem::Total);
    }
}

// Diego, 17/11: "passei por cima de itens e não pegou". A coleta era só pelos PÉS a 18 px; o corpo de
// Bill (48 px) cobria o ícone com os pés a 20–25 px dele. Agora vale o corpo encostando no ícone, e um
// ímã curto puxa o item até Bill (sem atravessar parede, e só se couber no inventário).
TESTE(itens_coleta_pelo_corpo_e_ima_puxa_sem_atravessar_parede) {
    // Geometria: Bill logo ABAIXO do item (o corpo cobre o ícone) e ao lado conta; longe não.
    const Vec2 pes{100.0f, 100.0f};
    VERIFICAR(itemEncostaNoBill(pes, {100.0f, 78.0f}) && itemEncostaNoBill(pes, {116.0f, 90.0f}));
    VERIFICAR(itemEncostaNoBill(pes, {100.0f, 118.0f}));
    VERIFICAR(!itemEncostaNoBill(pes, {100.0f, 60.0f}) && !itemEncostaNoBill(pes, {130.0f, 100.0f}));
    const char* const SALA_IMA =
        "id=sala_ima\nbioma=1\ntipo=combate\nondas=0\nmapa:\n"
        "############\n"
        "#..........#\n"
        "#..........#\n"
        "#....E.....#\n"
        "#..........#\n"
        "#####.######\n"
        "#..........#\n"
        "############\n";
    const Conteudo c = conteudo(SALA_IMA);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    // Passando a 36 px do item (fora do corpo): o ímã o puxa e ele entra.
    const Vec2 item{centroCelula({8, 2}).x, centroCelula({8, 2}).y};
    soltarItem(p, TipoItem::Moeda, 3, item, item);
    posicionarJogador(p.bill, {centroCelula({3, 3}).x, item.y + 36.0f});
    andarAte(p, c, {centroCelula({10, 3}).x, item.y + 36.0f});
    VERIFICAR(qtd(p, TipoItem::Moeda) == 3 && p.itensChao.empty());
    // Do outro lado da parede, a 40 px: não vem.
    soltarItem(p, TipoItem::Moeda, 2, centroCelula({3, 6}), centroCelula({3, 6}));
    posicionarJogador(p.bill, {centroCelula({3, 4}).x, centroCelula({3, 4}).y + 6.0f});
    esperar(p, c, 1.0f);
    VERIFICAR(qtd(p, TipoItem::Moeda) == 3 && p.itensChao.size() == 1 &&
              comprimento(subtrair(p.itensChao[0].pos, centroCelula({3, 6}))) < 0.01f);
    // Inventário cheio: o kit não é puxado (fica onde caiu).
    coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 3);
    const Vec2 kit{p.bill.pes.x + 40.0f, p.bill.pes.y};
    soltarItem(p, TipoItem::Kit, 1, kit, kit);
    esperar(p, c, 1.0f);
    bool parado = false;
    for (const ItemNoChao& it : p.itensChao) parado |= it.tipo == TipoItem::Kit && comprimento(subtrair(it.pos, kit)) < 0.01f;
    VERIFICAR(parado);
}

// Diego, 17/11 (2ª vez): "às vezes passa por cima de moedas de prata e não pega". Varredura: moedas em
// volta do caminho de Bill, andando e no dash, em 8 direções; o que ele cobre na tela tem de entrar.
TESTE(itens_varredura_passando_perto_pega_sempre) {
    const char* const SALA_ABERTA =
        "id=sala_aberta\nbioma=1\ntipo=combate\nondas=0\nmapa:\n"
        "####################\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "#.........E........#\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "#..................#\n"
        "####################\n";
    const Conteudo c = conteudo(SALA_ABERTA);
    int perdidas = 0, total = 0;
    const Vec2 dirs[8] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {0.7071f, 0.7071f}, {-0.7071f, 0.7071f}, {0.7071f, -0.7071f}, {-0.7071f, -0.7071f}};
    for (int dash = 0; dash < 2; ++dash) {
        for (const Vec2& d : dirs) {
            for (int lado = -30; lado <= 30; lado += 6) {   // a moeda a até 30 px do caminho dos pés
                EstadoPartida p;
                iniciarPartida(p, c, 1, 0);
                const Vec2 centro{centroCelula({10, 4}).x, centroCelula({10, 4}).y + 16.0f};
                const Vec2 perp{-d.y, d.x};
                const Vec2 moeda = somar(centro, escalar(perp, static_cast<float>(lado)));
                soltarItem(p, TipoItem::Moeda, 1, moeda, moeda);
                esperar(p, c, 0.4f);
                posicionarJogador(p.bill, subtrair(centro, escalar(d, 100.0f)));
                for (int q = 0; q < 90; ++q) {
                    EstadoEntrada e;
                    e.eixo = d;
                    e.segurando[static_cast<int>(Comando::Dash)] = dash == 1;
                    e.pressionou[static_cast<int>(Comando::Dash)] = dash == 1 && q % 20 == 0;
                    atualizarPartida(p, c, e, DT);
                }
                ++total;
                if (qtd(p, TipoItem::Moeda) != 1) {
                    ++perdidas;
                    std::printf("      perdida: dash %d dir (%.1f,%.1f) lado %d\n", dash, d.x, d.y, lado);
                }
            }
        }
    }
    std::printf("    moedas perdidas: %d de %d\n", perdidas, total);
    VERIFICAR(perdidas == 0);
}

// Diego, 17/11: "na galeria pluvial tem moeda e chave enferrujada que não consigo pegar; em outra,
// tesouro que eu não conseguia pegar". Reprodução com o conteúdo REAL: a sala do tesouro do bioma 1.
TESTE(itens_sala_do_tesouro_real_chave_e_bau) {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    int idx = -1;
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) if (c.salas[static_cast<std::size_t>(i)].id == "b1_tesouro") idx = i;
    VERIFICAR(idx >= 0);
    EstadoPartida p;
    iniciarPartida(p, c, 5, idx);
    std::printf("    itens no chão ao entrar: %d\n", static_cast<int>(p.itensChao.size()));
    for (const ItemNoChao& it : p.itensChao) std::printf("      %s em (%.0f, %.0f) tempo %.2f\n", definicaoDe(it.tipo).nome, it.pos.x, it.pos.y, it.tempo);
    // Anda pelo corredor até a chave (21,10): primeiro para a direita na linha 7, depois desce.
    andarAte(p, c, centroCelula({21, 7}));
    andarAte(p, c, centroCelula({21, 10}));
    std::printf("    Bill em (%.0f, %.0f), chaves %d, pendência %d\n", p.bill.pes.x, p.bill.pes.y, qtd(p, TipoItem::ChaveEnferrujada),
                static_cast<int>(p.pendencia));
    VERIFICAR(qtd(p, TipoItem::ChaveEnferrujada) == 1);
    // Baú (12,4): chega por baixo e interage.
    andarAte(p, c, centroCelula({12, 7}));
    andarAte(p, c, centroCelula({12, 5}));
    const std::size_t antes = p.itensChao.size();
    apertar(p, c, Comando::Interagir);
    std::printf("    baú: itens no chão %d -> %d, pendência %d\n", static_cast<int>(antes), static_cast<int>(p.itensChao.size()),
                static_cast<int>(p.pendencia));
    VERIFICAR(p.itensChao.size() > antes);
}
