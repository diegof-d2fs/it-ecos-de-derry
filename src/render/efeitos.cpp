#include "render/efeitos.h"

#include <algorithm>

namespace {

inline std::uint32_t misturar(std::uint32_t dst, std::uint32_t cor, std::uint32_t a) {
    const std::uint32_t inv = 255 - a;
    const std::uint32_t r = (((cor >> 16) & 0xFF) * a + ((dst >> 16) & 0xFF) * inv) / 255;
    const std::uint32_t g = (((cor >> 8) & 0xFF) * a + ((dst >> 8) & 0xFF) * inv) / 255;
    const std::uint32_t b = ((cor & 0xFF) * a + (dst & 0xFF) * inv) / 255;
    return 0xFF000000u | (r << 16) | (g << 8) | b;
}

}  // namespace

void desenharArea(Canvas& canvas, const Area& area, float camX, float camY, std::uint32_t cor, std::uint8_t alfaPreenchimento,
                  std::uint8_t alfaContorno, bool hachura) {
    const Retangulo r = envolvente(area);
    const int k = canvas.escala;
    const int x0 = std::max(0, paraTela(canvas, r.x - camX) - 2), x1 = std::min(canvas.w, paraTela(canvas, r.x + r.w - camX) + 2);
    const int y0 = std::max(0, paraTela(canvas, r.y - camY) - 2), y1 = std::min(canvas.h, paraTela(canvas, r.y + r.h - camY) + 2);
    const float inv = 1.0f / static_cast<float>(k);
    const float borda = 1.5f;   // espessura do contorno em unidades lógicas
    for (int Y = y0; Y < y1; ++Y) {
        std::uint32_t* linha = canvas.px + static_cast<std::size_t>(Y) * canvas.w;
        const float y = (static_cast<float>(Y) + 0.5f) * inv + camY;
        for (int X = x0; X < x1; ++X) {
            const float x = (static_cast<float>(X) + 0.5f) * inv + camX;
            if (!pontoNaArea(area, {x, y})) continue;
            const bool contorno = !pontoNaArea(area, {x + borda, y}) || !pontoNaArea(area, {x - borda, y}) ||
                                  !pontoNaArea(area, {x, y + borda}) || !pontoNaArea(area, {x, y - borda});
            std::uint32_t a = contorno ? alfaContorno : alfaPreenchimento;
            if (!contorno && hachura && ((X + Y) / (3 * k)) % 2 == 0) a = a * 2 > 255 ? 255 : a * 2;
            if (a) linha[X] = misturar(linha[X], cor, a);
        }
    }
}
