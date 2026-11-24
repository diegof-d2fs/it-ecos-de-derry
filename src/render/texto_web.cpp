#ifdef __EMSCRIPTEN__
// Texto na versão web (Etapa 18). O GDI não existe no navegador: as letras são rasterizadas com o
// stb_truetype a partir de fontes LIVRES (SIL OFL) que imitam as do Windows:
//   Georgia  → Gelasio (mesmas métricas)        Segoe UI → Noto Sans
// Mesmo contrato do render/texto.cpp: altura da fonte = alturaLogica × escala pixels (como o
// CreateFontW com altura negativa), sombra 1 unidade lógica abaixo e à direita, y = topo da linha.
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "render/texto.h"

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/arquivo.h"

namespace {

struct Glifo {
    int w = 0, h = 0, xoff = 0, yoff = 0;
    float avanco = 0.0f;
    std::vector<unsigned char> alfa;
};

struct ArquivoFonte {
    std::string bytes;
    stbtt_fontinfo info{};
    bool ok = false;
};

// Uma face já num tamanho em pixels (com o cache dos glifos usados).
struct Face {
    ArquivoFonte* arquivo = nullptr;
    float pixels = 0.0f;   // altura em pixels (as fontes de reserva usam a mesma)
    float escala = 0.0f;   // stbtt: unidades da fonte → pixels
    int ascendente = 0;    // pixels acima da linha de base
    int alturaLinha = 0;
    bool negrito = false;
    std::map<int, Glifo> glifos;
};

ArquivoFonte* arquivo(const char* caminho) {
    static std::map<std::string, std::unique_ptr<ArquivoFonte>> abertos;
    auto& a = abertos[caminho];
    if (!a) {
        a = std::make_unique<ArquivoFonte>();
        if (lerArquivoTexto(std::filesystem::path(caminho), a->bytes)) {
            const auto* dados = reinterpret_cast<const unsigned char*>(a->bytes.data());
            a->ok = stbtt_InitFont(&a->info, dados, stbtt_GetFontOffsetForIndex(dados, 0)) != 0;
        }
    }
    return a->ok ? a.get() : nullptr;
}

// Setas, ✓, ◀ ▶ e outros símbolos que a fonte principal não tem: Noto Sans Symbols 2 e Noto Sans Math (OFL).
ArquivoFonte* comOGlifo(ArquivoFonte* principal, int codigo, float& escala, float pixels) {
    if (stbtt_FindGlyphIndex(&principal->info, codigo) != 0) return principal;
    for (const char* reserva : {"/fontes/NotoSansSymbols2.ttf", "/fontes/NotoSansMath.ttf", "/fontes/NotoSans.ttf"}) {
        ArquivoFonte* a = arquivo(reserva);
        if (a && stbtt_FindGlyphIndex(&a->info, codigo) != 0) {
            escala = stbtt_ScaleForMappingEmToPixels(&a->info, pixels);
            return a;
        }
    }
    return principal;
}

const Glifo& glifo(Face& f, int codigo) {
    auto it = f.glifos.find(codigo);
    if (it != f.glifos.end()) return it->second;
    Glifo g;
    float escala = f.escala;
    ArquivoFonte* fonte = comOGlifo(f.arquivo, codigo, escala, f.pixels);
    int avanco = 0, lsb = 0;
    stbtt_GetCodepointHMetrics(&fonte->info, codigo, &avanco, &lsb);
    g.avanco = static_cast<float>(avanco) * escala + (f.negrito ? 1.0f : 0.0f);
    unsigned char* bmp = stbtt_GetCodepointBitmap(&fonte->info, escala, escala, codigo, &g.w, &g.h, &g.xoff, &g.yoff);
    if (bmp) {
        g.alfa.assign(bmp, bmp + static_cast<std::size_t>(g.w) * static_cast<std::size_t>(g.h));
        stbtt_FreeBitmap(bmp, nullptr);
    }
    return f.glifos.emplace(codigo, std::move(g)).first->second;
}

// Mistura uma cor com a cobertura `a` (0..255) num pixel ARGB opaco do canvas.
inline void pintar(std::uint32_t& destino, std::uint32_t cor, unsigned a) {
    if (a == 0) return;
    const unsigned inv = 255 - a;
    const unsigned r = (((cor >> 16) & 0xFF) * a + ((destino >> 16) & 0xFF) * inv) / 255;
    const unsigned g = (((cor >> 8) & 0xFF) * a + ((destino >> 8) & 0xFF) * inv) / 255;
    const unsigned b = ((cor & 0xFF) * a + (destino & 0xFF) * inv) / 255;
    destino = 0xFF000000u | (r << 16) | (g << 8) | b;
}

void desenharLinha(Canvas& canvas, Face& f, int x, int y, const std::wstring& texto, std::uint32_t cor) {
    float cx = static_cast<float>(x);
    const int base = y + f.ascendente;
    for (wchar_t c : texto) {
        const Glifo& g = glifo(f, static_cast<int>(c));
        const int gx0 = static_cast<int>(std::lround(cx)) + g.xoff, gy0 = base + g.yoff;
        for (int passada = 0; passada < (f.negrito ? 2 : 1); ++passada) {   // negrito sintético: +1 px
            const int ox = gx0 + passada;
            for (int j = 0; j < g.h; ++j) {
                const int py = gy0 + j;
                if (py < 0 || py >= canvas.h) continue;
                std::uint32_t* linha = canvas.px + static_cast<std::size_t>(py) * static_cast<std::size_t>(canvas.w);
                for (int i = 0; i < g.w; ++i) {
                    const int pxx = ox + i;
                    if (pxx < 0 || pxx >= canvas.w) continue;
                    pintar(linha[pxx], cor, g.alfa[static_cast<std::size_t>(j) * static_cast<std::size_t>(g.w) + static_cast<std::size_t>(i)]);
                }
            }
        }
        cx += g.avanco;
    }
}

}  // namespace

bool criarFonte(Fonte& fonte, const wchar_t* nome, int alturaLogica, int escala, bool negrito) {
    const std::wstring n = nome ? nome : L"";
    ArquivoFonte* a = arquivo(n == L"Georgia" ? "/fontes/Gelasio.ttf" : "/fontes/NotoSans.ttf");
    if (!a) return false;
    auto* f = new Face;
    f->arquivo = a;
    const float pixels = static_cast<float>(alturaLogica * escala);
    f->escala = stbtt_ScaleForMappingEmToPixels(&a->info, pixels);
    f->pixels = pixels;
    int asc = 0, desc = 0, gap = 0;
    stbtt_GetFontVMetrics(&a->info, &asc, &desc, &gap);
    f->ascendente = static_cast<int>(std::lround(static_cast<float>(asc) * f->escala));
    f->alturaLinha = static_cast<int>(std::lround(static_cast<float>(asc - desc) * f->escala));
    f->negrito = negrito;
    fonte.handle = f;
    fonte.alturaLogica = alturaLogica;
    return true;
}

void liberarFonte(Fonte& fonte) {
    delete static_cast<Face*>(fonte.handle);
    fonte = Fonte{};
}

std::wstring paraWide(const std::string& utf8) {
    std::wstring saida;
    for (std::size_t i = 0; i < utf8.size();) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        unsigned cp = 0;
        int n = 0;
        if (c < 0x80) { cp = c; n = 1; }
        else if ((c >> 5) == 0x6) { cp = c & 0x1F; n = 2; }
        else if ((c >> 4) == 0xE) { cp = c & 0x0F; n = 3; }
        else { cp = c & 0x07; n = 4; }
        for (int k = 1; k < n && i + static_cast<std::size_t>(k) < utf8.size(); ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(utf8[i + static_cast<std::size_t>(k)]) & 0x3F);
        saida.push_back(static_cast<wchar_t>(cp));
        i += static_cast<std::size_t>(n);
    }
    return saida;
}

float larguraTexto(Canvas& canvas, const Fonte& fonte, const std::wstring& texto) {
    auto* f = static_cast<Face*>(fonte.handle);
    if (!f) return 0.0f;
    float w = 0.0f;
    for (wchar_t c : texto) w += glifo(*f, static_cast<int>(c)).avanco;
    return w / static_cast<float>(canvas.escala);
}

void desenharTexto(Canvas& canvas, const Fonte& fonte, float x, float y, const std::wstring& texto, std::uint32_t cor,
                   bool sombra) {
    auto* f = static_cast<Face*>(fonte.handle);
    if (!f || !canvas.px) return;
    const int tx = paraTela(canvas, x), ty = paraTela(canvas, y);
    if (sombra) desenharLinha(canvas, *f, tx + canvas.escala, ty + canvas.escala, texto, 0xFF08080Cu);
    desenharLinha(canvas, *f, tx, ty, texto, cor);
}

void desenharTextoCentralizado(Canvas& canvas, const Fonte& fonte, float centroX, float y, const std::wstring& texto,
                               std::uint32_t cor) {
    desenharTexto(canvas, fonte, centroX - larguraTexto(canvas, fonte, texto) / 2.0f, y, texto, cor);
}
#endif
