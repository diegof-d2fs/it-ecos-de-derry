#include "jogo/pontuacao.h"

#include <algorithm>

int segundosDeJogo(const Estatisticas& s) {
    return s.tempo > 0.0f ? static_cast<int>(s.tempo) : 0;
}

ParcelasPontuacao detalharPontuacao(const Estatisticas& s, int multiplicadorPct) {
    const long long T = segundosDeJogo(s);
    ParcelasPontuacao p{};
    p.inimigos = 100LL * s.K;
    p.elites = 300LL * s.E;
    p.itens = 50LL * s.I;
    p.resgates = 500LL * s.R;
    p.chefes = 1000LL * s.B;
    p.salas = 250LL * s.C;
    p.vitoria = 5000LL * s.V;
    p.vitoriaRapida = s.V > 0 ? std::max(0LL, BONUS_VITORIA_RAPIDA - 2 * T) : 0;
    p.movimento = s.M / 2;
    p.vidas = 1000LL * s.U;
    p.tempo = T / SEGUNDOS_POR_PONTO;
    p.bruto = p.inimigos + p.elites + p.itens + p.resgates + p.chefes + p.salas + p.vitoria + p.vitoriaRapida
            - p.movimento - p.vidas - p.tempo;
    p.multiplicadorPct = multiplicadorPct;
    // ⌊bruto × D⌋ com D em %: conta inteira (sem erro de float). Bruto negativo vira 0 antes.
    p.total = std::max(0LL, p.bruto) * multiplicadorPct / 100;
    return p;
}

long long calcularPontuacao(const Estatisticas& s, int multiplicadorPct) {
    return detalharPontuacao(s, multiplicadorPct).total;
}
