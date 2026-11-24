#include "combate/area.h"

#include <algorithm>
#include <cmath>

namespace {

float produtoEscalar(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }

Vec2 pontoMaisProximo(const Retangulo& r, Vec2 p) {
    return {std::clamp(p.x, r.x, r.x + r.w), std::clamp(p.y, r.y, r.y + r.h)};
}

}  // namespace

Area areaCirculo(Vec2 centro, float raio) {
    Area a;
    a.forma = FormaArea::Circulo;
    a.centro = centro;
    a.raio = raio;
    return a;
}

Area areaArco(Vec2 centro, Vec2 direcao, float raio, float anguloGraus) {
    Area a = areaCirculo(centro, raio);
    a.forma = FormaArea::Arco;
    a.direcao = normalizar(direcao);
    a.anguloGraus = anguloGraus;
    return a;
}

Area areaLinha(Vec2 inicio, Vec2 direcao, float comprimento, float largura) {
    Area a;
    a.forma = FormaArea::Linha;
    a.centro = inicio;
    a.direcao = normalizar(direcao);
    a.comprimento = comprimento;
    a.raio = largura / 2.0f;
    return a;
}

Area areaAnel(Vec2 centro, float raioInterno, float raioExterno) {
    Area a = areaCirculo(centro, raioExterno);
    a.forma = FormaArea::Anel;
    a.raioInterno = raioInterno;
    return a;
}

bool pontoNaArea(const Area& a, Vec2 p) {
    const Vec2 d = subtrair(p, a.centro);
    switch (a.forma) {
        case FormaArea::Circulo: return produtoEscalar(d, d) <= a.raio * a.raio;
        case FormaArea::Anel: {
            const float d2 = produtoEscalar(d, d);
            return d2 <= a.raio * a.raio && d2 >= a.raioInterno * a.raioInterno;
        }
        case FormaArea::Arco: {
            const float dist = comprimento(d);
            if (dist > a.raio) return false;
            if (dist < 6.0f) return true;   // colado ao atacante: sempre dentro
            const float cosMeio = std::cos(a.anguloGraus * 0.5f * 3.14159265f / 180.0f);
            return produtoEscalar(d, a.direcao) / dist >= cosMeio;
        }
        case FormaArea::Linha: {
            const float ao = produtoEscalar(d, a.direcao);
            if (ao < 0.0f || ao > a.comprimento) return false;
            const float lateral = d.x * a.direcao.y - d.y * a.direcao.x;
            return std::fabs(lateral) <= a.raio;
        }
    }
    return false;
}

bool areaToca(const Area& a, const Retangulo& r) {
    switch (a.forma) {
        case FormaArea::Circulo: return pontoNaArea(a, pontoMaisProximo(r, a.centro));
        case FormaArea::Anel: {
            // Toca se o ponto do retângulo MAIS PERTO está dentro do raio externo e o MAIS LONGE
            // (um dos cantos) está fora do raio interno.
            const Vec2 perto = pontoMaisProximo(r, a.centro);
            if (produtoEscalar(subtrair(perto, a.centro), subtrair(perto, a.centro)) > a.raio * a.raio) return false;
            const float lx = std::max(std::fabs(r.x - a.centro.x), std::fabs(r.x + r.w - a.centro.x));
            const float ly = std::max(std::fabs(r.y - a.centro.y), std::fabs(r.y + r.h - a.centro.y));
            return lx * lx + ly * ly >= a.raioInterno * a.raioInterno;
        }
        case FormaArea::Arco:
            return pontoNaArea(a, pontoMaisProximo(r, a.centro)) ||
                   pontoNaArea(a, {r.x + r.w / 2, r.y + r.h / 2});
        case FormaArea::Linha: {
            // Amostra o eixo da linha a cada 6 px e testa um círculo da meia-largura.
            const int passos = static_cast<int>(a.comprimento / 6.0f) + 1;
            for (int i = 0; i <= passos; ++i) {
                const Vec2 p = somar(a.centro, escalar(a.direcao, a.comprimento * static_cast<float>(i) / passos));
                const Vec2 q = pontoMaisProximo(r, p);
                const Vec2 d = subtrair(q, p);
                if (produtoEscalar(d, d) <= a.raio * a.raio) return true;
            }
            return false;
        }
    }
    return false;
}

Retangulo envolvente(const Area& a) {
    if (a.forma == FormaArea::Linha) {
        const Vec2 fim = somar(a.centro, escalar(a.direcao, a.comprimento));
        const float x0 = std::min(a.centro.x, fim.x) - a.raio, y0 = std::min(a.centro.y, fim.y) - a.raio;
        const float x1 = std::max(a.centro.x, fim.x) + a.raio, y1 = std::max(a.centro.y, fim.y) + a.raio;
        return {x0, y0, x1 - x0, y1 - y0};
    }
    return {a.centro.x - a.raio, a.centro.y - a.raio, 2 * a.raio, 2 * a.raio};
}
