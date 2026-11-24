#include "mundo/caminho.h"

#include <array>
#include <cstdlib>

namespace {

bool livre(const SalaViva& s, int c, int l, bool voa) {
    const Tile& t = tileEm(s, c, l);
    return voa ? !t.bloqueiaVoo : !t.bloqueiaMovimento;
}

// BFS a partir do DESTINO: a distância de cada célula até ele. Fila em vetor fixo (sem alocação).
void campoDistancia(const SalaViva& s, Celula destino, bool voa, std::array<short, SALA_MAX_CELULAS>& dist) {
    dist.fill(-1);
    if (!dentroDaSala(s, destino.col, destino.lin)) return;
    std::array<short, SALA_MAX_CELULAS> fila{};
    int inicio = 0, fim = 0;
    const int d0 = destino.lin * s.cols + destino.col;
    dist[static_cast<std::size_t>(d0)] = 0;
    fila[static_cast<std::size_t>(fim++)] = static_cast<short>(d0);
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    while (inicio < fim) {
        const int atual = fila[static_cast<std::size_t>(inicio++)];
        const int c = atual % s.cols, l = atual / s.cols;
        for (int k = 0; k < 4; ++k) {
            const int nc = c + dc[k], nl = l + dl[k];
            if (!dentroDaSala(s, nc, nl) || !livre(s, nc, nl, voa)) continue;
            const int i = nl * s.cols + nc;
            if (dist[static_cast<std::size_t>(i)] >= 0) continue;
            dist[static_cast<std::size_t>(i)] = static_cast<short>(dist[static_cast<std::size_t>(atual)] + 1);
            fila[static_cast<std::size_t>(fim++)] = static_cast<short>(i);
        }
    }
}

}  // namespace

Celula proximoPasso(const SalaViva& s, Celula origem, Celula destino, bool voa) {
    if (origem.col == destino.col && origem.lin == destino.lin) return origem;
    std::array<short, SALA_MAX_CELULAS> dist;
    campoDistancia(s, destino, voa, dist);
    if (!dentroDaSala(s, origem.col, origem.lin)) return {-1, -1};
    // O vizinho com a menor distância ao destino. Empate: primeiro o EIXO em que falta mais (anda
    // "em diagonal", como uma pessoa). Com a ordem fixa horizontal-primeiro, um perseguidor lento
    // só acompanhava o x de quem anda de um lado para o outro e nunca chegava (Etapa 8.5).
    // A ordem continua fixa para a mesma situação: resultado determinístico.
    const bool vertical = std::abs(destino.lin - origem.lin) > std::abs(destino.col - origem.col);
    const int sc = destino.col >= origem.col ? 1 : -1, sl = destino.lin >= origem.lin ? 1 : -1;
    const int hc[4] = {sc, -sc, 0, 0}, hl[4] = {0, 0, sl, -sl};   // horizontal primeiro
    const int vc[4] = {0, 0, sc, -sc}, vl[4] = {sl, -sl, 0, 0};   // vertical primeiro
    const int* dc = vertical ? vc : hc;
    const int* dl = vertical ? vl : hl;
    Celula melhor{-1, -1};
    int melhorDist = 1 << 20;
    for (int k = 0; k < 4; ++k) {
        const int nc = origem.col + dc[k], nl = origem.lin + dl[k];
        if (!dentroDaSala(s, nc, nl)) continue;
        const int d = dist[static_cast<std::size_t>(nl * s.cols + nc)];
        if (d >= 0 && d < melhorDist) {
            melhorDist = d;
            melhor = {nc, nl};
        }
    }
    return melhor;
}

int distanciaCaminho(const SalaViva& s, Celula origem, Celula destino, bool voa) {
    std::array<short, SALA_MAX_CELULAS> dist;
    campoDistancia(s, destino, voa, dist);
    if (!dentroDaSala(s, origem.col, origem.lin)) return -1;
    return dist[static_cast<std::size_t>(origem.lin * s.cols + origem.col)];
}
