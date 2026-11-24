#include "render/canvas.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// Mistura "src sobre dst" com src pré-multiplicado: dst = src + dst × (1 − αsrc).
inline std::uint32_t misturar(std::uint32_t dst, std::uint32_t src) {
    const std::uint32_t a = src >> 24;
    if (a == 255) return src;
    if (a == 0) return dst;
    const std::uint32_t inv = 255 - a;
    const std::uint32_t rb = (((dst & 0x00FF00FFu) * inv) >> 8) & 0x00FF00FFu;
    const std::uint32_t g = (((dst & 0x0000FF00u) * inv) >> 8) & 0x0000FF00u;
    return (src + (rb | g)) | 0xFF000000u;
}

// Multiplica todos os canais de um pixel pré-multiplicado por k/255.
inline std::uint32_t atenuar(std::uint32_t p, std::uint32_t k) {
    const std::uint32_t rb = (((p & 0x00FF00FFu) * k) >> 8) & 0x00FF00FFu;
    const std::uint32_t ag = ((((p >> 8) & 0x00FF00FFu) * k) >> 8) & 0x00FF00FFu;
    return rb | (ag << 8);
}

struct RetTela {
    int x0, y0, x1, y1;   // [x0, x1) × [y0, y1), já recortado ao canvas
};

RetTela converter(const Canvas& c, float x, float y, float w, float h) {
    RetTela r{paraTela(c, x), paraTela(c, y), paraTela(c, x + w), paraTela(c, y + h)};
    r.x0 = std::max(0, r.x0);
    r.y0 = std::max(0, r.y0);
    r.x1 = std::min(c.w, r.x1);
    r.y1 = std::min(c.h, r.y1);
    return r;
}

}  // namespace

bool criarCanvas(Canvas& canvas, int larguraLogica, int alturaLogica, int escala) {
    const int largura = larguraLogica * escala, altura = alturaLogica * escala;
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = largura;
    info.bmiHeader.biHeight = -altura;   // negativo = linhas de cima para baixo
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    canvas.dc = CreateCompatibleDC(nullptr);
    if (!canvas.dc) return false;
    canvas.dib = CreateDIBSection(canvas.dc, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!canvas.dib || !bits) {
        DeleteDC(canvas.dc);
        canvas.dc = nullptr;
        return false;
    }
    canvas.bitmapAnterior = SelectObject(canvas.dc, canvas.dib);
    canvas.px = static_cast<std::uint32_t*>(bits);
    canvas.w = largura;
    canvas.h = altura;
    canvas.escala = escala;
    SetBkMode(canvas.dc, TRANSPARENT);
    return true;
}

void liberarCanvas(Canvas& canvas) {
    if (canvas.dc) {
        // Devolve o bitmap original ao DC antes de apagar a DIB (regra do GDI).
        SelectObject(canvas.dc, canvas.bitmapAnterior);
        DeleteObject(canvas.dib);
        DeleteDC(canvas.dc);
    }
    canvas = Canvas{};
}

int paraTela(const Canvas& canvas, float logico) {
    return static_cast<int>(std::lround(logico * static_cast<float>(canvas.escala)));
}

void limpar(Canvas& canvas, std::uint32_t cor) { std::fill(canvas.px, canvas.px + canvas.w * canvas.h, cor); }

void retanguloCheio(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor) {
    const RetTela r = converter(canvas, x, y, w, h);
    for (int yy = r.y0; yy < r.y1; ++yy) {
        std::fill(canvas.px + yy * canvas.w + r.x0, canvas.px + yy * canvas.w + std::max(r.x0, r.x1), cor);
    }
}

void retanguloAlpha(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor, std::uint8_t alpha) {
    const std::uint32_t src = atenuar(cor | 0xFF000000u, alpha);
    const RetTela r = converter(canvas, x, y, w, h);
    for (int yy = r.y0; yy < r.y1; ++yy) {
        std::uint32_t* linha = canvas.px + yy * canvas.w;
        for (int xx = r.x0; xx < r.x1; ++xx) {
            linha[xx] = misturar(linha[xx], src);
        }
    }
}

void contornoRetangulo(Canvas& canvas, float x, float y, float w, float h, std::uint32_t cor) {
    retanguloCheio(canvas, x, y, w, 1, cor);
    retanguloCheio(canvas, x, y + h - 1, w, 1, cor);
    retanguloCheio(canvas, x, y, 1, h, cor);
    retanguloCheio(canvas, x + w - 1, y, 1, h, cor);
}

void elipseAlpha(Canvas& canvas, float cx, float cy, float rx, float ry, std::uint32_t cor, std::uint8_t alpha) {
    const int tcx = paraTela(canvas, cx), tcy = paraTela(canvas, cy);
    const int trx = paraTela(canvas, rx), try_ = paraTela(canvas, ry);
    if (trx <= 0 || try_ <= 0) return;
    const std::uint32_t src = atenuar(cor | 0xFF000000u, alpha);
    for (int dy = -try_; dy <= try_; ++dy) {
        const int y = tcy + dy;
        if (y < 0 || y >= canvas.h) continue;
        // meia largura da elipse nesta linha: x²/rx² + y²/ry² <= 1
        const float t = 1.0f - static_cast<float>(dy * dy) / static_cast<float>(try_ * try_);
        const int meia = static_cast<int>(std::sqrt(std::max(0.0f, t)) * static_cast<float>(trx));
        const int x0 = std::max(0, tcx - meia), x1 = std::min(canvas.w - 1, tcx + meia);
        for (int x = x0; x <= x1; ++x) {
            canvas.px[y * canvas.w + x] = misturar(canvas.px[y * canvas.w + x], src);
        }
    }
}

void desenharRecorteTela(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                         bool espelhar, std::uint8_t opacidade) {
    if (imagem.px.empty() || opacidade == 0) return;
    if (sx < 0 || sy < 0 || sx + sw > imagem.w || sy + sh > imagem.h) return;
    // Recorta o destino aos limites do canvas (clipping).
    int inicioX = 0, inicioY = 0, fimX = sw, fimY = sh;
    if (dx < 0) inicioX = -dx;
    if (dy < 0) inicioY = -dy;
    if (dx + sw > canvas.w) fimX = canvas.w - dx;
    if (dy + sh > canvas.h) fimY = canvas.h - dy;
    if (inicioX >= fimX || inicioY >= fimY) return;

    for (int y = inicioY; y < fimY; ++y) {
        const std::uint32_t* origem = &imagem.px[static_cast<size_t>(sy + y) * imagem.w + sx];
        std::uint32_t* destino = canvas.px + (dy + y) * canvas.w + dx;
        for (int x = inicioX; x < fimX; ++x) {
            std::uint32_t p = origem[espelhar ? (sw - 1 - x) : x];
            if ((p >> 24) == 0) continue;            // caminho rápido: transparente
            if (opacidade != 255) p = atenuar(p, opacidade);
            destino[x] = misturar(destino[x], p);
        }
    }
}

void desenharRecorteTelaTinta(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                              bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca) {
    if (forca == 0) {
        desenharRecorteTela(canvas, imagem, sx, sy, sw, sh, dx, dy, espelhar, opacidade);
        return;
    }
    if (imagem.px.empty() || opacidade == 0) return;
    if (sx < 0 || sy < 0 || sx + sw > imagem.w || sy + sh > imagem.h) return;
    int inicioX = 0, inicioY = 0, fimX = sw, fimY = sh;
    if (dx < 0) inicioX = -dx;
    if (dy < 0) inicioY = -dy;
    if (dx + sw > canvas.w) fimX = canvas.w - dx;
    if (dy + sh > canvas.h) fimY = canvas.h - dy;
    for (int y = inicioY; y < fimY; ++y) {
        const std::uint32_t* origem = &imagem.px[static_cast<size_t>(sy + y) * imagem.w + sx];
        std::uint32_t* destino = canvas.px + (dy + y) * canvas.w + dx;
        for (int x = inicioX; x < fimX; ++x) {
            std::uint32_t p = origem[espelhar ? (sw - 1 - x) : x];
            if ((p >> 24) == 0) continue;
            p = tingir(p, cor, forca);
            if (opacidade != 255) p = atenuar(p, opacidade);
            destino[x] = misturar(destino[x], p);
        }
    }
}

void desenharRecorteTelaEscalado(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                                 int dw, int dh, bool espelhar, std::uint8_t opacidade, std::uint32_t cor, std::uint32_t forca) {
    if (imagem.px.empty() || opacidade == 0 || dw <= 0 || dh <= 0) return;
    if (sx < 0 || sy < 0 || sx + sw > imagem.w || sy + sh > imagem.h) return;
    const int x0 = std::max(0, dx), y0 = std::max(0, dy), x1 = std::min(canvas.w, dx + dw), y1 = std::min(canvas.h, dy + dh);
    for (int y = y0; y < y1; ++y) {
        const int oy = (y - dy) * sh / dh;
        const std::uint32_t* origem = &imagem.px[static_cast<size_t>(sy + oy) * imagem.w + sx];
        std::uint32_t* destino = canvas.px + y * canvas.w;
        for (int x = x0; x < x1; ++x) {
            const int ox = (x - dx) * sw / dw;
            std::uint32_t p = origem[espelhar ? (sw - 1 - ox) : ox];
            if ((p >> 24) == 0) continue;
            if (forca > 0) p = tingir(p, cor, forca);
            if (opacidade != 255) p = atenuar(p, opacidade);
            destino[x] = misturar(destino[x], p);
        }
    }
}

void desenharRecorteTelaGirado(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, float cx, float cy,
                               float angulo, float escala, std::uint8_t opacidade) {
    if (imagem.px.empty() || opacidade == 0 || escala <= 0.0f) return;
    if (sx < 0 || sy < 0 || sx + sw > imagem.w || sy + sh > imagem.h) return;
    const float raio = 0.5f * std::sqrt(static_cast<float>(sw * sw + sh * sh)) * escala + 1.0f;
    const int x0 = std::max(0, static_cast<int>(cx - raio)), x1 = std::min(canvas.w, static_cast<int>(cx + raio) + 1);
    const int y0 = std::max(0, static_cast<int>(cy - raio)), y1 = std::min(canvas.h, static_cast<int>(cy + raio) + 1);
    const float c = std::cos(angulo) / escala, s = std::sin(angulo) / escala;
    const float meioW = static_cast<float>(sw) * 0.5f, meioH = static_cast<float>(sh) * 0.5f;
    for (int y = y0; y < y1; ++y) {
        const float dy = static_cast<float>(y) + 0.5f - cy;
        std::uint32_t* destino = canvas.px + y * canvas.w;
        for (int x = x0; x < x1; ++x) {
            const float dx = static_cast<float>(x) + 0.5f - cx;
            // rotação inversa: do pixel de tela de volta ao recorte
            const int ox = static_cast<int>(std::floor(dx * c + dy * s + meioW));
            const int oy = static_cast<int>(std::floor(-dx * s + dy * c + meioH));
            if (ox < 0 || oy < 0 || ox >= sw || oy >= sh) continue;
            std::uint32_t p = imagem.px[static_cast<size_t>(sy + oy) * imagem.w + sx + ox];
            if ((p >> 24) == 0) continue;
            if (opacidade != 255) p = atenuar(p, opacidade);
            destino[x] = misturar(destino[x], p);
        }
    }
}

void contornoRecorteTela(Canvas& canvas, const Imagem& imagem, int sx, int sy, int sw, int sh, int dx, int dy,
                         bool espelhar, std::uint32_t cor, int espessura) {
    if (imagem.px.empty() || espessura <= 0) return;
    if (sx < 0 || sy < 0 || sx + sw > imagem.w || sy + sh > imagem.h) return;
    // "Opaco" = alfa ≥ 128 (as bordas semitransparentes da redução não engordam o contorno).
    auto opaco = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= sw || y >= sh) return false;
        return (imagem.px[static_cast<size_t>(sy + y) * imagem.w + sx + (espelhar ? sw - 1 - x : x)] >> 24) >= 128;
    };
    const std::uint32_t pintura = cor | 0xFF000000u;
    for (int y = -espessura; y < sh + espessura; ++y) {
        const int ty = dy + y;
        if (ty < 0 || ty >= canvas.h) continue;
        for (int x = -espessura; x < sw + espessura; ++x) {
            const int tx = dx + x;
            if (tx < 0 || tx >= canvas.w || opaco(x, y)) continue;
            bool vizinho = false;
            for (int oy = -espessura; oy <= espessura && !vizinho; ++oy) {
                for (int ox = -espessura; ox <= espessura && !vizinho; ++ox) vizinho = opaco(x + ox, y + oy);
            }
            if (vizinho) canvas.px[ty * canvas.w + tx] = pintura;
        }
    }
}

namespace {
// Interpola dois pixels pré-multiplicados: t = 0..256 (0 = a, 256 = b). Canais em pares de 16 bits.
inline std::uint32_t interpolar(std::uint32_t a, std::uint32_t b, std::uint32_t t) {
    const std::uint32_t u = 256 - t;
    const std::uint32_t rb = ((((a & 0x00FF00FFu) * u) + ((b & 0x00FF00FFu) * t)) >> 8) & 0x00FF00FFu;
    const std::uint32_t ag = (((((a >> 8) & 0x00FF00FFu) * u) + (((b >> 8) & 0x00FF00FFu) * t)) >> 8) & 0x00FF00FFu;
    return rb | (ag << 8);
}
}  // namespace

void desenharImagemAmostrada(Canvas& canvas, const Imagem& imagem, float origemX, float origemY, float passo,
                             std::uint8_t opacidade) {
    if (imagem.w < 2 || imagem.h < 2 || passo <= 0.0f || opacidade == 0) return;
    // 1:1 (as panorâmicas das cut scenes): cópia exata com a origem no pixel inteiro. O bilinear numa origem
    // fracionária mistura cada pixel com o vizinho e borrava a ilustração inteira (Diego, 24/11: "nas cutscenes
    // elas ficam muito embaçadas").
    if (std::fabs(passo - 1.0f) < 0.002f) {
        const int ox = static_cast<int>(std::lround(origemX)), oy = static_cast<int>(std::lround(origemY));
        for (int y = 0; y < canvas.h; ++y) {
            const int j = std::clamp(oy + y, 0, imagem.h - 1);
            const std::uint32_t* linha = imagem.px.data() + static_cast<std::size_t>(j) * static_cast<std::size_t>(imagem.w);
            std::uint32_t* destino = canvas.px + static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.w);
            for (int x = 0; x < canvas.w; ++x) {
                const std::uint32_t v = linha[std::clamp(ox + x, 0, imagem.w - 1)];
                if (opacidade == 255 && (v >> 24) == 255) destino[x] = v;
                else destino[x] = misturar(destino[x], atenuar(v, opacidade));
            }
        }
        return;
    }
    // Zoom: pixel mais próximo também (amostra no centro do pixel de tela). As ilustrações são pixel art; o
    // bilinear misturava cada bloco com o vizinho. A coluna de origem é calculada uma vez por quadro.
    static std::vector<int> colunas;
    colunas.resize(static_cast<std::size_t>(canvas.w));
    for (int x = 0; x < canvas.w; ++x) {
        colunas[static_cast<std::size_t>(x)] =
            std::clamp(static_cast<int>(std::floor(origemX + (static_cast<float>(x) + 0.5f) * passo)), 0, imagem.w - 1);
    }
    for (int y = 0; y < canvas.h; ++y) {
        const int j = std::clamp(static_cast<int>(std::floor(origemY + (static_cast<float>(y) + 0.5f) * passo)), 0, imagem.h - 1);
        const std::uint32_t* linha = imagem.px.data() + static_cast<std::size_t>(j) * static_cast<std::size_t>(imagem.w);
        std::uint32_t* destino = canvas.px + static_cast<std::size_t>(y) * static_cast<std::size_t>(canvas.w);
        for (int x = 0; x < canvas.w; ++x) {
            const std::uint32_t v = linha[colunas[static_cast<std::size_t>(x)]];
            if (opacidade == 255 && (v >> 24) == 255) destino[x] = v;
            else destino[x] = misturar(destino[x], atenuar(v, opacidade));
        }
    }
}

void desenharImagem(Canvas& canvas, const Imagem& imagem, float x, float y) {
    desenharRecorteTela(canvas, imagem, 0, 0, imagem.w, imagem.h, paraTela(canvas, x), paraTela(canvas, y));
}

bool salvarBmp(const Canvas& canvas, const wchar_t* caminho) {
    const int linhaBytes = ((canvas.w * 3 + 3) / 4) * 4;   // linhas BMP alinhadas em 4 bytes
    BITMAPFILEHEADER arquivo{};
    BITMAPINFOHEADER info{};
    info.biSize = sizeof(info);
    info.biWidth = canvas.w;
    info.biHeight = canvas.h;   // positivo: BMP guarda de baixo para cima
    info.biPlanes = 1;
    info.biBitCount = 24;
    info.biSizeImage = static_cast<DWORD>(linhaBytes * canvas.h);
    arquivo.bfType = 0x4D42;   // "BM"
    arquivo.bfOffBits = sizeof(arquivo) + sizeof(info);
    arquivo.bfSize = arquivo.bfOffBits + info.biSizeImage;

    HANDLE h = CreateFileW(caminho, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD escritos = 0;
    bool ok = WriteFile(h, &arquivo, sizeof(arquivo), &escritos, nullptr) &&
              WriteFile(h, &info, sizeof(info), &escritos, nullptr);
    std::vector<unsigned char> linha(static_cast<size_t>(linhaBytes), 0);
    for (int y = canvas.h - 1; ok && y >= 0; --y) {
        for (int x = 0; x < canvas.w; ++x) {
            const std::uint32_t p = canvas.px[y * canvas.w + x];
            linha[x * 3 + 0] = static_cast<unsigned char>(p & 0xFF);
            linha[x * 3 + 1] = static_cast<unsigned char>((p >> 8) & 0xFF);
            linha[x * 3 + 2] = static_cast<unsigned char>((p >> 16) & 0xFF);
        }
        ok = WriteFile(h, linha.data(), static_cast<DWORD>(linhaBytes), &escritos, nullptr);
    }
    CloseHandle(h);
    return ok;
}
