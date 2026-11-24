#pragma once
// Canvas: o quadro onde tudo é desenhado antes de ir para a janela.
//
// Resolução: o jogo pensa em 640 × 360 (unidades LÓGICAS, tiles de 32), mas o canvas tem
// 640·escala × 360·escala pixels de TELA (escala 3 num monitor 1080p). Assim os assets HD
// aparecem com todo o detalhe e o texto fica nítido. As funções abaixo recebem coordenadas
// LÓGICAS (float) e convertem; só `desenharRecorteTela` trabalha direto em pixels de tela.
//
// É uma DIB section do Windows: `px` aponta direto para a memória da imagem, e o mesmo `dc`
// permite escrever texto com o GDI. Um único buffer apresentado inteiro = sem piscar (F02).
#ifndef __EMSCRIPTEN__
#include <windows.h>
#endif

#include <cstdint>

#include "render/imagem.h"

struct Canvas {
    int w = 0;             // em pixels de tela
    int h = 0;
    int escala = 1;        // pixels de tela por unidade lógica
    std::uint32_t* px = nullptr;   // pertence à DIB (web: ao próprio canvas); liberado em liberarCanvas
#ifdef __EMSCRIPTEN__
    void* dc = nullptr;            // na web não há DC: o texto é desenhado direto em `px` (render/texto_web)
#else
    HDC dc = nullptr;
    HBITMAP dib = nullptr;
    HGDIOBJ bitmapAnterior = nullptr;
#endif
};

// Cor ARGB opaca a partir de RGB.
constexpr std::uint32_t rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return 0xFF000000u | (static_cast<std::uint32_t>(r) << 16) | (static_cast<std::uint32_t>(g) << 8) | b;
}

// Cria um canvas de (larguraLogica × escala) por (alturaLogica × escala).
bool criarCanvas(Canvas& canvas, int larguraLogica, int alturaLogica, int escala);
void liberarCanvas(Canvas& canvas);

// Converte uma coordenada lógica em pixel de tela (arredonda para o mais próximo).
int paraTela(const Canvas& canvas, float logico);

void limpar(Canvas& canvas, std::uint32_t cor);
void retanguloCheio(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor);
// Mistura uma cor sólida com opacidade 0..255 (sombras, painéis, escurecer a tela).
void retanguloAlpha(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor, std::uint8_t alpha);
// Contorno com espessura de 1 unidade lógica.
void contornoRetangulo(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor);
// Elipse preenchida translúcida (sombra oval sob os personagens).
void elipseAlpha(Canvas& canvas, float cx, float cy, float rx, float ry, std::uint32_t cor, std::uint8_t alpha);

// Copia um recorte (sx, sy, sw, sh) de `imagem` para (dx, dy), tudo em PIXELS DE TELA,
// com mistura alfa. `espelhar` inverte na horizontal; `opacidade` 0..255 multiplica o alfa.
void desenharRecorteTela(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                         bool espelhar = false, std::uint8_t opacidade = 255);
// Igual, mas tingindo cada pixel na direção de `cor` (0xRRGGBB) com força 0..255 (core/pixels: tingir).
void desenharRecorteTelaTinta(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                              bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca);
// Igual, mas AMPLIADO/REDUZIDO para dw × dh (vizinho mais próximo: o pixel art continua nítido). Os estágios
// do Pennywise têm tamanhos diferentes (24/11, Diego: "I e I+ parecem o mesmo").
void desenharRecorteTelaEscalado(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                                 int dw, int dh, bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca);
// GIRADO em volta do centro (cx, cy) em pixels de tela, com escala (vizinho mais próximo). O corte do taco é
// desenhado apontando para a direita e girado para o rumo do golpe: a diagonal fica tão bonita quanto o eixo
// (Diego, 24/11: "melhorar os golpes diagonais").
void desenharRecorteTelaGirado(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, float cx, float cy,
                               float angulo, float escala, std::uint8_t opacidade);
// Contorno da SILHUETA do recorte: pinta `cor` nos pixels transparentes vizinhos (até `espessura`
// pixels de tela) de um pixel opaco. Desenha só fora do sprite (pode vir antes ou depois dele).
void contornoRecorteTela(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                         bool espelhar, std::uint32_t cor, int espessura);
// Ilustração em tela cheia com zoom/panorâmica suaves (cenas da história): cada pixel de tela (x, y)
// lê a imagem em (origemX + (x + 0,5)·passo − 0,5, idem y), com interpolação BILINEAR — o movimento
// lento não "treme" de pixel em pixel. `passo` = pixels da imagem por pixel de tela.
void desenharImagemAmostrada(Canvas& canvas, const Imagem& imagem, float origemX, float origemY, float passo,
                             std::uint8_t opacidade = 255);
// Desenha a imagem inteira (já preparada na escala do canvas) com o canto em (x, y) lógico.
void desenharImagem(Canvas& canvas, const Imagem& imagem, float x, float y);

// Salva o quadro atual em BMP de 24 bits (modo de captura para testes e documentação).
bool salvarBmp(const Canvas& canvas, const wchar_t* caminho);
