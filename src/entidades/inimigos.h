#pragma once
// Inimigos comuns e IA (PRD v2 §9, SDD §12). Todos compartilham movimento, colisão, percepção
// e estados; o que muda é a TABELA de parâmetros e UMA função de comportamento por tipo,
// escolhida por uma tabela de ponteiros para função (técnica T03).
#include <cstdint>
#include <vector>

#include "combate/golpe.h"
#include "core/rng.h"
#include "core/tipos.h"
#include "mundo/sala.h"

// Os chefes-medo também são inimigos (mesma colisão, golpes, avisos, XP e piloto), com uma
// função de comportamento própria e padrões de ataque em tabela (SDD §3.4, §12.3).
enum class TipoInimigo : std::uint8_t { Balao, Rato, Sombra, Espectro, Sanguessuga, Afogado, Boneco, Cabelo, Mumia, MedoAfogado, Leproso, RaloSangue, Passaro, Lobisomem, PennywiseIV, Total };
constexpr int TOTAL_TIPOS_INIMIGO = static_cast<int>(TipoInimigo::Total);

enum class EstadoIA : std::uint8_t { Vagando, Perseguindo, Buscando, Preparando, Atacando, Recuperando, Fugindo };

struct ParametrosInimigo {
    const char* nome;
    const char* atlas;          // sprite (linhas = direção; colunas = espera, passo A, passo B, ataque)
    int hp, dano, armadura, agilidade;
    float velocidade;           // px/s
    int xp;
    float deteccao;             // células
    int custoOnda;              // pontos do orçamento da onda
    bool voa;                   // ignora água e buracos (bloqueiaVoo)
    float alturaCorpo;          // hurtbox: px acima dos pés
    bool chefe = false;         // chefe-medo: barra própria, fase 2, conta B (não K), sempre sabe onde Bill está
};

constexpr Vec2 CAIXA_INIMIGO{20.0f, 12.0f};   // colisão nos pés (SDD §10)

struct Inimigo {
    IdEntidade id = ID_NULO;
    TipoInimigo tipo = TipoInimigo::Balao;
    Vec2 pes;
    Direcao direcao = Direcao::Baixo;
    int hp = 1;
    bool vivo = true;
    EstadoIA estado = EstadoIA::Vagando;
    float timer = 0.0f;           // duração do estado atual (preparo, ataque, recuperação…)
    float recarga = 0.0f;         // tempo até poder atacar de novo
    float surgindo = 0.0f;        // > 0: aparecendo no spawn (não age nem é atingido)
    float atordoado = 0.0f;       // clarão
    float flash = 0.0f;           // piscar de 1 quadro ao ser atingido
    float animTempo = 0.0f;
    bool andando = false;
    Vec2 alvoRegistrado;          // para onde o ataque foi anunciado
    Celula ultimaVista{-1, -1};
    float tempoSemVer = 0.0f;
    Celula destinoVagar{-1, -1};
    Celula proximo{-1, -1};       // próximo passo do caminho (BFS)
    float recalcularCaminho = 0.0f;
    Vec2 empurrao;                // recuo (knockback) em px/s, decai rápido
    bool jaFugiu = false;         // rato: foge 1× quando fica com pouca vida
    // Inimigos × itens (SDD §24.5). O tipo do item fica como int (as entidades não conhecem a
    // tabela de itens, que mora em jogo/); −1 = não carrega nada.
    int itemCarregado = -1;
    int qtdCarregada = 0;
    Vec2 destinoFuga;             // o rato ladrão corre para cá (spawn mais longe do Bill)
    float tempoSemSerVisto = 0.0f;// rato ladrão fora da luz: aos 8 s some com o item
    float recargaRoubo = 0.0f;    // tentou e não conseguiu: espera antes de tentar de novo
    float timerRastro = 0.0f;     // intervalo do rastro: brasas do espectro, poças do afogado
    // Chefe (Etapa 8.3)
    int fase = 1;                 // 1 = o medo; 2 = o Pennywise que sai de dentro (≤ 50% da vida)
    int padrao = -1;              // padrão de ataque em curso (tabela do chefe)
    int ultimoPadrao = -1;        // o último que começou: o chefe não repete o mesmo seguido
    int golpesDeBill = 0;         // Eco "Livro de pássaros": cada 3º golpe no mesmo alvo
    Celula alvoCaca{-1, -1};      // caça (últimos da sala): onde Bill estava — campos PRÓPRIOS (o vagar
    float timerCaca = 0.0f;       // usa destinoVagar/timer; compartilhar fazia o caçador seguir o passeio)
    Celula raloDestino{-1, -1};   // Cabelo do ralo: o ralo por onde vai subir (campo próprio: o vagar usa destinoVagar)
    float lento = 0.0f;           // > 0: anda a 50% (a névoa do Inalador do Eddie)
    float recargaFuga = 0.0f;     // Boneco: a fuga é um arranque curto, com recarga (não foge para sempre)
    int forcaPct = 100;           // escala do bioma (vida e dano): +15% por bioma depois do 1º
    bool elite = false;           // "Assombrado" (PRD §9): +60% vida, +25% dano, XP ×2, aura violeta
    float transicao = 0.0f;       // > 0: o medo se rasgando (invulnerável, parado)
    float submerso = 0.0f;        // > 0: o Afogado está no fundo / o Pennywise II sumiu (não aparece nem é atingido)
    float rumo = 1.5708f;         // rad (0 = direita, π/2 = baixo): para onde o corpo aponta, girando suave. O
                                  // corpo de aranha do Pennywise IV é desenhado girado por ele (360°, 24/11)
    Vec2 direcaoBote;             // "Pela porta": a direção da linha anunciada
    Vec2 destinoTeleporte;        // Espectro: onde a fumaça anunciou que ele vai aparecer
    float teleporte = 0.0f;       // Espectro: > 0 = virou fumaça (some e reaparece no destino)
    float recargaTeleporte = 0.0f;
    Vec2 origem;                  // onde nasceu (o Ralo de Sangue: o ralo, que continua lá na fase 2)
    int inundacoes = 0;           // Ralo de Sangue: quantas vezes inundou (a paridade escolhe o par de quadrantes)
    float queimando = 0.0f;       // Eco "Fogo controlado" (Mike): s de queimadura que faltam
    int queimaPorSegundo = 0;
    float tickQueima = 0.0f;
    float aliado = 0.0f;          // > 0: Eco lendário "Imitação" (Richie): luta do lado de Bill por estes segundos
    int hpMax = 0;                // vida cheia DESTE inimigo (tabela × bioma × elite × dificuldade): fases e barra
    int velocidadePct = 100;      // % da velocidade da tabela (o Fácil deixa os comuns 5% mais lentos)
};

// Padrão de ataque de chefe: DADOS (SDD §3.4). A máquina de estados é uma só para todos.
// Circulo = área em volta do PRÓPRIO chefe (raio = comprimento): castiga quem fica colado batendo.
// Mergulho = afunda (preparo), marca um círculo onde Bill está (1,0 s) e emerge ali (Etapa 10.3).
// Ondas = dois anéis em volta dele, o 2º mais largo e mais tarde (largura = espessura do anel).
// Nuvem = círculo onde Bill está; ao cair, deixa uma nuvem pútrida que deixa lento (Etapa 11.4).
// Porta = some, reaparece por uma PORTA da sala e dá o bote numa linha anunciada até Bill (11.4).
// Cruz/Xis = duas linhas pelo centro do chefe (ortogonais / diagonais); Inundar = dois quadrantes
// OPOSTOS a partir do ralo, alternando o par a cada vez, que viram poças de sangue por 5 s; Golfada =
// círculo onde Bill está que deixa poça por 6 s (Etapa 12.3, o Ralo de Sangue).
// Chuva = `invocar` círculos de raio `largura` (um onde Bill está, os outros a até `comprimento` dele),
// caindo um depois do outro; cada um acende FOGO no piso por DURACAO_FOGO (Etapa 13.3, o Pássaro).
// Salto = agacha (preparo), some no ar, marca um círculo de raio `largura` onde Bill está (AVISO_SALTO) e
// cai ali (Etapa 14.2, o Lobisomem): a mesma máquina do Mergulho, mas pelo ar e com alcance limitado.
// CruzAlvo = cruz de raio `comprimento` centrada na posição REGISTRADA de Bill (Etapa 15, o Pennywise IV).
enum class FormaPadrao : std::uint8_t { Faixa, Leque, Invocar, Linha, Investida, Circulo, Mergulho, Ondas, Nuvem, Porta,
                                        Cruz, Xis, Inundar, Golfada, Chuva, Salto, CruzAlvo };
struct PadraoChefe {
    const char* nome;
    FormaPadrao forma;
    int fase;                     // em que fase ele aparece
    float distanciaMin, distanciaMax;   // px: só escolhe se Bill estiver nessa faixa
    float preparo, recuperacao;   // s (o preparo é o aviso no chão)
    float comprimento, largura;   // px (linha/faixa/investida)
    float prende;                 // s
    int invocar;                  // quantos inimigos chama (Invocar); quantos círculos caem (Chuva)
    int peso;
    float recarga;                // s de respiro depois da janela de punição (o chefe anda até Bill)
    TipoInimigo invocado = TipoInimigo::Rato;   // quem o Invocar chama
    int seguinte = -1;            // combo: índice do padrão que vem LOGO depois (sem respiro); −1 = nenhum
};
// Padrões do chefe `tipo` (nullptr/0 para quem não é chefe).
const PadraoChefe* padroesDoChefe(TipoInimigo tipo, int& quantos);
// Nome na barra do chefe: o medo na fase 1, o Pennywise na fase 2.
const char* nomeDoChefe(const Inimigo& ini);
// Tamanho do Pennywise de cada estágio na fase 2 (1 = folha 32×48): cresce a cada encontro (24/11, Diego: as
// diferenças de poder e tamanho têm de ser claras). A caixa de dano acompanha.
float escalaDoPennywise(TipoInimigo chefe);
constexpr float DURACAO_TRANSICAO_CHEFE = 1.4f;
constexpr float AVISO_EMERGIR = 1.0f;
constexpr float AVISO_PORTA = 1.0f;       // a linha da porta até Bill, antes do bote
constexpr float DURACAO_NUVEM = 5.0f;     // PRD §10.1: a nuvem pútrida dura 5 s     // PRD §10.1: o círculo de 1,0 s antes de o Afogado emergir
constexpr float RAIO_EMERGIR = 44.0f;
constexpr float AVISO_TELEPORTE = 0.5f;     // Espectro (PRD §9): a fumaça no destino antes de aparecer
constexpr float RECARGA_TELEPORTE = 4.0f;
constexpr float DURACAO_INUNDACAO = 5.0f;   // as poças de sangue de uma inundação
constexpr float DURACAO_GOLFADA = 6.0f;     // a poça de uma golfada
constexpr float ABERTURA_QUADRANTE = 80.0f; // graus: < 90 deixa os eixos da cruz secos (corredores)
constexpr float DURACAO_FOGO = 4.0f;        // o fogo que cada brasa da chuva acende no piso (Pássaro)
constexpr float INTERVALO_CHUVA = 0.15f;    // s entre uma brasa e a seguinte: a chuva CAI, não pisca
constexpr float AVISO_SALTO = 0.8f;         // o Lobisomem no ar: o círculo onde ele vai cair (Etapa 14.2)
// Fixo: não anda, não recua com golpes nem é empurrado (o Cabelo do ralo; o Ralo de Sangue na fase 1).
bool inimigoFixo(const Inimigo& ini);
// Padrões que NÃO precisam alcançar Bill de onde o chefe está (chamam ou viajam por baixo d'água).
bool padraoSemAlcance(FormaPadrao f);

// O que a IA enxerga do mundo e onde ela registra ataques. As regras da partida montam isto.
struct ContextoIA {
    const SalaViva& sala;
    Vec2 alvo;                    // pés do Bill
    bool alvoAtivo;               // Bill vivo
    Rng& rng;
    std::vector<Aviso>& avisos;
    std::vector<Golpe>& golpes;
    std::uint32_t& proximoGolpe;
    // Chefe: quem nasce (ratos invocados) entra depois do laço da IA; os ids vêm da partida.
    std::vector<Inimigo>* nascer = nullptr;
    IdEntidade* proximoId = nullptr;
    int hostis = 0;               // quantos hostis vivos (o chefe não invoca além do limite)
    const std::vector<Inimigo>* vivos = nullptr;   // só leitura: onde os outros estão (ralos ocupados)
    // Os últimos da sala CAÇAM Bill (Etapa 8.5): quem vaga anda até ele devagar, sem atacar antes de
    // vê-lo. Sem isso, o último balão podia vagar 60 s no escuro do outro lado da sala.
    bool cacar = false;
};
constexpr int LIMITE_HOSTIS_CACA = 2;
constexpr float FATOR_VELOCIDADE_CACA = 0.6f;
constexpr float INTERVALO_CACA = 2.0f;   // s até reavaliar para onde caçar

const ParametrosInimigo& parametrosDe(TipoInimigo tipo);
Inimigo criarInimigo(TipoInimigo tipo, IdEntidade id, Vec2 pes);
// Um passo de IA: percepção, estado comum e o comportamento próprio do tipo.
void atualizarInimigo(Inimigo& ini, ContextoIA& ctx, float dt);
// Caixa que recebe dano (corpo, acima dos pés).
Retangulo hurtboxInimigo(const Inimigo& ini);
bool podeSerAtingido(const Inimigo& ini);
