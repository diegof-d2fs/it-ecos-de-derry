#include "core/medidor.h"

#include <algorithm>

void registrarQuadro(MedidorQuadros& m, float ms) {
    m.amostrasMs[m.proxima] = ms;
    m.proxima = (m.proxima + 1) % static_cast<int>(m.amostrasMs.size());
    if (m.total < static_cast<int>(m.amostrasMs.size())) {
        ++m.total;
    }
}

float mediaMs(const MedidorQuadros& m) {
    if (m.total == 0) {
        return 0.0f;
    }
    float soma = 0.0f;
    for (int i = 0; i < m.total; ++i) {
        soma += m.amostrasMs[i];
    }
    return soma / static_cast<float>(m.total);
}

float percentil95Ms(const MedidorQuadros& m) {
    if (m.total == 0) {
        return 0.0f;
    }
    std::array<float, 240> copia = m.amostrasMs;
    const int indice = std::min(m.total - 1, (m.total * 95) / 100);
    std::nth_element(copia.begin(), copia.begin() + indice, copia.begin() + m.total);
    return copia[indice];
}
