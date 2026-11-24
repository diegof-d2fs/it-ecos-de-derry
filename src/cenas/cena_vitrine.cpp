// Cena de vitrine da Etapa 1 (Fundação): prova que janela, loop, canvas, sprites,
// animação, câmera, texto com acentos e escala inteira funcionam juntos.
// Ainda não há colisão com paredes (Etapa 3): Bill fica preso só aos limites da sala.
#include <cmath>
#include <cstdio>
#include <string>

#include "cenas/cena.h"
#include "core/medidor.h"
#include "render/sprite.h"
#include "render/texto.h"

namespace {

constexpr int SALA_COLS = 30;
constexpr int SALA_LINS = 17;
constexpr float VELOCIDADE_BILL = 128.0f;   // px/s = 120 + 8 × Agilidade 1 (PRD v2 §7.1)

// Sala de demonstração. Legenda: '#' parede · '.' piso · ',' piso secundário (musgo) · '~' água.
// (Na Etapa 3 isto vem de data/salas/*.txt.)
const char* const MAPA[SALA_LINS] = {
    "##############################",
    "##############################",
    "#............................#",
    "#............................#",
    "#............................#",
    "#............................#",
    "#...,,,,..............~~~~...#",
    "#..,,,,,,.............~~~~~..#",
    "#...,,,,..............~~~~...#",
    "#.....###....................#",
    "#.....###....................#",
    "#.....###....................#",
    "#............................#",
    "#.........,,.................#",
    "#........,,,,................#",
    "#............................#",
    "##############################",
};

char celulaMapa(int col, int lin) {
    if (col < 0 || lin < 0 || col >= SALA_COLS || lin >= SALA_LINS) return '#';
    return MAPA[lin][col];
}

// Texturas contínuas (assets tex_<ambiente>_<material>): a célula (c, l) usa o pedaço
// (c mod 4, l mod 4), então o material emenda na sala inteira, sem moldura por tile.
void desenharTextura(Canvas& canvas, const Sprite& textura, int col, int lin, float x, float y) {
    desenharFrame(canvas, textura, col % 4, lin % 4, x, y);
}

// Sombra em degradê: escurece `altura` unidades a partir de (x, y) para baixo.
void sombraVertical(Canvas& canvas, float x, float y, float largura, float altura, std::uint8_t alfaInicial) {
    const int passos = 8;
    for (int i = 0; i < passos; ++i) {
        const float t = static_cast<float>(i) / passos;
        const auto alfa = static_cast<std::uint8_t>(alfaInicial * (1.0f - t) * (1.0f - t));
        retanguloAlpha(canvas, x, y + altura * t, largura, altura / passos + 0.01f, rgb(4, 6, 10), alfa);
    }
}

// Os 9 ambientes do PRD v2 (prefixo das texturas tex_<ambiente>_<material>).
const char* const AMBIENTES_VITRINE[] = {"rua_derry", "clube", "galeria", "barrens", "neibolt",
                                         "poroes", "siderurgica", "cisterna", "covil"};
constexpr int TOTAL_AMBIENTES = static_cast<int>(sizeof(AMBIENTES_VITRINE) / sizeof(AMBIENTES_VITRINE[0]));

Sprite texturaDe(const Recursos& r, int ambiente, const char* material) {
    return spriteDe(r, std::string("tex_") + AMBIENTES_VITRINE[ambiente] + "_" + material);
}

void desenharSalaVitrine(Canvas& canvas, const Recursos& r, int ambiente, float camX, float camY) {
    const Sprite piso = texturaDe(r, ambiente, "piso");
    const Sprite piso2 = texturaDe(r, ambiente, "piso2");
    const Sprite face = texturaDe(r, ambiente, "parede_face");
    const Sprite topo = texturaDe(r, ambiente, "parede_topo");
    const Sprite agua = texturaDe(r, ambiente, "liquido");
    const int col0 = static_cast<int>(camX) / TILE, lin0 = static_cast<int>(camY) / TILE;
    const int col1 = col0 + LARGURA_LOGICA / TILE + 1, lin1 = lin0 + ALTURA_LOGICA / TILE + 1;
    for (int lin = lin0; lin <= lin1 && lin < SALA_LINS; ++lin) {
        for (int col = col0; col <= col1 && col < SALA_COLS; ++col) {
            const float x = col * TILE - camX, y = lin * TILE - camY;
            const char c = celulaMapa(col, lin);
            if (c == '#') {
                // Vista 3/4: parede com chão logo abaixo mostra a FACE; o resto mostra o TOPO.
                desenharTextura(canvas, celulaMapa(col, lin + 1) == '#' ? topo : face, col, lin, x, y);
                continue;
            }
            desenharTextura(canvas, c == ',' ? piso2 : c == '~' ? agua : piso, col, lin, x, y);
            // Profundidade desenhada pelo código: sombra ao pé da face e nas laterais.
            if (celulaMapa(col, lin - 1) == '#') sombraVertical(canvas, x, y, TILE, 12, 150);
            if (celulaMapa(col - 1, lin) == '#') retanguloAlpha(canvas, x, y, 4, TILE, rgb(4, 6, 10), 90);
            if (celulaMapa(col + 1, lin) == '#') retanguloAlpha(canvas, x + TILE - 4, y, 4, TILE, rgb(4, 6, 10), 90);
            if (c == '~' && celulaMapa(col, lin - 1) != '~') {   // borda da água: degrau escuro + espuma
                sombraVertical(canvas, x, y, TILE, 6, 170);
                retanguloAlpha(canvas, x, y + 1, TILE, 1, rgb(170, 200, 210), 70);
            }
        }
    }
}

struct ItemElenco {
    const char* atlas;
    int linha;
    int quadros;
    float fps;
    const wchar_t* rotulo;
};

const ItemElenco ELENCO[] = {
    {"ben_idle", 0, 2, 2.0f, L"Ben"},         {"eddie_idle", 0, 2, 2.0f, L"Eddie"},
    {"beverly_idle", 0, 2, 2.0f, L"Beverly"}, {"mike_idle", 0, 2, 2.0f, L"Mike"},
    {"elias_idle", 0, 2, 2.0f, L"Elias"},     {"balao", 0, 3, 6.0f, L"Balão"},
    {"rato", 2, 3, 8.0f, L"Rato"},            {"sombra", 0, 3, 5.0f, L"Sombra"},
    {"pennywise_1_actions", 0, 4, 4.0f, L"Pennywise I"},
    {"pennywise_4_actions", 0, 4, 4.0f, L"Pennywise IV"},
};

}  // namespace

void entrarVitrine(Jogo& jogo) {
    jogo.vitrine = EstadoVitrine{};
    jogo.vitrine.posBill = {SALA_COLS * TILE / 2.0f, 7.5f * TILE};
    jogo.vitrine.ambiente = jogo.ambienteInicial % TOTAL_AMBIENTES;
}

void atualizarVitrine(Jogo& jogo, const EstadoEntrada& entrada, float dt) {
    EstadoVitrine& v = jogo.vitrine;
    const Vec2 eixo = entrada.eixo;
    v.andando = comprimento(eixo) > 0.01f;
    v.dirBill = direcaoDominante(eixo, v.dirBill);
    v.posBill = somar(v.posBill, escalar(eixo, VELOCIDADE_BILL * dt));
    // Limites provisórios: o interior da moldura de paredes.
    v.posBill.x = limitar(v.posBill.x, TILE + 9.0f, SALA_COLS * TILE - TILE - 9.0f);
    v.posBill.y = limitar(v.posBill.y, TILE + 12.0f, SALA_LINS * TILE - TILE - 2.0f);
    v.tempoAnimacao = v.andando ? v.tempoAnimacao + dt : 0.0f;
    if (pressionou(entrada, Comando::AbaDireita)) v.ambiente = (v.ambiente + 1) % TOTAL_AMBIENTES;
    if (pressionou(entrada, Comando::AbaEsquerda)) v.ambiente = (v.ambiente + TOTAL_AMBIENTES - 1) % TOTAL_AMBIENTES;

    // Câmera: segue Bill com amortecimento e não mostra além das bordas da sala.
    const Vec2 alvo{v.posBill.x - LARGURA_LOGICA / 2.0f, v.posBill.y - ALTURA_LOGICA / 2.0f};
    v.camera = somar(v.camera, escalar(subtrair(alvo, v.camera), 0.15f));
    v.camera.x = limitar(v.camera.x, 0.0f, static_cast<float>(SALA_COLS * TILE - LARGURA_LOGICA));
    v.camera.y = limitar(v.camera.y, 0.0f, static_cast<float>(SALA_LINS * TILE - ALTURA_LOGICA));
}

void desenharVitrine(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const EstadoVitrine& v = jogo.vitrine;
    // A câmera fica em float: o canvas converte para pixel de tela arredondando cada posição,
    // e como os tiles estão em múltiplos inteiros de 32, todos arredondam igual (sem "tremer").
    const float camX = v.camera.x;
    const float camY = v.camera.y;

    limpar(canvas, rgb(10, 12, 18));
    desenharSalaVitrine(canvas, r, v.ambiente, camX, camY);

    // Elenco já aprovado, animado no próprio motor (checagem visual dos atlas).
    const float t = jogo.tempoTotal;
    float x = 3.0f * TILE;
    for (const ItemElenco& item : ELENCO) {
        const Sprite s = spriteDe(r, item.atlas);
        if (!spriteValido(s)) continue;
        const float larguraLogica = static_cast<float>(s.frameW) / canvas.escala;
        const Animacao anim{item.linha, 0, item.quadros, item.fps, true};
        const float xPe = x + larguraLogica / 2 - camX, yPe = 5.0f * TILE - camY;
        elipseAlpha(canvas, xPe, yPe - 2, larguraLogica / 3, 4, rgb(0, 0, 0), 110);
        desenharFramePes(canvas, s, colunaAnimacao(anim, t), item.linha, xPe, yPe);
        desenharTextoCentralizado(canvas, r.fontePequena, xPe, yPe + 2, item.rotulo, rgb(214, 206, 188));
        x += larguraLogica + 26;
    }

    // Bill: linha = direção, colunas = ciclo de passos (repouso quando parado).
    const Sprite bill = spriteDe(r, "bill_walk");
    const Animacao andar{static_cast<int>(v.dirBill), 0, 4, 8.0f, true};
    const int coluna = v.andando ? colunaAnimacao(andar, v.tempoAnimacao) : 0;
    const float bx = v.posBill.x - camX;
    const float by = v.posBill.y - camY;
    elipseAlpha(canvas, bx, by - 2, 9, 4, rgb(0, 0, 0), 120);
    desenharFramePes(canvas, bill, coluna, static_cast<int>(v.dirBill), bx, by);

    // Painel de instruções.
    retanguloAlpha(canvas, 6, 6, 300, 52, rgb(8, 10, 16), 190);
    contornoRetangulo(canvas, 6, 6, 300, 52, rgb(179, 32, 42));
    desenharTexto(canvas, r.fonteMedia, 14, 10,
                  L"Ambiente: " + paraWide(AMBIENTES_VITRINE[v.ambiente]) + L"  (PageUp/PageDown troca)", rgb(240, 232, 214));
    desenharTexto(canvas, r.fontePequena, 14, 28, L"WASD/setas: andar · F11: tela cheia · F1: medidor", rgb(190, 184, 170));
    desenharTexto(canvas, r.fontePequena, 14, 41, L"Esc: sair · Acentos: ação, coração, você, João", rgb(190, 184, 170));

    if (jogo.mostrarMedidor) {
        wchar_t linha[96];
        std::swprintf(linha, 96, L"quadro: média %.2f ms · p95 %.2f ms · escala %d×", mediaMs(jogo.medidor),
                      percentil95Ms(jogo.medidor), jogo.escalaAtual);
        retanguloAlpha(canvas, 6, ALTURA_LOGICA - 24, 300, 18, rgb(8, 10, 16), 190);
        desenharTexto(canvas, r.fontePequena, 12, ALTURA_LOGICA - 22, linha, rgb(150, 230, 160));
    }
    if (jogo.pausadoPorFoco) {
        retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 150);
        desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2, 150, L"Pausado", rgb(240, 232, 214));
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2, 190, L"Clique na janela para continuar",
                                  rgb(200, 192, 176));
    }
}
