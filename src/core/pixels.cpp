#include "core/pixels.h"

#include <algorithm>
#include <cmath>

namespace {

// Para cada pixel de destino ao longo de um eixo: quais pixels de origem ele cobre e com
// que peso (fração de área). Calculado uma vez por eixo, reaproveitado em todas as linhas.
struct Cobertura {
    int primeiro = 0;
    std::vector<float> pesos;
};

std::vector<Cobertura> calcularCoberturas(int tamOrigem, int tamDestino) {
    std::vector<Cobertura> saida(static_cast<size_t>(tamDestino));
    const double razao = static_cast<double>(tamOrigem) / tamDestino;
    for (int d = 0; d < tamDestino; ++d) {
        const double ini = d * razao, fim = (d + 1) * razao;
        Cobertura& c = saida[static_cast<size_t>(d)];
        c.primeiro = static_cast<int>(std::floor(ini));
        const int ultimo = std::min(tamOrigem - 1, static_cast<int>(std::ceil(fim)) - 1);
        for (int s = c.primeiro; s <= ultimo; ++s) {
            const double sobreposicao = std::min(fim, s + 1.0) - std::max(ini, static_cast<double>(s));
            c.pesos.push_back(static_cast<float>(sobreposicao / razao));
        }
    }
    return saida;
}

Imagem ampliarVizinho(const Imagem& origem, int novoW, int novoH) {
    Imagem d;
    d.w = novoW;
    d.h = novoH;
    d.px.resize(static_cast<size_t>(novoW) * novoH);
    for (int y = 0; y < novoH; ++y) {
        const int sy = y * origem.h / novoH;
        for (int x = 0; x < novoW; ++x) {
            d.px[static_cast<size_t>(y) * novoW + x] = origem.px[static_cast<size_t>(sy) * origem.w + x * origem.w / novoW];
        }
    }
    return d;
}

}  // namespace

std::uint32_t tingir(std::uint32_t pixel, std::uint32_t cor, std::uint32_t forca) {
    const std::uint32_t a = pixel >> 24;
    if (a == 0 || forca == 0) return pixel;
    if (forca > 255) forca = 255;
    std::uint32_t saida = pixel & 0xFF000000u;
    for (int deslocamento = 0; deslocamento <= 16; deslocamento += 8) {
        const std::uint32_t atual = (pixel >> deslocamento) & 0xFFu;
        const std::uint32_t alvo = (((cor >> deslocamento) & 0xFFu) * a) / 255;   // a cor também pré-multiplicada
        const std::uint32_t mix = (atual * (255 - forca) + alvo * forca) / 255;
        saida |= (mix & 0xFFu) << deslocamento;
    }
    return saida;
}

Imagem reduzirVizinho(const Imagem& origem, int novoW, int novoH) {
    Imagem d;
    if (novoW <= 0 || novoH <= 0 || origem.px.empty()) return d;
    d.w = novoW;
    d.h = novoH;
    d.px.resize(static_cast<size_t>(novoW) * static_cast<size_t>(novoH));
    for (int y = 0; y < novoH; ++y) {
        const int sy = std::min(origem.h - 1, static_cast<int>((2LL * y + 1) * origem.h / (2LL * novoH)));
        for (int x = 0; x < novoW; ++x) {
            const int sx = std::min(origem.w - 1, static_cast<int>((2LL * x + 1) * origem.w / (2LL * novoW)));
            d.px[static_cast<size_t>(y) * novoW + x] = origem.px[static_cast<size_t>(sy) * origem.w + sx];
        }
    }
    return d;
}

Imagem reamostrar(const Imagem& origem, int novoW, int novoH) {
    if (novoW <= 0 || novoH <= 0 || origem.px.empty()) {
        return Imagem{};
    }
    if (novoW == origem.w && novoH == origem.h) {
        return origem;
    }
    if (novoW >= origem.w && novoH >= origem.h) {
        return ampliarVizinho(origem, novoW, novoH);
    }
    const std::vector<Cobertura> cx = calcularCoberturas(origem.w, novoW);
    const std::vector<Cobertura> cy = calcularCoberturas(origem.h, novoH);

    // Passo 1: reduzir na horizontal (acumulando em float para não perder precisão).
    std::vector<float> intermediaria(static_cast<size_t>(novoW) * origem.h * 4, 0.0f);
    for (int y = 0; y < origem.h; ++y) {
        const std::uint32_t* linha = &origem.px[static_cast<size_t>(y) * origem.w];
        for (int x = 0; x < novoW; ++x) {
            const Cobertura& c = cx[static_cast<size_t>(x)];
            float acc[4] = {0, 0, 0, 0};
            for (size_t k = 0; k < c.pesos.size(); ++k) {
                const std::uint32_t p = linha[c.primeiro + static_cast<int>(k)];
                const float w = c.pesos[k];
                acc[0] += w * static_cast<float>(p >> 24);
                acc[1] += w * static_cast<float>((p >> 16) & 0xFF);
                acc[2] += w * static_cast<float>((p >> 8) & 0xFF);
                acc[3] += w * static_cast<float>(p & 0xFF);
            }
            float* destino = &intermediaria[(static_cast<size_t>(y) * novoW + x) * 4];
            for (int i = 0; i < 4; ++i) destino[i] = acc[i];
        }
    }
    // Passo 2: reduzir na vertical.
    Imagem d;
    d.w = novoW;
    d.h = novoH;
    d.px.resize(static_cast<size_t>(novoW) * novoH);
    for (int y = 0; y < novoH; ++y) {
        const Cobertura& c = cy[static_cast<size_t>(y)];
        for (int x = 0; x < novoW; ++x) {
            float acc[4] = {0, 0, 0, 0};
            for (size_t k = 0; k < c.pesos.size(); ++k) {
                const float* fonte = &intermediaria[(static_cast<size_t>(c.primeiro + static_cast<int>(k)) * novoW + x) * 4];
                for (int i = 0; i < 4; ++i) acc[i] += c.pesos[k] * fonte[i];
            }
            std::uint32_t canais[4];
            for (int i = 0; i < 4; ++i) {
                canais[i] = static_cast<std::uint32_t>(std::clamp(std::lround(acc[i]), 0L, 255L));
            }
            // Alfa pré-multiplicado: nenhum canal de cor pode passar do alfa.
            for (int i = 1; i < 4; ++i) canais[i] = std::min(canais[i], canais[0]);
            d.px[static_cast<size_t>(y) * novoW + x] = (canais[0] << 24) | (canais[1] << 16) | (canais[2] << 8) | canais[3];
        }
    }
    return d;
}

void realcarNitidez(Imagem& imagem, int forcaPct) {
    if (imagem.w < 3 || imagem.h < 3 || forcaPct <= 0) return;
    const std::vector<std::uint32_t> orig = imagem.px;
    const int w = imagem.w;
    for (int y = 1; y < imagem.h - 1; ++y) {
        for (int x = 1; x < w - 1; ++x) {
            const std::uint32_t c = orig[static_cast<std::size_t>(y * w + x)];
            std::uint32_t saida = c & 0xFF000000u;
            for (int s = 0; s < 24; s += 8) {
                int soma = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) soma += static_cast<int>((orig[static_cast<std::size_t>((y + dy) * w + x + dx)] >> s) & 0xFFu);
                const int v = static_cast<int>((c >> s) & 0xFFu);
                const int a = static_cast<int>(c >> 24);   // pré-multiplicado: o canal nunca passa do alfa
                const int r = v + (v * 9 - soma) * forcaPct / 900;
                saida |= static_cast<std::uint32_t>(r < 0 ? 0 : (r > a ? a : r)) << s;
            }
            imagem.px[static_cast<std::size_t>(y * w + x)] = saida;
        }
    }
}
