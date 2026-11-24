#include "core/rng.h"

namespace {

std::uint64_t splitmix64(std::uint64_t& estado) {
    std::uint64_t z = (estado += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

std::uint32_t rotacionar(std::uint32_t x, int k) { return (x << k) | (x >> (32 - k)); }

}  // namespace

void semear(Rng& rng, std::uint64_t seed) {
    std::uint64_t estado = seed;
    for (int i = 0; i < 4; i += 2) {
        const std::uint64_t v = splitmix64(estado);
        rng.s[i] = static_cast<std::uint32_t>(v);
        rng.s[i + 1] = static_cast<std::uint32_t>(v >> 32);
    }
    // xoshiro não pode ter estado todo zero.
    if ((rng.s[0] | rng.s[1] | rng.s[2] | rng.s[3]) == 0) {
        rng.s[0] = 1;
    }
}

std::uint32_t proximo(Rng& rng) {
    std::uint32_t* s = rng.s;
    const std::uint32_t resultado = rotacionar(s[1] * 5, 7) * 9;
    const std::uint32_t t = s[1] << 9;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotacionar(s[3], 11);
    return resultado;
}

int intervalo(Rng& rng, int minimo, int maximo) {
    if (maximo <= minimo) {
        return minimo;
    }
    const std::uint64_t largura = static_cast<std::uint64_t>(static_cast<std::int64_t>(maximo) - minimo) + 1;
    // Multiplicação de 64 bits (método de Lemire sem rejeição): viés desprezível para o jogo.
    const std::uint64_t r = (static_cast<std::uint64_t>(proximo(rng)) * largura) >> 32;
    return minimo + static_cast<int>(r);
}

float real01(Rng& rng) { return static_cast<float>(proximo(rng) >> 8) * (1.0f / 16777216.0f); }

bool chance(Rng& rng, float probabilidade) { return real01(rng) < probabilidade; }
