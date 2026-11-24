#pragma once
// Imagem em memória e reamostragem. Pura (sem Windows): testável em testes_regras.
// Pixels ARGB de 32 bits com alfa PRÉ-MULTIPLICADO (cada canal de cor já vem multiplicado
// pelo alfa), o formato que permite misturar e reduzir sem halos escuros nas bordas.
#include <cstdint>
#include <vector>

struct Imagem {
    int w = 0;
    int h = 0;
    std::vector<std::uint32_t> px;   // w * h pixels, linha a linha, de cima para baixo
};

// Muda o tamanho da imagem.
//  - Reduzir: média por ÁREA (cada pixel novo é a média ponderada dos pixels de origem que
//    ele cobre). Preserva o detalhe dos assets HD sem serrilhar nem borrar demais.
//  - Ampliar: vizinho mais próximo (nunca inventa pixels).
// Feito uma única vez no carregamento ou na troca de escala, nunca a cada quadro.
Imagem reamostrar(const Imagem& origem, int novoW, int novoH);
// Pixel mais próximo (amostra no CENTRO de cada pixel de destino): reduz pixel art sem borrar as bordas dos blocos.
Imagem reduzirVizinho(const Imagem& origem, int novoW, int novoH);

// Realce de nitidez (unsharp 3×3): pixel + forca% × (pixel − média dos 8 vizinhos e ele). Devolve o
// contraste fino que a redução por área tira. Feito uma vez por imagem (cenas da história), nunca por quadro.
void realcarNitidez(Imagem& imagem, int forcaPct);

// Tinge um pixel PRÉ-MULTIPLICADO na direção de `cor` (0xRRGGBB) com força 0..255, mantendo o
// alfa (e por isso a silhueta). 0 = original; 255 = cor chapada (o "piscar branco" do dano).
// Usado pela aparência do Bill (cura verde, coragem quente) e pelo acerto nos inimigos.
std::uint32_t tingir(std::uint32_t pixel, std::uint32_t cor, std::uint32_t forca);
