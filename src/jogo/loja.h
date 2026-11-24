#pragma once
// Loja do Elias (PRD §11.3, SDD §15) e a missão da manivela (PRD §14.3, critério F19).
//
//   * Ao entrar na sala de loja, 3 a 5 ofertas são sorteadas (kit, bateria, Eco, melhoria,
//     escudo, chave enferrujada, barquinho). Os preços vêm da tabela de itens (jogo/itens).
//   * O Elias (NPC "elias") vende pela Pendencia::Loja. Se Bill traz a MANIVELA, ele a recebe
//     primeiro e abre a grade da sala: a grade vira chão (passagem e visão), um baú aparece.
//   * A manivela cai junto com a Chave do Medo no bioma 1 (sempre antes da loja).
//   * A fala do Elias muda conforme o que derrubou Bill na tentativa anterior.
#include <string>
#include <vector>

#include "core/rng.h"
#include "jogo/ecos.h"
#include "jogo/itens.h"

struct EstadoPartida;

constexpr int PRECO_ECO = 80;            // PRD §11.3
constexpr int BIOMA_DA_MANIVELA = 1;     // o Elias e a grade de manutenção ficam na galeria

struct OfertaLoja {
    TipoItem item = TipoItem::Total;     // Total = é um Eco aleatório do amigo do bioma
    int preco = 0;
    bool vendida = false;
};

enum class ResultadoCompra : std::uint8_t { Comprou, SemMoedas, SemEspaco, Vendida, Invalida };

// Ofertas da loja (sem repetir; nada que não caiba ou não exista: melhoria no máximo, escudo
// já equipado, Eco sem Eco novo a oferecer ficam de fora).
std::vector<OfertaLoja> montarLoja(const EstadoPartida& p, Rng& rng);
// Compra a oferta `indice` com moedas de prata (o Eco é aplicado na hora).
ResultadoCompra comprar(EstadoPartida& p, int indice);
std::string nomeDaOferta(const EstadoPartida& p, const OfertaLoja& o);
// O Elias comenta a morte anterior (TipoInimigo, −1 = nenhuma) — `vez` varia a frase.
const char* falaDoElias(int assassinoAnterior, int vez);
