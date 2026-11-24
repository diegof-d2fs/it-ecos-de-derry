#pragma once
// Diálogos e tutorial (SDD §13.2–13.3; PRD §4.6, §15.3 e §15.4). Os textos moram em
// data/dialogos.txt (lido uma vez, como as salas); aqui ficam o formato, as condições e as
// regras puras. A caixa na tela é só desenho (cenas/cena_sala); o estado de "qual fala, qual
// linha, quantas letras já apareceram" vive na partida (Pendencia::Dialogo congela o mundo).
//
// Formato de um bloco:
//   @id_da_fala
//   quem=ben                      (opcional) amigo que oferece esta fala no Clube
//   quando=hub; mortes>=3         (opcional) condições, todas precisam valer
//   Falante|expressão: texto      uma linha por fala (a expressão é opcional)
//   escolha=Texto da opção|id     (opcional, 2–3) ao fim, o jogador escolhe e segue para "id"
//   efeito=flag:Nome              (opcional) guardado para as missões (Etapa 8)
// Linha em branco ou um novo '@' encerra o bloco. '#' no começo da linha é comentário.
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

enum class TipoCondicao : std::uint8_t {
    Sempre, Hub, NaoVisto, MortesMin, MortesMax, TentativasMin, TentativasMax, VitoriasMin, ResgatesMin, ChefesMin,
    MorreuPara
};

struct Condicao {
    TipoCondicao tipo = TipoCondicao::Sempre;
    int valor = 0;   // N de "mortes>=N"…; em MorreuPara, o TipoInimigo
};

struct LinhaDialogo {
    std::string falante;     // "Mike", "Bill"… (o retrato é retrato_<falante em minúsculas>)
    std::string expressao;   // P1: retratos por expressão; hoje só documenta o tom
    std::string texto;       // UTF-8; pode ter marcadores de botão: {ATACAR}, {DASH}…
};

struct EscolhaDialogo {
    std::string texto;
    std::string destino;     // id do bloco que continua a conversa
};

struct Dialogo {
    std::string id;
    std::string quem;
    std::vector<Condicao> condicoes;
    std::vector<LinhaDialogo> linhas;
    std::vector<EscolhaDialogo> escolhas;
    std::vector<std::string> efeitos;
};

// O que as condições consultam (vem do perfil; a partida não conhece o arquivo).
struct ContextoFala {
    int mortes = 0, tentativas = 0, vitorias = 0, resgates = 0, chefes = 0;
    int ultimoAssassino = -1;                    // TipoInimigo
    bool noClube = false;
    const std::set<std::string>* vistas = nullptr;
};

bool lerDialogos(const std::string& texto, std::vector<Dialogo>& dialogos, std::string& erro);
bool carregarDialogos(const std::filesystem::path& caminho, std::vector<Dialogo>& dialogos, std::string& erro);
// Problemas de consistência: escolha para id inexistente, id repetido, bloco sem fala, dica faltando.
std::vector<std::string> validarDialogos(const std::vector<Dialogo>& dialogos);

const Dialogo* buscarDialogo(const std::vector<Dialogo>& dialogos, const std::string& id);
bool condicoesSatisfeitas(const Dialogo& d, const ContextoFala& ctx);
// Fala nova que o amigo `quem` oferece no Clube: a 1ª do arquivo com as condições valendo e
// ainda não vista (no Clube, fala vista não se repete). nullptr = nenhuma.
const Dialogo* falaNovaDoAmigo(const std::vector<Dialogo>& dialogos, const std::string& quem, const ContextoFala& ctx);

// ---------- texto ----------
int contarCaracteres(const std::string& utf8);   // em letras (code points), não em bytes
// Troca {ATACAR}, {DASH}, {CONFIRMAR}, {VOLTAR}… pelo nome do botão no dispositivo em uso
// (teclado ou controle). Vale para as falas E para os textos de ajuda da interface.
std::string trocarBotoes(const std::string& texto, bool controle);
// Controle Nintendo (nome com "Nintendo"/"Switch"/"Pro Controller"): as letras das mesmas POSIÇÕES.
void definirRotulosNintendo(bool nintendo);

// ---------- tutorial (D01–D20) ----------
constexpr int TOTAL_DICAS = 20;
std::string idDaDica(int numero);              // 1 → "dica_D01"
const char* tituloDaDica(int numero);          // "Mover", "Atacar e combo"… (Como jogar)
inline bool dicaVista(std::uint32_t bits, int numero) { return (bits >> (numero - 1)) & 1u; }

constexpr float LETRAS_POR_SEGUNDO = 45.0f;   // digitação da caixa de diálogo
