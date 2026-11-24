#pragma once
// Piloto automático: o COMPUTADOR joga (PDF parte 2, critério técnico de 3,0; SDD §24.7).
//
// Regras de justiça (o piloto joga como uma pessoa, não trapaceia):
//   * só envia comandos (EstadoEntrada): os mesmos do teclado e do joystick;
//   * só usa o que o jogador vê: inimigos dentro da luz, avisos no chão, HUD e o mapa já
//     explorado (célula nunca vista é tratada como "provavelmente livre", e o plano se refaz
//     quando a lanterna revela uma parede);
//   * reage a um aviso só depois de um atraso humano (0,12 s).
//
// Decisão em camadas (a primeira que quer agir vence):
//   pendência → desviar → cuidar (kit, power-ups) → combater → missão → coletar → sair → explorar.
// No Clube: vai até o alçapão, interage e escolhe `dificuldadeDesejada` (Etapa 7.2).
#include <string>
#include <vector>

#include "core/entrada.h"
#include "jogo/partida.h"

enum class ObjetivoPiloto : std::uint8_t { Parado, Desviar, Combater, Explorar, Sair, Pendencia, Missao, Coletar, Clube };

constexpr float ATRASO_REACAO = 0.12f;   // s até "perceber" um aviso novo

struct MemoriaPiloto {
    // Tempo até "perceber" um aviso novo. O padrão (0,12 s) é o do piloto que joga e vence; a
    // simulação aceita `--reacao S` para medir um jogador mais lento (proxy humano, Etapa 12.1).
    float atrasoReacao = ATRASO_REACAO;
    // Proxy de PESSOA (Etapa 12.5): % dos avisos que passam despercebidos (sorteio fixo por golpe).
    // O piloto normal desvia de todo aviso; uma pessoa, não — e é aí que os inimigos anunciados ferem.
    int distracaoPct = 0;
    ObjetivoPiloto objetivo = ObjetivoPiloto::Parado;
    std::string descricao;                // para o overlay de depuração (F1) e os testes
    Celula destino{-1, -1};
    std::vector<Celula> caminho;          // plano atual (para desenhar)
    int quadro = 0;
    // Detector de travamento: quer andar, mas não sai do lugar.
    Vec2 posicaoReferencia;
    float tempoSemProgresso = 0.0f;
    int travamentos = 0;                  // contagem total (telemetria / testes)
    Celula ronda{-1, -1};                 // canto escuro para onde está rondando
    float tempoRonda = 0.0f;
    float cicloEspera = 0.0f;             // últimos inimigos caçam Bill: alterna esperar (8 s) e rondar (8 s)
    Celula evitarExplorar{-1, -1};        // alvo de exploração que se mostrou inalcançável
    float tempoEvitar = 0.0f;
    const void* salaAtual = nullptr;      // detecta troca de sala para zerar o plano
    // Alvo em combate: persistente (sem isso, atrás de um pilar ele alternava entre dois
    // inimigos a cada quadro e ficava parado para sempre). Se o alvo some de vista, vai até
    // o ÚLTIMO lugar onde o viu, por pouco tempo, como uma pessoa faria.
    IdEntidade alvo = ID_NULO;
    Vec2 ultimaPosicaoAlvo;
    float tempoSemVerAlvo = 0.0f;
    bool aproximando = false;             // em combate, mas fora do alcance (conta para o travamento)
    // v2: usar um item PELO INVENTÁRIO (abre, troca de aba, move o cursor, confirma, fecha).
    TipoItem itemMenu = TipoItem::Total;  // Total = não está usando item pelo menu
    bool confirmouMenu = false;
    int quadrosMenu = 0;                  // segurança: menu que não abriu → desiste
    float esperaKit = 0.0f;               // não aperta Q de novo antes de ver o efeito
    float tempoOlhando = 0.0f;            // sala concluída: explora um pouco antes de escolher a porta
    Dificuldade dificuldadeDesejada = Dificuldade::Normal;   // o que escolher no alçapão do Clube
    bool lojaFeita = false;               // já comprou o que queria nesta sala de loja
};

// Decide os comandos deste passo. Não altera a partida.
EstadoEntrada pilotar(const EstadoPartida& p, MemoriaPiloto& m, float dt);

const char* nomeObjetivo(ObjetivoPiloto o);
// Em que atributo o piloto põe o próximo ponto: Força e Agilidade até 4, depois Coragem e Foco.
int atributoPreferidoDoPiloto(const Jogador& j);
// Qual oferta da loja o piloto compra agora (−1 = nada que valha/caiba/dê para pagar).
int ofertaPreferidaDoPiloto(const EstadoPartida& p);
// Qual das ofertas de Eco o piloto escolhe: maior raridade; empate pela utilidade do efeito.
int ecoPreferidoDoPiloto(const std::vector<OfertaEco>& ofertas);
// Nota de uma porta para o piloto (maior = prefere). Depende da vida e do inventário.
int notaDaPorta(const EstadoPartida& p, const PortaSala& porta);
