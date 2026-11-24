#pragma once
// Ecos, as bênçãos dos amigos (PRD §12, SDD §3.5 e §15). Uma sala de Eco (altar) ou uma porta
// com a recompensa "Eco" oferece 3 Ecos do amigo do bioma; o jogador escolhe 1, que vale até o
// fim da tentativa. A raridade (Comum, Raro, Épico) muda os números; o resgate dá o Lendário.
//
// Regra do projeto: NENHUM Eco tem código espalhado. Os passivos entram nos atributos pelo
// Equipamento (ecoArmadura, ecoHp); os de evento são consultados por valorEco(partida, efeito)
// nos pontos de evento (golpe acertou, dash terminou, Bill caiu). Cada Eco é uma linha da tabela.
//
// Etapa 8.1: os Ecos do Ben (bioma 1). Etapa 10.4: os do Stan (bioma 2). Etapa 13.4: os do Mike (bioma 5). Etapa 14.3: os do Richie (bioma 6): os seis amigos têm Ecos; o
// sorteio só oferece Ecos que existem (nunca vende o que não funciona).
#include <cstdint>
#include <string>
#include <vector>

#include "core/rng.h"
#include "jogo/companheiro.h"

struct EstadoPartida;

enum class Raridade : std::uint8_t { Comum, Raro, Epico, Lendario, Total };

enum class EfeitoEco : std::uint8_t {
    ArmaduraFixa,       // Ben · Tijolo a tijolo: +armadura
    HpMax,              // Ben · Coração pesado: +vida máxima
    DanoContraParede,   // Ben · Planta baixa: golpe em inimigo encostado na parede (×%)
    BarreiraDash,       // Ben · Ponte firme: o dash deixa uma barreira de tijolos (ms)
    Poema,              // Ben · Lendário: a 1ª queda a 0 de vida no bioma vira 1 de vida
    MarcaTerceiroGolpe, // Stan · Livro de pássaros: cada 3º golpe de Bill no mesmo alvo dá +dano
    MoedasSalaSemDano,  // Stan · Escoteiro: sala de combate concluída sem levar dano dá moedas
    InvulnerabilidadeDash, // Stan · Calma: ms de invulnerabilidade a mais no fim do dash
    CriticoAtordoado,   // Stan · Ordem: +% de crítico em alvo atordoado (clarão)
    DashDuplo,          // Stan · Lendário (Asas): cargas EXTRAS de dash
    NevoaDash,          // Eddie · Inalador: o fim do dash deixa lentos os inimigos perto (ms)
    KitMaisForte,       // Eddie · Remédio: o kit cura % a mais
    GessoUmGolpe,       // Eddie · Gesso: 1× por sala, um golpe em Bill perde % do dano
    VelocidadePct,      // Eddie · Pressa: +% de velocidade
    Cuidado,            // Eddie · Lendário: ao resgatar, cura total e +1 kit
    MiraFirme,          // Beverly · Mira firme: +pontos de chance de acerto dos golpes de Bill
    Prata,              // Beverly · Prata: +% de dano contra chefes
    RicocheteClarao,    // Beverly · Ricochete: o clarão salta para +N alvos perto de quem acertou
    EspecialEstilingue, // Beverly · Estilingue: o especial também dispara um tiro de prata (dano)
    TiroCerteiro,       // Beverly · Lendário: críticos atordoam (ms)
    Marreta,            // Mike · Marreta: o 3º golpe do combo +% de dano e mais recuo
    Cronista,           // Mike · Cronista: +% de XP
    FogoControlado,     // Mike · Fogo controlado: o clarão queima (dano por segundo, 3 s)
    Resistencia,        // Mike · Resistência: +armadura e imune ao fogo do piso
    LuzGerador,         // Mike · Lendário (Luz do gerador): +células de raio de visão
    VozLocutor,         // Richie · Voz do locutor: o especial atordoa (ms; chefes, metade)
    OculosRichie,       // Richie · Óculos: inimigos fora da luz, a até N células além dela, aparecem em contorno
    PiadaRuim,          // Richie · Piada ruim: −N pontos na chance de acerto dos inimigos em Bill
    BipBip,             // Richie · Bip-bip: o dash fere quem Bill atravessa (1× por inimigo por dash)
    Imitacao,           // Richie · Lendário (Imitação): 1×/sala, o 1º inimigo comum que o especial acerta luta do lado de Bill (s)
    Total
};

struct DefinicaoEco {
    Amigo amigo;
    const char* nome;
    EfeitoEco efeito;
    int valor[4];          // por raridade: Comum, Raro, Épico, Lendário (inteiros: sem float na regra)
    const char* texto;     // "%d" = o valor da raridade
    bool lendario;         // só no resgate (nunca no altar nem na loja)
};

struct EcoAtivo {
    int definicao = 0;     // índice na tabela
    Raridade raridade = Raridade::Comum;
};

struct OfertaEco {
    int definicao = 0;
    Raridade raridade = Raridade::Comum;
};

int totalEcos();
const DefinicaoEco& definicaoEco(int indice);
const char* nomeRaridade(Raridade r);
int valorDaRaridade(const DefinicaoEco& d, Raridade r);
std::string descricaoEco(int definicao, Raridade r);   // "+2 de armadura"

// Amigo preso em cada bioma (1 Ben, 2 Stan, 3 Eddie, 4 Beverly, 5 Mike, 6 Richie).
Amigo amigoDoBioma(int bioma);
// Há Ecos (não lendários) desse amigo no jogo? (o gerador só põe porta/altar de Eco se houver)
bool temEcos(Amigo a);

// Chance de raridade no altar e nas portas (PRD §12): 70% Comum, 25% Raro, 5% Épico.
Raridade sortearRaridade(Rng& rng);
// Até 3 Ecos diferentes do amigo, sem repetir os que Bill já tem nem oferecer o Lendário.
std::vector<OfertaEco> sortearEcos(const std::vector<EcoAtivo>& ativos, Amigo amigo, Rng& rng);
int ecoLendarioDe(Amigo a);   // índice do Lendário do amigo (−1 = ainda não existe)

// Soma do efeito em todos os Ecos ativos (0 = nenhum).
int valorEco(const EstadoPartida& p, EfeitoEco efeito);
// Bill ganha o Eco (efeitos passivos aplicados na hora: vida, armadura).
void aplicarEco(EstadoPartida& p, int definicao, Raridade r);

// Barreira de tijolos do Eco "Ponte firme" (fica onde o dash terminou).
struct BarreiraTijolos {
    Vec2 centro;
    float vida = 0.0f;
};
constexpr float RAIO_BARREIRA_DASH = 26.0f;
