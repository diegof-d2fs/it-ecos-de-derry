#pragma once
// Golpes (dano em curso) e avisos de ataque (SDD §3.3, PRD §8.1 e §8.3).
#include <cstdint>
#include <vector>

#include "combate/area.h"
#include "core/tipos.h"

// Quem o golpe atinge: Jogador → inimigos; Hostil → Bill; Todos → Bill e inimigos (o balão).
enum class Equipe : std::uint8_t { Jogador, Hostil, Todos };

struct Golpe {
    std::uint32_t id = 0;
    Equipe equipe = Equipe::Hostil;
    IdEntidade origem = ID_NULO;      // quem desferiu (não se atinge)
    Area area;
    int dano = 0;
    int focoAtacante = 1;             // para a chance de acerto
    bool rolaAcerto = true;           // áreas, projéteis e armadilhas não erram (PRD §8.1)
    bool podeCritico = false;         // só Bill (e elites, P1)
    float critico = 0.0f;             // chance em %
    float recuo = 0.0f;               // empurrão em px
    float atordoa = 0.0f;             // segundos (clarão)
    float prende = 0.0f;              // segundos sem andar nem dar dash (a faixa da Múmia)
    float lentidao = 0.0f;            // segundos com Bill a 50% da velocidade (o agarrão do Afogado)
    bool drena = false;               // quem desferiu recupera a vida que tirou (a Sanguessuga)
    float nuvem = 0.0f;               // s: ao acontecer, deixa uma nuvem pútrida que deixa lento (o Leproso)
    float poca = 0.0f;                // s: ao acontecer, o chão da área vira poça de sangue por esse tempo (o Ralo)
    bool fogo = false;                // a "poça" é FOGO no piso (a chuva de brasas do Pássaro): tile Labareda
    bool inunda = false;              // a água "escorre": as poças temporárias que havia secam antes
    bool ricochete = false;           // salto do clarão (Eco Ricochete): não salta de novo
    bool efeitoCriado = false;        // o efeito visual do ataque já foi criado (EfeitoAtaque, só desenho)
    bool queima = false;              // queimadura (Eco "Fogo controlado"): não erra, não se esquiva, ignora armadura, sem impacto
    bool chama = false;               // fogo do PISO (grelha, labareda, brasa): o Eco "Resistência" ignora
    std::uint8_t passoCombo = 0;      // 1..3: qual golpe do combo de Bill (Eco "Marreta": o 3º); 0 = outro
    bool terreno = false;             // armadilha (PRD §14.2): julgada pelos PÉS, sem esquiva/crítico,
                                      // só terrestres; voadores, chefes e NPCs não são atingidos
    bool derrubaParede = false;       // Especial e Clarão derrubam parede rachada (segredo)
    int padraoChefe = -1;             // índice do padrão do chefe que gerou o golpe (só desenho: o efeito de cada poder)
    bool doAmigo = false;             // arma ou poder do companheiro (24/11): nunca derruba o último inimigo nem o chefe
    bool especial = false;            // o giro do Especial de Bill (Ecos do Richie: Voz do locutor, Imitação)
    float duracao = 0.1f;             // quanto tempo a área fica ativa
    std::vector<IdEntidade> atingidos;   // cada alvo é atingido no máximo 1× por golpe
};

// Aviso: só visual enquanto conta; ao chegar a zero vira o Golpe registrado.
// O alvo é registrado NA PREPARAÇÃO: um ataque em área não segue Bill depois do aviso.
struct Aviso {
    float tempoTotal = 0.5f;
    float tempoRestante = 0.5f;
    Golpe golpe;
    bool consomeOrigem = false;   // o balão: ao disparar, quem avisou morre junto (estoura)
    bool projetil = false;        // um objeto voa de `lancadoDe` até o aviso (o pino do Boneco): só desenho
    Vec2 lancadoDe;
    // Se quem avisou morrer antes, o aviso é cancelado (derrubar a sombra no preparo salva Bill).
};

constexpr float PREPARO_MINIMO_AVISO = 0.5f;   // PRD §8.3 (ataques com dano ≥ 8 ou em área)
