#pragma once
// Aparência do Bill (SDD §24.4; critério M08: "o uso de itens altera o visual do personagem").
// Função PURA: recebe o estado e devolve as camadas; o render só obedece. Assim a regra
// "escudo aparece no braço", "coragem acende a aura" é testável sem janela (t_aparencia).
#include <cstdint>

#include "entidades/jogador.h"
#include "jogo/itens.h"

struct AparenciaBill {
    bool escudo = false;           // tampa de lixeira no braço
    int melhorias = 0;             // pregos no taco (0..3): brilho metálico na arma
    float aura = 0.0f;             // 0..1: aura laranja pulsante (Ficha de coragem)
    bool rastro = false;           // rastro azul-claro nos passos (Tônico do Eddie)
    bool revelaInimigos = false;   // Óculos do Richie: inimigos fora da luz com contorno branco
    std::uint32_t tinta = 0;       // 0xRRGGBB aplicada ao sprite inteiro
    std::uint32_t forcaTinta = 0;  // 0..255 (0 = sem tinta)
    bool contorno = false;         // Pacto usado: contorno vermelho-escuro até o fim da tentativa
    std::uint32_t corContorno = 0;
};

constexpr std::uint32_t COR_TINTA_DANO = 0xFFFFFF;      // piscar branco de 1 quadro
constexpr std::uint32_t COR_TINTA_CURA = 0x5AE678;      // clarão verde do kit
constexpr std::uint32_t COR_TINTA_CORAGEM = 0xFF9030;   // tinta quente da coragem
constexpr std::uint32_t COR_CONTORNO_PACTO = 0x7A0F14;  // vermelho-escuro

// `tempo` = relógio para as pulsações (não altera nada).
AparenciaBill aparenciaDe(const Jogador& j, const Inventario& inv, float tempo);
