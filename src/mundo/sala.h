#pragma once
// Salas desenhadas à mão (SDD §3.1 e §8). O arquivo de texto vira um SalaModelo IMUTÁVEL;
// a tentativa usa uma SalaViva (cópia mutável: portas abrem, detritos quebram…).
//
// Regra de posse (T03): os SalaModelo ficam num std::vector carregado UMA vez e nunca
// redimensionado depois. Por isso SalaViva::modelo pode guardar o ponteiro com segurança.
#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "core/tipos.h"
#include "mundo/tile.h"

constexpr int SALA_MAX_COLS = 40;
constexpr int SALA_MAX_LINS = 24;
constexpr int SALA_MAX_CELULAS = SALA_MAX_COLS * SALA_MAX_LINS;

enum class TipoSala : std::uint8_t {
    Entrada, Combate, Eco, Tesouro, Loja, Refugio, Resgate, Chefe, Prologo, Clube, Covil, Teste,
    Bonus,   // Festa dos Balões (Etapa 8.4, SDD §24.5): fase bônus de 30 s
    Total
};

struct PortaSala {
    Celula celula;
    Direcao lado = Direcao::Baixo;   // N = Cima, S = Baixo, L = Direita, O = Esquerda
    bool aberta = true;
    // Só na tentativa (SalaViva): qual opção da próxima etapa esta porta leva (−1 = nenhuma) e
    // se está selada (a porta por onde se entrou e as que sobram, como em Hades).
    int opcao = -1;
    bool selada = false;
};

struct PontoSpawn {
    Celula celula;
    bool aquatico = false;
};

struct ObjetoSala {
    // B baú, K chave do medo, k chave enferrujada, c caixa, * altar, 1..9 NPC (npc=<d> <nome>),
    // Clube: M Mural dos Perdedores, Q quadro de recordes, H alçapão; e = enfeite (enfeite=).
    // l = Lembrancinha (o colecionável escondido do bioma, atrás de uma parede rachada 'r').
    char codigo = '?';
    Celula celula;
    bool usado = false;  // só na SalaViva: baú aberto, caixa quebrada, chave já posta no chão
    int quadro = -1;     // enfeite: quadro na folha objetos_<ambiente>
    // Caixa 'c' (Diego, 17/11: "o Bill bate e ela quebra depois de duas ou três batidas"): golpes que
    // ainda aguenta e o tremor do último golpe (só desenho). Só valem na SalaViva.
    int vida = 3;
    float tremor = 0.0f;
};

struct SalaModelo {
    std::string id, nome, ambiente;   // ambiente = prefixo das texturas (tex_<ambiente>_*)
    int bioma = 0;
    TipoSala tipo = TipoSala::Combate;
    int cols = 0, lins = 0;
    std::array<Tile, SALA_MAX_CELULAS> tiles{};
    Celula entradaJogador{-1, -1};
    std::vector<PortaSala> portas;
    std::vector<PontoSpawn> spawns;
    std::vector<ObjetoSala> objetos;
    int ondas = 0, orcamentoOnda = 0;
    bool musicaTensao = false;        // musica=tensao: a trilha de tensão entra nesta sala
    // entrada=fixa: Bill SEMPRE entra pelo 'E' (a porta mais perto dele fica selada). Para salas
    // de mão única (porta do Medo, canal que só o resgate atravessa), que travariam se Bill
    // entrasse pelo outro lado (Etapa 8.4).
    bool entradaFixa = false;
    std::array<std::string, 10> npcs; // npc=<dígito> <nome>: quem fica no NPC '1'..'9' (ex.: ben)
    Celula posChefe{-1, -1};          // '@': onde o chefe-medo surge (arena)
    // ao_resgatar=<col> <lin> <símbolo>: o mapa que muda quando o amigo é resgatado (F19).
    struct Alteracao { Celula celula; char simbolo = '.'; };
    std::vector<Alteracao> aoResgatar;
    // ao_resgatar_enfeite=<col> <lin> <quadro>: o enfeite daquela célula troca de quadro no resgate
    // (a comporta do Stan: 0 aberta → 1 fechada).
    // "passa" no fim: depois do resgate o enfeite deixa de bloquear (a porta que o Eddie destranca).
    struct TrocaEnfeite { Celula celula; int quadro = 0; bool passa = false; };
    // opcional=<col> <lin>: a armadilha dessa célula só fica ativa pela chance da dificuldade (SDD §24.2).
    std::vector<Celula> armadilhasOpcionais;
    std::vector<TrocaEnfeite> enfeitesAoResgatar;
};

struct SalaViva {
    const SalaModelo* modelo = nullptr;   // não é dono (ver regra de posse acima)
    int cols = 0, lins = 0;
    std::array<Tile, SALA_MAX_CELULAS> tiles{};
    std::vector<PortaSala> portas;
    std::vector<ObjetoSala> objetos;
    int versaoTerreno = 0;                // muda a cada alteração de mapa → o render refaz o cache
};

// Ambiente padrão de cada bioma (0 = prólogo/rua … 7 = covil), usado se o arquivo não definir.
const char* ambientePadrao(int bioma);

// Lê o texto de um arquivo de sala. Em erro, `erro` diz a linha e o motivo.
bool lerSala(const std::string& texto, SalaModelo& sala, std::string& erro);
bool carregarSala(const std::filesystem::path& caminho, SalaModelo& sala, std::string& erro);
// Carrega todos os data/salas/*.txt em ordem alfabética (determinística).
bool carregarSalas(const std::filesystem::path& pasta, std::vector<SalaModelo>& salas, std::string& erro);

// Regras de SDD §8.2. Devolve a lista de problemas (vazia = sala válida).
// Quantas portas o tipo de sala precisa para a tentativa nunca virar beco sem saída.
int minimoDePortas(TipoSala tipo);
std::vector<std::string> validarSala(const SalaModelo& sala);

SalaViva criarSalaViva(const SalaModelo& modelo);

bool dentroDaSala(const SalaViva& sala, int col, int lin);
// Fora dos limites = parede (quem sai do mapa sempre colide).
const Tile& tileEm(const SalaViva& sala, int col, int lin);
const Tile& tileEm(const SalaModelo& sala, int col, int lin);

// Centro da célula em pixels do mundo.
Vec2 centroCelula(Celula c);
