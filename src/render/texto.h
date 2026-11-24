#pragma once
// Texto com GDI escrito direto no canvas, na resolução de TELA (nítido em qualquer escala).
// As fontes são recriadas quando a escala muda (recursos.cpp); as coordenadas são lógicas.
#include <windows.h>

#include <cstdint>
#include <string>

#include "render/canvas.h"

struct Fonte {
    HFONT handle = nullptr;
    int alturaLogica = 0;
};

// `alturaLogica` em unidades do jogo; a fonte real tem alturaLogica × escala pixels.
bool criarFonte(Fonte& fonte, const wchar_t* nome, int alturaLogica, int escala, bool negrito);
void liberarFonte(Fonte& fonte);

// Converte UTF-8 (literais e arquivos de dados) para UTF-16 (API do Windows).
std::wstring paraWide(const std::string& utf8);

// Desenha texto com sombra (legibilidade sobre o cenário).
void desenharTexto(Canvas& canvas, const Fonte& fonte, float x, float y, const std::wstring& texto, std::uint32_t cor,
                   bool sombra = true);
float larguraTexto(Canvas& canvas, const Fonte& fonte, const std::wstring& texto);   // em unidades lógicas
void desenharTextoCentralizado(Canvas& canvas, const Fonte& fonte, float centroX, float y, const std::wstring& texto,
                               std::uint32_t cor);
