#pragma once
// Tradução do joystick para comandos (SDD §6, PRD v2 §6.2 e §17). Parte PURA das regras:
// recebe uma leitura já normalizada (sem saber se veio da SDL2 ou do XInput) e escreve em
// EstadoEntrada. A plataforma (plataforma/gamepad) só preenche LeituraControle.
#include <array>
#include <cstdint>

#include "core/entrada.h"
#include "core/tipos.h"

// Botões por POSIÇÃO física (Sul = A do Xbox = B do 8BitDo/Nintendo). A ordem é a mesma de
// SDL_GameControllerButton, para a plataforma copiar índice a índice.
enum class BotaoControle : std::uint8_t {
    Sul, Leste, Oeste, Norte,
    Selecionar, Guia, Iniciar,          // Back/View, Home/Xbox, Start/Menu
    AnalogicoEsq, AnalogicoDir,         // L3, R3
    OmbroEsq, OmbroDir,                 // LB, RB
    Cima, Baixo, Esquerda, Direita,     // direcional digital
    Total
};
constexpr int TOTAL_BOTOES = static_cast<int>(BotaoControle::Total);

struct LeituraControle {
    std::array<bool, TOTAL_BOTOES> botoes{};
    Vec2 analogicoEsq;        // -1..1, SEM zona morta (y positivo = para baixo)
    Vec2 analogicoDir;
    float gatilhoEsq = 0.0f;  // 0..1
    float gatilhoDir = 0.0f;
};

enum class OrigemControle : std::uint8_t { Nenhuma, Sdl, XInput };

// O que a plataforma sabe do controle neste quadro (para a tela de teste e os avisos).
struct InfoControle {
    OrigemControle origem = OrigemControle::Nenhuma;   // Nenhuma = sem controle conectado
    bool sdlCarregada = false;     // SDL2.dll ao lado do exe e iniciada
    bool xinputCarregado = false;
    char nome[64] = "";            // UTF-8
    float custoBuscaMs = 0.0f;     // duração da última busca por controle novo (lição nº 2)
    LeituraControle leitura;       // a última leitura bruta (zerada sem foco)
};

constexpr float ZONA_MORTA_ANALOGICO = 0.25f;   // radial, PRD v2 §17
constexpr float LIMIAR_GATILHO = 0.5f;          // gatilho "apertado"
constexpr float LIMIAR_NAVEGACAO = 0.5f;        // analógico vale como direcional nos menus

inline bool botao(const LeituraControle& l, BotaoControle b) { return l.botoes[static_cast<int>(b)]; }

// Eixo bruto de 16 bits com sinal (SDL e XInput) → -1..1, simétrico (-32768 também dá -1).
float normalizarEixo(int bruto);
// Gatilho bruto 0..maximo (SDL: 32767, XInput: 255) → 0..1.
float normalizarGatilho(int bruto, int maximo);
// Zona morta RADIAL com reescala: abaixo de `zona` dá zero; de `zona` a 1 vai de 0 a 1,
// mantendo a direção (sem o "travamento" nos eixos que a zona morta quadrada causa).
Vec2 aplicarZonaMorta(Vec2 v, float zona);

// Marca em `entrada.segurando` os comandos dos botões (soma ao teclado, não zera antes).
// Devolve o analógico esquerdo já com zona morta. Se houve qualquer atividade, define
// `ultimoUsado = Gamepad` (os ícones do tutorial seguem o último dispositivo).
Vec2 aplicarControle(const LeituraControle& leitura, EstadoEntrada& entrada);

// Fecha o eixo de movimento do quadro: o direcional digital (teclado ou cruz do controle)
// tem prioridade; sem ele, vale o analógico. Depois, o analógico além de LIMIAR_NAVEGACAO
// também marca os comandos direcionais, para navegar nos menus.
void combinarEixo(EstadoEntrada& entrada, Vec2 analogico);
