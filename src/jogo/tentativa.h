#pragma once
// Gerador da tentativa (SDD §9, PRD §5.1). Para cada bioma, sorteia QUAIS salas desenhadas à
// mão aparecem, em que ordem, e o que cada porta promete (recompensa ou tipo de sala).
//
//   Entrada → Combate ×2 portas → [Combate | Tesouro ou altar de Eco] → Combate (Chave do Medo)
//           → [Loja | Refúgio] → Resgate → Chefe → descida para o próximo bioma
//
// Função PURA: recebe as salas e um Rng; não conhece a partida. Mesma seed = mesma tentativa.
// A escolha de porta é FÍSICA (como em Hades): o jogador anda até a porta cujo ícone quer.
#include <cstdint>
#include <string>
#include <vector>

#include "core/rng.h"
#include "jogo/itens.h"
#include "mundo/sala.h"

// O que a sala promete ao ser concluída (ícone na porta que leva a ela).
enum class Recompensa : std::uint8_t {
    Nenhuma, Moedas, Kit, Bateria, Melhoria, ChaveEnferrujada, PontoAtributo, ChaveDoMedo,
    Eco,   // Etapa 8.1: ao concluir a sala, escolher 1 de 3 Ecos do amigo do bioma
    Total
};

struct DefinicaoRecompensa {
    const char* nome;
    int peso;              // no sorteio das portas de combate (0 = só fixa)
    TipoItem item;         // o que cai no chão ao concluir (Total = efeito especial)
    int qMin, qMax;
};
const DefinicaoRecompensa& definicaoRecompensa(Recompensa r);

// Uma porta: leva à sala `sala` (índice em Conteudo::salas), do tipo `tipo`, com `recompensa`.
struct OpcaoPorta {
    int sala = -1;
    TipoSala tipo = TipoSala::Combate;
    Recompensa recompensa = Recompensa::Nenhuma;
};

struct EtapaTentativa {
    std::vector<OpcaoPorta> opcoes;   // 1 ou 2 (bifurcação)
};

struct PlanoBioma {
    int bioma = 1;
    std::vector<EtapaTentativa> etapas;   // etapas[0] = sala de entrada (1 opção)
    std::vector<std::string> avisos;      // tipo de sala que faltou e foi trocado por combate
};

constexpr int SALAS_DE_COMBATE_POR_BIOMA = 3;   // C1, C2 e C3 (a da chave)

// Nome do bioma para o HUD e os cartões (0 = prólogo … 7 = Covil).
const char* nomeBioma(int bioma);
constexpr int BIOMA_COVIL = 7;   // o último: antecâmara + arena do Pennywise IV (Etapa 15)

// Um bioma é jogável se tem sala de entrada e ao menos uma de combate.
bool biomaJogavel(const std::vector<SalaModelo>& salas, int bioma);

// Monta o plano do bioma. Falha (false + erro) se o bioma não for jogável.
// `pesoKitPct` = dificuldade (Kit/Bateria aparecem mais no Fácil).
bool gerarBioma(const std::vector<SalaModelo>& salas, int bioma, Rng& rng, int pesoKitPct, PlanoBioma& plano,
                std::string& erro);

// Fisher–Yates com o Rng do jogo (o sorteio sem repetição usado pelo gerador).
void embaralhar(std::vector<int>& v, Rng& rng);
