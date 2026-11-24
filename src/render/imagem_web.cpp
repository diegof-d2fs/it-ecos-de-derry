#ifdef __EMSCRIPTEN__
// PNG (e JPEG) na versão web (Etapa 18): stb_image no lugar do GDI+. Mesmo formato de saída: ARGB de 32 bits
// com alfa PRÉ-MULTIPLICADO (o que o canvas mistura).
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG   // as ilustrações grandes vão em JPEG na web (tools/export_web.py)
#include "stb_image.h"

#include "render/imagem.h"

#include <string>

bool iniciarDecodificadorPng() { return true; }
void encerrarDecodificadorPng() {}

bool carregarPng(const std::wstring& caminho, Imagem& saida) {
    saida = Imagem{};
    std::string utf8;   // os caminhos do jogo são ASCII
    for (wchar_t c : caminho) utf8.push_back(c == L'\\' ? '/' : static_cast<char>(c));
    int w = 0, h = 0, canais = 0;
    unsigned char* rgba = stbi_load(utf8.c_str(), &w, &h, &canais, 4);
    if (!rgba) return false;
    saida.w = w;
    saida.h = h;
    saida.px.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    for (std::size_t i = 0; i < saida.px.size(); ++i) {
        const unsigned r = rgba[i * 4], g = rgba[i * 4 + 1], b = rgba[i * 4 + 2], a = rgba[i * 4 + 3];
        saida.px[i] = (a << 24) | ((r * a / 255) << 16) | ((g * a / 255) << 8) | (b * a / 255);
    }
    stbi_image_free(rgba);
    return true;
}
#endif
