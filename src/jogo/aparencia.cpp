#include "jogo/aparencia.h"

#include <cmath>

AparenciaBill aparenciaDe(const Jogador& j, const Inventario& inv, float tempo) {
    AparenciaBill a;
    a.escudo = inv.quantidades[static_cast<std::size_t>(TipoItem::Escudo)] > 0;
    a.melhorias = inv.quantidades[static_cast<std::size_t>(TipoItem::MelhoriaArma)];
    if (j.coragem > 0.0f) a.aura = 0.6f + 0.4f * std::sin(tempo * 6.0f);
    a.rastro = j.tonico > 0.0f;
    a.revelaInimigos = j.oculos > 0.0f;
    // Tinta: uma só por vez, pela prioridade dano > cura > coragem (o que é mais urgente de ler).
    if (j.flash > 0.0f) {
        a.tinta = COR_TINTA_DANO;
        a.forcaTinta = 255;
    } else if (j.cura > 0.0f) {
        a.tinta = COR_TINTA_CURA;
        a.forcaTinta = static_cast<std::uint32_t>(170.0f * j.cura / 0.4f);
    } else if (j.coragem > 0.0f) {
        a.tinta = COR_TINTA_CORAGEM;
        a.forcaTinta = static_cast<std::uint32_t>(40.0f + 30.0f * a.aura);
    }
    if (j.pactoUsado) {
        a.contorno = true;
        a.corContorno = COR_CONTORNO_PACTO;
    }
    return a;
}
