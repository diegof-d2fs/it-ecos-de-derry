#pragma once
// Salvar e carregar o jogo em 15 blocos, como um cartão de memória (pedido do Diego em 23/11/2026).
//
// O que se salva é o PONTO DE ENTRADA da sala atual: tudo o que persiste entre salas, no instante em que Bill
// entrou nela (a partida guarda esse ponto em entrarNaSala). Carregar refaz a tentativa e recoloca Bill na
// entrada daquela sala, com o que ele tinha ao chegar. Assim não se duplicam itens (pegar o baú, salvar e
// recarregar para pegar de novo) e não é preciso gravar inimigos, golpes e avisos no meio de uma luta. Os
// sorteios seguintes continuam os mesmos (os dois geradores de números aleatórios vão junto).
//
// Arquivos: <saves>/bloco_01.txt … bloco_15.txt, texto "chave=valor" (como perfil.txt). Leitura tolerante.
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "combate/atributos.h"
#include "combate/experiencia.h"
#include "core/rng.h"
#include "jogo/companheiro.h"
#include "jogo/dificuldade.h"
#include "jogo/ecos.h"
#include "jogo/itens.h"
#include "jogo/pontuacao.h"
#include "jogo/tentativa.h"

struct EstadoPartida;
struct Conteudo;

constexpr int TOTAL_BLOCOS = 15;

struct PontoSalvamento {
    bool valido = false;
    // tentativa
    std::uint64_t seed = 0;
    Dificuldade dificuldade = Dificuldade::Normal;
    Rng rng, rngGerador;
    PlanoBioma plano;
    int etapa = 0;
    OpcaoPorta salaPrometida;
    int indiceSala = 0;
    // Bill
    Atributos attr;
    Equipamento equip;
    int hp = 0;
    Progresso progresso;
    int pactos = 1;
    bool pactoUsado = false;
    int cargasLanterna = 0, cargasMaximo = 0;
    int dashExtras = 0;
    float coragem = 0.0f, tonico = 0.0f, oculos = 0.0f;
    // o resto da tentativa
    Inventario inventario;
    std::vector<EcoAtivo> ecos;
    bool companheiroAtivo = false;
    Amigo companheiro = Amigo::Total;
    std::vector<Amigo> resgatados;
    Estatisticas stats;
    int kitsUsados = 0, bonusMoedasPct = 0, assassinoAnterior = -1, conversasElias = 0, compras = 0, ultimoAgressor = -1;
    bool manivelaEntregue = false, poemaUsado = false, segredoAtivo = false, segredoUsado = false;
    std::uint32_t lembrancinhas = 0;
    // resumo para a tela do cartão
    long long pontos = 0;
};

// Resumo de um bloco ocupado (o que a tela mostra).
struct ResumoBloco {
    bool ocupado = false;
    int bioma = 0;
    std::string nomeSala;
    int nivel = 1;
    int segundos = 0;
    long long pontos = 0;
    Dificuldade dificuldade = Dificuldade::Normal;
    std::string data;      // "24/11/2026 14:32"
};

// O ponto de entrada de uma sala da tentativa (a partida chama no começo de entrarNaSala).
PontoSalvamento capturarPonto(const EstadoPartida& p, int indiceSala);
// Refaz a tentativa a partir do ponto (Bill na entrada da sala). false = o ponto não serve neste conteúdo.
bool restaurarPonto(EstadoPartida& p, const Conteudo& c, const PontoSalvamento& ponto, std::string& erro);

// Texto do arquivo. As salas vão pelo ID (o índice muda quando entram salas novas em data/salas).
std::string escreverPonto(const PontoSalvamento& ponto, const Conteudo& c, const std::string& data);
bool lerPonto(const std::string& texto, const Conteudo& c, PontoSalvamento& ponto, ResumoBloco& resumo, std::string& erro);

// Blocos (1..15) numa pasta de saves.
std::filesystem::path arquivoDoBloco(const std::filesystem::path& pasta, int bloco);
std::array<ResumoBloco, TOTAL_BLOCOS> listarBlocos(const std::filesystem::path& pasta, const Conteudo& c);
bool gravarBloco(const std::filesystem::path& pasta, int bloco, const PontoSalvamento& ponto, const Conteudo& c,
                 const std::string& data);
bool carregarBloco(const std::filesystem::path& pasta, int bloco, const Conteudo& c, PontoSalvamento& ponto, std::string& erro);
bool apagarBloco(const std::filesystem::path& pasta, int bloco);
