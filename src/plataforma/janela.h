#pragma once
// Janela Win32: criação, mensagens, foco, tela cheia e apresentação do canvas.
#include <windows.h>

#include <array>

#include "core/escala.h"
#include "render/canvas.h"

struct EstadoJanela {
    HWND hwnd = nullptr;
    int clienteW = 0;
    int clienteH = 0;
    bool focada = true;
    bool pedidoFechar = false;
    bool telaCheia = false;
    bool limparBordas = true;          // pinta as margens pretas só quando o tamanho muda
    WINDOWPLACEMENT posicaoJanela{};   // para voltar da tela cheia
    std::array<bool, 256> teclas{};    // estado bruto do teclado (códigos virtuais)
};

bool criarJanela(EstadoJanela& janela, HINSTANCE instancia, const wchar_t* titulo);
// Esvazia a fila de mensagens do Windows (não bloqueia).
void processarMensagens(EstadoJanela& janela);
// Copia o canvas para a janela em escala inteira, com vizinho mais próximo.
void apresentarCanvas(EstadoJanela& janela, const Canvas& canvas);
void alternarTelaCheia(EstadoJanela& janela);
Apresentacao apresentacaoAtual(const EstadoJanela& janela);
