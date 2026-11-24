#include "mundo/colisao.h"

#include <cmath>

namespace {

constexpr float FOLGA = 0.01f;   // distância mínima da parede depois de encostar

int celulaDeCoordenada(float v) { return static_cast<int>(std::floor(v / TILE)); }

bool bloqueia(const Tile& t, bool voa) { return voa ? t.bloqueiaVoo : t.bloqueiaMovimento; }

// Um eixo de cada vez: `eixoX` diz qual. Supõe que a posição atual NÃO colide.
Vec2 moverEixo(const SalaViva& sala, Vec2 pes, Vec2 tamanho, float passo, bool eixoX, bool voa) {
    Vec2 nova = pes;
    if (eixoX) nova.x += passo; else nova.y += passo;
    const Retangulo caixa = caixaNosPes(nova, tamanho);
    if (!colideComSala(sala, caixa, voa)) return nova;
    // Encosta na borda do tile que bloqueou (o da frente no sentido do movimento).
    if (eixoX) {
        if (passo > 0) nova.x = celulaDeCoordenada(caixa.x + caixa.w) * TILE - tamanho.x / 2 - FOLGA;
        else nova.x = (celulaDeCoordenada(caixa.x) + 1) * TILE + tamanho.x / 2 + FOLGA;
        // Se encostar ainda colidir (ex.: canto), não se move neste eixo.
        if (colideComSala(sala, caixaNosPes(nova, tamanho), voa)) nova.x = pes.x;
    } else {
        if (passo > 0) nova.y = celulaDeCoordenada(caixa.y + caixa.h) * TILE - tamanho.y / 2 - FOLGA;
        else nova.y = (celulaDeCoordenada(caixa.y) + 1) * TILE + tamanho.y / 2 + FOLGA;
        if (colideComSala(sala, caixaNosPes(nova, tamanho), voa)) nova.y = pes.y;
    }
    return nova;
}

}  // namespace

Retangulo caixaNosPes(Vec2 pes, Vec2 tamanho) {
    return {pes.x - tamanho.x / 2, pes.y - tamanho.y / 2, tamanho.x, tamanho.y};
}

bool colideComSala(const SalaViva& sala, const Retangulo& caixa, bool voa) {
    const int c0 = celulaDeCoordenada(caixa.x);
    const int c1 = celulaDeCoordenada(caixa.x + caixa.w - FOLGA / 2);
    const int l0 = celulaDeCoordenada(caixa.y);
    const int l1 = celulaDeCoordenada(caixa.y + caixa.h - FOLGA / 2);
    for (int l = l0; l <= l1; ++l) {
        for (int c = c0; c <= c1; ++c) {
            if (bloqueia(tileEm(sala, c, l), voa)) return true;
        }
    }
    return false;
}

Vec2 moverComColisao(const SalaViva& sala, Vec2 pes, Vec2 tamanho, Vec2 delta, bool voa) {
    const float maior = std::fmax(std::fabs(delta.x), std::fabs(delta.y));
    const int passos = static_cast<int>(std::ceil(maior / SUBPASSO_MAXIMO));
    if (passos <= 0) return pes;
    const Vec2 passo = escalar(delta, 1.0f / static_cast<float>(passos));
    for (int i = 0; i < passos; ++i) {
        if (passo.x != 0.0f) pes = moverEixo(sala, pes, tamanho, passo.x, true, voa);
        if (passo.y != 0.0f) pes = moverEixo(sala, pes, tamanho, passo.y, false, voa);
    }
    return pes;
}

const PortaSala* portaTocada(const SalaViva& sala, Vec2 pes, Vec2 tamanho) {
    const Retangulo caixa = caixaNosPes(pes, tamanho);
    for (const PortaSala& p : sala.portas) {
        const float x = static_cast<float>(p.celula.col * TILE), y = static_cast<float>(p.celula.lin * TILE);
        const bool sobrepoe = caixa.x < x + TILE && caixa.x + caixa.w > x && caixa.y < y + TILE && caixa.y + caixa.h > y;
        if (sobrepoe && p.aberta) return &p;
    }
    return nullptr;
}
