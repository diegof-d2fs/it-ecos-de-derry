// Testes de colisão e movimento do Bill (SDD §10, critérios F05 e F06).
#include "entidades/jogador.h"
#include "mundo/colisao.h"
#include "teste.h"

namespace {

// Sala 8×6 com uma parede fina no meio e uma quina.
//   ########
//   #......#
//   #...#..#
//   #...#..#
//   #......#
//   ########
SalaViva salaDeTeste(SalaModelo& modelo) {
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n########\n#......#\n#...#..#\n#...#..#\n#..E...#\n########\n", modelo, erro);
    return criarSalaViva(modelo);
}

EstadoEntrada comEixo(Vec2 eixo) {
    EstadoEntrada e;
    e.eixo = eixo;
    return e;
}

}  // namespace

TESTE(colisao_parede_nas_quatro_direcoes) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    const Vec2 inicio = centroCelula({2, 2});
    // Empurrar muito em cada direção: para encostado na parede externa, sem atravessar.
    Vec2 p = moverComColisao(sala, inicio, CAIXA_BILL, {-500, 0}, false);
    VERIFICAR_PERTO(p.x, TILE + CAIXA_BILL.x / 2, 0.05f);
    p = moverComColisao(sala, inicio, CAIXA_BILL, {0, -500}, false);
    VERIFICAR_PERTO(p.y, TILE + CAIXA_BILL.y / 2, 0.05f);
    p = moverComColisao(sala, inicio, CAIXA_BILL, {500, 0}, false);
    VERIFICAR(p.x < 4 * TILE);   // parou na parede fina da coluna 4
    VERIFICAR_PERTO(p.x, 4 * TILE - CAIXA_BILL.x / 2, 0.05f);
    p = moverComColisao(sala, inicio, CAIXA_BILL, {0, 500}, false);
    VERIFICAR_PERTO(p.y, 5 * TILE - CAIXA_BILL.y / 2, 0.05f);
    VERIFICAR(!colideComSala(sala, caixaNosPes(p, CAIXA_BILL), false));
}

TESTE(colisao_desliza_na_parede_em_diagonal) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    // Encostado na parede de cima e andando na diagonal: o x continua mudando.
    const Vec2 inicio{2.5f * TILE, TILE + CAIXA_BILL.y / 2 + 0.02f};
    const Vec2 p = moverComColisao(sala, inicio, CAIXA_BILL, {20, -20}, false);
    VERIFICAR_PERTO(p.x, inicio.x + 20, 0.05f);
    VERIFICAR(p.y >= TILE + CAIXA_BILL.y / 2 - 0.05f);
}

TESTE(colisao_dash_nao_atravessa_parede_fina) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    // A parede da coluna 4 tem 1 tile de largura; um dash inteiro (96 px) não passa.
    const Vec2 inicio = centroCelula({2, 2});
    const Vec2 p = moverComColisao(sala, inicio, CAIXA_BILL, {DASH_DISTANCIA, 0}, false);
    VERIFICAR(p.x < 4 * TILE);
}

TESTE(colisao_quina_na_diagonal_nao_trava_nem_atravessa) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    // Saindo de baixo da parede fina (linha 4) em diagonal para cima-direita.
    Vec2 p = centroCelula({3, 4});
    for (int i = 0; i < 120; ++i) {
        p = moverComColisao(sala, p, CAIXA_BILL, {2.0f, -2.0f}, false);
        VERIFICAR(!colideComSala(sala, caixaNosPes(p, CAIXA_BILL), false));
    }
    VERIFICAR(p.x > 4 * TILE);   // contornou a quina e seguiu
}

TESTE(jogador_anda_dash_e_conta_movimentos) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    Jogador j;
    posicionarJogador(j, centroCelula({1, 1}));
    // Andar 1 s para baixo: 128 px/s com AGI 1 → atravessa ~3 células, mas a sala para antes.
    for (int i = 0; i < 60; ++i) atualizarJogador(j, sala, comEixo({0, 1}), DT);
    VERIFICAR(j.movimentos == 3);   // linhas 2, 3 e 4
    VERIFICAR(j.direcao == Direcao::Baixo);
    // Andar contra a parede não conta movimento nem anima.
    const int antes = j.movimentos;
    for (int i = 0; i < 30; ++i) atualizarJogador(j, sala, comEixo({0, 1}), DT);
    VERIFICAR(j.movimentos == antes);
    VERIFICAR(!j.andando);

    // Dash: invulnerável durante 0,15 s e com recarga.
    EstadoEntrada dash = comEixo({1, 0});
    dash.pressionou[static_cast<int>(Comando::Dash)] = true;
    atualizarJogador(j, sala, dash, DT);
    VERIFICAR(estaInvulneravel(j));
    for (int i = 0; i < 10; ++i) atualizarJogador(j, sala, comEixo({0, 0}), DT);
    VERIFICAR(!estaInvulneravel(j));
    const float xDepois = j.pes.x;
    atualizarJogador(j, sala, dash, DT);   // ainda em recarga: não dispara
    VERIFICAR(!estaInvulneravel(j));
    VERIFICAR(j.pes.x - xDepois < 5.0f);
}

TESTE(jogador_agua_rasa_deixa_mais_lento) {
    SalaModelo m;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n##########\n#E.......#\n#~~~~~~~~#\n##########\n", m, erro);
    const SalaViva sala = criarSalaViva(m);
    Jogador seco, molhado;
    posicionarJogador(seco, centroCelula({1, 1}));
    posicionarJogador(molhado, centroCelula({1, 2}));
    for (int i = 0; i < 30; ++i) {
        atualizarJogador(seco, sala, comEixo({1, 0}), DT);
        atualizarJogador(molhado, sala, comEixo({1, 0}), DT);
    }
    const float andouSeco = seco.pes.x - centroCelula({1, 1}).x;
    const float andouMolhado = molhado.pes.x - centroCelula({1, 2}).x;
    VERIFICAR_PERTO(andouMolhado / andouSeco, 0.7f, 0.02f);
}

TESTE(colisao_porta_tocada_devolve_ponteiro_ou_nulo) {
    SalaModelo m;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n#####\n#E..L\n#####\n", m, erro);
    const SalaViva sala = criarSalaViva(m);
    VERIFICAR(portaTocada(sala, centroCelula({1, 1}), CAIXA_BILL) == nullptr);
    const PortaSala* p = portaTocada(sala, centroCelula({4, 1}), CAIXA_BILL);
    VERIFICAR(p != nullptr && p->lado == Direcao::Direita);
}
