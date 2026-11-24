#include "core/tipos.h"

#include <cmath>

Vec2 somar(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
Vec2 subtrair(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
Vec2 escalar(Vec2 v, float k) { return {v.x * k, v.y * k}; }
float comprimento(Vec2 v) { return std::sqrt(v.x * v.x + v.y * v.y); }

Vec2 normalizar(Vec2 v) {
    const float c = comprimento(v);
    if (c < 1e-6f) {
        return {0.0f, 0.0f};
    }
    return {v.x / c, v.y / c};
}

Direcao direcaoDominante(Vec2 v, Direcao atual) {
    const float ax = std::fabs(v.x);
    const float ay = std::fabs(v.y);
    if (ax < 1e-4f && ay < 1e-4f) {
        return atual;
    }
    // Empate (diagonal perfeita): mantém o eixo horizontal, que lê melhor de lado.
    if (ax >= ay) {
        return v.x < 0.0f ? Direcao::Esquerda : Direcao::Direita;
    }
    return v.y < 0.0f ? Direcao::Cima : Direcao::Baixo;
}

Direcao direcaoComHisterese(Vec2 v, Direcao atual, float razao) {
    const float ax = std::fabs(v.x), ay = std::fabs(v.y);
    if (ax < 1e-4f && ay < 1e-4f) return atual;
    const bool horizontal = atual == Direcao::Esquerda || atual == Direcao::Direita;
    const bool ficaHorizontal = horizontal ? ay <= ax * razao : ax > ay * razao;
    if (ficaHorizontal) return v.x < 0.0f ? Direcao::Esquerda : Direcao::Direita;
    return v.y < 0.0f ? Direcao::Cima : Direcao::Baixo;
}

Celula celulaDe(Vec2 posicao) {
    return {static_cast<int>(std::floor(posicao.x / TILE)), static_cast<int>(std::floor(posicao.y / TILE))};
}

float limitar(float valor, float minimo, float maximo) {
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}

int limitarInt(int valor, int minimo, int maximo) {
    if (valor < minimo) return minimo;
    if (valor > maximo) return maximo;
    return valor;
}
