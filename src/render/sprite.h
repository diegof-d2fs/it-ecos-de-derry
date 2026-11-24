#pragma once
// Sprites e animações a partir dos atlas do manifesto. O atlas apontado já está preparado
// na escala do canvas (Recursos::imagens); frameW/frameH são em PIXELS DE TELA.
#include "render/canvas.h"

struct Sprite {
    const Imagem* atlas = nullptr;   // pertence a Recursos; nunca liberado aqui
    int frameW = 0;                  // pixels de tela
    int frameH = 0;
    int colunas = 1;
    int linhas = 1;
};

struct Animacao {
    int linha = 0;          // linha do atlas (para personagens: a direção)
    int primeiraColuna = 0;
    int quadros = 1;
    float fps = 8.0f;
    bool loop = true;
};

bool spriteValido(const Sprite& sprite);

// Coluna da animação após `tempo` segundos.
int colunaAnimacao(const Animacao& animacao, float tempo);

// Desenha o frame (coluna, linha) com o PIVÔ NOS PÉS em (xPe, yPe) lógico: centro da base.
void desenharFramePes(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe,
                      bool espelhar = false, std::uint8_t opacidade = 255);
// Com tinta (cor 0xRRGGBB, força 0..255) e, à parte, só o contorno da silhueta (1 unidade lógica).
void desenharFramePesTinta(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe,
                           bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca);
// Com tinta e numa escala própria (1 = tamanho da folha), pivô nos pés.
void desenharFramePesEscala(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, float escala,
                            bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca);
void contornoFramePes(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe,
                      std::uint32_t cor);
// Esticado (largura e altura com escalas próprias), pivô nos pés: respiração, passada e o "peso" dos chefes
// (Diego, 24/11: "os chefões estão muito duros em tela").
void desenharFramePesEstica(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, float escalaX,
                            float escalaY, bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca);
// Girado em volta do CENTRO (cx, cy) lógico; `angulo` em radianos (0 = como na folha).
void desenharFrameGirado(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float cx, float cy, float angulo,
                         float escala, std::uint8_t opacidade = 255);
// Desenha o frame com o canto superior esquerdo em (x, y) lógico (tiles, ícones).
void desenharFrame(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float x, float y);
// Atalho para atlas de tiles: índice 0..N lido da esquerda para a direita, de cima para baixo.
void desenharTile(Canvas& canvas, const Sprite& tileset, int indice, float x, float y);
