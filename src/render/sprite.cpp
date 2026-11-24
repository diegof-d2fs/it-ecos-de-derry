#include "render/sprite.h"

#include <algorithm>

bool spriteValido(const Sprite& sprite) { return sprite.atlas != nullptr && sprite.frameW > 0 && sprite.frameH > 0; }

int colunaAnimacao(const Animacao& animacao, float tempo) {
    if (animacao.quadros <= 1) return animacao.primeiraColuna;
    int indice = static_cast<int>(tempo * animacao.fps);
    if (animacao.loop) {
        indice %= animacao.quadros;
    } else if (indice >= animacao.quadros) {
        indice = animacao.quadros - 1;
    }
    return animacao.primeiraColuna + indice;
}

void desenharFramePes(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, bool espelhar,
                      std::uint8_t opacidade) {
    if (!spriteValido(sprite)) return;
    desenharRecorteTela(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW,
                        sprite.frameH, paraTela(canvas, xPe) - sprite.frameW / 2, paraTela(canvas, yPe) - sprite.frameH,
                        espelhar, opacidade);
}

void desenharFramePesTinta(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe,
                           bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca) {
    if (!spriteValido(sprite)) return;
    desenharRecorteTelaTinta(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW,
                             sprite.frameH, paraTela(canvas, xPe) - sprite.frameW / 2, paraTela(canvas, yPe) - sprite.frameH,
                             espelhar, opacidade, cor, forca);
}

void desenharFramePesEscala(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, float escala,
                            bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca) {
    if (!spriteValido(sprite)) return;
    if (escala > 0.99f && escala < 1.01f) {
        desenharFramePesTinta(canvas, sprite, coluna, linha, xPe, yPe, espelhar, opacidade, cor, forca);
        return;
    }
    const int dw = static_cast<int>(static_cast<float>(sprite.frameW) * escala + 0.5f);
    const int dh = static_cast<int>(static_cast<float>(sprite.frameH) * escala + 0.5f);
    desenharRecorteTelaEscalado(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW, sprite.frameH,
                                paraTela(canvas, xPe) - dw / 2, paraTela(canvas, yPe) - dh, dw, dh, espelhar, opacidade, cor, forca);
}

void desenharFramePesEstica(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, float escalaX,
                            float escalaY, bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca) {
    if (!spriteValido(sprite)) return;
    const int dw = std::max(1, static_cast<int>(static_cast<float>(sprite.frameW) * escalaX + 0.5f));
    const int dh = std::max(1, static_cast<int>(static_cast<float>(sprite.frameH) * escalaY + 0.5f));
    if (dw == sprite.frameW && dh == sprite.frameH) {
        desenharFramePesTinta(canvas, sprite, coluna, linha, xPe, yPe, espelhar, opacidade, cor, forca);
        return;
    }
    desenharRecorteTelaEscalado(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW, sprite.frameH,
                                paraTela(canvas, xPe) - dw / 2, paraTela(canvas, yPe) - dh, dw, dh, espelhar, opacidade, cor, forca);
}

void desenharFrameGirado(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float cx, float cy, float angulo,
                         float escala, std::uint8_t opacidade) {
    if (!spriteValido(sprite)) return;
    desenharRecorteTelaGirado(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW, sprite.frameH,
                              cx * static_cast<float>(canvas.escala), cy * static_cast<float>(canvas.escala), angulo, escala,
                              opacidade);
}

void contornoFramePes(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float xPe, float yPe, std::uint32_t cor) {
    if (!spriteValido(sprite)) return;
    contornoRecorteTela(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW,
                        sprite.frameH, paraTela(canvas, xPe) - sprite.frameW / 2, paraTela(canvas, yPe) - sprite.frameH,
                        false, cor, std::max(1, canvas.escala / 2));
}

void desenharFrame(Canvas& canvas, const Sprite& sprite, int coluna, int linha, float x, float y) {
    if (!spriteValido(sprite)) return;
    desenharRecorteTela(canvas, *sprite.atlas, coluna * sprite.frameW, linha * sprite.frameH, sprite.frameW,
                        sprite.frameH, paraTela(canvas, x), paraTela(canvas, y));
}

void desenharTile(Canvas& canvas, const Sprite& tileset, int indice, float x, float y) {
    if (!spriteValido(tileset) || tileset.colunas <= 0) return;
    desenharFrame(canvas, tileset, indice % tileset.colunas, indice / tileset.colunas, x, y);
}
