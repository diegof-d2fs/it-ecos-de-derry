#include "combate/experiencia.h"

int nivelParaXp(int xpTotal) {
    int nivel = 1;
    while (nivel < NIVEL_MAXIMO && xpTotal >= LIMIARES_XP[nivel + 1]) ++nivel;
    return nivel;
}

int concederXp(Progresso& p, int quantidade) {
    if (quantidade <= 0) return 0;
    p.xp += quantidade;
    const int novo = nivelParaXp(p.xp);
    const int ganhos = novo - p.nivel;
    if (ganhos > 0) {
        p.nivel = novo;
        p.pontosPendentes += PONTOS_POR_NIVEL * ganhos;
    }
    return ganhos > 0 ? ganhos : 0;
}

bool aplicarPonto(Progresso& p, int& atributo) {
    if (p.pontosPendentes <= 0 || atributo >= ATRIBUTO_MAXIMO) return false;
    --p.pontosPendentes;
    ++atributo;
    return true;
}
