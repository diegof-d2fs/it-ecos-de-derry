#pragma once
// Gerador pseudoaleatório determinístico (SDD §9): xoshiro128** semeado por splitmix64.
// A mesma seed sempre produz a mesma sequência, o que permite reproduzir uma tentativa
// e escrever testes. Nunca usar rand() nem o relógio no meio da partida.
#include <cstdint>

struct Rng {
    std::uint32_t s[4] = {1, 2, 3, 4};
};

void semear(Rng& rng, std::uint64_t seed);
std::uint32_t proximo(Rng& rng);
int intervalo(Rng& rng, int minimo, int maximo);   // inteiro em [minimo, maximo]
float real01(Rng& rng);                            // real em [0, 1)
bool chance(Rng& rng, float probabilidade);        // probabilidade em [0, 1]
