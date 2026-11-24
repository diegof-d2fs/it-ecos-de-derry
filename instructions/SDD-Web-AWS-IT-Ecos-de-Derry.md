# SDD: IT Ecos de Derry no navegador (WebAssembly + AWS)

| | |
|---|---|
| Versão | 1.0 (23/11/2026) |
| Estado | **Planejado para o FINAL**: começa só depois do jogo inteiro pronto (Etapas 14 a 17 concluídas e entregues) |
| Autoria | Tech lead (AWS, C++, jogos), a pedido do Diego |
| Análise de origem | `instructions/ANALISE-Web-AWS-IT-Ecos-de-Derry.md` |
| Etapa no plano | **Etapa 18: Porte web** (HANDOFF §3) |

## 1. Objetivo

Publicar o jogo completo num domínio próprio, em HTTPS, para ser jogado no navegador **com a mesma
qualidade do executável Windows**: mesma arte HD, mesmas regras, todas as músicas e efeitos, teclado e
controle (8BitDo), 60 quadros por segundo estáveis, sem latência de rede durante o jogo e sem bloqueios de
navegador além dos que a própria plataforma impõe a todos (um clique para liberar o som).

### 1.1 Não objetivos

- Celular e tablet (o jogo é de teclado ou controle; tela de toque fica fora).
- Multijogador, contas, ranking online ou backend com servidor de jogo.
- Streaming de vídeo (GameLift Streams): descartado na análise (latência, compressão sobre pixel art,
  custo por hora de GPU por jogador).
- Reescrever o jogo em outra linguagem ou engine.

### 1.2 Restrições (vêm do projeto acadêmico)

- A **versão Windows continua existindo e sendo mantida** (para quem joga no PC, e é a de entrega e defesa).
  A web é um **segundo alvo**, não uma substituição. Nenhuma mudança do porte pode alterar o comportamento do
  `.exe`; os 286+ testes continuam passando nos dois alvos.
- O código segue em **structs + funções livres** (sem herança), legível para a defesa com alteração ao vivo.
- A fronteira **regras × plataforma** (CMake recusa `<windows.h>` nas pastas de regras) continua valendo e é
  a base do porte.

## 2. Requisitos e critérios de aceite

| # | Requisito | Medida de aceite |
|---|---|---|
| **R0** | **A versão Windows continua funcional e de primeira classe** (pedido do Diego, 23/11): quem joga no PC baixa e roda o `.exe` como hoje, com todas as funções (tela cheia, F-keys, 8BitDo, saves em `saves\`) | O `.exe` é compilado, testado e publicado a cada versão, junto com a web; os testes rodam nos dois alvos; nenhuma mudança do porte altera o comportamento do `.exe`; o site oferece também o download do `.exe` (zip) |
| R1 | Mesma jogabilidade | Mesma seed = mesma partida (regras compiladas do mesmo fonte); testes de regras passam em wasm (Node) |
| R2 | Mesma qualidade visual | Captura do navegador e do `.exe` (mesma seed, mesmo quadro) com diferença só de fonte (ver §6.4) |
| R3 | 60 fps estáveis | p95 do quadro ≤ 14 ms em 1920×1080 no Chrome, num notebook i5 de 2020 com GPU integrada; zero "travadas" > 50 ms durante 10 min de piloto |
| R4 | Sem lag | Nenhuma chamada de rede durante a partida (só downloads de assets sob demanda, com pré-busca) |
| R5 | Todas as músicas | 3 músicas com crossfade de tensão e 13+ efeitos tocando como no Windows |
| R6 | Controle | 8BitDo e controle Xbox no Chrome, Edge e Firefox, mesmas posições de botão |
| R7 | Saves | Perfil, recordes, cenas vistas e telemetria persistem entre visitas (IndexedDB) |
| R8 | Carregamento | Jogável com ≤ 50 MB baixados; segunda visita < 3 s até o menu (cache) |
| R9 | HTTPS e segurança | Nota A no Mozilla Observatory e no SSL Labs; HSTS; CSP sem `unsafe-eval` |
| R10 | Navegadores | Chrome, Edge, Firefox atuais: tudo; Safari: tudo, com controle "melhor esforço" |

## 3. Arquitetura

```
                    ┌──────────────── mesmo código-fonte C++ ─────────────────┐
                    │  regras (core, mundo, entidades, combate, jogo)          │  ← 63%, sem mudança
                    │  cenas + render em software (canvas de pixels)           │  ← quase sem mudança
                    └───────────────┬───────────────────────────┬─────────────┘
                     back-end WIN32 │                           │ back-end WEB
          plataforma/win32 (atual)  │                           │  plataforma/web (novo)
          Win32, DIB, GDI, GDI+,    │                           │  Emscripten: canvas + WebGL2,
          waveOut + MCI, XInput,    │                           │  Web Audio (AudioWorklet), Gamepad API,
          SDL2 dinâmico             │                           │  stb_image, stb_truetype, IDBFS
                                    ▼                           ▼
                          ecos_de_derry.exe              ecos.wasm + ecos.js + index.html
                                                                │
                              S3 (privado) ── CloudFront (OAC, HTTP/3, Brotli, cabeçalhos) ── Route 53
                                                     │                         ACM (TLS, us-east-1)
                                                 navegador
```

Princípio: **um back-end por plataforma, escolhido no CMake**. As cenas e as regras não sabem em qual estão.

### 3.1 Contrato da plataforma (o que cada back-end implementa)

Hoje essas funções existem só para Win32. O porte as declara num cabeçalho neutro e dá duas implementações.

| Módulo | Funções (assinaturas atuais, mantidas) | Win32 (hoje) | Web (novo) |
|---|---|---|---|
| `plataforma/janela` | `criarJanela`, `processarMensagens`, `apresentarCanvas`, `alternarTelaCheia`, `apresentacaoAtual` | Win32 + StretchDIBits | `<canvas>` + WebGL2 (textura + quad), Fullscreen API |
| `plataforma/tempo` | `iniciarRelogio`, `agoraSegundos`, `esperarProximoQuadro` | QPC + Sleep | `emscripten_get_now`; o laço vira `emscripten_set_main_loop` (rAF) |
| `plataforma/teclado` | `aplicarTeclado(teclas, entrada)` | tabela de VK | tabela de `KeyboardEvent.code` (mesmos comandos) |
| `plataforma/gamepad` | `iniciarGamepad`, `atualizarGamepad`, `simularGamepad` | SDL2 dinâmico + XInput | Gamepad API (`emscripten_get_gamepad_status`), mapeamento "standard" |
| `plataforma/audio` | `iniciarAudio`, `tocarEfeito`, `ligarAmbiente`, `definirMusica`, `definirTensao`, `atualizarAudio`, `definirVolumes` | waveOut + mixer em thread; MCI | Web Audio: mixer em AudioWorklet, músicas por `AudioBufferSourceNode`/`MediaElement` + `GainNode` |
| `render/imagem` | `carregarPng` | GDI+ | `stb_image` (ou `createImageBitmap` via JS) |
| `render/texto` | `criarFonte`, `desenharTexto`, `larguraTexto` | GDI (`TextOutW`) | `stb_truetype`: atlas de glifos por tamanho e escala, desenho no canvas |
| `render/canvas` | `criarCanvas` (DIB) e o resto (pixels) | DIB section | `std::vector<uint32_t>`; tudo o mais é igual |
| `core/arquivo` (saves) | `lerArquivoTexto`, `gravarArquivoTexto` | disco | IDBFS montado em `/saves` + `FS.syncfs` após gravar |
| `main.cpp` | laço, argumentos, comandos globais | `WinMain` | `main` + `emscripten_set_main_loop_arg`; argumentos vêm da URL (`?cena=final`) |

`render/canvas.h` hoje expõe `HDC`/`HBITMAP` dentro de `Canvas`: o porte move esses campos para o
back-end Win32 (um `void* nativo` opaco), sem mudar quem desenha.

## 4. Laço principal e tempo

- Win32: o laço atual (passo fixo de 1/60 s com acumulador) continua.
- Web: `emscripten_set_main_loop_arg(passoQuadro, &jogo, 0, true)` (0 = ritmo do `requestAnimationFrame`,
  inclusive 120/144 Hz). O **mesmo acumulador** decide quantos passos lógicos rodar; acima de 3 passos
  atrasados, descarta (aba em segundo plano não pode gerar "espiral da morte").
- Aba oculta (`visibilitychange`): pausa automática, igual à perda de foco no Windows (`pausadoPorFoco`).
- **Sem pthreads.** Áudio roda na thread do navegador (AudioWorklet), não na do jogo. Isso dispensa
  `SharedArrayBuffer` e os cabeçalhos COOP/COEP (que bloqueariam recursos de outra origem).

## 5. Vídeo

### 5.1 Apresentação

- O canvas de software continua sendo a fonte da verdade (640×360 lógicos × escala inteira).
- Cada quadro: `glTexSubImage2D` do buffer RGBA para uma textura e um quad em tela cheia (WebGL2). Ganho:
  escala e borda pretas feitas pela GPU; a cópia de 1920×1080×4 ≈ 8 MB por quadro é barata para o upload.
- A escala segue a regra atual (maior inteira que cabe); o resto vira borda preta (`apresentacaoAtual`).
- `devicePixelRatio`: o canvas HTML tem o tamanho em pixels físicos (nítido em telas 4K e Retina).

### 5.2 Orçamento de desempenho

Medido no `.exe` (23/11/2026): luta mais pesada 2,4 ms (p95 2,8) em 1280×720. Estimativa web: ×2,25 de
pixels (1080p) × 1,5–2 (wasm) ≈ 8–10 ms. Cabe em 16,6 ms. Ações:
1. `-O3 -msimd128 -flto` e `-sALLOW_MEMORY_GROWTH`.
2. **SIMD do WebAssembly** nos laços quentes: `misturar`/`atenuar` (canvas), escuridão (`render/luz`),
   `desenharRecorteTela`.
3. **Cut scene na GPU**: hoje o zoom bilinear em software leva ~8,7 ms em 720p (passaria do orçamento em
   1080p). Na web, `desenharImagemAmostrada` vira uma textura WebGL com amostragem linear e matriz de zoom/pan
   desenhada por baixo do canvas de software (o texto e o degradê continuam no canvas, com alfa). A imagem
   não passa mais pelo wasm.
4. Perfilar com o Chrome DevTools (Performance) a cada fase; o medidor F1 do jogo continua valendo.

### 5.3 Memória

- `.exe`: 316 MB no pico (todas as imagens HD + cópia preparada por escala).
- Web: heap wasm com crescimento até 2 GB; meta de pico ≤ 600 MB.
- As cut scenes já são sob demanda (`Recursos::sobDemanda`, 23/11). Ampliar a mesma ideia: texturas e
  sprites **por bioma** (o bioma 3 não precisa estar na memória no bioma 1). As fontes HD (`Recursos::fontes`)
  são liberadas depois de preparar a escala atual, e relidas do cache se a escala mudar.

## 6. Assets e carregamento

### 6.1 Pacotes

| Pacote | Conteúdo | Quando |
|---|---|---|
| `nucleo` | menu, HUD, Bill, amigos, efeitos, bioma 1, Clube, fontes, sons curtos | antes do menu (barra de progresso) |
| `bioma_N` | texturas, objetos, inimigos e chefe do bioma N | pré-busca ao entrar no bioma N−1 |
| `cena_*` | um arquivo por quadro de cut scene | pré-busca da cena seguinte possível; a cena pede os seus ao abrir |
| músicas | 3 MP3 | streaming (`<audio>` ligado ao Web Audio), sem esperar baixar tudo |

Meta: menu jogável com ≤ 50 MB. Os 173 MB atuais (115 MB de cut scenes) viram downloads pequenos e sob
demanda.

### 6.2 Formatos

- Imagens: **WebP sem perda** para sprites e ícones (bordas de pixel art intactas); WebP com perda
  (qualidade ≥ 90) ou AVIF para cut scenes e texturas grandes. O `export_final.py` ganha `--web`.
- Áudio: MP3 continua (todos os navegadores tocam); WAV curtos → OGG/Opus ou MP3 (o Safari toca Opus em
  versões recentes; manter MP3 como reserva).
- Nomes com hash de conteúdo (`cena_final_3.7c1e9a.webp`) e `Cache-Control: max-age=31536000, immutable`;
  só o `index.html` e o manifesto não ficam em cache longo.

### 6.3 Cache local

Service Worker com cache dos pacotes já baixados: segunda visita sem rede até o menu; atualização do jogo
troca o manifesto e o SW baixa só o que mudou.

### 6.4 Fontes (licença)

Segoe UI e Georgia **não podem ser redistribuídas**. Opções, em ordem:
1. Equivalentes livres (SIL OFL): **Gelasio** (métricas da Georgia) nas cenas da história; **Noto Sans** ou
   **Inter** no lugar da Segoe UI. Diferença visual mínima; teste de captura compara layout (quebras de linha).
2. Comprar licença webfont da Georgia (Monotype) se o Diego quiser identidade 100% igual.
Rasterização com `stb_truetype` (já com suavização), atlas por tamanho × escala, recriado quando a escala muda
(igual às fontes GDI hoje). A versão Windows pode passar a usar as mesmas fontes, para os dois ficarem iguais.

## 7. Áudio

- **Desbloqueio:** tela inicial "Clique ou aperte um botão para jogar" (exigência de todos os navegadores
  para som). O clique cria o `AudioContext` e já pede tela cheia.
- **Efeitos:** o mixer atual (8 vozes + ambiente, PCM 16 bits, 44,1 kHz) é C++ puro e é reaproveitado: roda
  dentro de um **AudioWorklet** (o wasm do mixer compilado à parte, ou o mixer em JS recebendo comandos por
  `MessagePort`). Alternativa mais simples: um `AudioBuffer` por efeito e um `AudioBufferSourceNode` por toque.
  Decisão na Fase 2 pela latência medida (meta < 30 ms do golpe ao som).
- **Músicas e crossfade de tensão:** dois canais (jogo e tensão) com `GainNode`, mesma curva cos/sen de hoje
  (`atualizarAudio`). `definirMusica` troca a fonte; volumes do menu Opções aplicados ao `GainNode` mestre.
- **Mudo sem foco:** igual ao Windows (aba oculta → ganho 0).

## 8. Entrada

### 8.1 Teclado

- Tabela `KeyboardEvent.code` → `Comando` (mesmo `aplicarTeclado`), `preventDefault` nas teclas do jogo.
- **Conflitos do navegador** e solução:

| Tecla | Problema | Solução web |
|---|---|---|
| Esc (Pausa/Voltar) | sai da tela cheia | Keyboard Lock API no Chromium (captura Esc em tela cheia); nos outros, Pausa também em **P** e **Tab**, e a dica mostra a tecla certa (`ajuda()` já adapta textos ao dispositivo) |
| F11 | tela cheia do navegador | trocar por botão na interface + Fullscreen API |
| F5 | recarrega a página | não usar; aviso de "sair sem salvar?" (`beforeunload`) só durante a tentativa |
| F1, F2, F10 | ajuda/menus do navegador | remapear depuração e piloto (F10 → **Ctrl+P**/menu "Assistir"); `preventDefault` onde o navegador permitir |

### 8.2 Controle

- Gamepad API com `mapping === "standard"`: mesmas posições do Windows (baixo = Dash/Confirmar). Rótulos
  Nintendo pelo `id` do controle (mesma regra de `definirRotulosNintendo`).
- O navegador só expõe o controle depois de um botão apertado: a tela inicial aceita "aperte um botão".
- A tela de teste de botões (F2 hoje) entra no menu Controles.

## 9. Persistência

- `/saves` montado com IDBFS: `perfil.txt`, `recordes.txt`, `telemetria.csv` continuam no mesmo formato.
- Depois de cada gravação: `FS.syncfs(false)`; na abertura: `FS.syncfs(true)` antes do menu.
- Exportar/importar perfil (baixar/enviar arquivo) nas Opções, porque o IndexedDB some se a pessoa limpar
  os dados do site.

## 10. Infraestrutura AWS

### 10.1 Componentes

| Serviço | Uso | Configuração |
|---|---|---|
| **Route 53** | registro do domínio e DNS | domínio `.com` (~US$ 13–15/ano); hosted zone; registros A/AAAA alias para o CloudFront |
| **ACM** (us-east-1) | certificado TLS | validação por DNS; renovação automática; apex + `www` |
| **S3** | arquivos do jogo | bucket privado, versionamento ligado, criptografia SSE-S3, **Block Public Access** total |
| **CloudFront** | entrega global | Origin Access Control (só o CloudFront lê o S3); HTTP/2 e HTTP/3; compressão Brotli/Gzip; TLS 1.2+ (política `TLSv1.2_2021`); redirecionar HTTP→HTTPS; price class com América do Sul |
| **Response Headers Policy** | segurança | HSTS (`max-age=63072000; includeSubDomains; preload`), CSP (abaixo), `X-Content-Type-Options: nosniff`, `Referrer-Policy: strict-origin-when-cross-origin`, `Permissions-Policy` (só `gamepad` e `fullscreen`) |
| **WAF** (opcional) | proteção | regras gerenciadas comuns + limite de taxa; liga só se houver abuso (custo fixo mensal) |
| **CloudWatch** | métricas e alarmes | alarme de custo (Budgets: US$ 10/mês) e de erro 5xx |

CSP inicial: `default-src 'self'; script-src 'self' 'wasm-unsafe-eval'; worker-src 'self'; media-src 'self' blob:;
img-src 'self' blob: data:; connect-src 'self'; style-src 'self'; frame-ancestors 'none'`.
(`wasm-unsafe-eval` é a permissão específica para compilar WebAssembly, sem liberar `eval` de JavaScript.)

Tipos MIME: `.wasm` → `application/wasm` (necessário para compilação em streaming), `.webp`, `.mp3`, `.js`.

### 10.2 Infraestrutura como código

**AWS CDK (TypeScript)** num diretório `infra/`: uma stack de certificado em us-east-1 e uma stack do site
(S3, CloudFront, políticas, Route 53). Nada é criado à mão no console. `cdk diff` revisado antes de cada
`cdk deploy`.

### 10.3 Entrega contínua

GitHub Actions:
1. `emsdk` (versão fixada) → `emcmake cmake` → build web.
2. **Testes de regras no Node** (o mesmo `testes_regras` compilado para wasm) + testes do `.exe` no Windows.
3. `export_final.py --web` (assets WebP com hash) → `aws s3 sync --delete` → invalidação só de `index.html`
   e manifesto.
4. Credenciais por **OIDC** (papel IAM com permissão mínima: escrever no bucket e invalidar a distribuição);
   nenhuma chave de acesso fixa no repositório.

### 10.4 Custos esperados

Hospedagem estática: CloudFront com 1 TB/mês grátis (milhares de primeiros acessos por mês), S3 ~US$ 0,03/GB,
hosted zone US$ 0,50/mês, domínio ~US$ 13–15/ano. Estimativa: **até US$ 5/mês** fora picos. Budget com alarme.

## 11. Segurança e privacidade

- Site 100% estático: sem servidor de aplicação, sem banco, sem dados pessoais coletados.
- Telemetria fica só no navegador do jogador (IndexedDB), como hoje fica no `saves\`.
- IAM com privilégio mínimo; contas de deploy separadas da conta raiz; MFA na raiz.
- As credenciais temporárias do `.env` **não entram** no repositório nem no build (`.gitignore`).

## 12. Testes

| Nível | O quê | Onde |
|---|---|---|
| Regras | os 286+ testes compilados para wasm | Node no CI |
| Paridade | mesma seed e mesmo roteiro (`--roteiro`) no `.exe` e no navegador → comparar estado final e capturas | Playwright (Chromium) |
| Desempenho | 10 min de piloto em 1080p; p95 do quadro e quadros > 50 ms | Chrome DevTools/Playwright tracing |
| Áudio | música, crossfade, efeitos, mudo sem foco | manual + `diagnosticoAudio` exposto no console |
| Controle | roteiro do HANDOFF §7 no Chrome, Edge, Firefox | manual com o 8BitDo |
| Segurança | Mozilla Observatory, SSL Labs, `securityheaders.com` | após deploy |

## 13. Plano de execução (Etapa 18)

| Fase | Entrega | Estimativa |
|---|---|---|
| 18.0 | emsdk, alvo web no CMake, `Canvas` sem tipos Win32, regras em wasm, testes no Node | 1–2 dias |
| 18.1 | Back-end web: canvas + WebGL2, laço rAF, teclado, controle, PNG, fontes (Gelasio/Noto), saves IDBFS | 4–6 dias |
| 18.2 | Áudio Web Audio: mixer, músicas, crossfade, tela de desbloqueio | 3–4 dias |
| 18.3 | Desempenho e carregamento: cut scene na GPU, SIMD, pacotes por bioma, WebP, Service Worker | 3–5 dias |
| 18.4 | AWS (CDK): domínio, certificado, S3, CloudFront, cabeçalhos, budget; CI/CD; testes em 4 navegadores | 2–3 dias |
| **Total** | | **~3 a 4 semanas** |

Opcional e independente: um **piloto de 1–2 dias** (18.0 + o piloto automático jogando no navegador sem os
gráficos finais) para provar a viabilidade antes do porte completo.

## 14. Riscos

| Risco | Prob. | Impacto | Mitigação |
|---|---|---|---|
| Fontes diferentes mudam quebras de linha | média | baixo | Gelasio tem métricas da Georgia; teste de paridade das telas com texto |
| PC fraco não segura 60 fps em 1080p | média | médio | SIMD, cut scene na GPU; cair para escala 2 só se p95 > 14 ms (opção nas Opções) |
| Safari: áudio e controle | média | médio | desbloqueio por clique; controle "melhor esforço" documentado |
| Memória do wasm | baixa | alto | pacotes por bioma; liberar fontes HD; alarme no medidor F1 |
| Esc/F-keys do navegador | alta | baixo | tabela do §8.1 |
| Divergência entre `.exe` e web | baixa | alto | mesmo fonte de regras; testes de paridade por seed no CI |

## 15. Decisões em aberto (Diego)

1. Nome do domínio.
2. Fontes: equivalentes livres ou comprar a Georgia?
3. Repositório público ou privado (afeta GitHub Actions e a defesa).
4. Página de entrada: só o jogo, ou capa com créditos e o botão "Baixar para Windows" (zip do `.exe`, que
   continua sendo publicado a cada versão, R0)?
