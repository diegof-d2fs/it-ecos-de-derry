#pragma once
// Entrada abstrata (SDD §6). As regras e as cenas só enxergam COMANDOS, nunca teclas ou
// botões: a plataforma traduz teclado e joystick para esta estrutura a cada quadro.
#include <array>
#include <cstdint>

#include "core/tipos.h"

enum class Comando : std::uint8_t {
    Cima, Baixo, Esquerda, Direita,
    Atacar, Especial, Clarao, Dash, Interagir, UsarKit,
    Inventario, Ficha, Mapa, Pausa, Confirmar, Voltar, AbaEsquerda, AbaDireita,
    TelaCheia, Debug, TesteControle, Piloto,
    Total
};
constexpr int TOTAL_COMANDOS = static_cast<int>(Comando::Total);

enum class Dispositivo : std::uint8_t { Teclado, Gamepad };

struct EstadoEntrada {
    std::array<bool, TOTAL_COMANDOS> segurando{};
    std::array<bool, TOTAL_COMANDOS> pressionou{};   // borda de descida neste quadro lógico
    std::array<bool, TOTAL_COMANDOS> soltou{};       // borda de subida neste quadro lógico
    Vec2 eixo;                                       // movimento -1..1, já normalizado
    Dispositivo ultimoUsado = Dispositivo::Teclado;
    bool gamepadConectado = false;
};

inline bool segurando(const EstadoEntrada& e, Comando c) { return e.segurando[static_cast<int>(c)]; }
inline bool pressionou(const EstadoEntrada& e, Comando c) { return e.pressionou[static_cast<int>(c)]; }

// Atualiza bordas a partir do estado "segurando" atual e do estado anterior.
void calcularBordas(EstadoEntrada& atual, const std::array<bool, TOTAL_COMANDOS>& anterior);
// Depois de um passo lógico, as bordas são consumidas (não repetem no próximo passo).
void limparBordas(EstadoEntrada& e);
// Solta tudo (perda de foco, abertura de menu): evita teclas "presas".
void limparEntrada(EstadoEntrada& e);
// Eixo de movimento a partir dos quatro comandos direcionais (diagonal normalizada).
Vec2 eixoDosDirecionais(const EstadoEntrada& e);
