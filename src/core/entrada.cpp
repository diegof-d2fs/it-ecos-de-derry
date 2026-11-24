#include "core/entrada.h"

void calcularBordas(EstadoEntrada& atual, const std::array<bool, TOTAL_COMANDOS>& anterior) {
    for (int i = 0; i < TOTAL_COMANDOS; ++i) {
        // |= : uma borda ainda não consumida (vários quadros de render sem passo lógico) não se perde.
        atual.pressionou[i] = atual.pressionou[i] || (atual.segurando[i] && !anterior[i]);
        atual.soltou[i] = atual.soltou[i] || (!atual.segurando[i] && anterior[i]);
    }
}

void limparBordas(EstadoEntrada& e) {
    e.pressionou.fill(false);
    e.soltou.fill(false);
}

void limparEntrada(EstadoEntrada& e) {
    e.segurando.fill(false);
    limparBordas(e);
    e.eixo = {0.0f, 0.0f};
}

Vec2 eixoDosDirecionais(const EstadoEntrada& e) {
    Vec2 v;
    if (segurando(e, Comando::Esquerda)) v.x -= 1.0f;
    if (segurando(e, Comando::Direita)) v.x += 1.0f;
    if (segurando(e, Comando::Cima)) v.y -= 1.0f;
    if (segurando(e, Comando::Baixo)) v.y += 1.0f;
    return normalizar(v);
}
