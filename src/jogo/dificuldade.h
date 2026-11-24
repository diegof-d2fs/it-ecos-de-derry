#pragma once
// Dificuldade (SDD §24.2, critérios M01 e M06). A dificuldade NÃO muda as regras de combate
// nem a legibilidade (todo ataque continua anunciado): muda PROBABILIDADES e ORÇAMENTO.
//
// Todos os valores são inteiros em porcentagem (75 = ×0,75; 15 = 15%), para a conta ser exata
// e o arquivo ficar fácil de ler. Os valores vêm de data/balanceamento.txt: na defesa, mudar
// uma probabilidade = mudar um número numa linha, sem recompilar.
#include <array>
#include <cstdint>
#include <string>

#include "core/rng.h"

enum class Dificuldade : std::uint8_t { Facil, Normal, Dificil, Total };
constexpr int TOTAL_DIFICULDADES = static_cast<int>(Dificuldade::Total);

struct ParametrosDificuldade {
    int orcamentoOnda = 100;        // % do orçamento de inimigos por onda
    int pesoFortes = 100;           // % do peso de sorteio dos inimigos fortes do bioma
    int chanceElite = 10;           // % por sala (bioma ≥ 2)
    int chanceDrop = 15;            // % ao derrotar um inimigo comum
    int pesoKit = 100;              // % do peso de Kit/Bateria nas recompensas de porta
    int chanceRoubo = 50;           // % de o Rato pegar um item ao passar por ele
    int armadilhasOpcionais = 60;   // % das armadilhas marcadas com '?' que ficam ativas
    int criticoInimigo = 3;         // % de crítico dos inimigos
    int danoRecebido = 100;         // % do dano que chega ao Bill
    int chanceSecreta = 60;         // % de existir sala secreta no bioma
    int pactosExtras = 0;           // Pactos de Sangue a mais no início
    int multiplicadorPontos = 100;  // D da pontuação, em % (SDD §24.3)
    int chanceMoedaAbate = 55;      // % de um inimigo comum soltar 1–3 moedas (Etapa 8.5: economia)
    // Pedido do Diego (23/11): no Fácil, inimigos comuns 10% mais fracos e 5% mais lentos; chefes 12% mais fracos.
    int forcaInimigos = 100;        // % da vida e do dano dos inimigos comuns
    int velocidadeInimigos = 100;   // % da velocidade dos inimigos comuns
    int forcaChefes = 100;          // % da vida e do dano dos chefes
    // Pedido do Diego (24/11): "deixe um pouco mais justo" o Covil no Difícil (o piloto vencia 18 de 30).
    int forcaCovil = 100;           // % da vida e do dano do Pennywise IV, por cima de forcaChefes
};

struct TabelaDificuldade {
    std::array<ParametrosDificuldade, TOTAL_DIFICULDADES> niveis;
};

// A tabela do SDD §24.2 (é também o que vale se o arquivo de dados faltar).
TabelaDificuldade tabelaDificuldadePadrao();

// Lê data/balanceamento.txt. Cada linha: `chave = facil normal dificil` (3 inteiros).
// Linhas vazias e com '#' são ignoradas. Chave desconhecida ou valores faltando → false,
// com a linha no `erro`. Chaves que o arquivo não cita mantêm o valor padrão.
bool lerBalanceamento(const std::string& texto, TabelaDificuldade& tabela, std::string& erro);

// A ÚNICA forma de consultar um parâmetro: gerador, ondas, drops, IA e pontuação passam por aqui.
const ParametrosDificuldade& parametros(const TabelaDificuldade& tabela, Dificuldade d);

const char* nomeDificuldade(Dificuldade d);          // "Fácil", "Normal", "Difícil"
const char* codigoDificuldade(Dificuldade d);        // "facil", "normal", "dificil" (arquivos)
bool dificuldadeDoCodigo(const std::string& codigo, Dificuldade& d);

// Helpers de uso comum (todos em %, conta inteira).
int aplicarPorcentagem(int valor, int porcento);     // arredonda para o inteiro mais próximo
bool sortearPorcentagem(Rng& rng, int porcento);     // true com `porcento`% de chance (0 e 100 exatos)
