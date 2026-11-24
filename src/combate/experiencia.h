#pragma once
// Experiência e níveis (PRD v2 §7.2, critérios F15 e F16).
#include "combate/atributos.h"

constexpr int NIVEL_MAXIMO = 10;
// XP ACUMULADO necessário para cada nível (índice = nível). O XP nunca é zerado.
// Etapa 8.5 (telemetria): a curva antiga (N10 = 1380) fazia Bill chegar ao chefe 1 no nível 5,
// porque só o bioma 1 rende ~350 XP. Esta foi escalada para os 6 biomas: chefe 1 no nível 3
// (PRD §10.2: "vencível no nível 2–3 com um Eco") e N10 perto do fim do bioma 6.
constexpr int LIMIARES_XP[NIVEL_MAXIMO + 1] = {0, 0, 70, 180, 400, 700, 1050, 1450, 1950, 2550, 3300};
constexpr int PONTOS_POR_NIVEL = 2;
constexpr int CURA_POR_NIVEL = 10;

int nivelParaXp(int xpTotal);

struct Progresso {
    int xp = 0;
    int nivel = 1;
    int pontosPendentes = 0;
};

// Soma XP; devolve quantos níveis foram ganhos (pode atravessar vários limiares de uma vez).
int concederXp(Progresso& p, int quantidade);

// Gasta 1 ponto num atributo. Falha (false) sem pontos ou com o atributo já em 9.
bool aplicarPonto(Progresso& p, int& atributo);
