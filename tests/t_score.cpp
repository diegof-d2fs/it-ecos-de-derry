// Testes da pontuação (PRD §16, SDD §24.3): os três exemplos do SDD conferidos à mão e as regras
// de borda (nunca negativa, T inteiro, bônus de vitória rápida só com vitória).
#include "jogo/pontuacao.h"
#include "teste.h"

namespace {

// Os valores dos exemplos do PRD §16.
Estatisticas exemploVitoria(float tempo) {
    Estatisticas s;
    s.K = 140; s.E = 10; s.I = 60; s.R = 6; s.B = 6; s.C = 24; s.V = 1; s.M = 8000; s.U = 1;
    s.tempo = tempo;
    return s;
}

Estatisticas exemploDerrota(float tempo) {
    Estatisticas s;
    s.K = 25; s.E = 1; s.I = 12; s.R = 1; s.B = 1; s.C = 5; s.V = 0; s.M = 1500; s.U = 2;
    s.tempo = tempo;
    return s;
}

}  // namespace

TESTE(score_exemplos_do_sdd) {
    // Vitória no Normal com T = 3000 s: 35.000 + 3.000 (rápida) − 600 (tempo) = 37.400.
    VERIFICAR(calcularPontuacao(exemploVitoria(3000.0f), 100) == 37400);
    // Derrota no bioma 2 com T = 900 s: 3.400 − 180 = 3.220.
    VERIFICAR(calcularPontuacao(exemploDerrota(900.0f), 100) == 3220);
    // A mesma vitória no Difícil: ⌊37.400 × 1,5⌋ = 56.100; no Fácil: ⌊37.400 × 0,75⌋ = 28.050.
    VERIFICAR(calcularPontuacao(exemploVitoria(3000.0f), 150) == 56100);
    VERIFICAR(calcularPontuacao(exemploVitoria(3000.0f), 75) == 28050);
}

TESTE(score_parcelas_somam_o_total) {
    const ParcelasPontuacao p = detalharPontuacao(exemploVitoria(3000.0f), 100);
    VERIFICAR(p.inimigos == 14000 && p.elites == 3000 && p.itens == 3000 && p.resgates == 3000);
    VERIFICAR(p.chefes == 6000 && p.salas == 6000 && p.vitoria == 5000 && p.vitoriaRapida == 3000);
    VERIFICAR(p.movimento == 4000 && p.vidas == 1000 && p.tempo == 600);
    VERIFICAR(p.bruto == 37400 && p.total == 37400);
}

TESTE(score_bonus_rapido_so_com_vitoria_e_nunca_negativo) {
    Estatisticas s = exemploDerrota(0.0f);
    VERIFICAR(detalharPontuacao(s, 100).vitoriaRapida == 0);
    Estatisticas lenta = exemploVitoria(5000.0f);           // 9000 − 10000 < 0 → bônus 0
    VERIFICAR(detalharPontuacao(lenta, 100).vitoriaRapida == 0);
    Estatisticas nada;
    nada.M = 5000; nada.U = 1; nada.tempo = 600.0f;         // só perdas
    VERIFICAR(calcularPontuacao(nada, 150) == 0);
}

TESTE(score_tempo_conta_em_segundos_inteiros) {
    Estatisticas s;
    s.K = 1;
    s.tempo = 4.99f;
    VERIFICAR(segundosDeJogo(s) == 4 && calcularPontuacao(s, 100) == 100);   // ⌊4/5⌋ = 0
    s.tempo = 5.0f;
    VERIFICAR(calcularPontuacao(s, 100) == 99);
    s.tempo = -1.0f;
    VERIFICAR(segundosDeJogo(s) == 0);
    // Andar custa: cada 2 células novas = −1 ponto (PRD §16).
    Estatisticas a;
    a.K = 10;
    a.M = 3;
    VERIFICAR(calcularPontuacao(a, 100) == 999);
}
