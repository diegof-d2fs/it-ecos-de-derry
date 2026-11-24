#include "combate/atributos.h"

#include <algorithm>

StatsDerivados calcularStats(const Atributos& a, const Equipamento& e) {
    StatsDerivados s;
    s.danoBase = (6 + 2 * a.forca + e.bonusDanoArma + 3 * e.melhorias) * (100 + e.bonusDanoPct) / 100;
    s.hpMax = 40 + 6 * a.coragem + e.bonusHp + e.ecoHp;
    s.armadura = a.coragem / 2 + e.armaduraEscudo + e.ecoArmadura;
    s.acertoBase = 80.0f + 3.0f * static_cast<float>(a.foco);
    s.critico = 2.0f * static_cast<float>(a.foco);
    s.danoClarao = 15 + 5 * a.foco;
    s.esquiva = chanceEsquiva(a.agilidade);
    s.velocidade = (120.0f + 8.0f * static_cast<float>(a.agilidade)) * static_cast<float>(100 + e.bonusVelocidadePct + e.ecoVelocidadePct) / 100.0f;
    s.recargaDash = 0.70f - 0.03f * static_cast<float>(a.agilidade);
    return s;
}

float chanceAcerto(int focoAtacante, int agilidadeAlvo) {
    const float v = 80.0f + 3.0f * static_cast<float>(focoAtacante) - 2.0f * static_cast<float>(agilidadeAlvo);
    return std::clamp(v, 60.0f, 98.0f);
}

int danoEfetivo(int dano, int armaduraAlvo) { return std::max(1, dano - armaduraAlvo); }

int danoCritico(int dano) { return dano * 3 / 2; }

float chanceEsquiva(int agilidade) { return std::min(30.0f, 2.0f + 3.0f * static_cast<float>(agilidade)); }
