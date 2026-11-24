#pragma once
// Recordes (SDD §16 e §24.3; critério M04: salvar em arquivo e exibir por melhor pontuação).
//
// Arquivo saves/recordes.txt, uma partida por linha:
//   pontos;data;dificuldade;tempo_s;bioma;vitoria;seed;piloto
// Ordem: pontos (maior primeiro), depois tempo (menor primeiro). Guardamos as 10 melhores do
// JOGADOR e as 10 melhores do COMPUTADOR, para que o filtro "Jogador" nunca fique vazio só
// porque o piloto jogou muitas vezes.
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "jogo/dificuldade.h"

constexpr int RECORDES_POR_CATEGORIA = 10;

struct Recorde {
    long long pontos = 0;
    std::string data;                 // "2026-11-16 21:30" (quem grava fornece; as regras não leem o relógio)
    Dificuldade dificuldade = Dificuldade::Normal;
    int tempo = 0;                    // segundos de jogo
    int bioma = 0;                    // onde a tentativa terminou
    bool vitoria = false;
    std::uint64_t seed = 0;
    bool piloto = false;              // jogada pelo computador
};

enum class FiltroRecordes : std::uint8_t { Todas, Jogador, Computador };

// true se `a` vem antes de `b` no ranking.
bool vemAntes(const Recorde& a, const Recorde& b);

// Linhas inválidas são puladas (contadas em `ignoradas`): um arquivo editado à mão não derruba o jogo.
std::vector<Recorde> lerRecordes(const std::string& texto, int* ignoradas = nullptr);
std::string escreverRecordes(const std::vector<Recorde>& lista);

// Insere, ordena e corta (10 por categoria). Devolve a colocação 1..10 da partida no ranking
// da categoria dela (jogador ou computador), ou 0 se não entrou.
int inserirRecorde(std::vector<Recorde>& lista, const Recorde& novo);

// Recorde da TENTATIVA (Diego, 24/11: "gravar ao derrotar cada chefão"). A mesma tentativa ocupa UMA linha,
// regravada a cada chefão derrotado e no fim (morte ou vitória): fechar o jogo depois de um chefe não perde os
// pontos. A tentativa é reconhecida por seed + dificuldade + quem jogou (a data tem hora e muda no caminho).
// Fica a MAIOR pontuação que a tentativa alcançou; a vitória sempre substitui. Devolve a colocação, como inserirRecorde.
int registrarRecordeDaTentativa(std::vector<Recorde>& lista, const Recorde& novo);

// Até 10 recordes do filtro, já em ordem.
std::vector<Recorde> recordesFiltrados(const std::vector<Recorde>& lista, FiltroRecordes filtro);

// Arquivo (gravação atômica). Arquivo ausente = lista vazia (primeira execução).
std::vector<Recorde> carregarRecordes(const std::filesystem::path& caminho);
bool gravarRecordes(const std::filesystem::path& caminho, const std::vector<Recorde>& lista);
