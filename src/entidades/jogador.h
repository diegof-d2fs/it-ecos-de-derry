#pragma once
// Bill: movimento em 8 direções, dash com invulnerabilidade, combo do taco, especial, clarão,
// vida e progresso (PRD v2 §6–§8, SDD §10–§11). Regras puras: recebe comandos, nunca teclas.
#include <cstdint>
#include <vector>

#include "combate/atributos.h"
#include "combate/experiencia.h"
#include "combate/golpe.h"
#include "core/entrada.h"
#include "core/sons.h"
#include "core/tipos.h"
#include "mundo/sala.h"

constexpr IdEntidade ID_JOGADOR = 1;
constexpr Vec2 CAIXA_BILL{18.0f, 10.0f};   // colisão dos pés (SDD §10)
constexpr float DASH_DISTANCIA = 96.0f;     // px
constexpr float DASH_DURACAO = 0.15f;       // s → 640 px/s
constexpr float INVULNERAVEL_APOS_DANO = 0.8f;
constexpr float JANELA_COMBO = 0.35f;
constexpr float RECARGA_ESPECIAL = 3.0f;
constexpr int CARGAS_LANTERNA_INICIAIS = 3;
constexpr int CARGAS_LANTERNA_MAXIMO = 5;   // limite da Bateria (PRD §11.1)
constexpr int BONUS_DANO_CORAGEM = 25;      // % (Ficha de coragem)
constexpr int BONUS_VELOCIDADE_TONICO = 30; // % (Tônico do Eddie)

enum class AcaoJogador : std::uint8_t { Livre, Atacando, Especial, Clarao };

constexpr float BUFFER_ATAQUE = 0.3f;

// Movimento fluido (Diego, 24/11: "deixe a movimentação do Bill mais natural e fluida, melhore a diagonal").
// Arranque e curva são imediatos (como no Hades: desviar é arrancar ou virar); só a PARADA desliza de leve.
// Medido (24/11): inércia também no arranque, mesmo de 0,03 s, fazia o piloto desviar tarde nas arenas
// (b6_chefe 9/10, Covil 8/10); sem ela, 10/10 em todas as salas.
constexpr float TEMPO_FREAR = 0.035f;       // s (constante de tempo exponencial da parada)
// Diagonal: o sprite só troca de eixo quando o novo domina por esta razão (sem piscar entre cima e lado).
constexpr float HISTERESE_DIRECAO = 1.35f;
// Quina: andando reto contra a ponta de uma parede, Bill escorrega de lado até este tanto para passar.
constexpr float FOLGA_QUINA = 7.0f;        // px

struct Jogador {
    Vec2 pes;                         // pivô: centro da base do sprite
    Direcao direcao = Direcao::Baixo;
    bool andando = false;
    float tempoAnimacao = 0.0f;
    Vec2 velocidade;                  // px/s de agora (suavizada; o dash a escreve direto)
    // Atributos, equipamento e valores derivados (recalcularJogador mantém em dia).
    Atributos attr;
    Equipamento equip;
    StatsDerivados stats = calcularStats(Atributos{}, Equipamento{});
    int hp = stats.hpMax;
    Progresso progresso;
    int pactos = 1;                   // Pactos de Sangue (vidas extras da tentativa)
    bool pactoUsado = false;          // já levantou com um pacto (contorno vermelho até o fim)
    float preso = 0.0f;               // > 0: preso pela faixa da Múmia (não anda nem dá dash; ataca)
    float lento = 0.0f;               // > 0: agarrado pelo Afogado (anda a 50%; o dash continua)
    bool naPoca = false;              // pisando numa poça do Afogado (a partida marca a cada passo)
    int cargasLanterna = CARGAS_LANTERNA_INICIAIS;
    int cargasMaximo = CARGAS_LANTERNA_MAXIMO;   // limite da Bateria (talento Lanterna reforçada sobe)
    // Power-ups ativos (segundos restantes; 0 = desligado) e o brilho verde da cura.
    float coragem = 0.0f, tonico = 0.0f, oculos = 0.0f;
    float cura = 0.0f;
    // Ações
    AcaoJogador acao = AcaoJogador::Livre;
    float timerAcao = 0.0f;
    int combo = 0;                    // último golpe do combo (1..3); 0 = começar do 1
    bool comboPedido = false;         // apertou Atacar durante um golpe: encadeia ao terminar
    // Buffer de entrada (Diego, 17/11: "tenho que apertar duas vezes"): o toque de Atacar fica guardado
    // por BUFFER_ATAQUE e sai assim que Bill puder golpear. Antes, um toque no hit-stop (o congelamento
    // de cada acerto: perto de inimigo, quase sempre) ou no dash era jogado fora.
    float bufferAtaque = 0.0f;
    float bufferEspecial = 0.0f;      // o mesmo buffer para o Especial e o Clarão (17/11: "dando ruim seguidamente")
    float bufferClarao = 0.0f;
    float janelaCombo = 0.0f;
    float recargaEspecial = 0.0f;
    Vec2 mira{0.0f, 1.0f};            // direção do golpe (eixo ou o último rumo)
    // Último rumo em que Bill andou, em QUALQUER ângulo (Diego, 17/11: "por que não ataca na diagonal?").
    // Parado, a mira era a direção do sprite (só 4); agora é este rumo (8 direções no teclado, analógico no controle).
    Vec2 ultimoRumo{0.0f, 1.0f};
    // Dash
    float dashRestante = 0.0f;
    Vec2 dashDirecao;
    float recargaDash = 0.0f;
    int dashExtras = 0;               // cargas a mais (Eco "Asas" do Stan): dash de novo durante a recarga
    int dashExtrasDisponiveis = 0;    // voltam todas quando a recarga termina
    // Dano recebido
    float invulneravel = 0.0f;        // após um dano (0,8 s, sprite translúcido, sem piscar)
    float flash = 0.0f;               // 1 quadro branco ao ser atingido
    // Contagem de M (pontuação)
    Celula celula{-1, -1};
    int movimentos = 0;
};

float velocidadeMovimento(int agilidade);   // 120 + 8 × AGI px/s
float recargaDoDash(int agilidade);         // 0,70 − 0,03 × AGI s
bool estaInvulneravel(const Jogador& j);    // dash ou logo após um dano
bool estaVivo(const Jogador& j);

// Recalcula os derivados depois de mudar atributo, equipamento ou power-up (HP acompanha o novo máximo).
void recalcularJogador(Jogador& j);
void posicionarJogador(Jogador& j, Vec2 pes);

// Ações de combate de um passo: combo, especial e clarão viram Golpes; sons viram eventos.
void acoesDoJogador(Jogador& j, const EstadoEntrada& entrada, float dt, std::vector<Golpe>& golpes,
                    std::uint32_t& proximoGolpe, std::vector<IdSom>& sons);
// Movimento de um passo (andar, dash, colisão, contagem de M). Atacando, Bill anda mais devagar.
constexpr float FATOR_AGARRADO = 0.5f;   // PRD §9: o Afogado deixa Bill 50% mais lento
constexpr float FATOR_POCA = 0.7f;       // a poça vale o mesmo que água rasa (−30%)
void atualizarJogador(Jogador& j, const SalaViva& sala, const EstadoEntrada& entrada, float dt);

Vec2 vetorDaDirecao(Direcao d);
// Direção do sprite com histerese: na diagonal mantém o eixo atual; só troca quando o outro domina.
Direcao direcaoSuave(Vec2 v, Direcao atual);
Retangulo hurtboxJogador(const Jogador& j);
Vec2 corpoJogador(const Jogador& j);        // centro do corpo (origem dos golpes)
// Golpe de Bill com acerto, crítico e foco dele (o taco, o especial, o tiro do Eco Estilingue).
Golpe golpeDoBill(const Jogador& j, std::uint32_t& proximoGolpe, const Area& area, int dano);
