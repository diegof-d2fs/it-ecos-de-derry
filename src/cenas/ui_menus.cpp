#include "cenas/ui_menus.h"

#include <algorithm>
#include <cmath>

#include "cenas/ui.h"
#include "render/sprite.h"
#include "render/texto.h"

namespace {

int tela(const Canvas& c, float v) { return paraTela(c, v); }

}  // namespace

void desenharFrameEm(Canvas& canvas, const Sprite& s, int coluna, int linha, float x, float y, float w, float h,
                     std::uint8_t opacidade) {
    if (!spriteValido(s) || w <= 0.0f || h <= 0.0f) return;
    const int dx = tela(canvas, x), dy = tela(canvas, y);
    const int dw = std::max(1, tela(canvas, x + w) - dx), dh = std::max(1, tela(canvas, y + h) - dy);
    desenharRecorteTelaEscalado(canvas, *s.atlas, coluna * s.frameW, linha * s.frameH, s.frameW, s.frameH, dx, dy, dw, dh, false,
                                opacidade, 0, 0);
}

void fundoDeMenu(Canvas& canvas, const Recursos& r, const char* nome, std::uint8_t veu) {
    const Sprite s = spriteDe(r, nome);
    if (spriteValido(s)) desenharFrameEm(canvas, s, 0, 0, 0.0f, 0.0f, LARGURA_LOGICA, ALTURA_LOGICA);
    else veu = std::max<std::uint8_t>(veu, 200);
    if (veu > 0) retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(4, 5, 10), veu);
}

void painelOrnado(Canvas& canvas, const Recursos& r, float x, float y, float w, float h) {
    const Sprite s = spriteDe(r, "ui_painel");
    if (!spriteValido(s)) {
        painel(canvas, x, y, w, h);
        return;
    }
    // 9 fatias: o canto é 1/4 da folha (24 de 96 lógicos), nunca maior que metade do painel.
    const int fw = s.frameW, fh = s.frameH;
    const int cw = fw / 4, ch = fh / 4;
    const int X0 = tela(canvas, x), Y0 = tela(canvas, y), X3 = tela(canvas, x + w), Y3 = tela(canvas, y + h);
    const int kx = std::min(cw, (X3 - X0) / 2), ky = std::min(ch, (Y3 - Y0) / 2);
    const int X1 = X0 + kx, X2 = X3 - kx, Y1 = Y0 + ky, Y2 = Y3 - ky;
    const int sx[4] = {0, cw, fw - cw, fw}, sy[4] = {0, ch, fh - ch, fh};
    const int dx[4] = {X0, X1, X2, X3}, dy[4] = {Y0, Y1, Y2, Y3};
    for (int a = 0; a < 3; ++a) {
        for (int b = 0; b < 3; ++b) {
            const int dw = dx[b + 1] - dx[b], dh = dy[a + 1] - dy[a];
            if (dw <= 0 || dh <= 0) continue;
            desenharRecorteTelaEscalado(canvas, *s.atlas, sx[b], sy[a], sx[b + 1] - sx[b], sy[a + 1] - sy[a], dx[b], dy[a], dw, dh,
                                        false, a == 1 && b == 1 ? 242 : 255, 0, 0);
        }
    }
}

void placaDeLinha(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, bool sel) {
    const Sprite s = spriteDe(r, "ui_placas");
    if (!spriteValido(s)) {
        if (sel) {
            retanguloAlpha(canvas, x, y, w, h, rgb(255, 214, 120), 40);
            contornoRetangulo(canvas, x, y, w, h, rgb(255, 214, 120));
        }
        return;
    }
    const int linha = sel && s.linhas > 1 ? 1 : 0;
    // 3 fatias na horizontal: as pontas (altura da placa) não esticam.
    const int ponta = std::min(s.frameH, s.frameW / 3);
    const int X0 = tela(canvas, x), X3 = tela(canvas, x + w), Y0 = tela(canvas, y), H = std::max(1, tela(canvas, y + h) - Y0);
    const int k = std::min(ponta * H / std::max(1, s.frameH), (X3 - X0) / 2);
    const int sy = linha * s.frameH;
    desenharRecorteTelaEscalado(canvas, *s.atlas, 0, sy, ponta, s.frameH, X0, Y0, k, H, false, 255, 0, 0);
    desenharRecorteTelaEscalado(canvas, *s.atlas, ponta, sy, s.frameW - 2 * ponta, s.frameH, X0 + k, Y0, X3 - X0 - 2 * k, H, false,
                                255, 0, 0);
    desenharRecorteTelaEscalado(canvas, *s.atlas, s.frameW - ponta, sy, ponta, s.frameH, X3 - k, Y0, k, H, false, 255, 0, 0);
}

void abaOrnada(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, bool sel, int icone,
               const std::wstring& texto) {
    const Sprite s = spriteDe(r, "ui_abas");
    if (spriteValido(s)) {
        desenharFrameEm(canvas, s, sel && s.colunas > 1 ? 1 : 0, 0, x, y + (sel ? 0.0f : 3.0f), w, h);
    } else {
        retanguloAlpha(canvas, x, y, w, h, sel ? rgb(255, 214, 120) : rgb(30, 32, 44), sel ? 70 : 200);
    }
    const std::uint32_t cor = sel ? rgb(60, 34, 18) : rgb(196, 186, 166);
    const Sprite ic = spriteDe(r, "ui_icones_abas");
    float tx = x + w / 2.0f;
    if (icone >= 0 && spriteValido(ic)) {
        const float lado = h - 10.0f;
        const float largura = lado + 4.0f + static_cast<float>(texto.size()) * 6.2f;
        const float ix = x + (w - largura) / 2.0f;
        desenharFrameEm(canvas, ic, icone % ic.colunas, 0, ix, y + 5.0f + (sel ? 0.0f : 3.0f), lado, lado, sel ? 255 : 170);
        tx = ix + lado + 4.0f + (largura - lado - 4.0f) / 2.0f;
    }
    desenharTextoCentralizado(canvas, r.fontePequena, tx, y + h / 2.0f - 7.0f + (sel ? 0.0f : 3.0f), texto, cor);
}

void iconeAtributo(Canvas& canvas, const Recursos& r, int i, float x, float y, float lado) {
    const Sprite s = spriteDe(r, "ui_icones_atributos");
    if (spriteValido(s)) desenharFrameEm(canvas, s, i % s.colunas, 0, x, y, lado, lado);
}

void molduraDeItem(Canvas& canvas, const Recursos& r, int nivel, float x, float y, float lado) {
    const Sprite s = spriteDe(r, "ui_moldura_item");
    if (spriteValido(s)) {
        desenharFrameEm(canvas, s, std::clamp(nivel, 0, s.colunas - 1), 0, x, y, lado, lado);
    } else {
        retanguloAlpha(canvas, x, y, lado, lado, rgb(10, 12, 20), 230);
        contornoRetangulo(canvas, x, y, lado, lado, rgb(255, 214, 120));
    }
}

void iconeAmpliado(Canvas& canvas, const Recursos& r, TipoItem tipo, float x, float y, float lado, std::uint8_t opacidade) {
    const DefinicaoItem& d = definicaoDe(tipo);
    const Sprite s = spriteDe(r, d.atlas);
    if (!spriteValido(s)) return;
    desenharFrameEm(canvas, s, d.quadro % s.colunas, d.quadro / s.colunas, x, y, lado, lado, opacidade);
}

void barraOrnada(Canvas& canvas, float x, float y, float w, float h, float fracao, std::uint32_t cor) {
    retanguloAlpha(canvas, x - 1, y - 1, w + 2, h + 2, rgb(0, 0, 0), 200);
    retanguloAlpha(canvas, x, y, w, h, rgb(40, 36, 34), 255);
    const float f = std::clamp(fracao, 0.0f, 1.0f);
    if (f > 0.0f) {
        retanguloAlpha(canvas, x, y, w * f, h, cor, 255);
        retanguloAlpha(canvas, x, y, w * f, std::max(1.0f, h * 0.35f), rgb(255, 255, 255), 60);   // brilho no topo
    }
}

void tituloDeSecao(Canvas& canvas, const Recursos& r, float cx, float y, float largura, const std::wstring& texto) {
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, y, texto, COR_UI_DESTAQUE);
    const float meio = largura / 2.0f;
    for (int k = 0; k < 3; ++k) {   // filete que some nas pontas
        const float frac = 1.0f - 0.3f * static_cast<float>(k);
        retanguloAlpha(canvas, cx - meio * frac, y + 19.0f, meio * frac * 2.0f, 1.0f, rgb(214, 170, 90),
                       static_cast<std::uint8_t>(90 + 60 * k));
    }
}

void brilhoDeSelecao(Canvas& canvas, float cx, float cy, float rx, float ry, float tempo) {
    const float pulso = 0.5f + 0.5f * std::sin(tempo * 3.0f);
    elipseAlpha(canvas, cx, cy, rx * (1.0f + 0.04f * pulso), ry * (1.0f + 0.06f * pulso), rgb(255, 176, 70),
                static_cast<std::uint8_t>(26 + 18 * pulso));
    elipseAlpha(canvas, cx, cy, rx * 0.7f, ry * 0.7f, rgb(255, 210, 130), static_cast<std::uint8_t>(18 + 12 * pulso));
}
