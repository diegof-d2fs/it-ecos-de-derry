#include "render/imagem.h"

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <cstring>

namespace {
ULONG_PTR g_tokenGdiPlus = 0;
}

bool iniciarDecodificadorPng() {
    Gdiplus::GdiplusStartupInput entrada;
    return Gdiplus::GdiplusStartup(&g_tokenGdiPlus, &entrada, nullptr) == Gdiplus::Ok;
}

void encerrarDecodificadorPng() {
    if (g_tokenGdiPlus != 0) {
        Gdiplus::GdiplusShutdown(g_tokenGdiPlus);
        g_tokenGdiPlus = 0;
    }
}

bool carregarPng(const std::wstring& caminho, Imagem& saida) {
    saida = Imagem{};
    Gdiplus::Bitmap bitmap(caminho.c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }
    const int w = static_cast<int>(bitmap.GetWidth());
    const int h = static_cast<int>(bitmap.GetHeight());
    if (w <= 0 || h <= 0) {
        return false;
    }
    Gdiplus::Rect area(0, 0, w, h);
    Gdiplus::BitmapData dados;
    // PARGB = alfa pré-multiplicado, o formato que o canvas mistura mais rápido.
    if (bitmap.LockBits(&area, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &dados) != Gdiplus::Ok) {
        return false;
    }
    saida.w = w;
    saida.h = h;
    saida.px.resize(static_cast<size_t>(w) * h);
    const auto* origem = static_cast<const std::uint8_t*>(dados.Scan0);
    for (int y = 0; y < h; ++y) {
        std::memcpy(&saida.px[static_cast<size_t>(y) * w], origem + static_cast<std::ptrdiff_t>(y) * dados.Stride,
                    static_cast<size_t>(w) * 4);
    }
    bitmap.UnlockBits(&dados);
    return true;
}
