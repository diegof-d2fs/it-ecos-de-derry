#pragma once
// Tipos fundamentais compartilhados por todo o jogo (SDD §3).
// Este arquivo pertence às "regras": não pode incluir nada do Windows.
#include <cstdint>

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Celula {
    int col = 0;
    int lin = 0;
};

struct Retangulo {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
};

using IdEntidade = std::uint32_t;
constexpr IdEntidade ID_NULO = 0;

// A ordem é a mesma das linhas dos atlas de sprites: baixo, esquerda, direita, cima.
enum class Direcao : std::uint8_t { Baixo = 0, Esquerda = 1, Direita = 2, Cima = 3 };

constexpr int TILE = 32;                    // lado de uma célula em pixels
constexpr int LARGURA_LOGICA = 640;         // resolução lógica 16:9 (PRD v2 §19.2)
constexpr int ALTURA_LOGICA = 360;
constexpr float DT = 1.0f / 60.0f;          // passo fixo da lógica (60 Hz)

// ---------- operações pequenas de vetor (por valor: são baratas de copiar) ----------
Vec2 somar(Vec2 a, Vec2 b);
Vec2 subtrair(Vec2 a, Vec2 b);
Vec2 escalar(Vec2 v, float k);
float comprimento(Vec2 v);
Vec2 normalizar(Vec2 v);                    // vetor nulo continua nulo

// Direção dominante de um vetor de movimento; com vetor nulo devolve `atual`.
Direcao direcaoDominante(Vec2 v, Direcao atual);
// Com histerese: mantém o eixo atual até o outro dominar por `razao` (> 1). Na diagonal o sprite não pisca
// entre "de lado" e "de costas" (Diego, 24/11: Bill e os chefes mais fluidos).
Direcao direcaoComHisterese(Vec2 v, Direcao atual, float razao);

// Célula que contém um ponto em pixels do mundo.
Celula celulaDe(Vec2 posicao);

float limitar(float valor, float minimo, float maximo);
int limitarInt(int valor, int minimo, int maximo);
