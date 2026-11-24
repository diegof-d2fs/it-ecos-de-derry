#pragma once
// Tradução do teclado para comandos abstratos (PRD v2 §6.2).
#include <array>

#include "core/entrada.h"

struct MapeamentoTecla {
    int teclaVirtual;   // código VK_* do Windows
    Comando comando;
};

// Marca em `entrada.segurando` todos os comandos cujas teclas estão pressionadas.
// Não zera antes: o joystick pode somar comandos no mesmo quadro (Etapa 2).
void aplicarTeclado(const std::array<bool, 256>& teclas, EstadoEntrada& entrada);
