# HANDOFF para o Codex: IT Ecos de Derry (24/11/2026, fim do dia)

Leia este arquivo INTEIRO antes de mexer em qualquer coisa. Depois leia `docs/HANDOFF.md` §8q e §8p (o que foi feito
hoje) só se precisar de detalhe. O dono do projeto é o **Diego** (fala português; responda em português).

## 1. O que é o projeto

Roguelike de ação em pixel art, **C++17 puro, sem engine**. Dois alvos do MESMO código:
- **Windows** (`.exe`, Win32/GDI): `build/ecos_de_derry.exe`. É a versão de entrega e defesa. NUNCA pode quebrar.
- **Navegador** (WebAssembly, Emscripten): `build-web/index.{html,js,wasm,data}`. Novo hoje (Etapa 18).

Regras (`src/core, mundo, entidades, combate, jogo`) não podem incluir `<windows.h>` (o CMake falha se incluir).

## 2. Estado AGORA (o que está pronto e o que NÃO está)

Pronto e verificado:
- Jogo completo; 304+ testes; piloto (computador) vence o jogo; menus repaginados; chefes em 8 direções; Pennywise IV
  palhaço-aranha; cenas nítidas; música com crossfade. Tudo descrito no `docs/HANDOFF.md`.
- Repositório PÚBLICO criado: **https://github.com/diegof-d2fs/it-ecos-de-derry** (1º commit `76a08c3` já enviado).
- Porte web funcionando localmente (testado com puppeteer: menu, dificuldade, Covil, cena, 120 fps, sem erros).

**NÃO commitado ainda** (está só no disco): todo o porte web + correções de hoje à tarde:
- `src/main_web.cpp`, `src/plataforma/audio_web.cpp`, `src/render/texto_web.cpp`, `src/render/imagem_web.cpp`,
  `web/shell.html`, `tools/export_web.py`, `tools/publicar_web.sh`, `third_party/stb/`, `third_party/fontes/`,
  mudanças em `CMakeLists.txt`, `canvas.*`, `texto.*`, `imagem.cpp`, `audio.cpp`, `terreno.cpp` (blocos `#ifdef __EMSCRIPTEN__`).
- Correções: ajudante não trava mais na luta (`jogo/companheiro.cpp`: posição de combate no flanco; desenho respira);
  golpe do Bill não some mais com o Georgie nem na esteira (`empurrarJogador` em `entidades/jogador.*`);
  `--fase-covil N` (e `?fase=N` na web); `--captura-limpa`; capturas ignoram o controle físico; Covil reequilibrado
  (Georgie: puxão 12, dano 8, velocidade 30; o "Salto de aranha" saiu da fase 3).

**Último resultado (24/11, 15h30):** 306 testes, 2 falhas, ambas do `b7_covil` em **8/10** pelo piloto. Histórico:
antes da correção do Georgie 10/10 (o bug deixava o Bill golpear em rajada); depois 6/10; Georgie suavizado 9/10;
tirar o salto da fase 3 deu 8/10 (variação entre seeds). Sugestão: devolver o "Salto de aranha" da fase 3
(`{"Salto de aranha", FormaPadrao::Salto, 3, 90.0f, 260.0f, 0.7f, 1.3f, 0.0f, 50.0f, 0.0f, 0, 2, 0.6f},` antes de
"Luzes Mortas") e, em vez disso, aumentar o preparo dos golpes da fase 3 (ex.: "Bote no escuro" 0.7 → 0.85) ou
`INTERVALO_ESCURIDAO`; medir com o teste. Para ver ONDE o piloto morre, um diagnóstico por seed já foi feito: a
morte foi na fase 3, pelo próprio Pennywise IV (`p.ultimoAgressor == 14`).
**Pendente verificar:** a bateria de testes estava rodando quando a sessão acabou. Resultado em
`C:\Users\diego\AppData\Local\Temp\claude\51ad7e0ca86d0c87\b8a2a18e-3128-44b0-a53f-de945cbed464\scratchpad\fim.txt`
(se não existir, rode de novo, ver §4). O critério é **0 falhas**; o teste mais sensível é
`piloto_limpa_e_sai_de_todas_as_salas_reais` (cada sala real 10/10, inclusive `b7_covil`).

## 3. Tarefas, nesta ordem

1. **Rodar os testes** (§4). Se `b7_covil` ficar abaixo de 10/10, ajuste SÓ números do Covil em
   `src/jogo/partida.h` (`PUXAO_GEORGIE`, `DANO_GEORGIE`, `VELOCIDADE_GEORGIE`) ou a tabela `PADROES_PENNYWISE_IV` em
   `src/entidades/inimigos.cpp` (preparo maior = mais fácil; peso menor = aparece menos). Nunca mude o piloto para
   "passar no teste". Rode de novo até 0 falhas.
2. **Recompilar a web** e conferir que abre (§5).
3. **Commitar e enviar** tudo para o GitHub (§6). Mensagem em português. Termine a mensagem com
   `Co-Authored-By:` se o Diego pedir; não inclua segredos.
4. **Atualizar a pasta da apresentação** `D:\algoritmos_prog_2\ECOS_APRESENTACAO` com o `.exe` novo: copie
   `build/ecos_de_derry.exe` e as pastas `build/assets` e `build/data` para lá. **NÃO toque em `ECOS_APRESENTACAO\saves`.**
   Só faça isso se o Diego não estiver com o jogo aberto (o `.exe` fica travado).
5. **AWS: só com o "ok" explícito do Diego** (ele ainda está testando no navegador). Quando ele autorizar, rode
   `bash tools/publicar_web.sh` (Git Bash). Ele cria um bucket S3 privado + CloudFront com HTTPS e devolve a URL
   `https://xxxx.cloudfront.net/`. As credenciais estão no `.env` (sessão SSO temporária, válida até ~19h50 de 24/11;
   se expirarem, peça ao Diego credenciais novas). **Nunca** crie domínio, Route 53 pago ou outros recursos sem perguntar.
   **Nunca** commite o `.env` nem `web/deploy.local.json`.
6. Opcional, se o Diego quiser: "Exportar/Importar saves" nas Opções da versão web (os saves ficam no IndexedDB do
   navegador; ver §7).
7. **Só no fim** (pedido do Diego): DEFESA.md e o SDD em PDF.

## 4. Compilar e testar (Windows)

```bash
cd /d/algoritmos_prog_2/rogue-like-it
export PATH="/c/msys64/ucrt64/bin:$PATH"
cmake --build build -j 8                       # se a pasta build sumir: cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
./build/testes_regras.exe                      # tudo (~5 min). Filtro: ./build/testes_regras.exe piloto
```
Armadilhas conhecidas:
- **Relógio do Windows**: se o make "não pega" uma mudança, os arquivos podem estar com data no futuro. Ajuste as datas
  para agora (PowerShell: `Get-ChildItem -Recurse -File | ? {$_.LastWriteTime -gt (Get-Date)} | % {$_.LastWriteTime=Get-Date}`)
  e rode `cmake --build build --clean-first`.
- Python com Pillow: `C:\Users\diego\AppData\Local\Microsoft\WindowsApps\python.exe` (o `python` do MSYS não tem PIL).
- Capturas do jogo: `build/ecos_de_derry.exe --captura X.bmp --captura-limpa --quadros N --sala ID --forte [--piloto]`
  (rode com `Start-Process -Wait` no PowerShell).

## 5. Versão web

```bash
cd /d/algoritmos_prog_2/rogue-like-it
/c/Users/diego/AppData/Local/Microsoft/WindowsApps/python.exe tools/export_web.py   # só se as imagens mudaram (gera assets/jogo_web)
export PATH="/c/msys64/ucrt64/bin:$PATH"; source /d/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-web -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release   # só na 1ª vez
cmake --build build-web -j 8
mkdir -p build-web/assets/musics && cp assets/musics/*.mp3 build-web/assets/musics/
cd build-web && /c/Users/diego/AppData/Local/Microsoft/WindowsApps/python.exe -m http.server 8765 --bind 127.0.0.1
```
Abrir `http://127.0.0.1:8765/`. Opções pela URL: `?sala=b7_covil&forte=1&piloto=1&fase=2`, `?bioma=3`, `?cena=final`.
Teste automático (puppeteer): `scratchpad/pup/teste_web.js` (Node do emsdk: `/d/emsdk/node/24.19.0_64bit/node.exe`).

Como funciona: `src/main_web.cpp` é o laço (emscripten_set_main_loop) e a entrada (teclado por `KeyboardEvent.code`,
controle pela Gamepad API); o canvas de software é copiado para o `<canvas id="tela">`. Texto: `stb_truetype` com
fontes livres em `third_party/fontes` (Gelasio = Georgia, Noto Sans = Segoe UI, Symbols2/Math para setas).
Imagens: `stb_image` (as ilustrações grandes vão em JPEG com nome .png em `assets/jogo_web`). Áudio: Web Audio
(efeitos) + `<audio>` (músicas em streaming). Saves: IDBFS em `/saves`. "Sair" na web volta ao menu inicial
(o item "Sair" do menu não aparece na web). Cuidados: `std::setlocale(LC_ALL, "C.UTF-8")` é obrigatório (senão o
swprintf para no primeiro acento); toda mudança de plataforma vai em arquivo `_web` ou em `#ifdef __EMSCRIPTEN__`,
sem alterar o comportamento do `.exe`.

## 6. Git

```bash
cd /d/algoritmos_prog_2/rogue-like-it
git status
git add -A && git commit -m "..."      # confira antes que nenhum .env, build, saves ou assets/*.png de alta entrou
git -c credential.helper= -c "credential.https://github.com/.helper=!gh auth git-credential" push
```
A conta ativa do `gh` é a **diegof-d2fs** (a diisilva também está logada; não use). O `.gitignore` já exclui build,
saves, masters de arte (`assets/*.png`), `assets/jogo_web`, `.env` e `web/deploy.local.json`.

## 7. Regras do Diego que não podem ser quebradas

- Textos do jogo: **nunca travessão** (—); todo texto com botão passa por `ajuda("{ATACAR} ...")`.
- Avisos e golpes só como sprites (nada de retângulo/círculo geométrico na tela de jogo).
- **Toda imagem nova** gerada por IA: mostre o caminho ao Diego e espere a aprovação antes de usar.
- A versão Windows continua funcional e é a principal.
- Nada que custe dinheiro ou seja público sem perguntar (domínio, recursos novos na AWS).
- Saves na web: hoje ficam no navegador (IndexedDB). A recomendação passada ao Diego: manter assim e criar
  "Exportar/Importar saves" (baixar e enviar um arquivo); S3 por jogador exigiria login (Cognito) e API: não fazer
  sem ele pedir.
