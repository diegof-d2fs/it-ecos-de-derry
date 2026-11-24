#pragma once
// Telemetria local (PRD §18): quanto tempo cada sala leva numa tentativa JOGADA (pessoa ou
// computador), para validar o ritmo de ~7,5 min por bioma com gente de verdade. A simulação
// (jogo/simulacao) mede o piloto; esta mede a janela, inclusive o TEMPO REAL: diálogos, loja e
// menus congelam o cronômetro do jogo, mas fazem parte da duração que a pessoa sente.
//
// Puro: a cena chama `passoTelemetria` a cada passo e grava o texto de `csvTelemetria` em
// saves/telemetria.csv (uma linha por sala; o arquivo acumula as tentativas).
#include <cstdint>
#include <string>
#include <vector>

#include "jogo/partida.h"

struct SalaTelemetria {
    int bioma = 0, etapa = 0;
    std::string sala;          // id do arquivo
    float segundosJogo = 0.0f; // cronômetro (para nos menus)
    float segundosReais = 0.0f;// relógio de parede (conta diálogos, loja e menus)
    int danoRecebido = 0;
};

struct Telemetria {
    bool ativa = false;
    std::vector<SalaTelemetria> salas;   // salas já fechadas
    SalaTelemetria atual;
    float inicioJogo = 0.0f;             // cronômetro ao entrar na sala atual
    int hpAntes = 0;
};

// Começa a medir a partida `p` (se for uma tentativa); chamada ao entrar na 1ª sala.
void iniciarTelemetria(Telemetria& t, const EstadoPartida& p);
// Depois de cada passo de lógica: soma o tempo real (`dtReal`, 0 nos passos extras do ×2/×4),
// o dano e, se Bill trocou de sala, fecha a anterior e abre a nova.
void passoTelemetria(Telemetria& t, const EstadoPartida& p, float dtReal);
// Fecha a sala atual (fim da tentativa). Depois disso, `t.salas` tem a tentativa inteira.
void encerrarTelemetria(Telemetria& t, const EstadoPartida& p);

constexpr const char* CABECALHO_TELEMETRIA =
    "data;seed;quem;dificuldade;fim;bioma;etapa;sala;segundos_jogo;segundos_reais;dano_recebido\n";
// Linhas da tentativa (sem cabeçalho). `fim`: morreu, completou, abandonou…
std::string csvTelemetria(const Telemetria& t, const std::string& data, std::uint64_t seed, bool piloto, Dificuldade d,
                          const char* fim);
// O que vai para o arquivo: o conteúdo atual + as linhas novas (cabeçalho se o arquivo é novo).
std::string anexarTelemetria(const std::string& conteudoAtual, const std::string& linhas);
