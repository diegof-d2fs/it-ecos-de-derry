#pragma once
// Perfil persistente (SDD §3.6 e §16; PRD §13): o que sobrevive entre tentativas.
//   * Lembranças (moeda meta) e os talentos comprados no Mural dos Perdedores;
//   * contadores (tentativas, mortes, vitórias), última dificuldade, dicas e falas já vistas;
//   * opções (dicas, volumes, tela cheia).
// Arquivo saves/perfil.txt em "chave=valor" (legível e editável), gravado de forma atômica.
// Chave desconhecida é ignorada (um perfil de versão futura não derruba o jogo).
#include <array>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

#include "jogo/dificuldade.h"

struct EstadoPartida;

enum class Talento : std::uint8_t {
    PactoDeSangue, CoragemHerdada, LanternaReforcada, BolsoFundo, TreinoBarrens, MaoLeve,
    SorteRichie, MapaBen, FolegoExtra,
    Total
};
constexpr int TOTAL_TALENTOS = static_cast<int>(Talento::Total);
constexpr int MAX_NIVEIS_TALENTO = 5;

struct DefinicaoTalento {
    const char* codigo;            // chave no arquivo (talento.<codigo>)
    const char* nome;
    const char* efeito;            // por nível
    int niveis;
    int custos[MAX_NIVEIS_TALENTO];
    bool disponivel;               // false = "em breve" (o sistema dele ainda não existe): não se vende
};
const DefinicaoTalento& definicaoTalento(Talento t);

struct OpcoesJogo {
    bool dicas = true;             // tutorial contextual (Etapa 7.3)
    int volumeGeral = 100;         // 0..100
    int volumeMusica = 80;
    int volumeEfeitos = 90;
    // Abre em TELA CHEIA (Diego, 17/11: em janela, a maior escala inteira que cabia em 90% da tela
    // de 1920×1080 era 1280×720 — dois terços da tela). F11 ou as Opções voltam para a janela.
    bool telaCheia = true;
    // Cenas da história (23/11): false = cada uma passa só na 1ª vez (a abertura passa a cada Novo jogo);
    // true = sempre que o momento acontecer.
    bool cenasSempre = false;
};

struct Perfil {
    int versao = 2;   // 2: tela cheia passou a ser o padrão (perfis da versão 1 migram para ela)
    int lembrancas = 0;
    std::array<int, TOTAL_TALENTOS> talentos{};   // nível comprado de cada um
    int tentativas = 0, mortes = 0, vitorias = 0;
    int resgates = 0, chefes = 0;                  // somados de todas as tentativas (falas do Clube)
    std::uint32_t lembrancinhas = 0;               // estante do Clube: bit b = a do bioma b (Etapa 8.4)
    long long melhorPontuacao = 0;
    Dificuldade ultimaDificuldade = Dificuldade::Normal;
    bool prologoVisto = false;
    std::uint32_t dicasVistas = 0;                 // bit i = dica D(i+1) já mostrada
    std::set<std::string> falasVistas;             // ids de data/dialogos.txt
    std::set<std::string> cenasVistas;             // ids de data/cenas.txt já assistidos
    int ultimoAssassino = -1;                      // TipoInimigo da última morte (falas do Clube)
    OpcoesJogo opcoes;
};

// ---------- arquivo ----------
std::string escreverPerfil(const Perfil& p);
// Leitura tolerante: valores ruins ficam no padrão. Devolve quantas linhas foram ignoradas.
int lerPerfil(const std::string& texto, Perfil& p);
bool perfilExiste(const std::filesystem::path& caminho);
Perfil carregarPerfil(const std::filesystem::path& caminho);   // ausente = perfil novo
bool gravarPerfil(const std::filesystem::path& caminho, const Perfil& p);

// ---------- Mural ----------
// Custo do PRÓXIMO nível (−1 = já no máximo ou indisponível).
int custoProximoNivel(const Perfil& p, Talento t);
// Gasta Lembranças e sobe 1 nível. false = sem saldo, no máximo ou "em breve".
bool comprarTalento(Perfil& p, Talento t);

// ---------- tentativa ----------
// Aplica os talentos no começo da tentativa (pactos, vida, lanterna, kits, pontos, moedas).
void aplicarTalentos(const Perfil& perfil, EstadoPartida& partida);
// Lembranças que a tentativa rende (PRD §13.1): 3 por sala concluída, 10 por chefe, 5 por
// resgate e 20 pela vitória.
int lembrancasDaTentativa(const EstadoPartida& p);
// Fecha a tentativa no perfil: contadores e Lembranças. O piloto NÃO ganha Lembranças para a
// pessoa (assistir o computador não pode "farmar" o Mural). Devolve as Lembranças ganhas.
int registrarFimDeTentativa(Perfil& perfil, const EstadoPartida& p, bool jogadaPeloPiloto);
