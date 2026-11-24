#pragma once
// Simulação sem janela (SDD §24.7.2): o piloto joga N tentativas inteiras, centenas de vezes
// mais rápido que o tempo real. Serve para provar o critério "o computador joga e vence" e
// para BALANCEAR (duração por sala, onde e de quê se morre) sem ninguém jogar.
// Usada pelos testes e por `ecos_de_derry.exe --simular N --dificuldade D`.
#include <cstdint>
#include <string>
#include <vector>

#include "jogo/partida.h"
#include "jogo/piloto.h"

// Limite de tempo de JOGO por tentativa simulada. A tentativa completa mira 45–60 min (PRD §18);
// com 30 min, tentativas de 4 biomas no Difícil já estouravam o limite sem estar presas (12.2).
constexpr float LIMITE_SIMULACAO_S = 90.0f * 60.0f;

// Venceu = derrotou o Pennywise IV no Covil (Etapa 15); Completou = acabou o conteúdo sem Covil (salas de teste).
enum class FimSimulacao : std::uint8_t { Completou, Morreu, TempoEsgotado, Venceu };

struct SalaSimulada {
    int etapa = 0;
    std::string sala;          // id do arquivo
    float segundos = 0.0f;     // tempo de jogo dentro da sala
    int danoRecebido = 0;
    int moedasAoEntrar = 0;    // economia: com quanto Bill chega (a loja é o que importa)
    int hpAoEntrar = 0, hpMaximo = 0;
    int abates = 0;            // inimigos comuns derrotados nesta sala
};

struct ResultadoSimulacao {
    std::uint64_t seed = 0;
    Dificuldade dificuldade = Dificuldade::Normal;
    FimSimulacao fim = FimSimulacao::TempoEsgotado;
    int bioma = 0, etapa = 0;
    std::string sala;          // onde terminou
    float tempo = 0.0f;
    long long pontos = 0;
    int hpFinal = 0, kitsUsados = 0, abates = 0, vidasUsadas = 0, nivel = 1;
    int resgates = 0;          // amigos resgatados (Etapa 7.4)
    std::string companheiro;   // quem acompanhava Bill no fim
    bool manivela = false;     // entregou a manivela ao Elias (Etapa 8.2)
    int compras = 0;           // itens comprados na loja
    int causa = -1;            // TipoInimigo do último golpe que feriu Bill (−1 = nenhum)
    std::vector<SalaSimulada> salas;
};

// Uma tentativa inteira jogada pelo piloto, até o fim do conteúdo, a morte ou `limiteSegundos`.
// `reacao`: o atraso de reação do piloto (padrão: o do jogo); maior = proxy de um jogador mais lento.
ResultadoSimulacao simularTentativa(const Conteudo& c, std::uint64_t seed, Dificuldade d, float limiteSegundos,
                                    float reacao = ATRASO_REACAO, int distracao = 0);

// Arquivos de saída (separador ';', como os recordes).
std::string csvTentativas(const std::vector<ResultadoSimulacao>& r);   // uma linha por tentativa
std::string csvSalas(const std::vector<ResultadoSimulacao>& r);        // uma linha por sala visitada
std::string resumoSimulacao(const std::vector<ResultadoSimulacao>& r); // texto curto (taxa, tempos, mortes)
const char* nomeFim(FimSimulacao f);
