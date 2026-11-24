# Análise: rodar IT Ecos de Derry no navegador, servido pela AWS

Pergunta do Diego (23/11/2026): é possível subir o jogo completo para a AWS e jogá-lo no navegador, sem
delay, sem lag, sem travar, com todas as músicas, a mesma qualidade de hoje, HTTPS com domínio próprio e
sem bloqueio de navegador? Ou o jogo teria que estar em outra linguagem ou tecnologia?

Decisão registrada: **o porte web só será feito no final, com o jogo inteiro pronto** (depois das Etapas
14 a 17). O plano técnico está em `instructions/SDD-Web-AWS-IT-Ecos-de-Derry.md`.

## Resposta curta

**Sim, é possível, e sem reescrever o jogo.** C++ roda no navegador: o compilador **Emscripten** gera
**WebAssembly**, que o navegador executa perto da velocidade nativa. Não precisa de outra linguagem nem de
engine, e o jogo foi construído de um jeito que ajuda.

## O que foi medido no código (23/11/2026)

| Parte | Tamanho | No navegador |
|---|---|---|
| **Regras** (combate, IA, piloto, salas, itens, história) | 11.140 linhas, **63%** | Compila sem mudar nada; já roda sem janela nos testes |
| Cenas, menus, HUD, renderizador próprio em pixels | ~5.000 linhas | Quase tudo igual: o jogo desenha num buffer de pixels, que é o que o navegador mostra |
| **Camada Windows** (janela, áudio, controle, fontes, PNG, `main.cpp`) | ~1.500 linhas | É o que se reescreve, como um segundo "back-end" ao lado do Windows |

Outras medidas:
- Quadro na luta mais pesada (Pássaro das Cinzas, piloto): **2,4 ms** em 1280×720 (p95 2,8 ms).
- Cena da história: ~8,7 ms em 1280×720 (zoom bilinear em software). É o ponto mais pesado: na web vai para a GPU.
- Memória do jogo no pico: **316 MB** (1280×720).
- Assets HD: **173 MB**, dos quais **115 MB são as cut scenes**; músicas e efeitos: 19 MB.

A versão Windows continua intacta e **continua funcional para quem joga no PC** (pedido do Diego): o CMake
passa a gerar dois alvos (`.exe` e web) do mesmo código. A web é um segundo alvo, não uma substituição, e
isso também preserva a defesa com alterações ao vivo.

## Os dois caminhos na AWS

### Caminho A (recomendado): o jogo roda no navegador da pessoa (WebAssembly)

A AWS só **entrega os arquivos**; depois de carregado, o jogo roda 100% no computador do jogador.
- **Delay e lag: zero de rede durante o jogo**, porque nada vai e volta ao servidor a cada quadro. É o único
  caminho que cumpre "sem delay" de verdade.
- **Qualidade:** a mesma; é o mesmo código desenhando os mesmos pixels HD.
- **Custo:** hospedagem estática, centavos a poucos dólares por mês.

### Caminho B: streaming (Amazon GameLift Streams)

O `.exe` atual roda **sem mudança** numa máquina com GPU da AWS e chega ao navegador como vídeo (WebRTC).
Desde janeiro de 2026 também em **São Paulo (sa-east-1)**.
- Prós: zero de porte; uma ou duas semanas de configuração.
- Contras: há sempre latência (codificar, rede, decodificar), então "sem delay" não é garantido; a
  compressão de vídeo prejudica pixel art, cenas escuras e partículas; cobra **por hora de capacidade de GPU
  alocada, mesmo ociosa, sem free tier**, e cada jogador simultâneo ocupa uma máquina (o jogo nem usa GPU).

**Veredito:** o A é o caminho certo. O B só faria sentido para publicar amanhã sem tocar no código.

## O que o caminho A exige, peça por peça

| Hoje (Windows) | No navegador | Observação |
|---|---|---|
| Janela Win32 + DIB | `<canvas>` + **WebGL** (o buffer de pixels vira textura) | Tela cheia só com um clique (regra do navegador) |
| Loop com `QueryPerformanceCounter` | `requestAnimationFrame` (`emscripten_set_main_loop`) | O passo fixo do jogo continua igual |
| Teclado Win32 | eventos de teclado do navegador | **Esc sai da tela cheia** e F5/F11 são do navegador: pausa precisa de tecla alternativa na web (ou Keyboard Lock, só Chromium) |
| SDL2 + XInput (8BitDo) | **Gamepad API** (SDL2 do Emscripten) | Funciona; o controle aparece depois de apertar um botão; exige HTTPS |
| GDI+ (PNG) | `stb_image` ou decodificação do navegador | Simples |
| **Fontes GDI (Segoe UI, Georgia)** | `stb_truetype`/FreeType com arquivos de fonte embutidos | **Licença:** Georgia e Segoe UI não podem ser redistribuídas; usar equivalentes livres (Gelasio tem as medidas da Georgia; Noto/Inter no lugar da Segoe) ou comprar licença webfont |
| Mixer `waveOut` em thread | **Web Audio** (AudioWorklet) | Roda na thread de áudio do navegador; o jogo não precisa de threads |
| MP3 por MCI + crossfade de tensão | Web Audio (`decodeAudioData` + nós de volume) | Todas as músicas e o crossfade mantidos |
| `saves\` em disco | **IndexedDB** (IDBFS) | Perfil, recordes e cenas vistas ficam no navegador do jogador |
| (não existe) | **Tela "Clique para jogar"** | Obrigatória: nenhum navegador toca som antes de um clique; todo jogo web faz isso |

### Os dois pontos de engenharia de verdade

1. **Desempenho.** A luta mais pesada fica em ~8–10 ms por quadro estimados em 1080p e WebAssembly: cabe
   em 60 fps. A cena da história (zoom bilinear em software) passa do orçamento em 1080p: na web vai para a
   **GPU (WebGL)**, onde é trivial. Ligar **SIMD do WebAssembly** nos laços de pixel. Evitar threads
   (pthreads): assim não se precisa dos cabeçalhos COOP/COEP, que bloqueiam recursos externos.
2. **Tamanho e memória.** Carregar o núcleo (sprites e texturas) com barra de progresso; cut scenes **sob
   demanda** (o jogo já faz isso) e músicas em streaming; PNG → **WebP** (tipicamente 30–60% menor) ou AVIF;
   **Service Worker** para a segunda visita ser instantânea. Meta: entrar jogando com ~30–50 MB baixados.

## Infraestrutura AWS (HTTPS completo, sem bloqueios)

- **S3** privado + **CloudFront** (Origin Access Control), Brotli, HTTP/2 e HTTP/3, `application/wasm`
  correto, cache longo com nomes versionados.
- **Route 53**: compra do domínio (~US$ 13–15/ano para `.com`) e DNS.
- **ACM**: certificado TLS gratuito e renovado sozinho (em us-east-1, por causa do CloudFront).
  Redirecionamento HTTP→HTTPS, HSTS.
- **Cabeçalhos de segurança** (política de resposta do CloudFront): CSP, `X-Content-Type-Options` etc.
  WAF opcional.
- **Infra como código** (CDK ou Terraform) e **CI/CD** (GitHub Actions): compila com o emsdk, roda os
  testes, publica e invalida o cache.
- **Custo:** o CloudFront tem 1 TB/mês grátis (alguns milhares de primeiros acessos por mês); hosted zone
  US$ 0,50/mês. Na prática, centavos a poucos dólares por mês.

## Esforço estimado

| Fase | O quê | Estimativa |
|---|---|---|
| 0 | emsdk + alvo web no CMake; regras em wasm; os testes rodando no Node | 1–2 dias |
| 1 | Back-end web: canvas/WebGL, loop, teclado, controle, PNG, fontes, saves | 4–6 dias |
| 2 | Áudio Web Audio: mixer, MP3, crossfade, tela "clique para jogar" | 3–4 dias |
| 3 | Desempenho e carregamento: cut scene na GPU, SIMD, WebP, lazy load, Service Worker | 3–5 dias |
| 4 | AWS: domínio, certificado, CloudFront, segurança, CI/CD; testes em Chrome/Edge/Firefox/Safari e 8BitDo | 2–3 dias |
| **Total** | | **~3 a 4 semanas de trabalho focado** |

Riscos: fontes (licença), Safari (áudio e controle), computadores muito fracos. Celular fica fora
(teclado ou controle).

## Recomendação

1. **Primeiro terminar o jogo** (Etapas 14–17): portar antes seria portar duas vezes o que ainda vai mudar.
2. Opcional, para tirar o risco cedo: um **piloto de 1–2 dias** (regras em WebAssembly e o piloto automático
   jogando no navegador, sem gráficos finais), sem tocar na versão Windows.
3. Depois da entrega, o porte completo pelo SDD.

Nenhum recurso AWS é criado nem domínio comprado sem confirmação do Diego. As credenciais em `.env` são
temporárias e precisarão ser renovadas quando o porte começar.

## Fontes consultadas (23/11/2026)

- [Amazon GameLift Streams: seis novas regiões, incluindo São Paulo (jan/2026)](https://aws.amazon.com/about-aws/whats-new/2026/01/glstreams-new-regions)
- [GameLift Streams: regiões e locais remotos](https://docs.aws.amazon.com/gameliftstreams/latest/developerguide/regions-quotas-rande.html)
- [GameLift Streams: preços (capacidade alocada, sem free tier)](https://aws.amazon.com/gamelift/streams/pricing)
- [AWS News Blog: GameLift Streams](https://aws.amazon.com/blogs/aws/scale-and-deliver-game-streaming-experiences-with-amazon-gamelift-streams)
- [Emscripten: pthreads e SharedArrayBuffer](https://emscripten.org/docs/porting/pthreads.html)
- [MDN: Cross-Origin-Embedder-Policy](https://developer.mozilla.org/docs/Web/HTTP/Reference/Headers/Cross-Origin-Embedder-Policy)
- [Chrome: habilitando SharedArrayBuffer (COOP/COEP)](https://developer.chrome.com/blog/enabling-shared-array-buffer.md.txt)
