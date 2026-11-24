#include "core/escala.h"

Apresentacao calcularApresentacao(int clienteW, int clienteH, int logicaW, int logicaH, int escalaMaxima) {
    Apresentacao a;
    if (logicaW <= 0 || logicaH <= 0 || clienteW <= 0 || clienteH <= 0) {
        return a;
    }
    const int porLargura = clienteW / logicaW;
    const int porAltura = clienteH / logicaH;
    a.escala = porLargura < porAltura ? porLargura : porAltura;
    if (escalaMaxima > 0 && a.escala > escalaMaxima) {
        a.escala = escalaMaxima;
    }
    if (a.escala <= 0) {
        a.escala = 0;
        return a;
    }
    a.largura = logicaW * a.escala;
    a.altura = logicaH * a.escala;
    a.x = (clienteW - a.largura) / 2;
    a.y = (clienteH - a.altura) / 2;
    return a;
}
