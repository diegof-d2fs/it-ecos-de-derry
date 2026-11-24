# IT: Ecos de Derry — SDD v1

**Documento de design de software** · versão 1.1 · 13/11/2026 (1.0 de 10/11; a 1.1 incorpora a **parte 2 do enunciado**: §24, com o plano §21 e a matriz §22 atualizados)
Implementa: `docs/PRD-v2-IT-Ecos-de-Derry.md`. Ordem de autoridade: (1) `instructions/T3 - Roguelike - 2026.pdf` e `instructions/T3 - Roguelike - 2026-part2.pdf`, (2) PRD v2, (3) este SDD.

---

## 0. Entradas analisadas e lacunas

| Entrada | Situação |
|---|---|
| T3 - Roguelike - 2026.pdf (3 páginas) | Lido integralmente |
| PRD-IT-Ecos-de-Derry.docx v1.0 | Lido integralmente; substituído pelo PRD v2 |
| Prompts-Assets-IT.md (P01–P55) | Lido; contrato de tiles (16 índices) mantido |
| assets/jogo/manifesto.txt + 55 assets | Conferidos visualmente (contact sheet) |
| tools/batch_images.py, export_final.py | Lidos; **o caminho `PROMPTS_MD` aponta para a raiz, mas o arquivo está em `instructions/`** → corrigir antes da próxima geração |
| Bomberman (`D:\algoritmos_prog_2\bomberman-univali-prog-2`) | CMakeLists, HANDOFF_CLAUDE.md (joystick) e trechos de bomberman.cpp consultados |
| Pesquisa-Roguelike.pdf, LEIA-ME-CLAUDE.md, `referencias/repos` | **Não estão no projeto.** As decisões abaixo não dependem deles; se aparecerem, revisar §17 |

---

## 1. Stack e build

| Item | Decisão |
|---|---|
| Linguagem | C++17 (`-std=c++17 -Wall -Wextra -pedantic`) |
| Compilador | g++ 16.2 MSYS2 UCRT64 (`C:\msys64\ucrt64\bin`), link estático (`-static`) |
| Build | CMake ≥ 3.16 + MinGW Makefiles; script `compilar.ps1` para quem não usa CMake |
| Bibliotecas do sistema | `gdi32`, `user32`, `gdiplus` (só para carregar PNG), `winmm` (tempo e áudio), `dwmapi` (`DwmFlush` para sincronizar com o compositor), `shell32` |
| Dinâmicas e opcionais | `SDL2.dll` 2.30.x x64 (oficial, zlib) e `xinput1_4.dll`, via `LoadLibraryExW` da pasta do exe |
| Sem | Engine, SDL para janela/render, DirectInput, threads de lógica, JSON, rede |

### 1.1 Alvos do CMake

```
regras (biblioteca estática)  ← src/core, src/mundo, src/entidades, src/combate, src/jogo
                                 NÃO inclui <windows.h>. Compila e testa sem janela.
ecos_de_derry (WIN32 exe)     ← src/plataforma, src/render, src/cenas, src/main.cpp + regras
testes_regras (console exe)   ← tests/*.cpp + regras
```

A separação é **imposta pelo build**: se alguém incluir `<windows.h>` em `regras`, o alvo de testes deixa de compilar no CI local. É a fronteira regra × apresentação pedida no PRD v1.

### 1.2 Pasta de entrega

```
EcosDeDerry/
  ecos_de_derry.exe
  SDL2.dll                (opcional; sem ela usa XInput)
  README-SDL2.txt         (licença zlib)
  assets/jogo/...         (PNG do contrato + manifesto.txt)
  data/salas/*.txt  data/dialogos.txt  data/tilesets.txt  data/balanceamento.txt
  saves/                  (criada em execução: perfil.txt, recordes.txt, telemetria.csv)
```

Todos os caminhos são relativos ao executável (`GetModuleFileNameW`). Se `saves/` não puder ser gravada, o fallback é `%APPDATA%\EcosDeDerry\`.

---

## 2. Arquitetura

```
┌─────────────────────────── ecos_de_derry.exe ───────────────────────────┐
│ plataforma/  janela Win32, loop, tempo, teclado, gamepad (SDL2/XInput),  │
│              foco, áudio                                                 │
│      │ produz EstadoEntrada (abstrato)            │ apresenta Canvas      │
│      ▼                                            ▲                       │
│ cenas/   máquina de cenas e overlays: menu, hub, sala, diálogo,          │
│          inventário, nível, Eco, loja, resultado (atualizar + desenhar)  │
│      │ chama regras                               │ chama render          │
│      ▼                                            │                       │
│ ┌──────────── regras (lib, sem Windows) ───────┐ render/  canvas,       │
│ │ core/ tipos, rng, tabelas, texto de dados    │   imagens, sprites,    │
│ │ mundo/ sala, colisão, visão, gerador, BFS    │   animação, texto,     │
│ │ entidades/ jogador, inimigos, chefes,        │   HUD, UI 9-slice,     │
│ │            projéteis, avisos, armadilhas     │   luz/fog              │
│ │ combate/ golpe, dano, atributos, XP          │                         │
│ │ jogo/ partida, missões, itens, ecos, loja,   │                         │
│ │       score, perfil, diálogo, tutorial       │                         │
│ └──────────────────────────────────────────────┘                         │
└──────────────────────────────────────────────────────────────────────────┘
```

**Regra de dependência:** `plataforma → cenas → (regras, render)`; `render` lê as regras por `const&`; **as regras nunca conhecem plataforma, render ou cenas**. Os dados de gameplay não contêm `HWND`, `HDC` nem `Gdiplus::*`.

### 2.1 Árvore de arquivos

```
CMakeLists.txt   compilar.ps1   README.md
src/
  main.cpp                         WinMain: cria Jogo, chama loop
  core/      tipos.h  rng.h/.cpp  config.h  tabelas.h/.cpp  leitor_texto.h/.cpp  util.h/.cpp
  mundo/     tile.h  sala.h/.cpp  carregador_salas.h/.cpp  colisao.h/.cpp
             visao.h/.cpp  caminho.h/.cpp (BFS)  gerador_tentativa.h/.cpp
  entidades/ jogador.h/.cpp  inimigos.h/.cpp  ia.h/.cpp  chefes.h/.cpp
             projeteis.h/.cpp  avisos.h/.cpp  armadilhas.h/.cpp  npcs.h/.cpp
  combate/   atributos.h/.cpp  golpe.h/.cpp  dano.h/.cpp  experiencia.h/.cpp
  jogo/      partida.h/.cpp  sala_viva.h/.cpp  itens.h/.cpp  inventario.h/.cpp
             ecos.h/.cpp  loja.h/.cpp  missoes.h/.cpp  pontuacao.h/.cpp
             perfil.h/.cpp  dialogo.h/.cpp  tutorial.h/.cpp  telemetria.h/.cpp
  render/    canvas.h/.cpp  imagem.h/.cpp  assets.h/.cpp  sprite.h/.cpp
             texto.h/.cpp  luz.h/.cpp  desenho_sala.h/.cpp  hud.h/.cpp  ui.h/.cpp
  cenas/     cena.h/.cpp  cena_menu.cpp  cena_prologo.cpp  cena_hub.cpp
             cena_sala.cpp  cena_resultado.cpp  cena_epilogo.cpp
             overlay_dialogo.cpp  overlay_inventario.cpp  overlay_nivel.cpp
             overlay_eco.cpp  overlay_loja.cpp  overlay_pausa.cpp  overlay_mapa.cpp
  plataforma/ janela.h/.cpp  entrada.h/.cpp  gamepad.h/.cpp  tempo.h/.cpp  audio.h/.cpp
data/
  salas/  b0_*.txt (prólogo/hub)  b1_*.txt … b6_*.txt  b7_covil.txt
  tilesets.txt  dialogos.txt  balanceamento.txt
tests/
  teste.h (macros CHECK)  main_testes.cpp  t_colisao.cpp  t_combate.cpp  t_xp.cpp
  t_visao.cpp  t_ia.cpp  t_salas.cpp  t_gerador.cpp  t_itens.cpp  t_score.cpp
  t_perfil.cpp  t_chefes.cpp  t_missoes.cpp
docs/  PRD-v2…  SDD-v1…  HANDOFF.md  APRENDIZADOS.md
tools/ (Python de produção de assets; fora da entrega)
```

Estilo de código (decisão para a defesa): **structs + funções livres** em namespaces por módulo; **sem herança nem métodos virtuais**; `enum class`; nomes e comentários em PT-BR; funções de no máximo ~60 linhas; `const&` para leitura e `&` para alteração; ponteiro só quando o "não encontrado" (`nullptr`) é um resultado válido.

---

## 3. Tipos fundamentais (T01)

```cpp
// core/tipos.h
struct Vec2       { float x = 0, y = 0; };
struct Celula     { int col = 0, lin = 0; };
struct Retangulo  { float x = 0, y = 0, w = 0, h = 0; };
using  IdEntidade = uint32_t;           constexpr IdEntidade ID_NULO = 0;
enum class Direcao : uint8_t { Baixo, Esquerda, Direita, Cima };   // = linhas dos atlas
constexpr int   TILE = 32;
constexpr float DT   = 1.0f / 60.0f;    // passo fixo da lógica
constexpr int   LARGURA_LOGICA = 640, ALTURA_LOGICA = 360;
```

### 3.1 Mundo

```cpp
// mundo/tile.h
enum class TipoTile : uint8_t {
  Vazio, Chao, Parede, AguaRasa, AguaFunda, Buraco, Lama, Detrito, PortaSala, PortaTrancada,
  Grade, Ponte, Ilusao, Espinhos, PisoCedendo, Jato, Fogo, Esteira, PocaSangue, Refugio };
struct Tile {
  TipoTile tipo = TipoTile::Vazio;
  uint8_t  indiceArte = 0;      // índice 0..15 no tileset do bioma (ou objeto)
  uint8_t  estado = 0;          // porta aberta, ponte baixada, armadilha disparada…
  bool bloqueiaMovimento = false;
  bool bloqueiaVisao     = false;
  bool bloqueiaVoo       = false;   // só parede/porta: a sanguessuga sobrevoa água e buraco
};
struct CelulaVisao { bool explorada = false; uint8_t luz = 0; }; // luz 0..255 neste quadro

constexpr int SALA_MAX_COLS = 40, SALA_MAX_LINS = 24;
enum class TipoSala : uint8_t { Entrada, Combate, Eco, Tesouro, Loja, Refugio, Resgate, Chefe, Prologo, Clube, Covil };
enum class Recompensa : uint8_t { Nenhuma, Eco, Moedas, Lembrancas, Kit, Bateria, ChaveEnferrujada, MelhoriaArma, PontoAtributo, ChaveDoMedo };

struct PortaSala { Celula celula; Direcao lado; Recompensa recompensa; int destinoIndice; bool aberta; };
struct PontoSpawn { Celula celula; bool aquatico; };
struct ObjetoSala { char codigo; Celula celula; uint8_t estado; int idNpc; };   // baú, alavanca, NPC, altar…

struct SalaModelo {                // lido do arquivo; IMUTÁVEL depois de carregado
  std::string id, nome; int bioma = 0; TipoSala tipo = TipoSala::Combate;
  int cols = 0, lins = 0;
  std::array<Tile, SALA_MAX_COLS * SALA_MAX_LINS> tiles{};
  Celula entradaJogador;
  std::vector<PortaSala> portas;  std::vector<PontoSpawn> spawns;  std::vector<ObjetoSala> objetos;
  int ondas = 0, orcamentoOnda = 0;
};
struct SalaViva {                  // cópia mutável durante a tentativa
  const SalaModelo* modelo = nullptr;              // T03: ponteiro para dado imutável que não se move
  std::array<Tile, SALA_MAX_COLS * SALA_MAX_LINS> tiles{};
  std::array<CelulaVisao, SALA_MAX_COLS * SALA_MAX_LINS> visao{};
  std::vector<PortaSala> portas; std::vector<ObjetoSala> objetos;
  int ondaAtual = 0; bool concluida = false; bool visaoSuja = true;
};
```

`SalaModelo` fica num `std::vector` carregado **uma vez** e que **nunca é redimensionado depois do carregamento**. Por isso `SalaViva::modelo` pode guardar o ponteiro com segurança, regra que fica documentada em `carregador_salas.h`.

### 3.2 Atores

```cpp
// combate/atributos.h
struct Atributos { int forca = 1, foco = 1, agilidade = 1, coragem = 1; };
struct StatsDerivados {            // recalculado por recalcularStats(); nunca editado à mão
  int hpMax, armadura, danoBase; float acerto, critico, esquiva, velocidade, recargaDash;
  int danoClarao, cargasMaxLanterna, cargasDash; float raioVisao;
};

// entidades/jogador.h
enum class EstadoJogador : uint8_t { Livre, Atacando, Especial, Dash, Clarao, Ferido, Morto };
struct Jogador {
  Vec2 pos; Vec2 vel; Direcao dir = Direcao::Baixo; EstadoJogador estado = EstadoJogador::Livre;
  Atributos attr; StatsDerivados stats; int hp = 0; int nivel = 1, xp = 0, pontosPendentes = 0;
  int pactos = 1, cargasLanterna = 3, cargasDash = 1;
  float timerEstado = 0, timerInvulneravel = 0, recargaDash = 0, recargaEspecial = 0;
  int comboIndice = 0; float janelaCombo = 0; uint32_t idGolpeAtual = 0;
  Celula ultimaCelula;                       // para contar M (movimentos)
  float medo = 0;                            // P1
};

// entidades/inimigos.h
enum class TipoInimigo : uint8_t { Balao, Rato, Sombra, Sanguessuga, Afogado, Boneco, Cabelo, Espectro, Total };
enum class EstadoIA : uint8_t { Vagando, Perseguindo, Buscando, Preparando, Atacando, Recuperando, Fugindo, Atordoado, Morrendo };
struct ParametrosInimigo {          // tabela de dados (core/tabelas.cpp + data/balanceamento.txt)
  const char* nome; int hp, dano, armadura, agilidade; float velocidade; int xp; float deteccaoCelulas;
  float tempoPreparo, alcanceAtaque; bool voa; Retangulo hurtbox; int custoOnda; const char* atlas;
};
struct Inimigo {
  IdEntidade id; TipoInimigo tipo; Vec2 pos; Vec2 vel; Direcao dir;
  int hp; EstadoIA estado; float timer; Vec2 alvoRegistrado; Celula ultimaVista; float tempoSemVer;
  std::vector<Celula> caminho; float recalcCaminho; uint32_t ultimoGolpeRecebido;
  bool elite = false; bool invocado = false; bool vivo = true; float animTempo = 0;
};
```

### 3.3 Combate, projéteis e avisos

```cpp
enum class Equipe : uint8_t { Jogador, Hostil, Neutro };
enum class FormaArea : uint8_t { Retangulo, Circulo, Linha, Cruz, Arco };
struct Area { FormaArea forma; Vec2 centro; Vec2 direcao; float raio, comprimento, largura, anguloArco; };
struct Golpe {                      // instância de dano em curso
  uint32_t id; Equipe equipe; IdEntidade origem; Area area; int dano; bool rolaAcerto, podeCritico;
  float recuo; float atordoa; float duracao;
};
struct Aviso {                      // telegraph: só visual + dispara um Golpe ao fim
  uint32_t id; Area area; float tempoTotal, tempoRestante; Golpe golpeAoFim; IdEntidade origem;
};
struct Projetil { uint32_t id; Equipe equipe; Vec2 pos, vel; float raio, vida; int dano; bool perfura; int sprite; };
struct ResultadoGolpe { bool acertou, esquivou, critico; int danoAplicado; bool matou; };
```

### 3.4 Chefes

```cpp
enum class TipoChefe : uint8_t { Mumia, Afogado, Leproso, Ralo, Passaro, Lobisomem, PennywiseFinal };
enum class EtapaPadrao : uint8_t { Aproximar, Preparar, Executar, Recuperar };
struct PassoPadrao { EtapaPadrao etapa; float duracao; FormaArea forma; float alcance; int danoExtra; int invocar; };
struct PadraoChefe { const char* nome; std::vector<PassoPadrao> passos; int faseMinima; float peso; };
struct Chefe {
  TipoChefe tipo; IdEntidade id; Vec2 pos; int hp, hpMax; int fase = 1; int estagioPennywise;
  int padraoAtual = -1, passoAtual = 0; float timer = 0; Vec2 alvoRegistrado;
  bool corpo2x2 = false; bool transicaoFeita[3] = {}; bool vulneravel = true; bool derrotado = false;
};
```

**Implementado (16/11, Etapa 8.3)** de forma mais simples que o `Chefe` acima: o chefe é um `TipoInimigo` (`ParametrosInimigo::chefe`) com `fase`, `padrao` e `transicao` no `Inimigo`, a tabela `PADROES_MUMIA` (`PadraoChefe`) e a máquina `decidirChefe` na tabela de comportamentos. Golpes, avisos, XP e piloto são os dos inimigos. Regra de dados testada: distância máxima de escolha ≤ alcance do golpe + **4 px** (era + 20: a "Mão estendida" saía de onde não alcançava e o chefe a repetia sem andar); recuperação ≥ 0,6 s. **Etapa 8.5:** `PadraoChefe` ganhou `recarga` (respiro por padrão, antes fixo no código), `invocado` (quem o Invocar chama) e `seguinte` (combo: o 2º padrão começa sem respiro); `FormaPadrao::Circulo` (anel em volta do chefe). Tabela da Múmia: 8 padrões (fase 1: Faixa, Leque, Ratos, **Redemoinho**; fase 2: Mão estendida, **Aproximação → Mão estendida**, **Gargalhada**, **Balões vermelhos**). O chefe só escolhe um ataque com o **segmento reto livre** até Bill (`segmentoLivre`, `mundo/visao`); sem ele, contorna o pilar.

Um chefe é **dados (lista de padrões) + uma máquina de estados genérica** (`atualizarChefe`). Os 7 chefes não são 7 engines; o que muda é a tabela de padrões e um `switch` pequeno para os efeitos especiais (mergulho, inundação, Georgie, Luzes Mortas).

### 3.5 Itens, Ecos e inventário

```cpp
enum class TipoItem : uint8_t { Kit, Bateria, FichaCoragem, Barquinho, Escudo, MelhoriaArma, ChaveEnferrujada, ChaveDoMedo, Manivela, Moeda, Lembranca, Pacto, Total };
struct DefinicaoItem { const char* nome; const char* descricao; int limite; bool missao; int icone; int preco; };
struct ItemNoChao { IdEntidade id; TipoItem tipo; int quantidade; Vec2 pos; bool coletado; };
struct Inventario {
  std::array<int, (int)TipoItem::Total> quantidades{};
  int arma = 0; bool escudoEquipado = false; int melhoriasArma = 0; int moedas = 0;
  float fichaCoragemRestante = 0;
};

enum class Amigo : uint8_t { Ben, Stan, Eddie, Beverly, Mike, Richie, Total };
enum class Raridade : uint8_t { Comum, Raro, Epico, Lendario };
enum class EfeitoEco : uint8_t { ArmaduraFixa, HpMax, DanoContraParede, BarreiraDash, Poema, MarcaPassaros,
  MoedasSemDano, InvulDash, CriticoAtordoado, DashDuplo, NevoaDash, BonusKit, Gesso, Velocidade, CuraResgate,
  AcertoFixo, DanoChefe, RicocheteClarao, EspecialProjetil, CriticoAtordoa, BonusTerceiroGolpe, BonusXp,
  ClaraoQueima, ResistenciaFogo, RaioVisao, EspecialAtordoaArea, ContornoInimigos, AcertoInimigoMenos,
  DanoDash, Imitacao };
struct DefinicaoEco { Amigo amigo; const char* nome; EfeitoEco efeito; float valor[4]; const char* texto; };
struct EcoAtivo { int definicao; Raridade raridade; };
```

**Implementado (16/11, Etapa 8.2)** em `jogo/loja`: `montarLoja`, `comprar` (→ `ResultadoCompra`), `falaDoElias`; o Elias abre a grade com a manivela (toda `Grade` da sala vira `Chao`). A manivela cai com a Chave do Medo no bioma 1; a grade guarda um baú (bônus), nunca a saída, porque a loja é opcional.

**Implementado (16/11, Etapa 8.1)** em `jogo/ecos`: tabela `ECOS[]` com valores inteiros por raridade, `sortearEcos`, `valorEco`, `aplicarEco`; os passivos passam por `Equipamento::ecoArmadura/ecoHp` (somados em `calcularStats`), em vez de `recalcularStats(jogador, ecos, …)`. Por enquanto, os Ecos do Ben e o lendário Poema; os outros amigos entram com os biomas deles. "Ponte firme": a barreira fica onde o dash termina e segura qualquer golpe hostil em quem está dentro dela (não há projéteis separados de áreas).

**Como os Ecos alteram as regras:** nenhum Eco tem código espalhado. `recalcularStats(jogador, ecos, inventario, perfil)` soma os efeitos passivos nos `StatsDerivados`; os efeitos de evento (ao dar dash, ao acertar, ao ser atingido) são consultados por `valorEco(partida, EfeitoEco)` nos pontos de evento. Assim é fácil explicar e alterar.

### 3.6 Partida e perfil

```cpp
struct Estatisticas { int K=0, E=0, I=0, R=0, B=0, C=0, V=0, M=0, U=0; float tempo=0; int lembrancasGanhas=0; };
struct EtapaTentativa { int bioma; int indiceModelo; TipoSala tipo; Recompensa recompensa; };
struct Mensagem { std::string texto; float vida; };

struct EstadoPartida {
  uint64_t seed; Rng rng;
  int bioma = 1; std::vector<EtapaTentativa> sequenciaBioma; int indiceEtapa = 0;
  SalaViva sala; Jogador jogador; Inventario inventario; std::vector<EcoAtivo> ecos;
  std::vector<Inimigo> inimigos; std::vector<Projetil> projeteis; std::vector<Aviso> avisos;
  std::vector<Golpe> golpes; std::vector<ItemNoChao> itensChao;
  std::optional<Chefe> chefe;
  std::bitset<64> flagsMissao;       // ex.: MANIVELA_ENTREGUE, RESGATE_BEN, CHEFE_1_DISPERSO…
  std::array<bool,(int)Amigo::Total> resgatados{};
  Estatisticas stats; std::deque<Mensagem> mensagens;  // máx. 3
  IdEntidade proximoId = 1; uint32_t proximoGolpe = 1;
  float hitStop = 0; bool terminou = false; bool vitoria = false;
};

struct Perfil {                      // salvo em saves/perfil.txt
  int versao = 1; int lembrancas = 0; std::array<int,16> talentos{};
  std::bitset<8> armasDesbloqueadas{1}; std::bitset<32> dicasVistas; std::set<std::string> falasVistas;
  int tentativas = 0, mortes = 0, vitorias = 0; std::array<int,(int)Amigo::Total> resgatesTotais{};
  int ultimoAssassino = -1; bool prologoVisto = false;
  Opcoes opcoes;                     // escala, dicas, modoCoragem, volume, cronometro, tremor
};

struct Jogo {                         // raiz; vive em main.cpp (dono de tudo)
  Perfil perfil; EstadoPartida partida; Conteudo conteudo; // conteudo = salas, diálogos, tabelas
  PilhaCenas cenas; EstadoEntrada entrada; Recursos* recursos; // recursos = assets do render
  bool modoDefesa = false; bool rodando = true;
};
```

---

## 4. Loop principal e tempo

```
WinMain → criarJanela → inicializarGdiPlus → carregarRecursos → carregarConteudo → carregarPerfil
loop:
  processar mensagens Win32 (PeekMessage)
  se sem foco: pausar (empilhar overlay de pausa), limparEntrada, Sleep(16), continuar
  acumulador += min(agora - anterior, 0,25 s)
  lerGamepad → montarEstadoEntrada (bordas "pressionou neste quadro")
  enquanto acumulador >= DT e passos < 5:  atualizarCenas(jogo, entrada, DT); limparBordas; acumulador -= DT
  desenharCenas(jogo, canvas) → apresentar(janela, canvas) → DwmFlush()
```

- **Passo fixo de 1/60 s**: a lógica é idêntica em qualquer máquina e os testes podem simular N passos.
- **Hit-stop**: `partida.hitStop > 0` congela atores e avisos, mas não a interface.
- `timeBeginPeriod(1)` ao iniciar e `timeEndPeriod(1)` ao sair.
- Medição: o tempo de cada quadro vai para um anel de 240 amostras; F1 no Modo Defesa mostra a média e o p95 (critério F01).

---

## 5. Cenas e overlays (T03: ponteiros para função)

```cpp
// cenas/cena.h
enum class IdCena : uint8_t { Menu, Prologo, Hub, Sala, Resultado, Epilogo, Creditos,
  Dialogo, Inventario, Nivel, EscolhaEco, Loja, Pausa, Mapa, Confirmacao, Total };
struct DefinicaoCena {
  const char* nome; bool modal;            // modal = congela as cenas de baixo
  void (*entrar)(Jogo&);
  void (*atualizar)(Jogo&, const EstadoEntrada&, float dt);
  void (*desenhar)(const Jogo&, Canvas&);
};
extern const DefinicaoCena TABELA_CENAS[(int)IdCena::Total];
struct PilhaCenas { std::array<IdCena, 8> itens; int topo = 0; };
void empilharCena(Jogo&, IdCena);  void desempilharCena(Jogo&);  void trocarCenaBase(Jogo&, IdCena);
```

- O desenho percorre a pilha de baixo para cima; a atualização roda só no topo (e nas de baixo, se o topo não for modal).
- Diálogo, Inventário, Nível, Eco, Loja, Pausa e Mapa são **modais**: **a lógica da sala congela** (PRD: "inimigos não agem enquanto o jogador lê").

### 5.1 Fluxo

```
Menu ─Novo/Continuar→ (prologoVisto? Hub : Prologo) ─→ Hub ─alçapão→ Sala ⇄ (overlays)
Sala ─HP 0 sem pacto→ Resultado(derrota) → Hub
Sala(Covil) ─vitória→ Resultado(vitória) → Epilogo → Creditos → Hub
```

---

## 6. Entrada (plataforma/entrada, plataforma/gamepad)

```cpp
enum class Comando : uint8_t { Cima, Baixo, Esquerda, Direita, Atacar, Especial, Clarao, Dash,
  Interagir, UsarKit, Inventario, Ficha, Mapa, Pausa, Confirmar, Voltar, AbaEsq, AbaDir, Total };
enum class Dispositivo : uint8_t { Teclado, Gamepad };
struct EstadoEntrada {
  std::array<bool,(int)Comando::Total> segurando{}, pressionou{}, soltou{};
  Vec2 eixo;                     // -1..1, já com zona morta; soma do teclado e do analógico, normalizada
  Dispositivo ultimoUsado = Dispositivo::Teclado; bool gamepadConectado = false; char nomeGamepad[64]{};
};
```

- **Teclado**: `WM_KEYDOWN`/`WM_KEYUP` → tabela tecla→comando (`TABELA_TECLAS`, editável). Auto-repetição ignorada (`lParam bit 30`), para que segurar uma tecla não acumule ações.
- **Gamepad**, ordem de tentativa (lição do Bomberman):
  1. `SDL2.dll` da pasta do exe: `SDL_Init(SDL_INIT_GAMECONTROLLER)` com os hints `SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS=1` (sem janela SDL; o foco é filtrado pelo jogo — lição 43) e `SDL_GAMECONTROLLER_USE_BUTTON_LABELS=0` (posição, não rótulo — lição 42), `SDL_GameControllerOpen`, leitura por estado (`SDL_GameControllerUpdate` + `GetButton/GetAxis`) a cada quadro. Botões **por posição** (`SDL_CONTROLLER_BUTTON_A` = sul).
  2. `XInputGetState` (`xinput1_4.dll`, fallback `xinput9_1_0.dll`), porta 0–3.
  3. Nenhum: só teclado.
- Zona morta radial de 0,25, com reescala para 0..1; o direcional digital tem prioridade sobre o analógico.
- **Hotplug**: com controle conectado, verificar a desconexão a cada quadro (barato); sem controle, procurar um novo **no máximo a cada 2 s** (a busca é cara).
- Desconectar na Sala empilha `Pausa` com a mensagem "Controle desconectado".
- Sem foco: zerar tudo e ignorar o gamepad.

---

## 7. Render

### 7.1 Canvas e apresentação

```cpp
struct Imagem { int w = 0, h = 0; std::vector<uint32_t> px; };          // ARGB pré-multiplicado
struct Canvas { int w, h; uint32_t* px; HDC dcMemoria; HBITMAP dib; };  // px aponta para a DIB section
```

- O canvas é uma **DIB section top-down de 32 bits, 640×360**, criada uma vez. Desenhar = escrever em `px`.
- O texto usa GDI `TextOutW` no `dcMemoria` da própria DIB, com fonte `NONANTIALIASED_QUALITY` (pixelada e com acentos). As fontes ficam em cache (lição do Bomberman).
- **Apresentação**: `StretchBlt(dcJanela, x0, y0, 640·k, 360·k, dcMemoria, 0, 0, 640, 360, SRCCOPY)` com `SetStretchBltMode(COLORONCOLOR)` (vizinho mais próximo), `k` = maior inteiro que cabe; as bordas são pintadas de preto **só quando o tamanho muda** (sem piscar). Se a área cliente for menor que 640×360: mostrar aviso e pausar.
- `WM_ERASEBKGND` retorna 1 (evita piscar); o desenho acontece só no loop, nunca em `WM_PAINT`, exceto em `ValidateRect`.

### 7.2 Imagens e sprites

- **GDI+ apenas no carregamento**: `Gdiplus::Bitmap::FromFile` → `LockBits(PixelFormat32bppPARGB)` → copiar para `Imagem` → liberar o Bitmap. Nenhum `Gdiplus::Graphics` por quadro (lição do Bomberman: era o gargalo).
- Funções do canvas: `limpar`, `retanguloCheio`, `retanguloAlpha`, `desenharImagem(canvas, img, origem, x, y, flags)` com espelhar e tint/alpha global, `desenharTexto`, `desenharNineSlice`, `linha`, `circuloAlpha`.
- `blit` com corte (clipping) e caminho rápido para alfa 0/255.
- `Sprite { const Imagem* atlas; int frameW, frameH, cols, linhas; Vec2 pivo; }` vem do `manifesto.txt` (lido por `carregarManifesto`). `Animacao { int linha, primeiro, quantidade; float fps; bool loop; }`.

### 7.3 Assets e escala de render (decidido em 10/11/2026)

| Item | Decisão |
|---|---|
| Fonte | `assets/jogo_hd/` (4× o contrato; ilustrações 2×). Se não existir, usa `assets/jogo/` (1×). |
| Manifesto | Medidas LÓGICAS + campo `fator` opcional (`core/manifesto`) |
| Canvas | 640·k × 360·k pixels de tela, com k = escala inteira da janela (máx. 6). API do canvas em coordenadas **lógicas** (float), convertidas por `paraTela`. |
| Preparação | `prepararEscala(recursos, k)`: reduz cada FRAME por média de área (`core/pixels::reamostrar`, testado) e recria as fontes em altura × k. Só roda quando k muda. |
| Texto | GDI em resolução de tela; antialias a partir de 2× |
| Desempenho medido | p95 de 2,4 ms em 2× e 3,2 ms em 3× (1920×1080) |

Masters, `_raw`, `_rejeitadas` e `_backup` não entram na entrega.

### 7.3.1 Cenários por texturas contínuas (decidido em 10/11/2026)

- Cada ambiente tem 5 assets `tex_<ambiente>_<material>` (128×128 lógicos, 4×4 frames; HD 512×512): `piso`, `piso2`, `parede_face`, `parede_topo`, `liquido`. Ambientes: `rua_derry`, `clube`, `galeria`, `barrens`, `neibolt`, `poroes`, `siderurgica`, `cisterna`, `covil`.
- **Regra de desenho:** a célula (c, l) usa o frame (c mod 4, l mod 4) da textura do seu material, e o material emenda na sala inteira. Parede com chão logo abaixo → `parede_face`; demais paredes → `parede_topo` (vista 3/4).
- **Transições por código:** sombra em degradê ao pé da face e nas laterais da parede, e degrau escuro com linha de espuma na borda da água (já na vitrine). **Na Etapa 3:** pré-renderizar o terreno da sala num cache (uma vez por sala ou alteração) e aplicar bordas orgânicas (máscara de ruído) entre materiais (piso ↔ piso2 ↔ líquido), que hoje ainda são retas.
- **Legibilidade:** os pisos devem ser calmos (pouco ruído) e **sem vermelho** (reservado a perigo). Se um piso competir com os personagens, escurecer o terreno no cache (fator por ambiente em `data/tilesets.txt`).

### 7.4 Ordem de desenho de uma sala

1. Chão e água (cache da sala pré-renderizado em `Imagem` quando a sala muda: `invalidarCacheSala`)
2. Objetos baixos, armadilhas e **avisos de ataque** (contorno, hachura e ícone ⚠, com alfa pulsando suavemente)
3. Itens no chão
4. Atores **ordenados pelo y dos pés** (inimigos, NPCs, chefe, Bill), sombras ovais embaixo
5. Projéteis e efeitos
6. Parte superior das paredes (camada que cobre pés atrás da parede)
7. **Luz e escuridão** (§7.5)
8. Números de dano e balões "!" dos NPCs
9. HUD → overlays

A câmera segue Bill com amortecimento (lerp 0,15 por quadro) e é limitada às bordas da sala; nas salas menores que a tela, a sala fica centralizada. **Posições de desenho arredondadas para pixel inteiro** (evita tremor de 1 px entre tiles: critério F02).

### 7.5 Luz e escuridão (F11)

- `atualizarVisao(sala, origem, raio)` (regras): shadowcasting simétrico em 8 octantes a partir da célula de Bill; marca `luz` (0..255, com queda suave no raio) e `explorada = true`.
- Render: mapa de luz por célula → **interpolação bilinear** por pixel (lanterna com borda suave) → multiplicar a cor. Célula explorada fora da luz: fator 0,35. Desconhecida: preto.
- Os atores só são desenhados se `luz da célula dos pés > 0`, e é a regra que decide isso, não o render: `estaVisivel(sala, pos)`.
- Custo: 230 mil pixels × 1 multiplicação ≈ 1 ms com `-O2`.

---

## 8. Salas: formato de arquivo e validação

### 8.1 Formato `data/salas/b1_c03.txt`

```
id=b1_c03
nome=Galeria — Cruzamento dos canos
bioma=1
tipo=combate
ondas=2
orcamento=8
# legenda do bioma em data/tilesets.txt; linhas do mapa a seguir
mapa:
########################
#......~~~~......s.....#
#..s...~~~~.....##.....#
N......====..d..##.....L
#..........J...........#
#....X.........s.......#
#####......E......######
######.....S.....#######
```

**Legenda comum** (`data/tilesets.txt` define o índice de arte de cada símbolo, por bioma):

| Símbolo | Significado | Símbolo | Significado |
|---|---|---|---|
| `#` | parede | `.` | chão (variação de arte aleatória e determinística por posição) |
| `,` | chão molhado/variante | `~` | água rasa (−30% velocidade) |
| `w` | água funda (bloqueia, exceto voo) | `o` | buraco |
| `=` | ponte (estado: levantada/baixada) | `d` | detrito frágil |
| `g` | grade (Elias) | `i` | parede ilusória (Richie) |
| `X` | piso cedendo | `J` | jato (direção pelo vizinho livre) |
| `^` | espinhos/pregos | `m` | lama |
| `f` | grelha de fogo | `>` `<` `v` `A` | esteiras |
| `b` | poça de sangue | `R` | refúgio (área segura) |
| `N` `S` `L` `O` | porta Norte/Sul/Leste/Oeste da sala | `T` | porta trancada (Chave do Medo) |
| `E` | entrada do jogador | `s` | ponto de spawn (`a` = aquático) |
| `K` | Chave do Medo | `k` | chave enferrujada |
| `B` | baú | `c` | caixa quebrável |
| `@1`…`@9` | NPC por ID (depois do mapa: `npc=@1 ben`) | `*` | altar de Eco / objeto especial |

### 8.2 Validação (`validarSala`, testada em `t_salas.cpp` para **todas** as salas)

1. Dimensões dentro do máximo; borda fechada por parede ou porta.
2. Exatamente 1 `E`; as portas e os NPCs têm vizinho caminhável.
3. **BFS** (4 vizinhos, tratando portas trancadas e grades como passáveis no "estado final") a partir de `E` alcança todas as portas, chaves, baús e NPCs.
4. Nenhum spawn a menos de 4 células de `E`.
5. Sala de combate: ≥ 4 spawns; sala de resgate: tem `T`; sala da chave: tem `K` alcançável **sem** passar por `T`.

---

## 9. Geração da tentativa

```cpp
std::vector<EtapaTentativa> gerarSequenciaBioma(const Conteudo&, int bioma, Rng&, const Perfil&);
```

1. Embaralhar (Fisher–Yates com o `Rng` da partida) os modelos de combate do bioma; pegar 3.
2. Montar: `Entrada → C1 → [escolha] → C2|Especial → C3(chave) → [Loja|Refúgio] → Resgate → Chefe`.
3. Nas bifurcações, a sala atual recebe **2 portas** com recompensas sorteadas pela tabela de pesos do bioma (sem repetir o mesmo tipo nas duas). A sala C3 carrega `ChaveDoMedo` como recompensa fixa.
4. Garantias: a sala de resgate vem depois da chave; há pelo menos 1 Eco por bioma (forçado se o sorteio não trouxer); o número de salas vem de `balanceamento.txt` (`salas_combate_por_bioma=3`).

**Formato de sala (Etapa 10.4):** `ao_resgatar_enfeite=<col> <lin> <quadro>` troca o quadro de um enfeite no resgate (a comporta do Stan); o validador exige um enfeite naquela célula. **Portas mínimas (10.1):** entrada 2, resgate 2, chefe 2, demais 3 (`minimoDePortas`).

**Etapa 8.5 (17/11):** o plano passou a ter **8 etapas**: `Entrada → C×2 → [C | Especial] → C(chave) → C×2 → [Loja | Refúgio] → Resgate → Chefe` (PRD §18: 3 combates + especial; 3–4 combates conforme a escolha). O orçamento de cada onda escala **+10% por etapa** e **+15% por onda** dentro da sala (`ESCALADA_ETAPA_PCT`, `ESCALADA_ONDA_PCT` em `jogo/partida.cpp`). Banco do bioma 1: 7 salas de combate (`b1_c01..c05` + as 2 de teste), sem repetir num plano. Quando restam ≤ 2 hostis, os que vagam **caçam** Bill (`ContextoIA::cacar`, 60% da velocidade, só atacam depois de vê-lo).

**Implementado na Etapa 6.3 (16/11)** em `jogo/tentativa`: `gerarBioma(salas, bioma, rng, pesoKitPct, PlanoBioma&)`; a 1ª etapa de combate também é bifurcação (2 combates com recompensas diferentes). O gerador usa `EstadoPartida::rngGerador` (seed ⊕ constante), separado do Rng do combate. Bioma sem sala de entrada = não jogável; depois do último bioma jogável, a partida termina como "fim do conteúdo".

**Rng**: `xoshiro128**` com seed de 64 bits (`splitmix64` para iniciar). A seed aparece no resultado; o Modo Defesa permite fixá-la. **Nunca** usar `rand()` nem o tempo no meio da partida.

---

## 10. Física e colisão (F05, F06)

- Bill: hitbox dos pés de **18×10 px** centrada no pivô; inimigos 20×12; chefes 2×2 com 52×28.
- `moverComColisao(const SalaViva&, Vec2& pos, Retangulo caixa, Vec2 delta, bool voa)`: move **eixo X e depois eixo Y**, em sub-passos de no máximo 8 px; em colisão, recua até a borda do tile. O resultado é um **deslizamento** natural nas paredes. Testa os tiles que `bloqueiaMovimento` (ou `bloqueiaVoo`, se `voa`).
- Entidades sólidas (NPC, chefe, baú): colisão AABB contra a lista da sala. **Inimigos não se sobrepõem**: separação suave entre eles (empurrão mútuo limitado) e nunca atravessam Bill.
- Dash: mesmo `moverComColisao` (o dash **não atravessa paredes**); atravessa inimigos e projéteis apenas por estar invulnerável.
- Contagem de **M**: quando `celulaDe(pés)` muda, `stats.M++`.
- Teste-chave: andar na diagonal contra uma quina não atravessa nem trava; velocidade alta (dash) contra parede fina de 1 tile não atravessa (sub-passos).

---

## 11. Combate (F12, F17)

### 11.1 Funções

```cpp
StatsDerivados calcularStats(const Atributos&, const Inventario&, const std::vector<EcoAtivo>&, const Perfil&);
float  chanceAcerto(int focoAtacante, int agilidadeAlvo, float bonus);     // limitar 60..98
int    danoEfetivo(int dano, int armaduraAlvo);                            // máx(1, dano-armadura)
ResultadoGolpe resolverGolpe(EstadoPartida&, const Golpe&, IdEntidade alvo);  // passos 1-7 do PRD §8.1
void   aplicarDanoJogador(EstadoPartida&, int dano, Vec2 origem);          // i-frames, pacto, derrota
void   aplicarDanoInimigo(EstadoPartida&, Inimigo&, int dano, Vec2 origem, float recuo);
void   matarInimigo(EstadoPartida&, Inimigo&);                             // XP, drop, K, mensagem — 1× (flag vivo)
```

### 11.2 Pipeline por quadro (ordem fixa e testável)

```
atualizarSala(partida, entrada, dt):
  1. se hitStop > 0: hitStop -= dt; atualizar só efeitos; retornar
  2. lerIntencaoJogador → atualizarJogador (movimento, combo, dash, clarão, interação)
  3. golpes do jogador → resolverGolpe contra inimigos/chefe/caixas/detritos
  4. atualizarArmadilhas (terreno em todos os atores elegíveis, 1× por ativação)
  5. se jogador morto: resolver pacto ou terminar(derrota)  ← ANTES de qualquer vitória
  6. se chefe final derrotado e jogador vivo: terminar(vitória)
  7. atualizarInimigos (IA) e atualizarChefe → criam Avisos e Projéteis
  8. atualizarAvisos (quando chega a 0 → Golpe) e atualizarProjeteis
  9. resolver golpes hostis contra Bill (e balões contra inimigos)
 10. remover mortos (erase-remove; nenhum ponteiro guardado entre quadros)
 11. se não restam hostis: próxima onda ou concluirSala (portas abrem, recompensa aparece)
 12. atualizarVisao se o tile mudou ou Bill trocou de célula; atualizar mensagens e score
```

### 11.3 Golpes de Bill (Taco)

| Golpe | Área | Duração ativa | Dano | Recuo |
|---|---|---|---|---|
| 1 | Arco 100°, raio 40 px | 0,10 s | base | 40 px |
| 2 | Arco 100°, raio 40 px | 0,10 s | base | 40 px |
| 3 | Arco 140°, raio 46 px | 0,14 s | base ×1,5 | 110 px |
| Especial (giro) | Círculo de raio 52 | 0,25 s | base ×2 | 90 px |
| Clarão | Linha de 4 células × 24 px | instantâneo | 15+5·FOC | 0 + atordoa 1,5 s |

O combo encadeia se Atacar for pressionado dentro da janela de 0,35 s após cada golpe; caso contrário volta ao golpe 1. Atacar durante o dash = **golpe de dash** (P1).

---

## 12. IA (F07)

### 12.1 Detecção

`podeVer(sala, origem, alvo, alcance)` = distância ≤ alcance **e** linha de Bresenham sem tile que `bloqueiaVisao`.

### 12.2 Estados comuns

```
Vagando ──vê Bill──► Perseguindo ──no alcance──► Preparando(aviso) ──► Atacando ──► Recuperando ─┐
   ▲  ◄─3 s sem ver─ Buscando(últimaVista) ◄─perdeu visão─┘                                       │
   └──────────────────────────────────────────────────────────────────────────────────────────────┘
Qualquer ── clarão ──► Atordoado(1,5 s)      Rato com HP<30% ──► Fugindo(2 s)
```

- **Vagar**: escolhe uma célula livre aleatória a até 4 células (lista finita de vizinhos válidos), anda até ela e espera de 0,5 a 1,5 s.
- **Perseguir**: BFS na grade (`caminho.cpp`, 4 vizinhos, máx. 40×24 = 960 nós), recalculado a cada 0,4 s **ou** quando o alvo muda de célula; segue o próximo nó com steering. Com linha de visão livre e caminho reto, vai direto.
- `TABELA_COMPORTAMENTO[tipo]` = ponteiro para a função da decisão específica (`decidirBalao`, `decidirRato`…), mais um exemplo de **ponteiro para função** (T03). As partes comuns (mover, colidir, aviso, dano) são as mesmas funções para todos.
- **Sombra × detrito**: se o próximo nó do caminho "com detritos passáveis" for um detrito, a Sombra entra em `Preparando` contra o detrito e o golpe transforma `Detrito → Chao` (`alterarTile`, que marca `visaoSuja` e `cacheSujo`).
- Limite: no máximo 12 hostis ativos; processamento em ordem estável de `id`.

### 12.3 Chefes

`atualizarChefe(partida, chefe, dt)`: se não há padrão ativo, escolhe um por peso entre os padrões com `faseMinima <= fase`; executa os passos (`Aproximar` usa o BFS de corpo 2×2 quando `corpo2x2`; `Preparar` cria um `Aviso` com **alvo registrado agora**; `Executar` dispara; `Recuperar` deixa o chefe vulnerável e parado). A transição de fase acontece em `hp <= limiar`, **uma vez** (`transicaoFeita[]`), não causa dano e troca o sprite (medo → Pennywise estágio N). Os efeitos únicos ficam num `switch(tipo)` de `executarEspecialChefe`.

---

## 13. Missões, diálogos e tutorial (F18, F19)

### 13.1 Flags

`enum class FlagMissao { DialogoMike, ManivelaColetada, ManivelaEntregue, GradeAberta, ChaveMedoB1…B6, ResgateBen…ResgateRichie, ChefeDisperso1…6, CovilAberto, Vitoria }`. Toda função de efeito começa com `if (flag já ativa) return;`, e assim **repetir uma conversa não repete a recompensa**.

`concluirResgate(partida, Amigo)`: flag + `R++` + XP 25 + Eco lendário + `alterarMapaResgate` (switch por amigo → `alterarTile` nos tiles marcados) + diálogo.

### 13.2 `data/dialogos.txt`

```
@mike_prologo_1
quando=sempre
Mike|preocupado: Bill! Os outros… eles saíram correndo, cada um pra um lado.
Bill|serio: E-eu também ouvi. Era o Georgie. Não era ele.
efeito=flag:DialogoMike

@ben_hub_morte_mumia
quando=hub; morreu_para=mumia; nao_visto
Ben|confuso: Sonhei com faixas me apertando. Que esquisito… você tá bem, Bill?
```

- **Implementado (16/11, Etapa 7.3)** em `jogo/dialogo` (formato acima + `quem=`, `escolha=texto|id`, condições `tentativas`, `vitorias`, `resgates`, `chefes`, `mortes<=`, `morreu_para=<sprite>`), `Pendencia::Dialogo` na partida e `jogo/clube` (fala nova por amigo por visita, `sincronizarPerfil`). O tutorial usa `pedirDica` + fila na partida (`verificarDicas` a cada quadro), em vez de `verificarDica(Jogo&)`: assim roda sem janela e é testado.
- `Falante|expressão` escolhe o retrato (P0: um retrato por personagem; as expressões ficam para P1).
- `quando=` é uma lista de condições avaliada por `condicaoSatisfeita(const Perfil&, const EstadoPartida*, cond)`.
- O parser fica em `leitor_texto.cpp` (o mesmo de salas e balanceamento) e mostra a leitura e escrita de arquivos da disciplina.

### 13.3 Tutorial

**Botões por dispositivo (PRD §15.4, implementado na 7.3/7.4):** todo texto que cita um botão usa marcadores (`{MOVER}`, `{ATACAR}`, `{ESPECIAL}`, `{CLARAO}`, `{DASH}`, `{INTERAGIR}`, `{KIT}`, `{INVENTARIO}`, `{FICHA}`, `{PAUSA}`, `{CONFIRMAR}`, `{VOLTAR}`, `{MAPA}`) trocados por `trocarBotoes(texto, controle)` (regra pura, testada). Nas cenas, os textos de ajuda passam por `ajuda("{INTERAGIR}: abrir o baú")` (`cenas/ui`), e a cena informa o dispositivo do quadro com `definirControleNaAjuda(entrada.ultimoUsado == Gamepad)`. O teste `ajuda_da_interface_nao_fixa_tecla_de_teclado` lê `cena_sala.cpp` e `cena_menu.cpp` e reprova qualquer texto com tecla fixa (`L"E: `, `Enter ou J`, `· Esc `…) fora de `ajuda(...)`.

`verificarDica(Jogo&, IdDica)` é chamada nos pontos de gatilho (PRD §15.4). Se `opcoes.dicas` estiver ligada e a dica não constar em `perfil.dicasVistas`: marca, salva e empilha um `Dialogo` com o texto `@dica_Dxx`, trocando os nomes de botão conforme `entrada.ultimoUsado` (marcadores `{ATACAR}`, `{DASH}`… em `dialogos.txt`).

---

## 14. Experiência e atributos (F15, F16)

```cpp
constexpr int LIMIARES_XP[11] = {0, 0, 40, 110, 200, 320, 470, 650, 860, 1100, 1380};
int  nivelParaXp(int xpTotal);                     // pura, testável
int  concederXp(Jogador&, int quantidade);         // retorna os níveis ganhos; soma pontos (2/nível), +10 HP, limite 10
bool aplicarPonto(Jogador&, int& campo);           // falha se não há pontos ou o atributo é 9; chama recalcularStats
```

A tela de nível só abre quando a sala está sem hostis ou o jogador pede (C). A Coragem aumenta HP máx e HP atual na mesma quantidade, sem curar além do novo máximo.

---

## 15. Itens, inventário, loja e Ecos

**Implementado na Etapa 6.2 (16/11)** em `jogo/itens` + `jogo/partida`: `coletarNoInventario(Jogador&, Inventario&, TipoItem, qtd)` (devolve quantas entraram; missão ignora limite), `usarItem(Jogador&, Inventario&, TipoItem) → ResultadoUso`, `soltarItem`, `bauProximo`, `bauTrancado`. Tipos a mais que o §3.5: `Tonico`, `Oculos`, `Lembrancinha` (14 itens). Bateria e Pacto ficam no `Jogador`. As assinaturas abaixo (com `EstadoPartida`) continuam valendo para chaves, loja e Ecos.

```cpp
bool coletarItem(EstadoPartida&, ItemNoChao&);              // respeita o limite; deixa no chão se cheio; I++ 1×
bool usarItem(EstadoPartida&, TipoItem);                    // kit com HP cheio → false, sem gastar
bool usarChave(EstadoPartida&, Celula portaCelula);        // chave correta? abre (alterarTile) e consome; incorreta → mensagem
bool comprar(EstadoPartida&, int indiceOferta);
std::array<int,3> sortearEcos(const EstadoPartida&, Amigo, Rng&);  // sem repetir Ecos já ativos
void aplicarEco(EstadoPartida&, int definicao, Raridade);         // adiciona e recalcula stats
```

---

## 16. Pontuação, perfil e telemetria

```cpp
long long calcularPontuacao(const Estatisticas&);   // ÚNICA função; HUD, resultado, menu e testes
```

- **Implementado (16/11, Etapa 7.2)**: o Clube é a sala `hub_clube.txt` (`tipo=clube`) no mesmo motor da partida (`iniciarClube`, `Pendencia::Mural/Quadro/Alcapao`), e `jogo/clube::atenderClube` faz a ponte com o perfil. Símbolos novos de sala: `M` Mural, `Q` quadro, `H` alçapão, `npc=<d> <nome>`, `enfeite=<quadro> <col> <lin>`. O fluxo implementado é Menu → Clube → alçapão → Sala → Resultado → Clube (a `IdCena::Hub` virou o modo Clube da cena de sala).
- **Implementado (16/11, Etapa 7.1)** em `jogo/perfil` (leitura tolerante, talentos com `disponivel`, `aplicarTalentos`, `lembrancasDaTentativa`, `registrarFimDeTentativa`; o piloto não ganha Lembranças).
- `saves/perfil.txt`: formato `chave=valor` por linha, com `versao=1`. Gravado ao entrar no Clube, ao comprar no Mural e ao ver uma dica. A gravação é **atômica**: escreve `perfil.tmp` e depois `MoveFileExW(…, MOVEFILE_REPLACE_EXISTING)`.
- `saves/recordes.txt`: top 10 (`pontos;data;bioma;vitoria;seed`).
- `saves/telemetria.csv`: `tentativa;bioma;sala;tipo;segundos;danoRecebido;resultado`, para validar os 45–60 min.
- O abandono da tentativa não grava score; a morte grava.

---

## 17. Áudio (P0 desde 13/11: critério M07)

Substituído por **§24.8**: mixer `waveOut` próprio (8 vozes + música), eventos de som emitidos pelas regras e consumidos pela cena, `data/sons.txt`. Os arquivos vêm do Diego. (O plano antigo, `PlaySoundW` com um canal só, não deixaria tocar golpe e passo ao mesmo tempo.)

---

## 18. Memória e posse

| Recurso | Dono | Criação | Liberação |
|---|---|---|---|
| `Jogo` | `main.cpp` (pilha de WinMain) | início | fim do programa |
| `Conteudo` (salas-modelo, diálogos, tabelas) | `Jogo` | carregado uma vez | nunca redimensionado depois |
| `Recursos` (imagens, fontes, DIB) | `Jogo` via `std::unique_ptr` | `carregarRecursos` | `liberarRecursos` (seleciona os objetos GDI de volta antes de `DeleteObject`) |
| `EstadoPartida` | `Jogo` | `iniciarPartida` (reinicia **tudo**: novo objeto atribuído) | sobrescrito na próxima tentativa |
| Vetores de atores | `EstadoPartida` | `push_back` | erase-remove no passo 10 |

**Regra:** ponteiros e referências para elementos de `vector` (`Inimigo*`, `ItemNoChao*`) **valem apenas dentro da função** que os obteve. Entre quadros, guardar o **ID** e buscar de novo com `Inimigo* buscarInimigo(EstadoPartida&, IdEntidade)`, que pode retornar `nullptr` (T03).

---

## 19. Testes

### 19.1 Automatizados (`testes_regras.exe`, sem janela)

| Arquivo | Casos |
|---|---|
| t_colisao | parede nas 4 direções; diagonal na quina; dash contra parede fina; borda da sala; porta fechada × aberta; NPC e chefe 2×2 bloqueiam; deslizamento |
| t_combate | exemplo verificável PRD §7.3; acerto limitado a 60..98; dano mínimo 1; esquiva zera o dano; o mesmo golpe não acerta 2× o mesmo alvo; i-frames; Pacto revive com 50%; morte sem Pacto termina; vitória só com Bill vivo |
| t_xp | XP atravessando 2 limiares; nível máximo 10; pontos 2/nível; atributo máximo 9; Coragem aumenta HP máx e HP atual |
| t_visao | sala começa toda desconhecida; parede bloqueia a visão; célula explorada continua explorada; porta aberta revela; inimigo fora da luz não fica visível |
| t_ia | inimigo nunca entra em parede (10 mil passos simulados com seed); perseguição alcança o alvo por BFS; perde a visão → busca 3 s → vaga; Sombra transforma detrito em chão; 2 inimigos não ficam sobrepostos |
| t_salas | **todas** as salas de `data/salas` passam em `validarSala` |
| t_gerador | 1000 seeds: sequência válida, chave antes do resgate, ≥ 1 Eco por bioma, determinismo (mesma seed = mesma sequência) |
| t_itens | kit com HP cheio não gasta; limite de estoque deixa o item no chão; chave errada não abre nem é consumida; chave certa abre e muda a colisão |
| t_score | exemplos do PRD §16 (35.000 e 3.400); nunca negativo |
| t_perfil | gravar e ler de volta = igual; arquivo corrompido → perfil padrão sem travar |
| t_chefes | transição de fase 1×; aviso mantém o alvo registrado; HP de um encontro não afeta o seguinte; o Pennywise IV dá V; os chefes 1–6 não dão V |
| t_missoes | recompensa de NPC não se repete; grade, ponte, comporta e ilusão mudam a colisão de verdade |
| t_controle | ✅ (Etapa 2) eixos, zona morta radial, botões por posição → comandos, LT/LT+↑, prioridade do digital |
| t_dificuldade | §24.2: probabilidades escalam; determinismo seed + dificuldade |
| t_aparencia | §24.4: camadas por item e expiração dos power-ups |
| t_ladrao | §24.5: rato pega e devolve; espectro queima; missão é imune |
| t_companheiro | §24.6: segue sem atravessar paredes, teleporta, troca, habilidades |
| t_piloto | §24.7: limpa salas de teste; conclui bioma 1 (≥ 9/10 seeds); **vence a tentativa completa (≥ 8/10 Normal, 10/10 Fácil)**; nunca fica travado > 5 s |
| t_score (amplia) | fórmula §24.3 com T e D: 37.400 / 3.220 / 56.100 |

Rodar com `ctest` ou `build/testes_regras.exe`, que **devem passar antes de qualquer commit**.

### 19.2 Manuais (roteiro em `docs/HANDOFF.md` a cada etapa)

Foco perdido e devolvido; escala 1×/2×/3× e tela cheia; joystick (8BitDo/SDL2 e Xbox/XInput) conectando e desconectando; uma tentativa completa cronometrada; derrota com e sem Pacto; reinício limpa as flags; Modo Defesa.

---

## 20. Modo Defesa (debug)

Ativado com `--defesa` na linha de comando ou Ctrl+F12. As teclas usam **Ctrl** (F1 simples = medidor, F2 = teste do controle e F10 = piloto valem sempre): Ctrl+F1 = hitboxes, IDs, FPS e p95 · Ctrl+F2 = invencível · Ctrl+F3 = concluir sala · Ctrl+F4 = menu "ir para bioma N / chefe N / Covil" · Ctrl+F5 = +100 XP · Ctrl+F6 = matar hostis · Ctrl+F7 = revelar o mapa · Ctrl+F8 = fixar seed · Ctrl+F9 = +100 moedas e +50 Lembranças. Fica fora do caminho normal do jogador.

---

## 21. Plano de implementação (etapas com critério de saída)

Cada etapa termina com: build limpo (sem warnings novos), `testes_regras` passando, teste manual registrado e **`docs/HANDOFF.md` + `docs/APRENDIZADOS.md` atualizados**.

| # | Etapa | Entrega | Critério de saída |
|---|---|---|---|
| 0 | Documentos | PRD v2, SDD v1, prompts v2, handoff | Aprovação do Diego |
| 1 | Fundação | CMake (3 alvos), janela Win32, loop fixo, Canvas DIB, carregar PNG + manifesto, texto, escala inteira, foco, teclado | Bill parado desenhado em ×2/×3; p95 < 5 ms; testes "olá" rodando |
| 2 | Gamepad | SDL2 dinâmico + XInput + hotplug + tela de teste de botões | ✅ código e testes; **o teste físico com o 8BitDo ficou para o fim (Etapa 16)**, por decisão do Diego |
| 3 | Sala e movimento | Leitor de salas, tileset, colisão, câmera, Bill andando e com dash, contagem de M, cache de terreno com bordas orgânicas | ✅ (13/11) t_colisao e t_salas passam; andar por uma sala de teste sem bugs |
| 4 | Visão | Shadowcasting, memória, render de luz | ✅ (13/11) t_visao passa (inclui simetria em todas as salas); sala revela-se ao explorar |
| 5 | Combate núcleo | Combo, especial, clarão, avisos, Balão/Rato/Sombra, ondas, morte, XP, nível, atributos. **`atualizarPartida` nas regras e simulação sem janela (§24.7.1)** | ✅ (13/11) t_combate e t_ia passam; sala de 2 ondas vencida em 10/10 seeds sem janela |
| **5b** | **Piloto automático v1** (§24.7) | `jogo/piloto`: navegação BFS + eixo, explorar, combater, desviar de avisos, sair; F10 liga/desliga; overlay de depuração (F1); `--simular` | ✅ (13/11) t_piloto: 3 salas × 10 seeds = 30/30, 0 mortes, 0 travamentos |
| 6 | Estrutura da tentativa ✅ (16/11, em 5 fatias) | Gerador, portas com recompensa, itens, chaves, baús, inventário (overlay), HUD com **cronômetro e faixa de itens**, score com **T e D**, resultado, **recordes ordenados**, **pausa**, **dificuldade** (tabela §24.2), **pendências** e piloto v2 | t_gerador, t_itens, t_score, t_dificuldade passam; o piloto atravessa uma sequência de salas e resolve menus |
| **6b** ✅ 16/11 | **Itens no personagem e inimigos × itens** (§24.4, §24.5) | `aparenciaDe` + tinta/contorno no canvas, sobreposição de arma e escudo, 2 power-ups novos, Rato ladrão, Espectro queimando, Sombra quebrando caixa | t_aparencia, t_ladrao passam; captura mostrando Bill com escudo + aura |
| 7 | Narrativa e UI | Diálogo, tutorial (D01–D20), menu completo (+ **Assistir o computador jogar**, filtro de recordes), perfil, hub, Mural, Pacto, **companheiro** (§24.6) | Menu atende F04; tutorial aparece 1×; morrer volta ao hub com Lembranças; t_companheiro passa |
| **7b** | **Áudio** (§24.8) | Mixer `waveOut`, `data/sons.txt`, eventos de som das regras, volumes nas Opções. Arquivos: pasta indicada pelo Diego | Golpe + passo + música tocando juntos sem estalos; jogo mudo sem dispositivo não trava |
| 8 | **Fatia vertical**: Bioma 1 completo | Prólogo, Elias/manivela/grade, Ben/ponte, Múmia→Pennywise I, Ecos do Ben, loja, refúgio, **sala secreta + Lembrancinha + Festa dos Balões**, Mike companheiro, piloto v3 | Jogar prólogo + bioma 1 em ~10 min; cronometrar; **t_piloto: o piloto conclui o bioma 1 em ≥ 9/10 seeds (Normal)** |
| 9 | Assets v2 (em paralelo desde a etapa 3) | P56–P105 gerados, revisados e exportados; **P106–P111 (andar dos companheiros)**, sobreposições de arma/escudo e arte da parede rachada | Contact sheets aprovados |
| 10–14 | Biomas 2–6 | Salas, inimigos 4–8, chefes 2–6, resgates, Ecos, segredo e bônus por bioma, ficha do piloto por chefe | Cada bioma jogável e cronometrado (~7,5 min) **e concluído pelo piloto em ≥ 9/10 seeds** |
| 15 | Covil e final | Pennywise IV em 3 fases, Georgie, Luzes Mortas, epílogo, créditos, piloto v5 | Vitória completa; t_chefes passa; **o piloto vence a tentativa inteira (≥ 8/10 Normal, 10/10 Fácil)** |
| 16 | Polimento P1 + joystick físico | Medo, elites, cartões de bioma, balanceamento (com `--simular`), **teste com o 8BitDo (roteiro do HANDOFF §7)** | Tentativa vitoriosa de 45–60 min medida 3×; 8BitDo aprovado nos 2 modos |
| 17 | Entrega e defesa | **Repositório GitHub + arquivo com o link**, pacote limpo, README, `docs/DEFESA.md`, ensaio de alterações individuais, ensaio da apresentação com o piloto (velocidade ×4) | Roda numa pasta nova em outra máquina; o piloto vence ao vivo |

**Por que o piloto começa na Etapa 5b, e não no fim:** ele vale 3,0 da nota e depende de a partida ser simulável sem janela. Construído cedo, cada sistema novo nasce compatível com ele, e o próprio piloto vira teste automático de cada etapa (e ferramenta de balanceamento).

---

## 22. Matriz de rastreabilidade (critério → módulo → evidência)

| ID | Módulos / funções principais | Teste |
|---|---|---|
| F01 | plataforma/tempo, render/canvas, `apresentar` | p95 no F1 do Modo Defesa |
| F02 | DIB + `StretchBlt` + `WM_ERASEBKGND`; arredondamento da câmera | manual |
| F03 | render/*, manifesto, paleta por bioma | revisão de capturas |
| F04 | cena_menu (Como jogar, Itens, Pontuação) | manual |
| F05 | entidades/jogador `atualizarMovimento`, plataforma/entrada | t_colisao |
| F06 | mundo/colisao `moverComColisao` | t_colisao |
| F07 | entidades/ia, mundo/caminho | t_ia |
| F08 | jogo/partida `terminarPartida`, chefes | t_chefes |
| F09 | jogo/itens, inventario `usarChave`, `usarItem` | t_itens |
| F10 | combate/dano `aplicarDanoJogador` | t_combate |
| F11 | mundo/visao, render/luz | t_visao |
| F12 | combate/golpe, dano | t_combate |
| F13 | jogo/pontuacao, render/hud | t_score |
| F14 | entidades/armadilhas | t_ia (inimigo × armadilha), t_missoes |
| F15 | combate/experiencia | t_xp |
| F16 | overlay_nivel, `aplicarPonto` | t_xp |
| F17 | combate/atributos `calcularStats` | t_combate (exemplo §7.3) |
| F18 | jogo/dialogo, missoes, cena_hub, cena_prologo | manual + t_missoes |
| F19 | mundo/sala `alterarTile`; missoes `alterarMapaResgate`; ia (detrito) | t_missoes, t_ia |
| F20 | entidades/chefes (Pennywise IV) | t_chefes |
| T01 | core/tipos, todos os .h | revisão |
| T02 | todas as assinaturas (`const&` × `&`) | revisão |
| T03 | `buscarInimigo` → `nullptr`; `TABELA_CENAS` e `TABELA_COMPORTAMENTO` (ponteiros para função); `SalaViva::modelo`; Canvas `px` | revisão + t_ia |
| T04 | árvore §2.1 | revisão |
| M01, M06 | jogo/dificuldade `parametros`; gerador, ondas, drops | t_dificuldade |
| M02 | 6 biomas + Covil (dados em `data/salas`) | t_salas, t_gerador |
| M03, M04 | `Estatisticas::tempo`, `calcularPontuacao`, jogo/recordes, render/hud | t_score, t_perfil |
| M05, M09 | jogo/itens, render/hud (faixa e notificação) | t_itens + captura |
| M07 | plataforma/audio, `EventoSom`, `data/sons.txt` | manual |
| M08 | jogo/aparencia `aparenciaDe`, canvas tinta/contorno | t_aparencia + captura |
| M10 | entidades/ia (rato, espectro, sombra) | t_ladrao |
| M11 | entidades/companheiro, `HABILIDADES_COMPANHEIRO` | t_companheiro |
| M12 | rng + gerador + ondas | t_gerador |
| M13 | tile `r`, sala `secreta`, Festa dos Balões, Lembrancinhas | t_salas (validador) + manual |
| M14 | combate/experiencia + Mural | t_xp |
| M15 | overlay_pausa; foco; desconexão do controle | manual |
| T05 | jogo/piloto `pilotar`, `atualizarPartida` sem janela, `--simular` | **t_piloto** + apresentação ao vivo |

---

## 23. Riscos técnicos

| Risco | Mitigação |
|---|---|
| `StretchBlt` lento em 4K | Medir; se p95 > 8 ms, apresentar com `StretchDIBits` direto da memória ou limitar a escala máxima a ×4 |
| Shadowcasting com artefatos em quinas | Algoritmo simétrico (o de Albert Ford) e testes com salas de quinas |
| BFS de corpo 2×2 | Grade "inflada": precalcular `livre2x2[c][l]` e rodar BFS comum nela |
| Arquivos de sala com erro de digitação | `validarSala` com mensagem de linha e coluna; teste cobre todas as salas |
| Sprite sheet com frame fora do lugar | Contact sheet e manifesto revisados; frames referenciados apenas pelo manifesto |
| Defesa: código grande para explicar | Módulos pequenos, comentários de "porquê" e um roteiro por integrante em `docs/DEFESA.md` (etapa 17) |
| **Piloto automático não vence** (vale 3,0 da nota) | Começa cedo (Etapa 5b) e cada etapa seguinte só termina com o piloto passando nela; simulação sem janela em `t_piloto` com várias seeds; contadores de "travado" e de causa de morte na telemetria do piloto |
| Simulação sem janela divergir do jogo real | O jogo real e a simulação chamam a MESMA `atualizarPartida`; as cenas só desenham e repassam comandos; decisões de menu são estados da partida (§24.7.1) |

---

## 24. Requisitos da parte 2 do enunciado (M2, recebidos em 13/11/2026)

Fonte: `instructions/T3 - Roguelike - 2026-part2.pdf`. Ordem de autoridade atualizada: (1) os dois PDFs do professor, (2) PRD v2, (3) este SDD. O PDF exige **manter todos os critérios do trabalho anterior** (F01–F20, T01–T04) e acrescenta 15 critérios funcionais (0,4 cada) e 2 técnicos: "uso dos quesitos anteriores" (1,0) e **"opção para o Computador/NPC jogar sozinho o jogo e ser capaz de ganhar e passar as fases" (3,0, o item mais caro da nota)**.

**Regra de leitura (pedido do Diego):** onde já entregávamos mais do que o PDF pede, **nada é reduzido**. Os critérios novos se somam ao design.

### 24.1 Tabela de cobertura

| ID | Critério do PDF (parte 2) | Situação antes | Como passa a ser entregue | Seção |
|---|---|---|---|---|
| M01 | 3 níveis de dificuldade | ❌ 1 dificuldade + Modo Coragem | **Fácil / Normal / Difícil**, escolhida ao sair do Clube; o Modo Coragem continua como opção de acessibilidade, separado | 24.2 |
| M02 | Ao menos 3 fases diferentes | ✅ prólogo + 6 biomas + Covil | Mantido (8 ambientes, chefes e mecânicas próprias) | PRD §4.4 |
| M03 | Tempo em tela e na pontuação | 🟡 cronômetro opcional; tempo fora da fórmula | Cronômetro **sempre visível** no HUD; termo de tempo e bônus de vitória rápida na fórmula | 24.3 |
| M04 | Salvar em arquivo e exibir por melhor pontuação | ✅ `saves/recordes.txt` (top 10) | Mantido + dificuldade, tempo e marca "computador" em cada linha; tela de recordes com filtro | 24.3 |
| M05 | Ao menos 7 itens diferentes | ✅ 12 itens | Mantido + 2 power-ups novos (24.4) = 14 | PRD §11.1 |
| M06 | Dificuldade altera as probabilidades | ❌ | Tabela de probabilidades por dificuldade em `data/balanceamento.txt` (spawn, elites, drops, roubo, armadilhas, crítico inimigo, segredos) | 24.2 |
| M07 | Som | 🟡 P1 | **P0.** Motor de áudio próprio (mixer `waveOut`); os arquivos de som são providenciados pelo Diego | 24.8 |
| M08 | Uso de itens altera o visual do personagem | ❌ | Camadas de aparência do Bill (arma na mão, escudo, aura de power-up, cor de cura/dano, contorno do Eco lendário) calculadas por função pura | 24.4 |
| M09 | Itens pegos aparecem na interface | 🟡 kits e Ecos no HUD | Faixa de itens no HUD com contador, notificação "+item" com ícone e a aba Itens do inventário | 24.4 |
| M10 | Inimigos pegam ou destroem itens | ❌ (só detritos) | **Rato ladrão** pega item do chão e foge; **Espectro** queima consumíveis; **Sombra** quebra caixas (o conteúdo se perde). Itens de missão são imunes | 24.5 |
| M11 | Companheiro | ❌ | **Companheiro de tentativa**: Mike no bioma 1 e, depois, o último amigo resgatado, com IA e uma habilidade de apoio por amigo | 24.6 |
| M12 | Aleatoriedade de itens e inimigos | ✅ ondas por orçamento, recompensas sorteadas | Mantido; a dificuldade passa a pesar os sorteios | §9, 24.2 |
| M13 | Fases bônus ou itens secretos | 🟡 salas de tesouro/Eco; Lembrancinhas em P1 | **Parede rachada → sala secreta** em todo bioma, **fase bônus "Festa dos Balões"** e **Lembrancinhas** promovidas a P0 | 24.5 |
| M14 | Farmar experiência para ficar mais forte | ✅ XP/níveis na tentativa + Lembranças/Mural entre tentativas | Mantido; a fase bônus e a sala secreta também rendem XP | PRD §7, §13 |
| M15 | Opção de pausar | ✅ planejada (Etapa 7) | Antecipada para a Etapa 6; pausa também com perda de foco e desconexão do controle | §5 |
| T05 | Computador joga sozinho, ganha e passa as fases | ❌ | **Piloto automático** que usa os MESMOS comandos do jogador (sem trapaça), com simulação sem janela que prova a vitória em testes | 24.7 |
| T06 | Uso dos quesitos anteriores | ✅ T01–T04 | Mantido; piloto, companheiro e dificuldade usam tabelas de ponteiros para função e dados | §22 |

### 24.2 Dificuldade (M01, M06)

`enum class Dificuldade : uint8_t { Facil, Normal, Dificil };` gravada no `EstadoPartida` (e a última escolha no `Perfil`). Escolha na **porta de saída do Clube** (caixa com 3 opções) e em "Novo jogo". Ela não muda as regras de combate nem a legibilidade (todo ataque continua anunciado): muda **probabilidades e orçamento**.

| Parâmetro (`data/balanceamento.txt`) | Fácil | Normal | Difícil |
|---|---|---|---|
| Orçamento de inimigos por onda | ×0,75 | ×1,0 | ×1,3 |
| Peso de sorteio dos inimigos fortes do bioma | ×0,5 | ×1,0 | ×1,6 |
| Chance de elite por sala (bioma ≥ 2) | 0% | 10% | 25% |
| Chance de drop ao derrotar um inimigo comum | 22% | 15% | 10% |
| Peso de Kit/Bateria nas recompensas de porta | ×1,5 | ×1,0 | ×0,7 |
| Chance de o Rato roubar um item ao passar por ele | 25% | 50% | 80% |
| Armadilhas opcionais ativas (símbolo marcado com `?` no mapa) | 30% | 60% | 100% |
| Crítico dos inimigos | 0% | 3% | 6% |
| Dano recebido | ×0,8 | ×1,0 | ×1,25 |
| Chance de existir sala secreta por bioma | 80% | 60% | 45% |
| Pactos de Sangue iniciais | +1 | +0 | +0 |
| Multiplicador da pontuação (D) | ×0,75 | ×1,0 | ×1,5 |
| Chance de um inimigo comum soltar 1–3 moedas (Etapa 8.5) | 65% | 55% | 45% |

- Uma única função, `const ParametrosDificuldade& parametros(const TabelaDificuldade&, Dificuldade)`, é lida por gerador, ondas, drops, IA e pontuação. Na defesa, alterar uma probabilidade = alterar uma linha de dado.
- **Implementado (16/11, Etapa 6.1):** valores inteiros em % (`chance_drop_pct = 22 15 10`, uma linha por parâmetro em `data/balanceamento.txt`); a tabela fica em `Conteudo::dificuldades` e a partida guarda uma cópia em `EstadoPartida::ajustes`. "Inimigo forte" = custo de onda ≥ 4.
- Teste `t_dificuldade`: com a mesma seed, 1000 sorteios de drop no Difícil dão menos itens que no Fácil; o orçamento escala; seed + dificuldade é determinístico.

### 24.3 Tempo e pontuação (M03, M04)

- `Estatisticas::tempo` soma `DT` **só** nos passos de lógica da partida (pausa, menus, diálogo e janela sem foco não contam). O HUD mostra `mm:ss` no centro superior, sempre.
- Fórmula (substitui a do PRD §16 e continua numa única função `calcularPontuacao`):

```
bruto = 100K + 300E + 50I + 500R + 1000B + 250C + 5000V
      + V·máx(0, 9000 − 2T)        ← bônus de vitória rápida (T em segundos)
      − ⌊M/2⌋ − 1000U − ⌊T/5⌋      ← movimento, vidas e tempo
S = máx(0, ⌊bruto × D⌋)            ← D = multiplicador da dificuldade (24.2)
```

  Exemplos para `t_score`: vitória no Normal com T = 3000 s (demais valores como no PRD) → 35.000 + 3.000 − 600 = **37.400**; derrota no bioma 2 com T = 900 s → 3.400 − 180 = **3.220**; a mesma vitória no Difícil → ⌊37.400 × 1,5⌋ = **56.100**.
- Implementado como `calcularPontuacao(const Estatisticas&, int multiplicadorPct)` (conta inteira) + `detalharPontuacao` (parcelas para a tela de resultado), em `jogo/pontuacao`.
- `saves/recordes.txt`: `pontos;data;dificuldade;tempo_s;bioma;vitoria;seed;piloto` (piloto = 1 se jogada pelo computador). Ordenação: pontos desc, depois tempo asc; empate exato mantém o mais antigo na frente. Guardam-se **10 por categoria** (jogador e computador), para o filtro "Jogador" não ficar vazio quando o piloto joga muito. A tela de **Recordes** (menu e quadro do Clube) mostra o top 10 com filtro "Todas / Jogador / Computador". Depois de cada partida, a tela de resultado mostra a colocação obtida.

### 24.4 Itens visíveis no personagem e na interface (M08, M09)

`AparenciaBill aparenciaDe(const Jogador&, const Inventario&, const std::vector<EcoAtivo>&, float tempo)`: **função pura** (testável) que devolve as camadas; o render só obedece.

| Origem | Mudança visual |
|---|---|
| Arma equipada (taco, estilingue, cano) | Sprite de ataque próprio e a arma desenhada na mão ao andar e parado (sobreposição por direção) |
| Escudo equipado | Tampa de lixeira no braço (sobreposição) |
| Ficha de coragem (power-up, 30 s) | Aura laranja pulsante + tinta quente no sprite |
| **Tônico do Eddie** (power-up novo: +30% de velocidade por 20 s) | Rastro azul-claro nos passos |
| **Óculos do Richie** (power-up novo: revela inimigos e segredos por 30 s) | Contorno branco e paredes rachadas destacadas |
| Kit de cura | Clarão verde de 0,4 s e partículas subindo |
| Dano recebido / Pacto usado | Piscar branco de 1 quadro / contorno vermelho-escuro até o fim da tentativa |
| Eco lendário | Contorno na cor do amigo (PRD §12) |

- O canvas ganha `desenharFramePesTinta(…, cor, forca)` e `contornoSprite(…)` (operações por pixel no frame já preparado).
- **Implementado (16/11, Etapa 6b):** `aparenciaDe(const Jogador&, const Inventario&, float tempo)` em `jogo/aparencia` (os Ecos entram quando existirem); `contornoFramePes`; `tingir` em `core/pixels`. Escudo provisório com o ícone do item até a sobreposição por direção. Óculos: revelam inimigos fora da luz (desenhados depois da escuridão); paredes rachadas entram com os segredos.
- HUD (M09): faixa inferior direita com os ícones dos consumíveis e as quantidades; notificação central "**+ Bateria**" com ícone ao coletar; itens de missão num slot separado ao lado do HP.
- Teste `t_aparencia`: equipar escudo liga a camada do escudo; coragem ativa a aura e expira em 30 s; sem itens = aparência base.

### 24.5 Inimigos × itens e segredos (M10, M13)

**Inimigos × itens** (`entidades/ia`, comportamento extra na tabela de ponteiros para função):
- **Rato ladrão:** se passa a ≤ 1 célula de um `ItemNoChao` que não é de missão, rola a chance da dificuldade, pega o item (fica visível na boca) e passa ao estado `Fugindo`, rumo ao spawn mais distante. Se morrer, **solta o item**; se fugir por 8 s sem ser visto, some com ele ("Um rato levou a sua Bateria!").
- **Espectro de fuligem:** o rastro de brasas **destrói** consumíveis no chão em que encosta (efeito de queima + mensagem).
- **Sombra:** já quebrava detritos; agora também quebra **caixas** no caminho (o conteúdo se perde).
- Itens de missão (chaves, manivela) nunca são pegos nem destruídos (garantia do PRD §5.3).
- Teste `t_ladrao`: o rato pega item comum e ignora a chave; ao morrer, o item volta ao chão na posição dele; o Espectro destrói consumível e não destrói a chave.

**Segredos e fases bônus:**
- **Parede rachada** (`r` no mapa): parece parede, com rachaduras discretas; quebra com o **Especial** ou o **Clarão** e abre caminho para a **sala secreta** (`tipo=secreta`), com baú garantido, Lembrancinha e XP. Uma sala secreta possível por bioma (chance pela dificuldade).
- **Lembrancinhas** (P1 → **P0**): 1 colecionável escondido por bioma (o barquinho, a figurinha, o anel…). Ficam numa estante do Clube (persistem no perfil) e dão Lembranças.
- **Fase bônus "Festa dos Balões"**: porta rara (recompensa `Bonus`) que leva a uma arena onde balões sobem por 30 s; cada balão estourado dá moedas e XP, e os balões vermelhos são armadilhas. Ninguém morre na fase bônus (HP mínimo 1). No máximo uma por bioma.
- Validador de salas: a sala secreta só é alcançável pela parede rachada (a BFS confirma), e a parede rachada tem que ter os dois lados caminháveis.
- **Implementado (16/11, Etapa 8.4)**: a "sala secreta" é um NICHO dentro das salas de combate (atrás de `r`), não uma sala separada: não exige porta nova no gerador e a validação continua por sala. Só a 1ª sala com `r` do bioma guarda o segredo (se a chance sorteou); nas outras o `r` vira parede. A Festa dos Balões é a sala `tipo=bonus` (porta rara de 20% no especial da etapa 2); balões são `BalaoFesta` (não inimigos), e a vida mínima é 1 na festa. O resgate muda o mapa por `ao_resgatar=` na sala; salas de mão única usam `entrada=fixa`.

### 24.6 Companheiro (M11)

**Aprovado pelo Diego em 16/11/2026 (sem ajustes):** um companheiro de tentativa que segue Bill, controlado pela IA e **invulnerável** (não há escolta; ele nunca causa uma derrota).
- **Bioma 1:** **Mike** acompanha Bill desde o prólogo ("Eu vou com você até achar o Ben"). Depois do resgate do Ben, Mike volta para "manter a saída livre", o que prepara o sumiço dele no bioma 5.
- **A partir do bioma 2:** o **último amigo resgatado** vira o companheiro, e o anterior volta ao Clube pela rota segura.
- Cada amigo tem **uma habilidade de apoio** com recarga (tabela `HABILIDADES_COMPANHEIRO`, ponteiros para função):

| Companheiro | Habilidade (recarga) |
|---|---|
| Mike | Lanterna extra: +1 célula de visão e marca no chão o item mais próximo (passiva) |
| Ben | Barreira de tijolos por 2 s contra projéteis na frente de Bill (12 s) |
| Stan | Aponta armadilhas e paredes rachadas no raio de visão (passiva) |
| Eddie | Cura 10% do HP quando Bill fica abaixo de 30% (1× por sala) |
| Beverly | Disparo de estilingue no inimigo mais próximo, 6 de dano (3 s) |
| Richie | Provoca: o inimigo mais próximo mira nele por 2 s (10 s) |

- Movimento: segue um ponto 1,5 célula atrás de Bill pelo caminho BFS. Se ficar a mais de 8 células ou travar por 2 s, **teleporta** para perto de Bill (fora da visão dos inimigos). Não ativa armadilhas, não pega itens e não colide com Bill.
- Assets novos: andar em 4 direções × 4 quadros para Mike, Ben, Stan, Eddie, Beverly e Richie (lote P106–P111; o `*_idle` atual serve de referência de identidade).
- Teste `t_companheiro`: segue sem atravessar paredes (10 mil passos); teleporta ao ficar longe; troca para o último resgatado; Eddie cura só 1× por sala.
- **Implementado (16/11, Etapa 7.4)** em `jogo/companheiro` (`Companheiro`, `HABILIDADES_COMPANHEIRO` com ponteiros para função `agir`, `entrarCompanheiro`, `companheiroEntrouNaSala`, `atualizarCompanheiro`, consultas `bonusDeLuz`/`barreiraAtiva`/`revelaArmadilhas`/`itemMarcado`/`provocado`). Detalhes de implementação: a barreira do Ben sobe quando um aviso hostil vai acertar Bill (≤ 0,25 s) e segura **qualquer** golpe hostil por 2 s; o Richie provoca o inimigo visível mais perto de Bill (raio 6 células), que passa a mirar no Richie; a Beverly atira num raio de 7 células. O resgate mínimo (SDD §13.1) entrou junto: falar com o amigo preso (NPC `1..9` da sala de resgate) conta `R`, dá 25 XP, abre `resgate_<amigo>` e troca o companheiro; ponte e Eco lendário ficam para a Etapa 8. Até o lote P106–P111, o companheiro usa o sprite parado com balanço de passo.

### 24.7 Piloto automático: o computador joga e vence (T05)

**Objetivo:** com a opção ligada, o computador conduz Bill do Clube ao epílogo, **passando por todas as fases e vencendo o Pennywise**, sem trapaça. Ele só envia `EstadoEntrada` (os mesmos comandos do teclado e do joystick) e só "enxerga" o que o jogador enxergaria: visão e exploração da sala, avisos na tela e HUD.

#### 24.7.1 Pré-requisito de arquitetura (vale desde já)

- **Toda a partida é simulável sem janela:** `void atualizarPartida(EstadoPartida&, const Conteudo&, const EstadoEntrada&, float dt)` fica nas regras (`jogo/partida`), e as cenas só desenham e repassam comandos.
- **Revisão (16/11):** escolher porta NÃO é pendência (escolha física, como em Hades). Implementado: `Pendencia { Nenhuma, SubirNivel, Pausa, Inventario }` (Etapa 6.4: pausa e inventário também são pendências, para o tempo parar e tudo ser testável por comandos; o piloto espera quando a pessoa pausa), `resolverPendencia(p, escolha)`, `fecharPendencia(p)`; os demais tipos entram com os sistemas deles.
- As escolhas que seriam "telas" viram **decisões pendentes** do estado: `enum class Pendencia { Nenhuma, Dialogo, SubirNivel, EscolherEco, Loja, EscolherPorta, Resultado }` + os dados da escolha. A cena (overlay) desenha a pendência. Quem resolve é o jogador (comandos) **ou** o piloto, pelas mesmas funções `resolverPendencia(...)`.
- Assim, a mesma partida roda em três modos: jogador, piloto com janela (apresentação) e **piloto sem janela** (testes e balanceamento, centenas de vezes mais rápido que o tempo real).

#### 24.7.2 Módulo `jogo/piloto.{h,cpp}` (regras, sem Windows)

```cpp
struct MemoriaPiloto {                         // estado próprio do piloto (entre quadros)
  Objetivo objetivo; std::vector<Celula> caminho; Celula destino;
  float tempoTravado = 0; Vec2 ultimaPosicao; int salasSemProgresso = 0;
  std::array<float,(int)Comando::Total> segurarAte{};   // segurar botão por N s (ex.: LT)
};
EstadoEntrada pilotar(const EstadoPartida&, const Conteudo&, MemoriaPiloto&, float dt);
```

Decisão em camadas: cada camada é uma função pequena e testável, e a primeira que "quer agir" vence.

1. **Pendência aberta** → resolve o menu. Diálogo: Confirmar. Subir de nível: distribui pela tabela de prioridade (FOR e AGI até 4, depois COR e FOC). Eco: maior "nota" (tabela por Eco, com sinergia com a arma). Loja: kit se tiver < 2, depois melhoria de arma. Porta: Chave do Medo > Kit (se HP < 50%) > Eco > Melhoria > Moedas.
2. **Sobreviver:** se o pé de Bill está, ou vai estar em ≤ 0,25 s, dentro de um **aviso de ataque** ou na trajetória de um projétil → **dash** para a célula segura mais próxima (busca no "mapa de perigo"). HP < 35% com kit → UsarKit. HP < 20% sem kit → recuar para longe dos inimigos.
3. **Combater:** escolhe o alvo (chefe > quem ameaça > mais próximo) e mantém a **distância de ataque da arma**: taco corpo a corpo, entrando no fim do aviso do inimigo; estilingue a 4–5 células, com linha de visão. Usa o Especial quando há ≥ 2 inimigos no alcance e o Clarão contra Sombra, no escuro ou quando sobram cargas. Cada chefe tem uma **ficha de estratégia** em dado (`data/piloto_chefes.txt`: distância preferida, quando atacar, de que lado ficar, quais avisos exigem dash).
4. **Missão:** pegar a Chave do Medo e a manivela, falar com NPC (Interagir), entregar a manivela ao Elias, quebrar parede rachada conhecida.
5. **Coletar:** itens visíveis, incluindo perseguir o Rato ladrão que levou um item.
6. **Explorar:** caminho BFS até a célula **não explorada** mais próxima e alcançável.
7. **Sair:** com a sala concluída, vai até a porta escolhida.

- **Navegação:** BFS/A* na grade, com custo extra para armadilhas, água rasa e avisos. O caminho vira um eixo analógico (`Vec2` para o próximo ponto, com "olhar à frente" de 1 célula para cortar quinas sem enroscar). **Detector de travamento:** 1,5 s sem sair da célula → recalcula, tenta um dash lateral e depois troca de objetivo.
- **Reação humana:** atraso de reação configurável (padrão 0,12 s) e um pequeno erro de mira. Assim ele joga "como gente", e o mesmo piloto vira teste de justiça dos chefes: se ele não consegue desviar com 0,12 s, o aviso está curto demais.
- **Apresentação:** opção no menu "**Assistir o computador jogar**" (escolhe dificuldade e velocidade ×1/×2/×4) e tecla **F10**, que liga e desliga a qualquer momento (o jogador pode assumir e devolver). Faixa "COMPUTADOR JOGANDO" no HUD. Com F1, mostra o caminho planejado, o objetivo atual ("Explorar", "Desviar: aviso da Múmia") e o mapa de perigo. As partidas do piloto entram no ranking marcadas como "computador".
- **Prova automática (`t_piloto`, sem janela):**
  - Etapa a etapa: "o piloto limpa a sala de teste X em ≤ N s" e "conclui o bioma 1 em ≥ 9 de 10 seeds (Normal)".
  - Na entrega: **vence a tentativa completa em ≥ 8 de 10 seeds no Normal e em 10 de 10 no Fácil**, sem Modo Defesa. Tempo esperado: segundos por tentativa (a lógica é barata e não há render).
  - Ferramenta de console `--simular N --dificuldade D`: roda N tentativas do piloto e grava `saves/simulacao.csv` (seed, venceu, bioma da morte, causa, tempo de jogo, pontuação). Serve também para **balancear** a meta de 45–60 min.
  - **Implementado (16/11, Etapa 6.5):** `jogo/simulacao` + `--simular N [--dificuldade D] [--seed S] [--saves P]`, sem janela; também `simulacao_salas.csv` (tempo e dano por sala) e `simulacao_resumo.txt`. Piloto v2: kit (Q), power-ups **pelo inventário**, porta por nota (`notaDaPorta`), baús, e olha a sala concluída antes de escolher. O "recuar com pouca vida" ficou de fora por decisão medida (HANDOFF §8).
- **Ordem de construção** (ver §21): v1 (Etapa 5b) anda, explora, combate e sai das salas · v2 (Etapa 6) pendências, portas, itens e loja · v3 (Etapa 8) missões e chefe do bioma 1 · v4 (biomas 2–6) uma ficha de chefe por bioma · v5 (Etapa 15) Covil e vitória completa.

### 24.8 Áudio (M07, agora P0)

**Implementado em 13/11 (adiantado da Etapa 7b), com os arquivos do Diego em `assets/musics`:**
- Efeitos (WAV PCM 16 bits, 44,1 kHz): mixer próprio `waveOut` numa thread acordada pelo dispositivo (`CALLBACK_EVENT`), 4 buffers de 1024 quadros, 8 vozes + 1 voz de ambiente em loop. Leitor de WAV puro e testado em `core/sons` (`lerWav`).
- **Músicas (MP3): MCI do Windows**, e não streaming próprio (decisão: não exige decodificador de MP3 no projeto). A música de jogo e a de **tensão** ficam abertas juntas; `definirTensao(0..1)` faz o crossfade de potência constante (cos/sen) em 1,5 s.
- Mapa em `data/sons.txt` (o teste `t_sons` confere que cada arquivo citado existe e é válido). Tensão hoje: salas com `musica=tensao` (ex.: a rua do bueiro do prólogo). Na Etapa 5: também quando um chefe-medo ou o Pennywise estiver a ≤ 8 células ou na arena de chefe.
- Sons ainda sem arquivo (ficam mudos até o Diego conseguir): dash, clarão, aviso de ataque, porta, balão estourando, parede quebrando, rato roubando, rugidos dos chefes, navegação de menu, pausa, vitória.
- `--mudo` (testes) e `--diag-audio arq.txt` (prova sem ouvir: estado e volume reais de cada faixa no MCI).

Plano original (mantido como referência):

- **Os arquivos de som são providenciados pelo Diego**, que indica a pasta; a licença fica registrada nos créditos. O código fica pronto antes, com uma tabela de IDs.
- `plataforma/audio.{h,cpp}`: mixer próprio com `waveOut` (44,1 kHz, 16 bits, estéreo; 3 buffers de 20 ms; **8 vozes** de efeito + 1 de música em loop, com crossfade de 0,5 s entre faixas). WAV PCM carregados na memória no início; música em streaming do disco se passar de ~20 MB. Nenhuma dependência nova. Se o dispositivo de áudio falhar, o jogo segue mudo.
- As regras **não tocam som**: emitem eventos (`std::vector<EventoSom>` no `EstadoPartida`: golpe, acerto, dash, item, porta, aviso, nível…), e a cena os consome. Assim, a simulação sem janela fica muda de graça.
- `data/sons.txt`: `id;arquivo;volume;variacoes` (ex.: `golpe_taco;sfx/golpe_taco_*.wav;0.8;3`). Música por contexto: menu, Clube, cada bioma, chefe, vitória, derrota.
- Opções: volume geral, de música e de efeitos; mudo ao perder o foco (opcional).
- Lista mínima de IDs (para o Diego buscar): passo, golpe (taco/estilingue/cano), acerto, crítico, dano no Bill, dash, clarão, kit, coleta de item, moeda, porta abrindo, porta trancada, baú, aviso de ataque, subida de nível, Eco escolhido, morte de inimigo, balão estourando, parede quebrando, rato roubando, rugido de chefe (6 + Pennywise), vitória, derrota, menu (mover/confirmar/voltar), pausa. Músicas: menu, Clube, 6 biomas, chefe, Covil, vitória, derrota.

### 24.9 Entrega e defesa (novidades)

- **Entrega:** um arquivo de texto com o **link do GitHub** (ou upload no material didático). O projeto ainda não é um repositório git: falta criá-lo (privado até a entrega), com o `.gitignore` atual (o `.env` nunca sobe) e um `README.md` de compilação e execução. Pendente: conta e visibilidade (Diego).
- **Defesa em 2 etapas:** perguntas individuais e **modificações individuais no código**. O código orientado a dados (salas, dificuldade, itens, Ecos, fichas dos chefes, sons) e o `docs/DEFESA.md`, com um roteiro de "como alterar X" por integrante, tornam isso treinável.
- **Modalidade:** dupla ou trio (trabalho individual perde 20%). Decisão em aberto do Diego.
