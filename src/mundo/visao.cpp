#include "mundo/visao.h"

#include <cmath>

namespace {

// ---------- frações exatas (as inclinações do algoritmo; evita erro de arredondamento) ----------
struct Fracao {
    int n = 0, d = 1;   // d > 0
};

int divisaoPiso(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }   // b > 0
int divisaoTeto(int a, int b) { return -divisaoPiso(-a, b); }

// Arredonda (profundidade × f) para o inteiro mais próximo; empate para cima / para baixo.
int arredondaEmpateCima(int prof, Fracao f) { return divisaoPiso(2 * prof * f.n + f.d, 2 * f.d); }
int arredondaEmpateBaixo(int prof, Fracao f) { return divisaoTeto(2 * prof * f.n - f.d, 2 * f.d); }

// ---------- quadrantes: (profundidade, coluna) → célula da sala ----------
enum class Quadrante { Norte, Sul, Leste, Oeste };

Celula transformar(Quadrante q, Celula origem, int prof, int col) {
    switch (q) {
        case Quadrante::Norte: return {origem.col + col, origem.lin - prof};
        case Quadrante::Sul: return {origem.col + col, origem.lin + prof};
        case Quadrante::Leste: return {origem.col + prof, origem.lin + col};
        default: return {origem.col - prof, origem.lin + col};
    }
}

struct Varredura {
    const SalaViva& sala;
    Celula origem;
    int profundidadeMaxima;
    bool* visto;   // cols × lins, marcado quando a célula é vista
};

bool bloqueiaVisao(const Varredura& v, Celula c) { return tileEm(v.sala, c.col, c.lin).bloqueiaVisao; }

void marcar(Varredura& v, Celula c) {
    if (dentroDaSala(v.sala, c.col, c.lin)) v.visto[c.lin * v.sala.cols + c.col] = true;
}

// Uma "linha" do quadrante (todas as colunas numa profundidade) entre duas inclinações.
void varrer(Varredura& v, Quadrante q, int prof, Fracao inicio, Fracao fim) {
    if (prof > v.profundidadeMaxima) return;
    const int colMin = arredondaEmpateCima(prof, inicio), colMax = arredondaEmpateBaixo(prof, fim);
    int anterior = -1;   // -1 = nenhuma, 0 = chão, 1 = parede
    for (int col = colMin; col <= colMax; ++col) {
        const Celula c = transformar(q, v.origem, prof, col);
        const bool parede = bloqueiaVisao(v, c);
        // Simétrica: a célula de chão só é vista se o CENTRO dela estiver dentro do cone.
        const bool simetrica = col * inicio.d >= prof * inicio.n && col * fim.d <= prof * fim.n;
        if (parede || simetrica) marcar(v, c);
        const Fracao inclinacao{2 * col - 1, 2 * prof};
        if (anterior == 1 && !parede) inicio = inclinacao;
        if (anterior == 0 && parede) varrer(v, q, prof + 1, inicio, inclinacao);
        anterior = parede ? 1 : 0;
    }
    if (anterior == 0) varrer(v, q, prof + 1, inicio, fim);
}

void lancarSombras(Varredura& v) {
    marcar(v, v.origem);
    for (Quadrante q : {Quadrante::Norte, Quadrante::Sul, Quadrante::Leste, Quadrante::Oeste}) {
        varrer(v, q, 1, Fracao{-1, 1}, Fracao{1, 1});
    }
}

const CelulaVisao DESCONHECIDA{};

}  // namespace

void iniciarVisao(MapaVisao& visao, const SalaViva& sala) {
    visao = MapaVisao{};
    visao.cols = sala.cols;
    visao.lins = sala.lins;
}

void atualizarVisao(MapaVisao& visao, const SalaViva& sala, Vec2 origem, float raioCelulas) {
    if (visao.cols != sala.cols || visao.lins != sala.lins) iniciarVisao(visao, sala);
    bool visto[SALA_MAX_CELULAS] = {};
    Varredura v{sala, celulaDe(origem), static_cast<int>(std::ceil(raioCelulas)), visto};
    lancarSombras(v);

    // Luz com queda suave até a borda do raio (a borda "respira" em vez de cortar seco).
    const float alcance = raioCelulas + 0.5f;
    for (int l = 0; l < sala.lins; ++l) {
        for (int c = 0; c < sala.cols; ++c) {
            CelulaVisao& cv = visao.celulas[static_cast<std::size_t>(l * sala.cols + c)];
            cv.luz = 0;
            if (!visto[l * sala.cols + c]) continue;
            const float dx = (c + 0.5f) - origem.x / TILE, dy = (l + 0.5f) - origem.y / TILE;
            const float t = std::sqrt(dx * dx + dy * dy) / alcance;
            if (t >= 1.0f) continue;
            const float t2 = t * t;
            cv.luz = static_cast<std::uint8_t>(255.0f * (1.0f - t2 * t2) + 0.5f);
            if (cv.luz == 0) cv.luz = 1;
            cv.explorada = true;
        }
    }
}

void revelarTudo(MapaVisao& visao) {
    for (int i = 0; i < visao.cols * visao.lins; ++i) visao.celulas[static_cast<std::size_t>(i)].explorada = true;
}

const CelulaVisao& visaoEm(const MapaVisao& visao, int col, int lin) {
    if (col < 0 || lin < 0 || col >= visao.cols || lin >= visao.lins) return DESCONHECIDA;
    return visao.celulas[static_cast<std::size_t>(lin * visao.cols + col)];
}

bool estaVisivel(const MapaVisao& visao, Vec2 posicao) {
    const Celula c = celulaDe(posicao);
    return visaoEm(visao, c.col, c.lin).luz > 0;
}

bool linhaDeVisao(const SalaViva& sala, Celula a, Celula b, int alcanceCelulas) {
    bool visto[SALA_MAX_CELULAS] = {};
    Varredura v{sala, a, alcanceCelulas, visto};
    lancarSombras(v);
    return dentroDaSala(sala, b.col, b.lin) && visto[b.lin * sala.cols + b.col];
}

bool segmentoLivre(const SalaViva& sala, Vec2 a, Vec2 b, bool paraAndar) {
    const Vec2 d = subtrair(b, a);
    const int passos = std::max(1, static_cast<int>(comprimento(d) / 4.0f));
    for (int i = 0; i <= passos; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(passos);
        const Celula c = celulaDe(somar(a, escalar(d, t)));
        const Tile& tile = tileEm(sala, c.col, c.lin);
        if (paraAndar ? tile.bloqueiaMovimento : tile.bloqueiaVisao) return false;
    }
    return true;
}
