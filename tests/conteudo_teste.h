#pragma once
// Auxiliares de conteúdo para os testes.
#include <algorithm>
#include <vector>

#include "mundo/sala.h"

// Tira as salas dos biomas depois de `ultimo` (o Clube e o prólogo, bioma 0, ficam). Serve aos
// testes que medem "a tentativa do bioma 1" e precisam que ela termine em "fim do conteúdo",
// sem depender de quantos biomas já existem em data/salas.
inline void limitarAoBioma(std::vector<SalaModelo>& salas, int ultimo) {
    salas.erase(std::remove_if(salas.begin(), salas.end(), [&](const SalaModelo& s) { return s.bioma > ultimo; }),
                salas.end());
}
