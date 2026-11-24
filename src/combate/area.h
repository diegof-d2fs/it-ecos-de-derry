#pragma once
// Áreas de golpes e avisos (SDD §3.3). A mesma descrição serve para três coisas: o dano
// (sobreposição com a caixa do alvo), o aviso desenhado no chão e os testes. Assim o que o
// jogador VÊ no aviso é exatamente o que ACERTA.
#include "core/tipos.h"

// Anel (Etapa 10.3): coroa entre raioInterno e raio — o centro é SEGURO (as ondas do Afogado).
enum class FormaArea : std::uint8_t { Circulo, Arco, Linha, Anel };

struct Area {
    FormaArea forma = FormaArea::Circulo;
    Vec2 centro;                 // círculo/arco: centro; linha: ponto de partida
    Vec2 direcao{1.0f, 0.0f};    // arco e linha (unitário)
    float raio = 0.0f;           // círculo/arco: raio; linha: meia-largura
    float comprimento = 0.0f;    // linha
    float anguloGraus = 0.0f;    // arco: abertura total
    float raioInterno = 0.0f;    // anel: dentro deste raio não acerta
};

Area areaCirculo(Vec2 centro, float raio);
Area areaArco(Vec2 centro, Vec2 direcao, float raio, float anguloGraus);
Area areaLinha(Vec2 inicio, Vec2 direcao, float comprimento, float largura);
Area areaAnel(Vec2 centro, float raioInterno, float raioExterno);

bool pontoNaArea(const Area& a, Vec2 p);
// Sobreposição área × retângulo (caixa de dano). Exata para círculo e linha; o arco testa o
// ponto do retângulo mais próximo do centro e o centro do retângulo (suficiente para caixas
// pequenas como as dos atores).
bool areaToca(const Area& a, const Retangulo& r);
// Retângulo que envolve a área (para desenhar só onde importa).
Retangulo envolvente(const Area& a);
