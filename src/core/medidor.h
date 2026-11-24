#pragma once
// Medidor de tempo de quadro (critério F01): guarda as últimas 240 amostras em anel
// e calcula média e percentil 95, mostrados no Modo Defesa.
#include <array>

struct MedidorQuadros {
    std::array<float, 240> amostrasMs{};
    int proxima = 0;
    int total = 0;
};

void registrarQuadro(MedidorQuadros& m, float ms);
float mediaMs(const MedidorQuadros& m);
float percentil95Ms(const MedidorQuadros& m);
