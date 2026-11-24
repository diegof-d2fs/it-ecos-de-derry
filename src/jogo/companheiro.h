#pragma once
// Companheiro de tentativa (M11, SDD §24.6; aprovado pelo Diego em 16/11/2026).
//
//   * Bioma 1: o Mike acompanha Bill (desde o prólogo). Ao resgatar um amigo, ele vira o
//     companheiro e o anterior volta ao Clube pela rota segura.
//   * Controlado pela IA e INVULNERÁVEL: não é escolta, nunca causa uma derrota.
//   * Segue um ponto 1,5 célula atrás de Bill pelo caminho BFS; se ficar a mais de 8 células
//     ou travar por 2 s, teleporta para perto de Bill. Não ativa armadilhas, não pega itens e
//     não colide com Bill.
//   * Cada amigo tem UMA habilidade de apoio: tabela HABILIDADES_COMPANHEIRO (ponteiros para
//     função, T06), como as camadas do piloto e as telas do menu.
#include <cstdint>
#include <string>
#include <vector>

#include "core/tipos.h"

struct EstadoPartida;
struct Inimigo;

// Ordem dos resgates do PRD §4.4 não importa aqui; a ordem é a da tabela de habilidades.
enum class Amigo : std::uint8_t { Mike, Ben, Stan, Eddie, Beverly, Richie, Total };
constexpr int TOTAL_AMIGOS = static_cast<int>(Amigo::Total);

const char* codigoAmigo(Amigo a);                  // "mike" (sprite mike_idle, retrato_mike, npc=1 mike)
const char* nomeAmigo(Amigo a);                    // "Mike"
Amigo amigoDoCodigo(const std::string& codigo);    // Total = não é um dos seis

// Arma e poder de cada amigo (pedido do Diego, 24/11/2026): um objeto e um poder ligados à lore dele.
//   * Só ajuda com 2 ou mais inimigos na tela (ou contra o chefe ainda na forma do medo).
//   * Um ataque a cada INTERVALO_ATAQUE_AMIGO, alternando arma e poder; dano = o do Bill no início do jogo.
//   * Nunca vence sozinho: o golpe do amigo não derruba o último inimigo da sala nem um chefe (fica com 1 de vida).
//   * Quando o chefe vira Pennywise, o amigo diz que está com medo e para de ajudar; no Covil (o último), luta até o fim.
struct CombateAmigo {
    const char* arma;              // nome (HUD e textos)
    const char* poder;
    const char* descricaoArma;
    const char* descricaoPoder;
    // O que o PODER faz além do dano (a arma só fere e empurra um pouco).
    float atordoa;                 // s (o chefe, pela metade)
    float lento;                   // s a 50% da velocidade
    float recuo;                   // px
    const char* falaMedo;          // quando o chefe vira Pennywise
    const char* falaCovil;         // ao ver o Pennywise IV: desta vez fica
};
const CombateAmigo& combateDe(Amigo a);

struct AtaqueDoAmigo {
    bool ativo = false;
    bool poder = false;            // false = arma (voa do amigo até o alvo); true = poder (surge no alvo)
    IdEntidade alvo = ID_NULO;
    Vec2 de, para;                 // origem e alvo (o alvo é seguido enquanto o golpe voa)
    float tempo = 0.0f;            // desde o lançamento
    float duracao = 0.0f;          // voo da arma / preparo do poder: o dano sai no fim
    bool resolvido = false;        // o golpe já foi registrado
};

constexpr float INTERVALO_ATAQUE_AMIGO = 3.0f;    // s entre ataques (Diego: "de 3 em 3 s")
constexpr float PRIMEIRO_ATAQUE_AMIGO = 1.0f;     // s ao entrar na sala / ao surgir a briga
constexpr float VOO_ARMA_AMIGO = 0.28f;
constexpr float PREPARO_PODER_AMIGO = 0.22f;
constexpr float EFEITO_ATAQUE_AMIGO = 0.45f;      // s do efeito no alvo depois do dano (só desenho)
constexpr float ALCANCE_ATAQUE_AMIGO = 8.0f * TILE;
constexpr int MINIMO_INIMIGOS_AMIGO = 2;

struct Companheiro {
    bool ativo = false;
    Amigo quem = Amigo::Total;
    Vec2 pes;
    Direcao direcao = Direcao::Baixo;
    bool andando = false;
    float tempoAnimacao = 0.0f;
    std::vector<Celula> caminho;      // plano BFS até o ponto atrás de Bill
    float replanejar = 0.0f;
    float tempoTravado = 0.0f;
    int teleportes = 0;               // telemetria / testes
    // Habilidade
    float recarga = 0.0f;             // até poder usar de novo
    float efeito = 0.0f;              // duração do efeito ativo (barreira do Ben, provocação do Richie)
    bool usouNaSala = false;          // Eddie: 1 cura por sala
    IdEntidade alvo = ID_NULO;        // Richie: quem foi provocado
    Vec2 tiroDe, tiroPara;            // Beverly: traço do estilingue (só visual)
    float tiro = 0.0f;
    int usos = 0;                     // quantas vezes a habilidade agiu (testes / HUD)
    // Arma e poder (24/11)
    float recargaAtaque = PRIMEIRO_ATAQUE_AMIGO;
    bool proximoPoder = false;        // alterna: arma, poder, arma…
    AtaqueDoAmigo ataque;
    int ataquesArma = 0, ataquesPoder = 0;   // testes e telemetria
    // Em luta (24/11, Diego: "quando o Bill está batendo em alguém o ajudante fica travado"): ele toma posição no
    // flanco do Bill, de frente para o inimigo, e dá passinhos; parado, respira (o desenho usa o relógio).
    bool emLuta = false;
    float relogio = 0.0f;
    bool comMedo = false;             // o chefe virou Pennywise (fora do Covil): não ajuda mais nesta sala
    bool falouNoCovil = false;
    bool avisouArmadilha = false;     // Stan: "Bill, cuidado ali!" na 1ª armadilha vista em cada sala
};

struct HabilidadeCompanheiro {
    const char* nome;
    const char* descricao;
    float recarga;                                // 0 = passiva (ou 1 vez por sala)
    void (*agir)(EstadoPartida& p, float dt);     // chamada a cada passo enquanto o amigo acompanha
};
const HabilidadeCompanheiro& habilidadeDe(Amigo a);

constexpr float DISTANCIA_SEGUIR = 1.5f;          // células atrás de Bill
constexpr float DISTANCIA_TELEPORTE = 8.0f;       // células
constexpr float TEMPO_TRAVADO_TELEPORTE = 2.0f;   // s
constexpr Vec2 CAIXA_COMPANHEIRO{16.0f, 9.0f};

// O amigo passa a acompanhar Bill (aparece em `origem`, ou perto de Bill se origem = {0,0}).
void entrarCompanheiro(EstadoPartida& p, Amigo quem, Vec2 origem = Vec2{});
// Sala nova: o companheiro chega junto, ao lado de Bill (e o Eddie pode curar de novo).
void companheiroEntrouNaSala(EstadoPartida& p);
// Um passo: segue Bill e usa a habilidade (pela tabela).
void atualizarCompanheiro(EstadoPartida& p, float dt);

// Efeitos consultados pelas regras e pelo desenho.
int bonusDeLuz(const EstadoPartida& p);                 // Mike: +1 célula de lanterna
bool barreiraAtiva(const EstadoPartida& p);             // Ben: golpes hostis não atingem Bill
bool revelaArmadilhas(const EstadoPartida& p);          // Stan: armadilhas marcadas na luz
// Stan: esta célula é uma armadilha na luz que ele aponta (o passarinho pousa em cima; 24/11).
bool stanApontaArmadilha(const EstadoPartida& p, int col, int lin);
IdEntidade itemMarcado(const EstadoPartida& p);         // Mike: item visível mais perto de Bill
bool provocado(const EstadoPartida& p, IdEntidade inimigo);   // Richie: esse inimigo mira no Richie

// Arma e poder (24/11)
int danoDoAmigo();                                      // a força do Bill no início do jogo (taco, atributos 1)
int inimigosNaTela(const EstadoPartida& p);             // hostis vivos e visíveis (o que conta para ajudar)
// O golpe do amigo pode derrubar este inimigo? Não, se for chefe ou o último hostil vivo da sala.
bool amigoPodeDerrubar(const EstadoPartida& p, const Inimigo& alvo);
