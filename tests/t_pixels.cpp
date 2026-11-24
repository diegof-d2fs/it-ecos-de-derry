// Testes do reamostrador de imagens (assets HD reduzidos para a escala da tela).
#include "core/manifesto.h"
#include "core/pixels.h"
#include "teste.h"

namespace {

Imagem imagemCheia(int w, int h, std::uint32_t cor) {
    Imagem i;
    i.w = w;
    i.h = h;
    i.px.assign(static_cast<size_t>(w) * h, cor);
    return i;
}

}  // namespace

TESTE(reamostrar_mesmo_tamanho_copia) {
    const Imagem a = imagemCheia(4, 3, 0xFF112233u);
    const Imagem b = reamostrar(a, 4, 3);
    VERIFICAR(b.w == 4 && b.h == 3 && b.px == a.px);
}

TESTE(reamostrar_reducao_faz_media_por_area) {
    // 4×4 metade esquerda branca, metade direita preta → 2×2: coluna esquerda branca, direita preta.
    Imagem a = imagemCheia(4, 4, 0xFF000000u);
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 2; ++x) a.px[static_cast<size_t>(y) * 4 + x] = 0xFFFFFFFFu;
    const Imagem b = reamostrar(a, 2, 2);
    VERIFICAR(b.px[0] == 0xFFFFFFFFu && b.px[1] == 0xFF000000u);
    // → 1×1: média cinza 50% (127 ou 128 por arredondamento).
    const Imagem c = reamostrar(a, 1, 1);
    const int cinza = static_cast<int>((c.px[0] >> 16) & 0xFF);
    VERIFICAR(cinza >= 127 && cinza <= 128);
    VERIFICAR((c.px[0] >> 24) == 255);
}

TESTE(reamostrar_razao_nao_inteira_4_para_3) {
    // Caso real: assets em 4× reduzidos para a escala 3 (monitor 1080p).
    const Imagem a = imagemCheia(128, 192, 0xFF406080u);
    const Imagem b = reamostrar(a, 96, 144);
    VERIFICAR(b.w == 96 && b.h == 144);
    bool uniforme = true;
    for (std::uint32_t p : b.px) uniforme = uniforme && p == 0xFF406080u;
    VERIFICAR(uniforme);   // cor sólida continua idêntica: pesos somam 1
}

TESTE(reamostrar_alfa_premultiplicado_sem_halo) {
    // Metade opaca vermelha, metade transparente → borda 50% alfa com cor ≤ alfa (sem halo).
    Imagem a = imagemCheia(2, 1, 0u);
    a.px[0] = 0xFFFF0000u;
    const Imagem b = reamostrar(a, 1, 1);
    const std::uint32_t alfa = b.px[0] >> 24, vermelho = (b.px[0] >> 16) & 0xFF;
    VERIFICAR(alfa >= 127 && alfa <= 128);
    VERIFICAR(vermelho <= alfa);
    VERIFICAR((b.px[0] & 0xFFFF) == 0);   // nada de verde/azul inventado
}

TESTE(reamostrar_ampliacao_vizinho) {
    Imagem a = imagemCheia(2, 1, 0xFF000000u);
    a.px[1] = 0xFFFFFFFFu;
    const Imagem b = reamostrar(a, 4, 2);
    VERIFICAR(b.px[0] == 0xFF000000u && b.px[1] == 0xFF000000u && b.px[2] == 0xFFFFFFFFu && b.px[7] == 0xFFFFFFFFu);
    VERIFICAR(reamostrar(a, 0, 5).px.empty());
}

TESTE(manifesto_campo_fator_opcional) {
    std::vector<EntradaManifesto> m;
    std::string erro;
    VERIFICAR(lerManifesto("a.png 128 192 32 48 4 4 pes 4\nb.png 16 16 16 16 1 1 centro\n", m, erro));
    VERIFICAR(m.size() == 2u && m[0].fator == 4 && m[1].fator == 1);
    VERIFICAR(!lerManifesto("a.png 16 16 16 16 1 1 centro 0\n", m, erro));   // fator inválido
}
