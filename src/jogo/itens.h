#pragma once
// Itens, inventário e power-ups (PRD §11.1, SDD §3.5 e §15; critérios F10, M05, M09).
//
// Uma TABELA descreve cada item (nome, efeito, limite, ícone); as regras só consultam a tabela.
// Este módulo não conhece a partida: opera sobre o Jogador e o Inventario. A partida
// (jogo/partida) cuida do chão, dos baús, dos drops, da pontuação (I) e dos sons.
//
// Onde mora cada quantidade (sem duplicar): Bateria = Jogador::cargasLanterna; Pacto =
// Jogador::pactos; todo o resto = Inventario::quantidades. `quantidadeDe` esconde isso.
#include <array>
#include <cstdint>
#include <string>

#include "core/rng.h"
#include "core/tipos.h"
#include "entidades/jogador.h"

enum class TipoItem : std::uint8_t {
    Kit, Bateria, FichaCoragem, Tonico, Oculos, Barquinho, Escudo, MelhoriaArma,
    ChaveEnferrujada, ChaveDoMedo, Manivela, Moeda, Pacto, Lembrancinha,
    Total
};
constexpr int TOTAL_ITENS = static_cast<int>(TipoItem::Total);

enum class CategoriaItem : std::uint8_t { Consumivel, PowerUp, Equipamento, Chave, Missao, Moeda, Segredo };

struct DefinicaoItem {
    const char* nome;
    const char* descricao;      // o que faz (inventário, "Itens e efeitos" do menu)
    CategoriaItem categoria;
    int limite;                 // quanto se pode carregar; 0 = sem limite
    const char* atlas;          // ícone: atlas do manifesto + índice do quadro
    int quadro;
    int preco;                  // Loja do Elias (0 = não vende)
};

const DefinicaoItem& definicaoDe(TipoItem tipo);
// Itens de missão nunca são recusados por limite, roubados nem destruídos (PRD §5.3).
bool ehItemDeMissao(TipoItem tipo);

struct Inventario {
    std::array<int, TOTAL_ITENS> quantidades{};
};

constexpr float DURACAO_CORAGEM = 30.0f;   // +25% de dano
constexpr float DURACAO_TONICO = 20.0f;    // +30% de velocidade
constexpr float DURACAO_OCULOS = 30.0f;    // revela inimigos e paredes rachadas
constexpr int CURA_KIT_PCT = 35;           // % do HP máximo
constexpr int CURA_BARQUINHO = 10;         // HP
constexpr int MAXIMO_PACTOS = 3;

int quantidadeDe(const Jogador& j, const Inventario& inv, TipoItem tipo);
// Quantos ainda cabem (um número grande se não houver limite).
int espacoPara(const Jogador& j, const Inventario& inv, TipoItem tipo);

// Guarda até `quantidade` unidades (o que não cabe fica de fora). Devolve quantas entraram.
// Escudo e melhoria valem na hora (recalcula os derivados do Bill).
int coletarNoInventario(Jogador& j, Inventario& inv, TipoItem tipo, int quantidade);

enum class ResultadoUso : std::uint8_t { Usou, NaoTem, NaoPrecisa, NaoUsavel };
// Usa 1 unidade. Kit e barquinho com HP cheio → NaoPrecisa, SEM gastar (PRD §11.1).
// Power-up já ativo renova a duração.
ResultadoUso usarItem(Jogador& j, Inventario& inv, TipoItem tipo);

// Power-ups: o tempo passa; ao acabar, os derivados voltam ao normal.
void atualizarPowerUps(Jogador& j, float dt);

// ---------- sorteios (drops de inimigo e conteúdo de baú) ----------
struct ItemSorteado {
    TipoItem tipo = TipoItem::Total;   // Total = nada
    int quantidade = 0;
};
// Item que um inimigo comum solta (já decidido que solta). `pesoKitPct` = dificuldade.
ItemSorteado sortearDrop(Rng& rng, int pesoKitPct);
// Um item de baú (mais generoso que o drop: inclui escudo, melhoria e power-ups).
ItemSorteado sortearItemDeBau(Rng& rng, int pesoKitPct);
