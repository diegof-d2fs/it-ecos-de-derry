#include <cmath>
// Testes de atributos, XP, áreas e ações do Bill (PRD §7–§8, SDD §11 e §14).
#include "combate/area.h"
#include "combate/atributos.h"
#include "combate/experiencia.h"
#include "entidades/jogador.h"
#include "jogo/partida.h"
#include "teste.h"

TESTE(combate_exemplo_verificavel_do_prd) {
    // PRD §7.3: FOR 3, FOC 2, AGI 2, COR 3, taco (+2) e escudo (+2).
    const Atributos a{3, 2, 2, 3};
    Equipamento e;
    e.bonusDanoArma = 2;
    e.armaduraEscudo = 2;
    const StatsDerivados s = calcularStats(a, e);
    VERIFICAR(s.danoBase == 14);
    VERIFICAR(s.hpMax == 58);
    VERIFICAR(s.armadura == 3);
    VERIFICAR_PERTO(s.esquiva, 8.0f, 1e-4f);
    VERIFICAR_PERTO(s.velocidade, 136.0f, 1e-4f);
    VERIFICAR_PERTO(s.recargaDash, 0.64f, 1e-4f);
    VERIFICAR_PERTO(chanceAcerto(2, 1), 84.0f, 1e-4f);   // contra AGI 1
    VERIFICAR(danoEfetivo(14, 1) == 13);
    VERIFICAR(danoEfetivo(danoCritico(14), 1) == 20);
    VERIFICAR_PERTO(s.critico, 4.0f, 1e-4f);
}

TESTE(combate_limites_das_formulas) {
    VERIFICAR_PERTO(chanceAcerto(1, 9), 65.0f, 1e-4f);
    VERIFICAR_PERTO(chanceAcerto(1, 30), 60.0f, 1e-4f);   // nunca abaixo de 60
    VERIFICAR_PERTO(chanceAcerto(9, 0), 98.0f, 1e-4f);    // nunca acima de 98
    VERIFICAR(danoEfetivo(3, 10) == 1);                   // dano mínimo 1
    VERIFICAR_PERTO(chanceEsquiva(9), 29.0f, 1e-4f);
    VERIFICAR_PERTO(chanceEsquiva(20), 30.0f, 1e-4f);     // esquiva máxima 30%
}

TESTE(xp_atravessa_limiares_e_da_pontos) {
    Progresso p;
    VERIFICAR(nivelParaXp(LIMIARES_XP[2] - 1) == 1 && nivelParaXp(LIMIARES_XP[2]) == 2 && nivelParaXp(100000) == NIVEL_MAXIMO);
    VERIFICAR(concederXp(p, 30) == 0 && p.nivel == 1);
    VERIFICAR(concederXp(p, LIMIARES_XP[3] - 30 + 10) == 2);   // atravessou os limiares do N2 e do N3 de uma vez
    VERIFICAR(p.nivel == 3 && p.pontosPendentes == 4);
    concederXp(p, 100000);
    VERIFICAR(p.nivel == NIVEL_MAXIMO);
    int forca = 8;
    VERIFICAR(aplicarPonto(p, forca) && forca == 9);
    VERIFICAR(!aplicarPonto(p, forca) && forca == 9);   // máximo 9
    Progresso semPontos;
    int foco = 1;
    VERIFICAR(!aplicarPonto(semPontos, foco));
}

TESTE(combate_coragem_aumenta_hp_maximo_e_atual) {
    Jogador j;
    recalcularJogador(j);
    j.hp = 20;
    j.attr.coragem = 2;
    recalcularJogador(j);
    VERIFICAR(j.stats.hpMax == 52 && j.hp == 26);   // +6 no máximo e no atual
}

TESTE(combate_areas_acertam_o_que_mostram) {
    const Area circulo = areaCirculo({100, 100}, 30);
    VERIFICAR(areaToca(circulo, {125, 95, 10, 10}));
    VERIFICAR(!areaToca(circulo, {140, 95, 10, 10}));
    const Area arco = areaArco({100, 100}, {1, 0}, 40, 100);
    VERIFICAR(areaToca(arco, {125, 95, 10, 10}));    // à frente
    VERIFICAR(!areaToca(arco, {65, 95, 10, 10}));    // atrás: não acerta
    const Area linha = areaLinha({0, 0}, {0, 1}, 128, 24);
    VERIFICAR(areaToca(linha, {-5, 100, 10, 10}));
    VERIFICAR(!areaToca(linha, {30, 100, 10, 10}));
    VERIFICAR(!areaToca(linha, {-5, 145, 10, 10}));   // além do alcance (+ a meia-largura de 12 px)
}

TESTE(combate_combo_especial_e_clarao) {
    Jogador j;
    recalcularJogador(j);
    std::vector<Golpe> golpes;
    std::vector<IdSom> sons;
    std::uint32_t prox = 1;
    EstadoEntrada atacar;
    atacar.pressionou[static_cast<int>(Comando::Atacar)] = true;
    const EstadoEntrada nada;
    // Golpe 1; apertar de novo durante o golpe encadeia o 2; depois o 3 (mais forte).
    acoesDoJogador(j, atacar, DT, golpes, prox, sons);
    VERIFICAR(golpes.size() == 1 && j.combo == 1);
    acoesDoJogador(j, atacar, DT, golpes, prox, sons);
    for (int i = 0; i < 20; ++i) acoesDoJogador(j, nada, DT, golpes, prox, sons);
    VERIFICAR(golpes.size() == 2 && j.combo == 2);
    acoesDoJogador(j, atacar, DT, golpes, prox, sons);
    for (int i = 0; i < 25; ++i) acoesDoJogador(j, nada, DT, golpes, prox, sons);
    VERIFICAR(golpes.size() == 3 && j.combo == 3);
    VERIFICAR(golpes[2].dano > golpes[0].dano && golpes[2].recuo > golpes[0].recuo);
    // Sem apertar dentro da janela, o combo volta ao golpe 1.
    for (int i = 0; i < 40; ++i) acoesDoJogador(j, nada, DT, golpes, prox, sons);
    acoesDoJogador(j, atacar, DT, golpes, prox, sons);
    VERIFICAR(j.combo == 1);

    // Especial: dano ×2 e recarga de 3 s.
    Jogador e;
    recalcularJogador(e);
    std::vector<Golpe> g2;
    EstadoEntrada especial;
    especial.pressionou[static_cast<int>(Comando::Especial)] = true;
    acoesDoJogador(e, especial, DT, g2, prox, sons);
    VERIFICAR(g2.size() == 1 && g2[0].dano == e.stats.danoBase * 2);
    for (int i = 0; i < 30; ++i) acoesDoJogador(e, nada, DT, g2, prox, sons);
    acoesDoJogador(e, especial, DT, g2, prox, sons);
    VERIFICAR(g2.size() == 1);   // ainda em recarga

    // Clarão: gasta carga, não erra, atordoa; sem carga não sai.
    Jogador c;
    recalcularJogador(c);
    c.cargasLanterna = 1;
    std::vector<Golpe> g3;
    EstadoEntrada clarao;
    clarao.pressionou[static_cast<int>(Comando::Clarao)] = true;
    acoesDoJogador(c, clarao, DT, g3, prox, sons);
    VERIFICAR(g3.size() == 1 && !g3[0].rolaAcerto && g3[0].atordoa > 1.0f && c.cargasLanterna == 0);
    for (int i = 0; i < 30; ++i) acoesDoJogador(c, nada, DT, g3, prox, sons);
    acoesDoJogador(c, clarao, DT, g3, prox, sons);
    VERIFICAR(g3.size() == 1);
}

// Diego, 17/11: "tenho que apertar duas vezes para golpear". Um toque durante o hit-stop (o
// congelamento do acerto) ou no fim do dash era descartado; agora fica guardado e o golpe sai.
TESTE(combate_toque_de_ataque_no_hit_stop_nao_se_perde) {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    lerSala("id=s\nbioma=1\ntipo=combate\nondas=0\nmapa:\n##########\n#........#\n#...E....#\n#........#\n##########\n", m, erro);
    c.salas.push_back(m);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    p.hitStop = 0.08f;   // congelado por um acerto
    EstadoEntrada toque;
    toque.pressionou[static_cast<int>(Comando::Atacar)] = true;
    atualizarPartida(p, c, toque, DT);   // o toque cai no congelamento
    for (int q = 0; q < 8; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.acao == AcaoJogador::Atacando || p.bill.combo >= 1);   // o golpe saiu sem 2º toque
}

TESTE(combate_balao_que_estoura_mantem_o_sprite_da_explosao) {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    VERIFICAR(lerSala("id=s\nbioma=1\ntipo=combate\nondas=0\nmapa:\n##########\n#........#\n#...E....#\n#........#\n##########\n", m, erro));
    c.salas.push_back(m);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    p.inimigos.push_back(criarInimigo(TipoInimigo::Balao, 99, centroCelula({6, 2})));
    Aviso a;
    a.tempoTotal = 0.8f;
    a.tempoRestante = DT / 2.0f;
    a.golpe.origem = 99;
    a.golpe.area = areaCirculo(centroCelula({6, 2}), 48.0f);
    a.golpe.equipe = Equipe::Todos;
    a.consomeOrigem = true;
    p.avisos.push_back(a);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.inimigos.empty());
    VERIFICAR(p.efeitosAtaque.size() == 1 && p.efeitosAtaque[0].atacante == TipoInimigo::Balao);
    atualizarPartida(p, c, EstadoEntrada{}, DT);   // o golpe acerta, sem criar um efeito genérico duplicado
    VERIFICAR(p.efeitosAtaque.size() == 1 && p.efeitosAtaque[0].atacante == TipoInimigo::Balao);
}

// Diego, 17/11: "o Bill só ataca para cima, baixo, direita ou esquerda". Parado depois de andar na
// diagonal, o golpe sai na DIAGONAL (o último rumo), não na direção do sprite.
TESTE(combate_ataque_parado_sai_na_diagonal_do_ultimo_rumo) {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    lerSala("id=s\nbioma=1\ntipo=combate\nondas=0\nmapa:\n##########\n#........#\n#........#\n#...E....#\n#........#\n#........#\n##########\n", m, erro);
    c.salas.push_back(m);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    EstadoEntrada anda;
    anda.eixo = normalizar(Vec2{1.0f, -1.0f});
    for (int q = 0; q < 10; ++q) atualizarPartida(p, c, anda, DT);
    for (int q = 0; q < 5; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);   // parou
    EstadoEntrada toque;
    toque.pressionou[static_cast<int>(Comando::Atacar)] = true;
    atualizarPartida(p, c, toque, DT);
    bool diagonal = false;
    for (const Golpe& g : p.golpes) {
        if (g.equipe != Equipe::Jogador) continue;
        diagonal |= std::fabs(g.area.direcao.x - 0.7071f) < 0.05f && std::fabs(g.area.direcao.y + 0.7071f) < 0.05f;
    }
    VERIFICAR(diagonal);
}
