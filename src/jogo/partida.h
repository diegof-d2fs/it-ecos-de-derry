#pragma once
// A partida (SDD §3.6, §11.2 e §24.7.1). TODO o estado de jogo vive aqui, nas regras, e
// `atualizarPartida` é a ÚNICA função que avança o jogo. As cenas só desenham e repassam
// comandos; por isso a mesma partida roda com janela (jogador), com o piloto automático e
// sem janela nenhuma (testes e simulação).
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "combate/golpe.h"
#include "core/entrada.h"
#include "core/rng.h"
#include "core/sons.h"
#include "entidades/inimigos.h"
#include "entidades/jogador.h"
#include "jogo/companheiro.h"
#include "jogo/dialogo.h"
#include "jogo/ecos.h"
#include "jogo/loja.h"
#include "jogo/dificuldade.h"
#include "jogo/itens.h"
#include "jogo/pontuacao.h"
#include "jogo/tentativa.h"
#include "jogo/historia.h"
#include "jogo/salvamento.h"
#include "mundo/sala.h"
#include "mundo/visao.h"

// Conteúdo lido de data/ uma vez. Regra de posse (T03): `salas` nunca cresce depois de
// carregado, então ponteiros para os modelos (SalaViva::modelo) continuam válidos.
struct Conteudo {
    std::vector<SalaModelo> salas;
    TabelaDificuldade dificuldades = tabelaDificuldadePadrao();   // data/balanceamento.txt
    std::vector<Dialogo> dialogos;   // data/dialogos.txt (nunca redimensionado: ponteiros estáveis)
    std::vector<CenaHistoria> cenas;  // data/cenas.txt: cut scenes (idem: o tocador guarda ponteiro)
};

enum class TipoTexto : std::uint8_t { DanoInimigo, Critico, DanoBill, Errou, Esquivou, Info };

// Festa dos Balões (fase bônus, SDD §24.5): balões sobem por 30 s; estourar dá moedas e XP;
// os vermelhos são armadilhas. Ninguém morre na festa (a vida para em 1).
struct BalaoFesta {
    IdEntidade id = ID_NULO;
    Vec2 pos;              // base do balão (a corda)
    float fase = 0.0f;     // balanço lateral
    bool vermelho = false;
    int cor = 0;           // 0..2: amarelo, azul, verde (só desenho)
};
constexpr float DURACAO_FESTA = 30.0f;
constexpr float INTERVALO_BALAO = 0.45f;
constexpr int CHANCE_BALAO_VERMELHO_PCT = 20;
constexpr int MOEDAS_POR_BALAO = 2, XP_POR_BALAO = 2, DANO_BALAO_VERMELHO = 6;

// "Manifestação dispersa": o Pennywise se desfazendo onde o chefe caiu (só visual).
// Covil (Etapa 15, PRD §4.5). Fase 2: a ilusão de Georgie chama Bill (puxa de leve) e fere ao toque.
struct GeorgieIlusao {
    bool ativo = false;
    Vec2 pes;
    Direcao direcao = Direcao::Baixo;
    float anim = 0.0f;
    float recarga = 0.0f;   // depois de ferir Bill, some e reaparece longe; não fere de novo por este tempo
    float chamado = 0.0f;   // até o próximo "Billy…"
};
constexpr float VELOCIDADE_GEORGIE = 34.0f;    // anda devagar até Bill
constexpr float PUXAO_GEORGIE = 26.0f;         // a voz puxa Bill de leve (px/s; o dash escapa)
constexpr float RAIO_TOQUE_GEORGIE = 20.0f;
constexpr int DANO_GEORGIE = 12;
// Fase 3: as Luzes Mortas. Um círculo por amigo resgatado gira em volta do centro da arena; fora de todos,
// Bill sofre dano contínuo (de DANO_ESCURIDAO a cada INTERVALO_ESCURIDAO).
constexpr float RAIO_LUZ_MORTA = 54.0f;
constexpr float ORBITA_LUZES = 140.0f;
constexpr float VELOCIDADE_LUZES = 0.45f;      // rad/s
constexpr int DANO_ESCURIDAO = 3;
constexpr float INTERVALO_ESCURIDAO = 0.5f;
constexpr float DURACAO_VITORIA = 3.5f;        // a Coisa se quebrando antes do resultado (e da cena final)

struct Dispersao {
    Vec2 pos;
    float vida = 1.6f;
    TipoInimigo chefe = TipoInimigo::Mumia;
};
constexpr float DURACAO_DISPERSAO = 1.6f;

// Número de dano e avisos curtos que sobem e somem (o render só desenha).
struct TextoFlutuante {
    Vec2 pos;
    std::string texto;
    TipoTexto tipo = TipoTexto::Info;
    float vida = 0.8f;
};

// Item no chão. Ao surgir (drop, baú) ele "pula" de `origem` até `pos`; só pode ser pego
// depois do pulo, para o jogador VER o que caiu.
struct ItemNoChao {
    IdEntidade id = ID_NULO;
    TipoItem tipo = TipoItem::Moeda;
    int quantidade = 1;
    Vec2 pos;
    Vec2 origem;
    float tempo = 0.0f;
};
constexpr float DURACAO_PULO_ITEM = 0.35f;
constexpr float RAIO_COLETA = 18.0f;        // px entre os pés do Bill e o item (o mínimo; ver itemEncostaNoBill)
// Coleta pelo que se VÊ (Diego, 17/11: "passei por cima e não pegou"): o corpo do Bill (48 px de
// altura) cobre o ícone muito antes de os PÉS chegarem a 18 px dele. Vale o encontro do corpo com o
// ícone, e um ímã curto puxa o item até Bill (Hades), desde que caiba no inventário e sem parede no meio.
constexpr float COLETA_LARGURA = 18.0f;     // |dx| entre os pés e o item
constexpr float COLETA_ACIMA = 26.0f;       // o item até 26 px acima dos pés (atrás das pernas)
constexpr float COLETA_ABAIXO = 20.0f;      // ou até 20 px abaixo (o corpo de Bill cobre o ícone)
constexpr float RAIO_IMA = 48.0f;
constexpr float VELOCIDADE_IMA = 260.0f;    // px/s
bool itemEncostaNoBill(Vec2 pesBill, Vec2 item);
// Sala AVULSA: Bill entra como chega na tentativa naquele bioma (nível, pontos em Força/Agilidade, kit
// e a vida máxima medida na simulação). Usada pelos testes do piloto e pelo --forte (Modo Defesa:
// cair direto numa luta do fim do jogo sem morrer no primeiro golpe).
void chegarComoNaTentativa(EstadoPartida& p);
constexpr float RAIO_INTERAGIR = 44.0f;     // px até o centro de um baú

// Brasa do rastro do Espectro: queima itens comuns que toca (SDD §24.5). Só visual para o Bill.
struct Brasa {
    Vec2 pos;
    float vida = 4.0f;
    IdEntidade dono = ID_NULO;   // o Espectro que a deixou (causa da morte, se Bill cair)
};
// Poça que o Afogado deixa (SDD §24.5, PRD §9): deixa Bill lento como água rasa. É um efeito no
// chão (não muda o terreno: o cache do render não precisa ser refeito a cada passo dele).
struct Poca {
    Vec2 pos;
    float vida = 6.0f;
};
constexpr float VIDA_POCA = 6.0f;
constexpr float INTERVALO_POCA = 0.5f;
constexpr float RAIO_POCA = 14.0f;
// Piso cedendo que alguém pisou: rachando; ao fim, vira buraco (jogo/armadilhas).
struct PisoCedendo {
    Celula celula;
    float timer = 0.0f;
};
// Nuvem pútrida do Leproso (Etapa 11.4): Bill dentro dela anda a 50% (Jogador::lento renovado).
// Poça de sangue temporária (o Ralo de Sangue, Etapa 12.3): a célula vira PocaSangue (o mesmo dano de
// terreno da armadilha 'b', que o piloto já evita) e volta ao tile original quando a vida acaba.
// O fogo da chuva de brasas do Pássaro (13.3) usa a MESMA lista, com o tile Labareda.
struct PocaTemporaria {
    Celula celula;
    Tile original;
    float vida = 0.0f;
};

// Lasca de madeira (só desenho, mas nas regras para ser determinística): sai da caixa a cada golpe e
// num estouro quando ela quebra; cai com gravidade (altura) e quica uma vez.
struct Estilhaco {
    Vec2 pos, vel;
    float altura = 0.0f, vz = 0.0f;
    float vida = 0.6f;
    int tom = 0;                      // 0..2: tons de madeira
};
// O ataque de verdade de um inimigo (só desenho, Diego 17/11): quando o golpe sai, a garra, a fuligem,
// o cabelo, a água… aparecem sobre a área que acertou. Quem atacou decide o efeito (a cena escolhe a folha).
struct EfeitoAtaque {
    Area area;
    TipoInimigo atacante = TipoInimigo::Total;
    int fase = 1;
    int padrao = -1;                  // padrão do chefe (o Pennywise IV: teia, patas, Luzes Mortas)
    bool fogo = false, nuvem = false, poca = false;
    float vida = 0.35f;
};
constexpr float DURACAO_EFEITO_ATAQUE = 0.35f;
constexpr int VIDA_CAIXA = 3;
constexpr float DURACAO_QUEIMA = 3.0f;   // Eco "Fogo controlado" (Mike)
constexpr float TREMOR_CAIXA = 0.2f;

// Traço de luz (só desenho): o tiro de prata do Estilingue e o salto do clarão do Ricochete.
struct Traco {
    Vec2 de, para;
    float vida = 0.15f;
    bool prata = false;               // prata (tiro) ou amarelo (clarão)
};

struct NuvemPutrida {
    Vec2 centro;
    float raio = 48.0f;
    float vida = 5.0f;
};
constexpr float VIDA_BRASA = 4.0f;          // PRD §14.2: brasas do Espectro, 6 de dano/s por 4 s (Etapa 13.2)
constexpr int DANO_BRASA = 6;
constexpr float INTERVALO_BRASA = 0.35f;
constexpr float RAIO_BRASA = 14.0f;
constexpr float TEMPO_LADRAO_SUMIR = 8.0f;   // rato com item fora da luz por 8 s: some com ele
constexpr float RAIO_ROUBO = 1.2f * 32.0f;   // ≤ 1 célula (com folga)

// "+ Bateria" com ícone no centro da tela (critério M09). Máx. 3, as mais novas embaixo.
struct Notificacao {
    TipoItem tipo = TipoItem::Moeda;
    int quantidade = 1;
    float vida = 2.0f;
};

// Decisão que congela o mundo até ser resolvida (SDD §24.7.1). Quem resolve é o jogador ou o
// piloto, pelos MESMOS comandos. Diálogo, Eco, Loja e Resultado entram com os seus sistemas.
// (Escolher porta NÃO é pendência: como em Hades, escolhe-se andando até a porta.)
// No Clube (Etapa 7.2): Mural (comprar talentos), Quadro (recordes) e Alcapao (dificuldade).
// Dialogo (Etapa 7.3): caixa de diálogo; o mundo congela enquanto se lê.
// EscolherEco (Etapa 8.1): 3 cartas de Eco; o mundo congela até escolher.
// Loja (Etapa 8.2): as ofertas do Elias; o mundo congela enquanto se compra.
enum class Pendencia : std::uint8_t { Nenhuma, SubirNivel, Pausa, Inventario, Mural, Quadro, Alcapao, Dialogo, EscolherEco, Loja };

// Conversa aberta. `atual` aponta para Conteudo::dialogos (carregado uma vez, nunca cresce).
struct EstadoDialogo {
    const Dialogo* atual = nullptr;
    int linha = 0;
    float letras = 0.0f;       // letras já "digitadas" da linha atual
    bool escolhendo = false;   // fim da conversa com escolhas: o cursor está nelas
};

// Menu de pausa (PRD §15.5). "Abandonar" não grava pontuação; "Sair" fecha o jogo.
// Salvar (23/11, Diego): abre o cartão de memória (cena); só na tentativa (no Clube não há o que salvar).
enum class OpcaoPausa : std::uint8_t { Continuar, Inventario, Ficha, Salvar, Abandonar, Sair, Total };
constexpr int TOTAL_OPCOES_PAUSA = static_cast<int>(OpcaoPausa::Total);
// No Clube, "Abandonar" vira "Voltar ao menu principal" (não há tentativa a perder).
const char* nomeOpcaoPausa(OpcaoPausa o, bool noClube = false);

// Abas do inventário (PRD §11.2). Itens: os que se USAM; as outras só mostram.
enum class AbaInventario : std::uint8_t { Itens, Equipamento, Missao, Ecos, Ficha, Total };
constexpr int TOTAL_ABAS = static_cast<int>(AbaInventario::Total);
const char* nomeAba(AbaInventario a);
// Itens listados (e usáveis) na aba Itens, na ordem da tela.
constexpr TipoItem ITENS_USAVEIS[] = {TipoItem::Kit, TipoItem::Barquinho, TipoItem::FichaCoragem, TipoItem::Tonico,
                                     TipoItem::Oculos};
constexpr int TOTAL_ITENS_USAVEIS = 5;

constexpr int CURA_REFUGIO_PCT = 30;   // PRD §5.2
constexpr int TOTAL_ATRIBUTOS = 4;     // ordem na tela: Força, Foco, Agilidade, Coragem

// Balão de fala de um amigo no Clube (até os diálogos completos da Etapa 7.3).
struct BalaoFala {
    int objeto = -1;           // índice em sala.objetos do NPC que fala (−1 = nenhum)
    std::string texto;
    float vida = 0.0f;
};
constexpr float DURACAO_BALAO = 4.0f;

struct EstadoPartida {
    std::uint64_t seed = 0;
    Rng rng;
    Dificuldade dificuldade = Dificuldade::Normal;
    ParametrosDificuldade ajustes;   // cópia da linha da dificuldade, fixada ao iniciar (SDD §24.2)
    // Sala atual
    int indiceSala = 0;
    SalaViva sala;
    MapaVisao visao;
    // Atores e combate
    Jogador bill;
    std::vector<Inimigo> inimigos;
    std::vector<Golpe> golpes;
    std::vector<Aviso> avisos;
    std::vector<TextoFlutuante> textos;
    // Itens (Etapa 6.2)
    Inventario inventario;
    std::vector<ItemNoChao> itensChao;
    std::vector<Notificacao> notificacoes;
    std::vector<Brasa> brasas;
    std::vector<Poca> pocas;
    std::vector<NuvemPutrida> nuvens;
    std::vector<PocaTemporaria> pocasDeSangue;
    std::vector<Traco> tracos;
    std::vector<Estilhaco> estilhacos;
    std::vector<EfeitoAtaque> efeitosAtaque;
    float recargaBrasaBill = 0.0f;   // as brasas ferem Bill no máximo 1× por segundo
    std::vector<Golpe> golpesNovos;   // nascidos durante a resolução (ricochete): entram no fim dela
    bool danoNaSala = false;          // Bill levou dano nesta sala (Eco "Escoteiro")
    bool gessoUsado = false;          // Eco "Gesso" (Eddie): 1× por sala
    bool salaIluminada = false;       // o Mike ligou o gerador (13.4): a sala inteira à vista
    bool chefeNaFase2 = false;        // Modo Defesa (--pennywise): o chefe já surge se rasgando no Pennywise
    std::vector<Vec2> nevoasDash;     // onde o Inalador soltou névoa (só desenho; some sozinho)
    std::vector<float> vidaNevoas;
    std::vector<float> recargaArmadilha;   // por célula (pregos: 1× por segundo)
    std::vector<float> timerJato;          // por célula: quanto falta para o próximo jato
    std::vector<PisoCedendo> pisosCedendo;
    bool eliteNaSala = false;         // a sala sorteou um Assombrado (sai numa das ondas)
    std::vector<Dispersao> dispersoes;   // chefe derrotado (visual)
    // Covil (Etapa 15)
    GeorgieIlusao georgie;
    bool luzesMortas = false;
    int quantasLuzes = 0;            // uma por amigo resgatado nesta tentativa (mínimo 1)
    float anguloLuzes = 0.0f;
    float tickEscuridao = 0.0f;
    float timerVitoria = 0.0f;       // > 0: o Pennywise IV caiu; ao zerar, a vitória
    // Festa dos Balões (Etapa 8.4)
    bool festaAtiva = false;
    float tempoFesta = 0.0f;
    float proximoBalao = 0.0f;
    int baloesEstourados = 0;
    std::vector<BalaoFesta> baloes;
    float esperaAvisoCheio = 0.0f;    // "Sem espaço" não repete a cada quadro
    IdEntidade proximoId = ID_JOGADOR + 1;
    std::uint32_t proximoGolpe = 1;
    float hitStop = 0.0f;          // pausa curtíssima no impacto (peso do golpe)
    // Ondas
    int ondaAtual = 0;             // 0 = ainda não começou
    float timerOnda = 0.0f;        // contagem até a próxima onda
    bool salaConcluida = false;
    // Bill insistindo numa porta fechada (pedido do Diego, 23/11): na 2ª tentativa seguida, um aviso
    // explica por que ela não abre (inimigos, chefe, festa, selada, Mike). Só regras: o piloto e os testes veem.
    int tentativasPorta = 0;       // empurrões contra porta fechada (zera depois de ESQUECER_TENTATIVAS s)
    bool empurrandoPorta = false;
    float tempoEmpurrando = 0.0f;  // segurar contra a porta também conta como nova tentativa a cada 1,2 s
    float desdeTentativa = 99.0f;
    float recargaAvisoPorta = 0.0f;
    std::string avisoPorta;        // o texto do aviso (a cena desenha enquanto tempoAvisoPorta > 0)
    float tempoAvisoPorta = 0.0f;
    // Tentativa (Etapa 6.3). Fora dela ("sala avulsa": --sala N e testes) a porta leva à
    // próxima sala do arquivo, como antes.
    bool emTentativa = false;
    Rng rngGerador;                  // separado do combate: lutar não muda o mapa da tentativa
    PlanoBioma plano;
    int etapa = 0;                   // índice em plano.etapas da sala atual
    OpcaoPorta salaPrometida;        // o que a porta prometeu (entregue ao concluir a sala)
    // Pendência (mundo congelado, tempo não conta)
    Pendencia pendencia = Pendencia::Nenhuma;
    int cursor = 0;
    AbaInventario aba = AbaInventario::Itens;
    OpcaoPausa confirmando = OpcaoPausa::Total;   // Abandonar/Sair esperando "Sim" (Total = nenhum)
    bool inventarioVeioDaPausa = false;           // fechar o inventário volta ao menu de pausa
    std::string avisoMenu;                        // "Vida cheia", "Usou: Tônico do Eddie"…
    float tempoAvisoMenu = 0.0f;
    // Fim da tentativa
    bool terminou = false;
    bool vitoria = false;
    bool fimDoConteudo = false;      // passou do último bioma que já existe (não é derrota)
    bool abandonou = false;          // saiu pelo menu: não grava pontuação (SDD §16)
    bool pedidoSair = false;         // "Sair do jogo" confirmado: a plataforma fecha a janela
    int ultimoAgressor = -1;         // TipoInimigo do último golpe que feriu Bill (telemetria)
    int kitsUsados = 0;              // telemetria / simulação
    int bonusMoedasPct = 0;          // talento Mão leve: moedas que caem valem mais
    // Ecos (Etapa 8.1): os escolhidos nesta tentativa e as ofertas do altar/porta em aberto.
    std::vector<EcoAtivo> ecos;
    std::vector<OfertaEco> ofertasEco;
    std::vector<BarreiraTijolos> barreiras;   // Ponte firme: onde o dash terminou
    bool estavaEmDash = false;
    std::vector<IdEntidade> bipBipAtingidos;   // Eco "Bip-bip" (Richie): quem este dash já feriu
    bool imitacaoUsada = false;                // Eco "Imitação" (Richie): 1× por sala
    // Cartão de memória (23/11): o ponto de ENTRADA da sala atual (o que "Salvar jogo" grava) e o pedido da
    // pausa para a cena abrir a tela dos 15 blocos.
    PontoSalvamento pontoDaSala;
    bool pedidoSalvar = false;
    bool poemaUsado = false;                  // Poema: 1× por bioma
    // Loja do Elias e manivela (Etapa 8.2).
    std::vector<OfertaLoja> loja;          // sorteada ao entrar na sala de loja
    bool manivelaEntregue = false;         // a grade do Elias já abriu (nesta tentativa)
    int assassinoAnterior = -1;            // do perfil: o Elias comenta a morte anterior
    int conversasElias = 0;
    int compras = 0;                       // telemetria: itens comprados na loja
    // Segredos (Etapa 8.4, SDD §24.5): no máximo uma parede rachada por bioma, se a chance da
    // dificuldade sorteou (senão a parede é só parede). A Lembrancinha vai para a estante.
    bool segredoAtivo = false;          // este bioma tem segredo
    bool segredoUsado = false;          // a sala do segredo já foi escolhida (a 1ª com 'r')
    bool salaTemSegredo = false;        // a sala atual guarda o segredo
    std::uint32_t lembrancinhas = 0;    // bit b = a Lembrancinha do bioma b foi achada nesta tentativa
    // Companheiro e resgates (Etapa 7.4, SDD §24.6).
    Companheiro companheiro;
    std::vector<Amigo> resgatados;   // nesta tentativa, em ordem (o último acompanha Bill)
    // Prólogo (Rua Witcham, só na 1ª vez): falar com o Mike abre a rua; a saída começa a tentativa.
    bool noPrologo = false;
    // Clube (hub, Etapa 7.2). As regras pedem; quem tem o perfil atende (jogo/clube).
    bool noClube = false;
    int filtroQuadro = 0;            // FiltroRecordes do quadro (0 Todas, 1 Jogador, 2 Computador)
    int pedidoCompra = -1;           // talento escolhido no Mural (−1 = nenhum)
    bool pedidoTentativa = false;    // alçapão confirmado: começar a tentativa em `dificuldade`
    bool pedidoMenu = false;         // pausa do Clube → voltar ao menu principal
    BalaoFala balao;
    int conversas = 0;               // quantas vezes falou com alguém nesta visita (varia a fala)
    // Diálogos e tutorial (Etapa 7.3). O perfil não entra na partida: jogo/clube copia o que
    // precisa (fala nova de cada amigo, dicas vistas) e depois recolhe o que mudou.
    EstadoDialogo dialogo;
    std::array<const Dialogo*, 10> falaNova{};   // por NPC '1'..'9': a fala nova desta visita (ou nullptr)
    std::vector<std::string> falasVistasAgora;   // ids lidos agora, a gravar no perfil
    bool dicasLigadas = false;                   // Opções → dicas (e nunca para o piloto)
    std::uint32_t dicasVistas = 0;               // bit n−1 = dica Dn já mostrada
    std::vector<int> dicasNaFila;                // pedidas e ainda não mostradas (uma por vez)
    Estatisticas stats;
    // Saída para a plataforma (consumida pela cena a cada quadro)
    std::vector<IdSom> sons;
    bool trocouDeSala = false;
};

// Sala avulsa: começa na sala `salaInicial` do arquivo (testes, --sala N).
void iniciarPartida(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, int salaInicial,
                    Dificuldade dificuldade = Dificuldade::Normal);
// Tentativa de verdade: o gerador monta o bioma 1 (ou o primeiro jogável) e Bill entra pela
// sala de entrada. Falha só se nenhum bioma tiver salas suficientes.
// `biomaInicial` > 1 (apresentação, 24/11): a tentativa começa naquele bioma, com Bill chegando como chegaria
// (nível, atributos, kit e vida: chegarComoNaTentativa) e o amigo que o acompanharia ali; segue normalmente até o Covil.
bool iniciarTentativa(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade dificuldade,
                      std::string& erro, int biomaInicial = 1);
// O amigo que acompanha Bill ao ENTRAR no bioma (o resgatado no bioma anterior; no 1º, o Mike).
Amigo companheiroAoEntrar(int bioma);
// Clube dos Perdedores (hub): a sala de tipo clube, sem inimigos, sem cronômetro, toda iluminada.
// `ultima` = dificuldade sugerida no alçapão. Falha se o conteúdo não tiver sala de Clube.
bool iniciarClube(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade ultima);
// Prólogo (PRD §4.3): a sala "hub_prologo", com o Mike esperando ("!"). As portas abrem depois
// da conversa; a saída pede a tentativa (pedidoTentativa), como o alçapão do Clube.
constexpr const char* ID_SALA_PROLOGO = "hub_prologo";
bool iniciarPrologo(EstadoPartida& p, const Conteudo& c, std::uint64_t seed, Dificuldade ultima);
// Luz da lanterna em células (Clube: a sala toda; Mike acompanhando: +1).
float raioDaLuz(const EstadoPartida& p);
// Luzes Mortas (Etapa 15): centro do círculo i (0..quantasLuzes−1) e se um ponto está dentro de algum.
Vec2 centroLuzMorta(const EstadoPartida& p, int i);
bool dentroDeLuzMorta(const EstadoPartida& p, Vec2 pos);
int indiceDoClube(const Conteudo& c);   // −1 = não há
// O Elias ao alcance (NPC "elias"), ou nullptr.
const ObjetoSala* eliasProximo(const EstadoPartida& p);
// Nome do NPC (npc=<d> <nome>) de um objeto '1'..'9' ("" se não é NPC).
const std::string& nomeDoNpc(const EstadoPartida& p, const ObjetoSala& o);
// Altar de Eco ainda aceso ao alcance, ou nullptr.
const ObjetoSala* altarProximo(const EstadoPartida& p);
// Oferece 3 Ecos do amigo do bioma (Pendencia::EscolherEco). false = não há Eco novo a oferecer.
bool abrirEscolhaDeEco(EstadoPartida& p);
// Amigo preso ao alcance numa sala da tentativa (resgate), ou nullptr.
const ObjetoSala* amigoParaResgatar(const EstadoPartida& p);
// Objeto do Clube ou do prólogo ao alcance (Mural, quadro, alçapão ou amigo), ou nullptr.
const ObjetoSala* objetoDoClubeProximo(const EstadoPartida& p);
// Abre a caixa de diálogo (Pendencia::Dialogo). false se `d` é nulo ou sem falas.
bool abrirDialogo(EstadoPartida& p, const Dialogo* d);
// Tutorial: pede a dica Dn (1..20). Só entra na fila se as dicas estão ligadas e ela nunca foi
// vista; aparece assim que não houver outro menu aberto.
void pedirDica(EstadoPartida& p, int numero);
// Aviso curto dentro de um menu (Mural: "Lembranças insuficientes"…).
void avisarNoMenu(EstadoPartida& p, const std::string& t);
// Entra numa sala; `vindoPor` (pode ser nullptr) põe Bill junto da porta do lado oposto.
void entrarNaSala(EstadoPartida& p, const Conteudo& c, int indice, const PortaSala* vindoPor);
// Um passo de lógica (1/60 s). Ordem fixa do SDD §11.2.
void atualizarPartida(EstadoPartida& p, const Conteudo& c, const EstadoEntrada& entrada, float dt);

int hostisVivos(const EstadoPartida& p);
// Põe um item no chão: pula de `origem` até `destino` (drops e baús).
void soltarItem(EstadoPartida& p, TipoItem tipo, int quantidade, Vec2 origem, Vec2 destino);
// Baú fechado ao alcance do Bill (para a dica "E: abrir"), ou nullptr.
const ObjetoSala* bauProximo(const EstadoPartida& p);
// Baú trancado: só na sala de tesouro (precisa de chave enferrujada).
bool bauTrancado(const EstadoPartida& p);
// Orçamento de inimigos de uma onda: o da sala (ou o padrão) × a porcentagem da dificuldade.
int orcamentoDaOnda(const ParametrosDificuldade& d, int orcamentoSala);
// Pendência: aplica a escolha (SubirNivel: atributo 0..3). Devolve false se não pôde.
bool resolverPendencia(EstadoPartida& p, int escolha);
void fecharPendencia(EstadoPartida& p);
// Abre o menu de pausa (Esc/Start, ou a plataforma: controle desconectado). Não faz nada se a
// partida acabou ou se já há um menu aberto.
void abrirPausa(EstadoPartida& p);
// Objetivo atual do bioma, para a aba Missão (texto curto).
std::string objetivoAtual(const EstadoPartida& p);
// Atributo `i` (0 Força, 1 Foco, 2 Agilidade, 3 Coragem) do Bill, para a tela e o piloto.
int atributoDoBill(const Jogador& j, int i);
const char* nomeAtributo(int i);
// Para onde a porta leva na tentativa (nullptr fora da tentativa ou porta sem opção).
// `descida` = true quando é a saída do bioma (depois do chefe).
const OpcaoPorta* destinoDaPorta(const EstadoPartida& p, const PortaSala& porta, bool& descida);
// Porta do Medo (T) ao alcance do Bill: devolve a célula ou {-1,-1}.
Celula portaDoMedoProxima(const EstadoPartida& p);
// O chefe-medo do bioma (1 = Múmia; Total = ainda não existe: a arena usa ondas comuns).
TipoInimigo chefeDoBioma(int bioma);
// O chefe vivo da sala (para a barra no HUD e o piloto), ou nullptr.
const Inimigo* chefeNaSala(const EstadoPartida& p);
// Pontuação neste instante (a mesma fórmula do resultado: SDD §24.3).
long long pontuacaoAtual(const EstadoPartida& p);
const Inimigo* buscarInimigo(const EstadoPartida& p, IdEntidade id);   // nullptr se não existe (T03)
