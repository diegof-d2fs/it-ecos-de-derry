#include "render/luz.h"

#include <cmath>
#include <vector>

namespace {

// Fator 0..256 de uma célula (256 = cor plena). Fora da sala = 0 (preto).
int fatorCelula(const MapaVisao& v, int col, int lin) {
    const CelulaVisao& c = visaoEm(v, col, lin);
    if (!c.explorada) return 0;
    const float memoria = LUZ_MEMORIA * 256.0f;
    const float luz = static_cast<float>(c.luz) * (256.0f / 255.0f);
    return static_cast<int>(luz > memoria ? luz : memoria);
}

inline std::uint32_t atenuar(std::uint32_t p, int f) {   // f = 0..256
    const std::uint32_t k = static_cast<std::uint32_t>(f);
    const std::uint32_t rb = (((p & 0x00FF00FFu) * k) >> 8) & 0x00FF00FFu;
    const std::uint32_t g = (((p & 0x0000FF00u) * k) >> 8) & 0x0000FF00u;
    return 0xFF000000u | rb | g;
}

}  // namespace

void aplicarEscuridao(Canvas& canvas, const MapaVisao& visao, float camX, float camY) {
    const int k = canvas.escala;
    const float inv = 1.0f / static_cast<float>(k * TILE);
    // Colunas de célula que aparecem na tela (+1 de cada lado para a interpolação).
    const int c0 = static_cast<int>(std::floor(camX / TILE)) - 1;
    const int nCols = LARGURA_LOGICA / TILE + 4;
    std::vector<int> linhaSup(static_cast<std::size_t>(nCols)), linhaInf(static_cast<std::size_t>(nCols));
    std::vector<int> v(static_cast<std::size_t>(nCols));
    int linhaCache = -100000;
    // Origem do canvas em pixels do mundo × escala (mesmo arredondamento do terreno).
    const int origemX = paraTela(canvas, camX), origemY = paraTela(canvas, camY);

    for (int Y = 0; Y < canvas.h; ++Y) {
        // Posição em "células" do centro do pixel, deslocada meio tile: os valores ficam nos centros.
        const float gy = (static_cast<float>(Y + origemY) + 0.5f) * inv - 0.5f;
        const int l0 = static_cast<int>(std::floor(gy));
        const int ty = static_cast<int>((gy - static_cast<float>(l0)) * 256.0f);
        if (l0 != linhaCache) {
            for (int i = 0; i < nCols; ++i) {
                linhaSup[static_cast<std::size_t>(i)] = fatorCelula(visao, c0 + i, l0);
                linhaInf[static_cast<std::size_t>(i)] = fatorCelula(visao, c0 + i, l0 + 1);
            }
            linhaCache = l0;
        }
        bool tudoPleno = true, tudoPreto = true;
        for (int i = 0; i < nCols; ++i) {
            const int a = linhaSup[static_cast<std::size_t>(i)], b = linhaInf[static_cast<std::size_t>(i)];
            v[static_cast<std::size_t>(i)] = a + (((b - a) * ty) >> 8);
            tudoPleno &= v[static_cast<std::size_t>(i)] >= 256;
            tudoPreto &= v[static_cast<std::size_t>(i)] <= 0;
        }
        std::uint32_t* px = canvas.px + static_cast<std::size_t>(Y) * canvas.w;
        if (tudoPleno) continue;
        if (tudoPreto) {
            for (int X = 0; X < canvas.w; ++X) px[X] = 0xFF000000u;
            continue;
        }
        // Por TRECHOS entre centros de célula: dentro de um trecho o fator varia em linha reta,
        // então basta somar um passo fixo por pixel (ponto fixo 16.16). Trecho constante (preto,
        // penumbra ou luz plena: a maior parte da tela) é tratado de uma vez.
        const int lado = k * TILE;
        for (int i = 0; i + 1 < nCols; ++i) {
            // O trecho i vai do centro da célula (c0+i) ao centro da (c0+i+1).
            const int inicioMundo = (c0 + i) * lado + lado / 2;   // pixel de tela × escala, no mundo
            int x0 = inicioMundo - origemX, x1 = x0 + lado;
            const int a = v[static_cast<std::size_t>(i)], b = v[static_cast<std::size_t>(i + 1)];
            if (x1 <= 0 || x0 >= canvas.w) continue;
            const int primeiro = x0 < 0 ? 0 : x0, ultimo = x1 > canvas.w ? canvas.w : x1;
            if (a == b) {
                if (a >= 256) continue;
                if (a <= 0) {
                    for (int X = primeiro; X < ultimo; ++X) px[X] = 0xFF000000u;
                } else {
                    for (int X = primeiro; X < ultimo; ++X) px[X] = atenuar(px[X], a);
                }
                continue;
            }
            const int passo = ((b - a) * 65536) / lado;
            int f16 = a * 65536 + passo * (primeiro - x0) + passo / 2;
            for (int X = primeiro; X < ultimo; ++X, f16 += passo) {
                const int f = f16 >> 16;
                if (f >= 256) continue;
                px[X] = f <= 0 ? 0xFF000000u : atenuar(px[X], f);
            }
        }
    }
}
