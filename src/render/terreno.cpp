#include "render/terreno.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "render/sprite.h"

namespace {

constexpr std::size_t LIMITE_CACHE_BYTES = 96u * 1024u * 1024u;   // acima disso, desenha direto
constexpr float AMPLITUDE_BORDA = 9.0f;   // quanto a borda entre materiais "ondula" (px lógicos)

// ---------- ruído de valor (determinístico, sem tabela) ----------
float hash01(int x, int y, int semente) {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 374761393u + static_cast<std::uint32_t>(y) * 668265263u +
                      static_cast<std::uint32_t>(semente) * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return static_cast<float>(h & 0xFFFFFFu) / 16777216.0f;
}

float suave(float t) { return t * t * (3.0f - 2.0f * t); }

float ruidoValor(float x, float y, int semente) {
    const float fx = std::floor(x), fy = std::floor(y);
    const int ix = static_cast<int>(fx), iy = static_cast<int>(fy);
    const float tx = suave(x - fx), ty = suave(y - fy);
    const float a = hash01(ix, iy, semente), b = hash01(ix + 1, iy, semente);
    const float c = hash01(ix, iy + 1, semente), d = hash01(ix + 1, iy + 1, semente);
    return (a + (b - a) * tx) + ((c + (d - c) * tx) - (a + (b - a) * tx)) * ty;
}

// Deslocamento −1..1 com duas oitavas (forma grande + recorte miúdo).
float ondulacao(float x, float y, int semente) {
    const float n = 0.65f * ruidoValor(x / 14.0f, y / 14.0f, semente) + 0.35f * ruidoValor(x / 5.0f, y / 5.0f, semente + 7);
    return n * 2.0f - 1.0f;
}

// ---------- materiais ----------
bool ehPiso(TipoTile t) {
    const MaterialPiso m = materialDe(t);
    return m == MaterialPiso::Piso || m == MaterialPiso::PisoVariante || m == MaterialPiso::Liquido ||
           m == MaterialPiso::LiquidoFundo;
}
bool ehLiquido(TipoTile t) {
    const MaterialPiso m = materialDe(t);
    return m == MaterialPiso::Liquido || m == MaterialPiso::LiquidoFundo;
}
bool ehParede(TipoTile t) { return materialDe(t) == MaterialPiso::Parede; }

struct Texturas {
    Sprite piso, piso2, liquido, face, topo;
};

Texturas texturasDe(const Recursos& r, const std::string& ambiente) {
    auto t = [&](const char* material) { return spriteDe(r, "tex_" + ambiente + "_" + material); };
    return {t("piso"), t("piso2"), t("liquido"), t("parede_face"), t("parede_topo")};
}

// Cor opaca de uma textura contínua no pixel de tela (X, Y): como cada célula usa o frame
// (col mod 4, lin mod 4), o atlas inteiro se repete a cada 4 células e basta o módulo.
std::uint32_t amostrar(const Sprite& s, int X, int Y) {
    if (!spriteValido(s)) return 0xFF202020u;
    const Imagem& img = *s.atlas;
    return img.px[static_cast<std::size_t>((Y % img.h) * img.w + (X % img.w))] | 0xFF000000u;
}

std::uint32_t misturarCor(std::uint32_t a, std::uint32_t b, float t) {   // t = peso de b
    const auto canal = [&](int desloc) {
        const float va = static_cast<float>((a >> desloc) & 0xFF), vb = static_cast<float>((b >> desloc) & 0xFF);
        return static_cast<std::uint32_t>(va + (vb - va) * t + 0.5f) << desloc;
    };
    return 0xFF000000u | canal(16) | canal(8) | canal(0);
}

std::uint32_t escurecer(std::uint32_t p, float fator) { return misturarCor(p, 0xFF000000u, 1.0f - fator); }

// Assoalho da armadilha: quatro tábuas com veio e pregos. Depois da queda, as pontas
// irregulares continuam presas à moldura; o centro revela as vigas, não um quadrado preto.
// É amostrado em coordenadas lógicas para ficar igual no cache e no desenho direto.
std::uint32_t corAssoalho(float u, float v, bool caiu) {
    const int faixa = std::clamp(static_cast<int>(v / 8.0f), 0, 3);
    const float naTabua = v - 8.0f * faixa;
    const float lasca = (naTabua < 2.0f ? 2.0f : naTabua > 5.0f ? -2.0f : 0.0f);
    const float pontaEsquerda[4] = {14.0f, 10.0f, 17.0f, 12.0f};
    const float pontaDireita[4] = {21.0f, 17.0f, 23.0f, 19.0f};
    const bool tabua = !caiu || v < 3.0f || v >= 29.0f ||
                       u < pontaEsquerda[faixa] + lasca || u >= pontaDireita[faixa] - lasca;
    if (!tabua) {
        // Vigas transversais visíveis sob o assoalho e lascas caídas na abertura.
        if ((v >= 8.0f && v < 10.0f) || (v >= 22.0f && v < 24.0f))
            return (static_cast<int>(u + v) % 7 < 2) ? 0xFF4A3020u : 0xFF382418u;
        if ((v > 12.0f && v < 14.0f && u > 15.0f && u < 19.0f) ||
            (v > 25.0f && v < 27.0f && u > 12.0f && u < 16.0f)) return 0xFF65402Au;
        return 0xFF241913u;
    }
    const int ruido = static_cast<int>(hash01(static_cast<int>(u * 2), static_cast<int>(v * 2), 61) * 13.0f);
    const int tom = 74 + faixa * 5 + ruido + static_cast<int>(4.0f * std::sin(u * 0.6f + faixa * 2.0f));
    std::uint32_t cor = rgb(static_cast<std::uint8_t>(tom + 19), static_cast<std::uint8_t>(tom * 0.67f),
                            static_cast<std::uint8_t>(tom * 0.43f));
    if (naTabua < 1.0f || naTabua > 7.0f) cor = 0xFF302016u;  // fresta entre tábuas
    else if (naTabua < 2.0f) cor = misturarCor(cor, 0xFFCF9560u, 0.45f);
    else if (static_cast<int>(u + 2.0f * std::sin(v * 0.8f)) % 11 == 0) cor = escurecer(cor, 0.76f);  // veio
    if ((u < 5.0f || u > 27.0f) && naTabua > 3.0f && naTabua < 5.0f) cor = 0xFF251B16u;  // pregos
    if (caiu && ((u >= pontaEsquerda[faixa] + lasca - 1.0f && u < pontaEsquerda[faixa] + lasca + 1.0f) ||
                 (u >= pontaDireita[faixa] - lasca - 1.0f && u < pontaDireita[faixa] - lasca + 1.0f)))
        cor = 0xFFD19A62u;  // madeira recém-partida
    return cor;
}

// Cor final de um tipo de piso no pixel (X, Y). Tintas por tipo (lama, água funda) ficam aqui,
// para a borda orgânica e o miolo da célula usarem exatamente a mesma cor.
std::uint32_t corDoPiso(const Texturas& tx, TipoTile tipo, int X, int Y) {
    switch (materialDe(tipo)) {
        case MaterialPiso::PisoVariante: {
            const std::uint32_t c = amostrar(tx.piso2, X, Y);
            // Lama: marrom-escuro bem dominante, para não se confundir com a grama do piso variante.
            return tipo == TipoTile::Lama ? misturarCor(escurecer(c, 0.8f), 0xFF4A3420u, 0.62f) : c;
        }
        case MaterialPiso::Liquido: return amostrar(tx.liquido, X, Y);
        // Água funda (bloqueia): escura E com pouco contraste. Só escurecer não bastava no riacho
        // de pedras do Barrens: as pedras continuavam visíveis e diziam "dá para pisar" (10.1).
        case MaterialPiso::LiquidoFundo: return misturarCor(escurecer(amostrar(tx.liquido, X, Y), 0.55f), 0xFF14303Cu, 0.6f);
        case MaterialPiso::Buraco: return 0xFF050608u;
        default: return amostrar(tx.piso, X, Y);
    }
}

// Tipo de piso "visto" no ponto lógico (x, y), já com a borda ondulada. Parede nunca invade
// (a colisão é reta; a arte da parede também precisa ser) e não é invadida.
TipoTile pisoOndulado(const SalaViva& sala, float x, float y) {
    const int c = static_cast<int>(std::floor(x / TILE)), l = static_cast<int>(std::floor(y / TILE));
    const TipoTile proprio = tileEm(sala, c, l).tipo;
    if (!ehPiso(proprio)) return proprio;
    const float dx = AMPLITUDE_BORDA * ondulacao(x, y, 11), dy = AMPLITUDE_BORDA * ondulacao(x, y, 29);
    const TipoTile vizinho =
        tileEm(sala, static_cast<int>(std::floor((x + dx) / TILE)), static_cast<int>(std::floor((y + dy) / TILE))).tipo;
    return ehPiso(vizinho) ? vizinho : proprio;
}

// A célula tem algum vizinho (8 direções) de piso com material diferente? Só essas precisam
// do cálculo por pixel; o resto é textura pura (rápido).
bool celulaDeBorda(const SalaViva& sala, int c, int l) {
    const MaterialPiso m = materialDe(tileEm(sala, c, l).tipo);
    for (int dl = -1; dl <= 1; ++dl) {
        for (int dc = -1; dc <= 1; ++dc) {
            const TipoTile v = tileEm(sala, c + dc, l + dl).tipo;
            if (ehPiso(v) && (materialDe(v) != m || v != tileEm(sala, c, l).tipo)) return true;
        }
    }
    return false;
}

// Pinta o piso de uma célula pixel a pixel (com ou sem ondulação), mais a espuma e o degrau
// escuro na borda de cima da água.
void pintarPisoCelula(Canvas& cv, const SalaViva& sala, const Texturas& tx, int c, int l) {
    const int k = cv.escala, lado = TILE * k;
    const bool borda = celulaDeBorda(sala, c, l);
    const TipoTile proprio = tileEm(sala, c, l).tipo;
    const bool assoalho = proprio == TipoTile::PisoCedendo ||
                           (proprio == TipoTile::Buraco && tileEm(sala, c, l).estado == 1);
    const bool aguaPerto = ehLiquido(proprio) || ehLiquido(tileEm(sala, c, l + 1).tipo) ||
                           ehLiquido(tileEm(sala, c - 1, l + 1).tipo) || ehLiquido(tileEm(sala, c + 1, l + 1).tipo);
    const float inv = 1.0f / static_cast<float>(k);
    for (int py = 0; py < lado; ++py) {
        const int Y = l * lado + py;
        if (Y < 0 || Y >= cv.h) continue;
        std::uint32_t* linha = cv.px + static_cast<std::size_t>(Y) * cv.w;
        const float y = (static_cast<float>(Y) + 0.5f) * inv;
        for (int px = 0; px < lado; ++px) {
            const int X = c * lado + px;
            if (X < 0 || X >= cv.w) continue;
            const float x = (static_cast<float>(X) + 0.5f) * inv;
            const TipoTile t = borda && !assoalho ? pisoOndulado(sala, x, y) : proprio;
            std::uint32_t cor = assoalho ? corAssoalho(static_cast<float>(px) * inv, static_cast<float>(py) * inv,
                                                       proprio == TipoTile::Buraco) : corDoPiso(tx, t, X, Y);
            if (borda && aguaPerto && ehLiquido(t)) {
                if (!ehLiquido(pisoOndulado(sala, x, y - 1.5f))) cor = misturarCor(cor, 0xFFB0CCD6u, 0.45f);   // espuma
                else if (!ehLiquido(pisoOndulado(sala, x, y - 7.0f))) cor = escurecer(cor, 0.62f);               // degrau
            }
            linha[X] = cor;
        }
    }
}

// Sombra em degradê: escurece `altura` unidades a partir de (x, y) para baixo.
void sombraVertical(Canvas& canvas, float x, float y, float largura, float altura, std::uint8_t alfaInicial) {
    constexpr int PASSOS = 8;
    for (int i = 0; i < PASSOS; ++i) {
        const float t = static_cast<float>(i) / PASSOS;
        const auto alfa = static_cast<std::uint8_t>(alfaInicial * (1.0f - t) * (1.0f - t));
        retanguloAlpha(canvas, x, y + altura * t, largura, altura / PASSOS + 0.01f, rgb(4, 6, 10), alfa);
    }
}

// Paredes (vista 3/4), sombras de profundidade e marcadores provisórios (portas, objetos de
// terreno que ganham arte própria nas próximas etapas).
void pintarDetalhesCelula(Canvas& cv, const SalaViva& sala, const Texturas& tx, int c, int l, float x, float y) {
    const TipoTile t = tileEm(sala, c, l).tipo;
    if (ehParede(t)) {
        const bool chaoAbaixo = !ehParede(tileEm(sala, c, l + 1).tipo) && l + 1 < sala.lins;
        desenharFrame(cv, chaoAbaixo ? tx.face : tx.topo, c % 4, l % 4, x, y);
        return;
    }
    if (ehParede(tileEm(sala, c, l - 1).tipo)) sombraVertical(cv, x, y, TILE, 12, 150);
    if (ehParede(tileEm(sala, c - 1, l).tipo)) retanguloAlpha(cv, x, y, 4, TILE, rgb(4, 6, 10), 90);
    if (ehParede(tileEm(sala, c + 1, l).tipo)) retanguloAlpha(cv, x + TILE - 4, y, 4, TILE, rgb(4, 6, 10), 90);
    switch (t) {
        case TipoTile::PortaSala:
            if (tileEm(sala, c, l).bloqueiaMovimento) {   // fechada (sala em combate): grade de ferro
                retanguloAlpha(cv, x, y, TILE, TILE, rgb(10, 8, 8), 200);
                for (int b = 0; b < 5; ++b) retanguloCheio(cv, x + 3 + 6.0f * b, y + 1, 2, TILE - 2, rgb(96, 70, 60));
                retanguloCheio(cv, x + 1, y + 10, TILE - 2, 2, rgb(96, 70, 60));
                retanguloCheio(cv, x + 1, y + 22, TILE - 2, 2, rgb(96, 70, 60));
            } else {   // aberta: passagem com chão mais claro
                retanguloAlpha(cv, x + 2, y + 2, TILE - 4, TILE - 4, rgb(220, 210, 170), 40);
                contornoRetangulo(cv, x + 2, y + 2, TILE - 4, TILE - 4, rgb(30, 26, 20));
            }
            break;
        case TipoTile::Buraco:
            if (tileEm(sala, c, l).estado != 1) sombraVertical(cv, x, y, TILE, 10, 200);
            break;
        default: break;
    }
}

void pintarSalaInteira(Canvas& cv, const SalaViva& sala, const Recursos& r) {
    const Texturas tx = texturasDe(r, sala.modelo->ambiente);
    limpar(cv, rgb(0, 0, 0));
    for (int l = 0; l < sala.lins; ++l) {
        for (int c = 0; c < sala.cols; ++c) {
            if (!ehParede(tileEm(sala, c, l).tipo)) pintarPisoCelula(cv, sala, tx, c, l);
        }
    }
    for (int l = 0; l < sala.lins; ++l) {
        for (int c = 0; c < sala.cols; ++c) {
            pintarDetalhesCelula(cv, sala, tx, c, l, static_cast<float>(c * TILE), static_cast<float>(l * TILE));
        }
    }
}

// Sem cache (sala enorme numa tela 4K): desenha só as células visíveis, sem bordas onduladas.
void desenharDireto(Canvas& canvas, const SalaViva& sala, const Recursos& r, float camX, float camY) {
    const Texturas tx = texturasDe(r, sala.modelo->ambiente);
    const int c0 = static_cast<int>(std::floor(camX / TILE)), l0 = static_cast<int>(std::floor(camY / TILE));
    for (int l = l0; l <= l0 + ALTURA_LOGICA / TILE + 1; ++l) {
        for (int c = c0; c <= c0 + LARGURA_LOGICA / TILE + 1; ++c) {
            if (!dentroDaSala(sala, c, l)) continue;
            const float x = c * TILE - camX, y = l * TILE - camY;
            const TipoTile t = tileEm(sala, c, l).tipo;
            if (t == TipoTile::PisoCedendo || (t == TipoTile::Buraco && tileEm(sala, c, l).estado == 1)) {
                const int k = canvas.escala;
                for (int py = 0; py < TILE * k; ++py) {
                    const int Y = paraTela(canvas, y) + py;
                    if (Y < 0 || Y >= canvas.h) continue;
                    for (int px = 0; px < TILE * k; ++px) {
                        const int X = paraTela(canvas, x) + px;
                        if (X >= 0 && X < canvas.w)
                            canvas.px[static_cast<std::size_t>(Y) * canvas.w + X] =
                                corAssoalho(static_cast<float>(px) / k, static_cast<float>(py) / k, t == TipoTile::Buraco);
                    }
                }
            } else if (!ehParede(t)) {
                const Sprite& s = materialDe(t) == MaterialPiso::PisoVariante ? tx.piso2
                                  : ehLiquido(t)                               ? tx.liquido
                                                                               : tx.piso;
                desenharFrame(canvas, s, c % 4, l % 4, x, y);
            }
            pintarDetalhesCelula(canvas, sala, tx, c, l, x, y);
        }
    }
}

}  // namespace

void liberarCacheTerreno(CacheTerreno& cache) {
    liberarCanvas(cache.canvas);
    cache = CacheTerreno{};
}

void prepararCacheTerreno(CacheTerreno& cache, const SalaViva& sala, const Recursos& recursos, int escala) {
    if (cache.modelo == sala.modelo && cache.versao == sala.versaoTerreno && cache.escala == escala) return;
    liberarCacheTerreno(cache);
    cache.modelo = sala.modelo;
    cache.versao = sala.versaoTerreno;
    cache.escala = escala;
    const std::size_t bytes = static_cast<std::size_t>(sala.cols) * TILE * escala *
                              static_cast<std::size_t>(sala.lins) * TILE * escala * 4u;
    if (bytes > LIMITE_CACHE_BYTES || !criarCanvas(cache.canvas, sala.cols * TILE, sala.lins * TILE, escala)) {
        cache.valido = false;
        return;
    }
    pintarSalaInteira(cache.canvas, sala, recursos);
#ifndef __EMSCRIPTEN__
    GdiFlush();
#endif
    cache.valido = true;
}

void desenharTerreno(Canvas& canvas, const CacheTerreno& cache, const SalaViva& sala, const Recursos& recursos,
                     float camX, float camY) {
    if (!cache.valido) {
        desenharDireto(canvas, sala, recursos, camX, camY);
        return;
    }
    // Copia por linhas o retângulo visível do cache (pixels de tela, com recorte).
    const Canvas& origem = cache.canvas;
    int sx = paraTela(canvas, camX), sy = paraTela(canvas, camY);
    int dx = 0, dy = 0;
    int w = canvas.w, h = canvas.h;
    if (sx < 0) { dx = -sx; w += sx; sx = 0; }
    if (sy < 0) { dy = -sy; h += sy; sy = 0; }
    if (sx + w > origem.w) w = origem.w - sx;
    if (sy + h > origem.h) h = origem.h - sy;
    if (dx + w > canvas.w) w = canvas.w - dx;
    if (dy + h > canvas.h) h = canvas.h - dy;
    if (w <= 0 || h <= 0) return;
    for (int y = 0; y < h; ++y) {
        std::memcpy(canvas.px + static_cast<std::size_t>(dy + y) * canvas.w + dx,
                    origem.px + static_cast<std::size_t>(sy + y) * origem.w + sx, static_cast<std::size_t>(w) * 4u);
    }
}
