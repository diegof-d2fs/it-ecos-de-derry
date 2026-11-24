#pragma once
// Atributos e valores derivados (PRD v2 §7.1, critério F17). As fórmulas ficam TODAS aqui,
// numa função pura: o HUD, a Ficha, o combate e os testes usam o mesmo cálculo.
#include <cstdint>

constexpr int ATRIBUTO_MAXIMO = 9;

struct Atributos {
    int forca = 1, foco = 1, agilidade = 1, coragem = 1;
};

// Bônus de equipamento que entram nos derivados (armas, escudo, melhorias).
struct Equipamento {
    int bonusDanoArma = 2;     // taco: +2 (PRD §8.4)
    int melhorias = 0;         // "pregos no taco": +3 cada
    int armaduraEscudo = 0;    // escudo: +2
    int bonusDanoPct = 0;      // Ficha de coragem ativa: +25
    int bonusVelocidadePct = 0;   // Tônico do Eddie ativo: +30
    int bonusHp = 0;              // talento Coragem herdada (Mural): +5 por nível
    int ecoArmadura = 0;          // Ecos passivos (jogo/ecos): Tijolo a tijolo
    int ecoHp = 0;                // Coração pesado
    int ecoVelocidadePct = 0;     // Pressa (Eddie)
    int ecoKitPct = 0;            // Remédio (Eddie): o kit cura este % a mais
};

struct StatsDerivados {
    int hpMax = 0, armadura = 0, danoBase = 0, danoClarao = 0;
    float acertoBase = 0.0f;   // em % (sem a agilidade do alvo)
    float critico = 0.0f;      // em %
    float esquiva = 0.0f;      // em %
    float velocidade = 0.0f;   // px/s
    float recargaDash = 0.0f;  // s
};

StatsDerivados calcularStats(const Atributos& a, const Equipamento& e);

// Acerto contra um alvo: limitar(80 + 3×FOC − 2×AGI_alvo, 60, 98) %.
float chanceAcerto(int focoAtacante, int agilidadeAlvo);
// Dano efetivo: máx(1, dano − armadura).
int danoEfetivo(int dano, int armaduraAlvo);
// Dano crítico antes da armadura: ⌊dano × 1,5⌋.
int danoCritico(int dano);
// Esquiva de inimigos: mesma regra do Bill, mín(30, 2 + 3×AGI) %.
float chanceEsquiva(int agilidade);
