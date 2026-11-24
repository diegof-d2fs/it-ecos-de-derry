#include "render/texto.h"

namespace {

COLORREF paraColorref(std::uint32_t argb) {
    return RGB((argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF);
}

// O GDI escreve RGB e zera o byte de alfa dos pixels do texto. O canvas é apresentado
// sem considerar alfa, mas desenhos posteriores misturam com base nele: devolvemos alfa 255.
void restaurarAlfa(Canvas& canvas, int x, int y, int w, int h) {
    const int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    const int x1 = x + w > canvas.w ? canvas.w : x + w, y1 = y + h > canvas.h ? canvas.h : y + h;
    for (int yy = y0; yy < y1; ++yy) {
        for (int xx = x0; xx < x1; ++xx) {
            canvas.px[yy * canvas.w + xx] |= 0xFF000000u;
        }
    }
}

}  // namespace

bool criarFonte(Fonte& fonte, const wchar_t* nome, int alturaLogica, int escala, bool negrito) {
    // Em escala 1 a fonte é pequena: sem antialias fica mais legível. Em escala ≥ 2 há
    // pixels suficientes para o ClearType/antialias deixar as letras suaves e nítidas.
    const DWORD qualidade = escala >= 2 ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY;
    fonte.handle = CreateFontW(-alturaLogica * escala, 0, 0, 0, negrito ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE,
                               DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, qualidade,
                               DEFAULT_PITCH | FF_DONTCARE, nome);
    fonte.alturaLogica = alturaLogica;
    return fonte.handle != nullptr;
}

void liberarFonte(Fonte& fonte) {
    if (fonte.handle) DeleteObject(fonte.handle);
    fonte = Fonte{};
}

std::wstring paraWide(const std::string& utf8) {
    if (utf8.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring saida(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), saida.data(), n);
    return saida;
}

float larguraTexto(Canvas& canvas, const Fonte& fonte, const std::wstring& texto) {
    HGDIOBJ antiga = SelectObject(canvas.dc, fonte.handle);
    SIZE tamanho{};
    GetTextExtentPoint32W(canvas.dc, texto.c_str(), static_cast<int>(texto.size()), &tamanho);
    SelectObject(canvas.dc, antiga);
    return static_cast<float>(tamanho.cx) / static_cast<float>(canvas.escala);
}

void desenharTexto(Canvas& canvas, const Fonte& fonte, float x, float y, const std::wstring& texto, std::uint32_t cor,
                   bool sombra) {
    GdiFlush();   // garante que pixels escritos direto em canvas.px já estão visíveis ao GDI
    HGDIOBJ antiga = SelectObject(canvas.dc, fonte.handle);
    const int n = static_cast<int>(texto.size());
    const int tx = paraTela(canvas, x), ty = paraTela(canvas, y);
    const int deslocamentoSombra = canvas.escala;   // 1 unidade lógica
    if (sombra) {
        SetTextColor(canvas.dc, RGB(8, 8, 12));
        TextOutW(canvas.dc, tx + deslocamentoSombra, ty + deslocamentoSombra, texto.c_str(), n);
    }
    SetTextColor(canvas.dc, paraColorref(cor));
    TextOutW(canvas.dc, tx, ty, texto.c_str(), n);
    SIZE tamanho{};
    GetTextExtentPoint32W(canvas.dc, texto.c_str(), n, &tamanho);
    SelectObject(canvas.dc, antiga);
    GdiFlush();
    restaurarAlfa(canvas, tx, ty, tamanho.cx + deslocamentoSombra + 1, tamanho.cy + deslocamentoSombra + 1);
}

void desenharTextoCentralizado(Canvas& canvas, const Fonte& fonte, float centroX, float y, const std::wstring& texto,
                               std::uint32_t cor) {
    desenharTexto(canvas, fonte, centroX - larguraTexto(canvas, fonte, texto) / 2.0f, y, texto, cor);
}
