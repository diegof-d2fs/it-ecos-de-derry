#pragma once
// Escala inteira da imagem lógica dentro da área cliente da janela (PRD v2 §19.2).
// Pura: não conhece o Windows, por isso é testada sem janela.

struct Apresentacao {
    int escala = 0;      // 0 = a janela é menor que a resolução lógica
    int x = 0;           // canto superior esquerdo da imagem ampliada, centralizada
    int y = 0;
    int largura = 0;     // tamanho da imagem ampliada
    int altura = 0;
};

// Maior escala inteira que cabe; `escalaMaxima` 0 = sem limite.
Apresentacao calcularApresentacao(int clienteW, int clienteH, int logicaW, int logicaH, int escalaMaxima = 0);
