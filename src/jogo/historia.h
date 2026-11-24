#pragma once
// Cenas da história (cut scenes) — pedido do Diego em 23/11/2026: a abertura do Novo jogo (~1 min 30 s),
// uma ao chegar a cada bioma depois de um chefe e a final, de 1 a 2 min. Cada cena é uma sequência de
// QUADROS: uma ilustração 640×360 que se move devagar (zoom ou panorâmica), com um texto que aparece
// letra a letra. Os dados moram em data/cenas.txt; aqui ficam o formato e o "tocador" (tempo, avanço,
// pular), puros e testáveis. A cena de desenho (cenas/cena_historia.cpp) só lê este estado.
#include <filesystem>
#include <string>
#include <vector>

// Movimento lento da câmera sobre a ilustração durante o quadro.
enum class MovimentoQuadro : std::uint8_t { ZoomEntra, ZoomSai, Esquerda, Direita, Cima, Baixo };

struct QuadroHistoria {
    std::string imagem;     // nome no manifesto (cena_abertura_1…); "-" = tela preta só com o texto
    float segundos = 10.0f; // tempo do quadro na tela (sem pressa: dá para ver a imagem e ler)
    MovimentoQuadro movimento = MovimentoQuadro::ZoomEntra;
    std::string texto;      // UTF-8; '|' quebra a linha
};

struct CenaHistoria {
    std::string id;         // abertura, bioma_2 … bioma_7, final
    std::string titulo;     // aparece no primeiro quadro ("Capítulo 2: O Barrens"; nunca travessão em título ou texto)
    std::vector<QuadroHistoria> quadros;
};

// Formato de data/cenas.txt ('#' comenta):
//   [id] Título da cena
//   imagem;segundos;movimento;texto do quadro
// movimento: zoom_entra, zoom_sai, esquerda, direita, cima, baixo.
bool lerCenasHistoria(const std::string& texto, std::vector<CenaHistoria>& cenas, std::string& erro);
bool carregarCenasHistoria(const std::filesystem::path& caminho, std::vector<CenaHistoria>& cenas, std::string& erro);
const CenaHistoria* buscarCenaHistoria(const std::vector<CenaHistoria>& cenas, const std::string& id);
// A cena que abre ao CHEGAR ao bioma `bioma` (2..7): "bioma_N". Biomas fora disso: "".
std::string cenaDaChegadaAoBioma(int bioma);
float duracaoCena(const CenaHistoria& cena);   // soma dos quadros + entrada e saída em preto
// Tempo do quadro na tela: o do arquivo, mas nunca menos que o de digitar e ler o texto.
float duracaoQuadro(const QuadroHistoria& q);

// ---------- ritmo ----------
constexpr float HISTORIA_ENTRADA = 1.2f;       // a primeira imagem surge do preto
constexpr float HISTORIA_TRANSICAO = 1.4f;     // uma imagem se dissolve na outra
constexpr float HISTORIA_SAIDA = 1.6f;         // a última imagem some no preto
constexpr float HISTORIA_ATRASO_TEXTO = 1.0f;  // o texto começa depois que a imagem apareceu
constexpr float HISTORIA_LETRAS_POR_SEGUNDO = 30.0f;
constexpr float HISTORIA_LEITURA_MINIMA = 3.5f;   // depois do texto completo, pelo menos isto para ler
constexpr float HISTORIA_SEGURAR_PULAR = 1.0f;    // segurar Voltar por 1 s pula a cena inteira
constexpr float HISTORIA_TOQUE_MINIMO = 1.5f;     // Confirmar só troca de quadro depois disto

// Tempo mínimo de um quadro com `letras` letras: digitação + leitura. O arquivo de dados é testado
// contra isto (nenhum quadro some antes de dar para ler).
float tempoMinimoDoQuadro(int letras);

// ---------- tocador ----------
struct TocadorHistoria {
    const CenaHistoria* cena = nullptr;   // aponta para Conteudo::cenas (nunca redimensionado)
    int quadro = 0;
    float tempoQuadro = 0.0f;     // segundos desde que o quadro começou a aparecer
    float tempoCena = 0.0f;
    float segurandoPular = 0.0f;  // segundos com Voltar seguro
    float saida = 0.0f;           // > 0: a última imagem está sumindo no preto
    float ultimoToque = 99.0f;    // segundos desde a última tecla (a dica de botões reaparece)
    bool pulou = false;
    bool terminou = false;
};

void iniciarTocador(TocadorHistoria& t, const CenaHistoria* cena);
// `avancar` = Confirmar/Atacar apertado neste passo; `segurandoVoltar` = Voltar seguro agora.
// Confirmar com o texto ainda digitando mostra o texto todo; com o texto completo, passa ao quadro seguinte.
void atualizarTocador(TocadorHistoria& t, bool avancar, bool segurandoVoltar, float dt);
// Letras do texto do quadro atual já visíveis (digitação).
int letrasVisiveis(const TocadorHistoria& t);
int letrasDoQuadro(const QuadroHistoria& q);
// 0..1: quanto do movimento de câmera do quadro já aconteceu (inclui a transição de entrada).
float progressoDoQuadro(const TocadorHistoria& t);
