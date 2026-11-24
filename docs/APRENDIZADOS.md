# Aprendizados — IT: Ecos de Derry

Registro acumulado de lições que valem para as próximas etapas. Formato: **lição** — por quê — como aplicar. Acrescente no fim; não apague (se algo mudar, marque como superado).

## Herdados do Bomberman (Diego + Gabriel, out/2026)

1. **8BitDo SN30 Pro aparece como "Pro Controller" (VID_057E/PID_2009) no DirectInput e gera eixos presos e botões fantasmas.** — O diagnóstico do Bomberman mostrou `lX=-1000` preso e máscara de botões variando sozinha. — Usar **SDL2 GameController** (`SDL2.dll` oficial ao lado do exe, carregada por `LoadLibraryExW`) e XInput como fallback; **nunca DirectInput**. "Reconhecido" (enumerado) não é o mesmo que "funcional": testar com o botão apertado.
2. **Procurar joystick novo todo segundo travava o jogo sem joystick (~200 ms por busca).** — A enumeração é cara. — Sem controle conectado, procurar no máximo a cada 2 s; com controle, só checar a desconexão.
3. **Joystick nunca cria um jogador automaticamente.** — No Bomberman, plugar o controle criou o P2 sozinho e ninguém conseguia jogar. — O dispositivo é atribuído por escolha explícita; aqui há só 1 jogador e teclado e joystick funcionam juntos.
4. **`Gdiplus::Graphics::DrawImage` por quadro foi o gargalo de desempenho**, assim como a interpolação bicúbica no preparo de sprites. — Usar GDI+ só para decodificar PNG no carregamento; desenhar num buffer próprio (DIB) e apresentar com `StretchBlt`/`COLORONCOLOR`.
5. **Não confiar em binários de terceiros instalados na máquina** (ex.: a `SDL2.dll` do PCSX2). — Empacotar a DLL oficial com licença (zlib) e documentar a versão.

## Da produção de assets (Gemini, nov/2026)

6. **Nunca anexar o retrato de um personagem como referência de outro.** — O modelo copia a identidade (aconteceu com Beverly e Mike). — Para layout, usar o sprite de outro personagem só junto com uma "IDENTIDADE OBRIGATÓRIA" escrita.
7. **A API de imagem não devolve alfa: pedir fundo #00FF00 uniforme e recortar localmente.** — Pedir "transparente" gerava xadrez pintado. — `batch_images.py` acrescenta a nota de chroma; `_raw/` guarda o bruto.
8. **Efeitos (FX) com a cena completa como referência viravam colagens com moldura.** — Usar ícones isolados como referência de FX.
9. **Os caminhos das ferramentas quebram quando os documentos mudam de pasta.** — `Prompts-Assets-IT.md` foi movido para `instructions/` e os scripts ainda apontam para a raiz. — Os scripts devem localizar os arquivos de prompt por lista configurável e falhar com uma mensagem clara.
10. **IDs com 3 dígitos (P100+) quebram regex `P\d\d`.** — Usar `P\d{2,3}`.

## Do design (PRD v2, 10/11/2026)

11. **"Vidas utilizadas" do PDF combina com um hub roguelite sem trair "perde se a vida chegar a zero":** Pacto de Sangue = vida extra dentro da tentativa; sem Pacto, HP 0 encerra a partida e mostra o score. — Evita a discussão "roguelite não é roguelike" na defesa.
12. **Para 45–60 min, o conteúdo precisa ser controlado por dados** (salas por bioma, ondas, HP dos chefes em `data/balanceamento.txt`), com telemetria por sala. — Ajustar a duração sem recompilar.
13. **Reaproveitar os sprites de Pennywise I–III como fase 2 dos chefes-medo** (forma grande e lenta → palhaço pequeno e rápido). — Economiza arte e mantém a regra "Pennywise mais forte a cada reencontro".
14. **A defesa é individual e com modificação de código ao vivo**: structs + funções livres, sem herança, comentários em PT-BR, módulos pequenos, Modo Defesa para pular até qualquer bioma.

## Da Etapa 1 — Fundação (10/11/2026)

15. **No heredoc do bash, `\n` dentro de código Python vira quebra de linha real** e quebrou regex e strings ao gerar patches. — Para alterar código, usar a ferramenta de edição de arquivo, não `python - <<EOF` com escapes.
16. **Com `$PATH` do MSYS2 na frente, `python` passa a ser o Python do MSYS (sem PIL).** — Para scripts de imagem, chamar o Python do Windows ou não exportar o PATH do MSYS no mesmo comando.
17. **O GDI zera o byte de alfa dos pixels onde escreve texto numa DIB de 32 bits.** — Depois de `TextOutW`, restaurar alfa 255 na área (`restaurarAlfa`); chamar `GdiFlush()` antes e depois de misturar acesso direto a `px` com chamadas GDI.
18. **Monitores acima de 60 Hz geram quadros de desenho sem passo lógico.** — Bordas de entrada são acumuladas (`|=`) até um passo consumir; comandos globais tratados fora do passo lógico (F1, F11) precisam consumir a própria borda, ou alternam duas vezes.
19. **`SetProcessDPIAware()` é obrigatório** para a escala inteira ficar nítida; sem ele, o Windows amplia a janela inteira com filtro e borra a pixel art.
20. **Masters gerados por IA podem ter linhas de direção trocadas ou misturadas** (`bill_walk`: linha 2 = direita, linha 3 meio a meio). — Conferir cada folha direcional ampliada antes de usar; corrigir na exportação (`FIX_SIDES`) em vez de gerar de novo.
21. **Tilesets gerados por IA tendem a desenhar cada tile com moldura** (não seamless). — Só aparece de verdade quando o tile é repetido no motor: sempre testar o tileset numa sala de 30×17, não no contact sheet.
22. **Modo `--captura`** (roda N passos, salva BMP e métricas) permite verificar cada etapa visualmente e medir o p95 sem ninguém operar a janela. — Usar em todas as etapas.
23. **Playwright não serve para programa Win32; o equivalente é um script PowerShell + C# (`tools/testar_janela.ps1`)** que manda `PostMessage(WM_KEYDOWN/UP)` direto para o HWND do jogo (seguro: não digita em outros programas) e captura com `PrintWindow(..., PW_RENDERFULLCONTENT)`, que funciona mesmo com a janela coberta. — Tudo que é da janela (teclas, F11, redimensionar, foco) se verifica sem ninguém operar.
24. **O Windows bloqueia `SetForegroundWindow` vindo de outro processo (foreground lock)**, de forma intermitente. — Tentar várias vezes com `AttachThreadInput` + `BringWindowToTop` e, em último caso, um toque de Alt. Rodar o teste 3× para provar que está estável.
25. **Script `.ps1` com acentos precisa de BOM UTF-8** no Windows PowerShell 5.1; sem ele, o arquivo é lido como ANSI e dá erro de sintaxe.
26. **`DrawTextW` com `DT_VCENTER` só centraliza texto de UMA linha.** — Medir com `DT_CALCRECT` e centralizar à mão.
27. **Não reduzir a arte ao tamanho lógico do sprite.** — O export 1× (32×48, 64 cores, alfa binário) apagou rostos que os masters tinham. Melhor: assets 4× + canvas na resolução da tela + redução por área uma vez por escala. A lógica continua em 640×360, então as regras não mudam.
28. **Ao reduzir um atlas, reduzir frame a frame**, não a folha inteira: em razões não inteiras (4 → 3), a média por área mistura pixels de frames vizinhos e cria "fantasmas" na borda.
29. **API de desenho em coordenadas lógicas (float) com conversão única (`paraTela`)**: as cenas não sabem a escala; a câmera em float com tiles em múltiplos inteiros de 32 arredonda igual para todos (sem frestas).
30. **Para cenário, pedir TEXTURAS CONTÍNUAS por material, não atlas de tiles.** — O atlas gera tiles emoldurados que não emendam. Uma textura única + corte (c mod 4, l mod 4) no motor emenda na sala inteira; as transições ficam por código.
31. **O modelo de imagem cria "motivos repetidos internos"** (bolinhas, xadrez, grade 2×2) que só aparecem ao ladrilhar. — Revisar SEMPRE com montagem 3×3; o prompt precisa dizer "um único trecho, sem repetir o mesmo motivo em grade". Quanto mais liso o material pedido, maior o risco: pedir formas grandes e calmas.
32. **Métricas automáticas de emenda dão falso positivo em pixel art** (rejunte escuro ao lado de pedra clara é transição normal), e "consertar" fundindo bordas borra. — A métrica é só alerta; o olho na montagem 3×3 decide; se falhar, gerar de novo.
33. **`trim_flat_border` (feito para tirar letterbox de ilustrações) destrói texturas:** confunde a linha de rejunte da borda com faixa e corta, criando emenda. — Cada tipo de asset tem o seu pós-processamento.
34. **Avaliar o cenário COM personagens em cima, dentro do motor.** — A folha 3×3 aprovou pisos que, no jogo, competiam com os sprites (ruído fino) ou violavam a paleta (vermelho = perigo). Critério de piso: calmo, contraste menor que o dos personagens, sem vermelho.
35. **Cópia pós-build (POST_BUILD) só roda quando o exe é religado.** — Assets regerados não chegavam ao jogo; usar `add_custom_target(... ALL)` para copiar em todo build.

## Da produção de arte v2 (10/11/2026, noite)

36. **O modelo pinta fundo (água, chão, painéis) mesmo pedindo chroma verde**, principalmente quando o tema sugere um ambiente (afogado → água, ralo → ladrilhos). — A nota de chroma agora proíbe explicitamente água, chão, ladrilhos, painéis e faixas, e cada caso tem nota própria. Sintoma no export: "frames ausentes" com MUITOS frames.
37. **Nunca preencher frames ausentes cegamente**: só quando faltam ≤ 2 frames no FIM da linha (pose final). Muitos ausentes = grade não reconhecida; preencher esconderia o defeito.
38. **`--force` deve arquivar o original só DEPOIS do sucesso.** — Com a API falhando, mover antes fez 9 assets sumirem do jogo.
39. **Não rodar comandos longos com `| grep` / `| tail`**: o pipe segura a saída e, se o comando estourar o tempo, não se vê nada (nem o código HTTP do erro). Rodar sem pipe, ou em segundo plano lendo o arquivo de saída.
40. **A API de imagem pode ficar instável numa noite inteira** (muitas recusas seguidas, bem abaixo da cota). — Gerar em lotes pequenos, revisar entre os lotes e deixar o jogo sempre com uma versão válida de cada asset.

## Da Etapa 2 — gamepad (13/11/2026)

41. **A lição 40 estava errada no diagnóstico: o 429 era o TETO DE GASTOS MENSAL do projeto** ("Your project has exceeded its monthly spending cap"), não instabilidade nem limite de taxa (a tela de limites mostrava 105/250 RPD). — Ler o CORPO do 429, não só o código: `batch_images.py` agora mostra a mensagem e o `quotaId` e **para na hora** quando é teto de gastos (retentar não adianta). Ajuste em https://ai.studio/spend (página "Gasto").
42. **SDL2 mapeia controles Nintendo pelo RÓTULO por padrão** (`SDL_GAMECONTROLLER_USE_BUTTON_LABELS=1`): o "A" de um Switch Pro / 8BitDo em modo Switch vira o botão LESTE. — Para mapear por posição, definir o hint como `"0"` antes do `SDL_InitSubSystem`.
43. **Sem janela SDL, `SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS` precisa ser `"1"`**: a SDL nunca acha que o jogo está em foco. O foco é filtrado pelo próprio jogo (sem foco, a leitura é zerada). O SDD dizia `0`; corrigido.
44. **A busca por controle via SDL2 custa ~0,01 ms** (medido na tela de teste), contra ~200 ms da enumeração DirectInput do Bomberman. O intervalo de 2 s fica por segurança, e o custo aparece na tela de teste.

## Da Etapa 3 e do áudio (13/11/2026, noite)

45. **Palavras no prompt viram texto na imagem.** "Quadro-negro de *recordes*" e "pôster" fizeram o modelo escrever em inglês ("HIGH SCORES", "POSTER") mesmo com a proibição de texto. — Descrever o conteúdo sem palavras ("só rabiscos de giz", "só uma estrela").
46. **"Luz" e "névoa escura" fazem o modelo pintar um fundo escuro atrás do efeito.** — Descrever o efeito como objeto com contorno e dizer que FORA do contorno tudo é verde; para névoa, "nuvem de bordas irregulares menor que o frame".
47. **O gerador não faz faixas de 8:1 ou 6:1**: espreme e empilha os ícones. — Pedir o master numa grade folgada (4×2, 3×2: `ASPECT_OVERRIDE` + `MASTER_GRID`) e o export remonta a linha única.
48. **Recorte por componentes falha quando uma figura encosta na célula vizinha** (o pino do palhaço nos pés do de cima). — Folhas de grade uniforme (`EVEN_SPLIT`) agora são cortadas ESTRITAMENTE pelo retângulo da célula. De quebra, `objetos_siderurgica` deixou de perder 6 frames.
49. **A música pedida antes de iniciar o áudio nunca tocava**: a cena entrava antes de `iniciarAudio` e `definirMusica` achava que já estava tocando. — As cenas entram DEPOIS do áudio no `main`. E a verificação sem ouvir é o `--diag-audio`, que pergunta ao MCI o estado real ("playing", volume).
50. **MCI toca MP3 com loop e volume sem decodificador próprio** (`open … type mpegvideo`, `play … repeat`, `setaudio … volume to N`). Duas faixas abertas ao mesmo tempo com volumes complementares dão o crossfade da trilha de tensão.
51. **Gerar código C++ por script Python dentro de heredoc do bash quebra os `\n`** dos literais (viram quebras reais). — Para trechos com escapes, editar o arquivo direto (Edit), não por script.
52. **O PowerShell 5.1 grava UTF-8 com BOM** em `Set-Content -Encoding utf8`. — Não usar para código-fonte; se usar, remover o BOM.

## Da Etapa 4 — visão (13/11/2026)

53. **Escuridão por pixel com `float` e `floor` custou ~3,5 ms por quadro em 2×.** — Interpolar por TRECHOS entre centros de célula em ponto fixo 16.16, tratando de uma vez os trechos constantes (preto, penumbra, luz plena): caiu para ~0,5 ms. Medir o p95 com `--captura` a cada efeito de tela inteira.
54. **Shadowcasting simétrico com inclinações em frações inteiras** (não `double`): as quinas decidem por comparação exata. O teste de simetria percorre as salas reais; cada sala nova entra nele automaticamente.

## Da Etapa 5 — combate (13/11/2026)

55. **Pôr a partida inteira nas regras desde o início do combate valeu na hora**: os testes rodam partidas reais quadro a quadro sem janela (55 testes em 0,4 s), inclusive "uma sala de 2 ondas é vencível em 10 de 10 seeds". É a base do piloto automático e da ferramenta de balanceamento.
56. **O aviso de ataque e o dano usam a MESMA função (`pontoNaArea`)**: o desenho no chão não pode divergir do que acerta. Testar as bordas com folga: a linha tem as pontas arredondadas pela meia-largura.
57. **Quem avisou e morreu cancela o ataque** (derrubar a sombra no preparo salva o Bill); o balão é a exceção explícita (`consomeOrigem`: o aviso dispara e ele estoura junto).

## Da Etapa 5b — piloto automático (13/11/2026)

58. **O piloto falhou 1 vez em 30 por ficar parado no meio da sala**: o último inimigo vagava no escuro, longe do alcance de detecção, e os dois nunca se encontravam. — Rondar o canto escuro MAIS DISTANTE pelo caminho, trocando de alvo a cada 6 s. O teste imprime, por sala, saídas/mortes/travamentos/tempo médio: ler esse resumo depois de cada mudança.
59. **Justiça do piloto é regra de código, não de boa vontade**: ele só recebe `const EstadoPartida&`, só envia `EstadoEntrada`, filtra inimigos por `estaVisivel`, planeja pelo mapa explorado (desconhecido = "provavelmente livre") e tem atraso de reação de 0,12 s nos avisos. Isso vale como argumento na defesa.

## Da Etapa 6.1 — dificuldade, pontuação e recordes (16/11/2026)

60. **Probabilidades e multiplicadores em inteiros (%), não em `float`**: ⌊37.400 × 1,5⌋ e "15% de drop" ficam exatos, e o arquivo de dados fica legível (`chance_drop_pct = 22 15 10`). E `sortearPorcentagem` não gasta número do Rng em 0% ou 100%: um parâmetro "zerado" não desloca o resto da partida.
61. **Um teste que põe −1 em todos os campos antes de ler o arquivo de dados** prova que cada parâmetro veio do arquivo e que dados e código não divergem. Começar com os padrões esconderia uma chave esquecida.
62. **Rodar o piloto em TODAS as dificuldades revelou um defeito antigo dele**, não da dificuldade: atrás de um pilar, dois inimigos apareciam e sumiam alternadamente, e o alvo trocava a cada quadro (oscilação de 2 quadros, tudo parado por 3 min). Para achar, a foto não basta: imprimir a linha do tempo (posição, objetivo, estado da IA) e depois **quadro a quadro**, porque a amostra a cada 2 s escondia o ciclo de período 2. — Toda escolha de alvo precisa de histerese e de memória curta, e o detector de travamento não pode ter zonas cegas (ele ignorava o modo Combater).
63. **O exe de janela não bloqueia o PowerShell**: `.\build\ecos_de_derry.exe --captura ...` volta na hora e o BMP ainda não existe. Usar `Start-Process -Wait -PassThru`.
64. **O heredoc do bash quebrou de novo** (lições 15 e 51): um `\n` num `printf` virou quebra real, e um texto longo com aspas fez o bash recusar o heredoc. — Para texto grande, gravar o script num arquivo (Write) e rodar; para código C++ com escapes, usar Edit.

## Da Etapa 6.2 — itens (16/11/2026)

65. **Uma quantidade, um lugar.** Bateria e Pacto já existiam no `Jogador` (cargas da lanterna, pactos); em vez de copiar para o inventário, `quantidadeDe`/`coletarNoInventario` sabem onde cada item mora. Duas cópias da mesma contagem divergem na primeira correção apressada.
66. **Power-up como cronômetro + bônus em `calcularStats`**, não como "dano × 1,25" espalhado no golpe: a Ficha, o HUD e os testes leem o mesmo `stats.danoBase`, e o fim do efeito é só um `recalcularJogador`.
67. **Para verificar UI e interação, `--roteiro` + `--captura` + `--seed` é melhor que mandar teclas para a janela**: é determinístico, não depende de foco e permite fotografar o mesmo instante várias vezes (abrindo o baú, itens no ar, notificação). O roteiro com PostMessage + foco real deu resultados estranhos (Bill deslocado, troca de sala) que não valeu a pena perseguir.
68. **No PowerShell, `$Args` é variável automática**: um parâmetro com esse nome chega vazio. E `Get-Content`/`Set-Content` sem `-Encoding UTF8` estragam os acentos de um script (mojibake): editar scripts com a ferramenta de edição.
69. **Objeto que ocupa célula (baú, caixa) bloqueia só na `SalaViva`**, não no modelo: o validador (BFS no "estado final") continua enxergando o mapa como desenhado, e a BFS do piloto e da IA já desvia sozinha, porque lê os tiles vivos.

## Da Etapa 6.3 — tentativa (16/11/2026)

70. **Dois Rngs: um para o mapa da tentativa, outro para o combate.** Com um só, qualquer mudança no combate (um crítico a mais) mudaria as salas seguintes, e "mesma seed = mesma tentativa" deixaria de valer para quem joga diferente. O gerador é função pura de (salas, bioma, rng): testável em milissegundos.
71. **Escolha física > menu.** A pendência `EscolherPorta` do SDD seria um menu redundante: em Hades a escolha é andar até a porta com o ícone desejado. Menos código, mais jogo, e o piloto trata como destino de navegação. Pendência fica para decisões que não têm lugar no mundo (nível, Eco, loja).
72. **Pendência que congela o mundo deve abrir sozinha só onde não atrapalha** (fim do combate) e também sob demanda (C). Ligar isso quebrou zero testes porque o piloto ganhou a camada `resolverMenu` antes de tudo: cada sistema novo nasce com o piloto sabendo usá-lo.
73. **Um teste de "tentativa inteira pelo piloto" vale mais que dezenas de testes unitários de integração**: ele exercita gerador, portas, recompensas, coleta, pendência, chave, porta do Medo e descida juntos. Rodá-lo nas 3 dificuldades × 10 seeds custa ~20 s.
74. **Edite C++ com Edit, não com `Get-Content | Set-Content` do PowerShell**: além do BOM (lição 52), é fácil esquecer `-NoNewline` e o `-Encoding`. Foi preciso remover o BOM à mão.

## Da Etapa 6.4 — pausa, inventário e resultado (16/11/2026)

75. **Menu de jogo = pendência nas regras; tela pós-jogo = cena.** Pausa e inventário mexem no estado (usar item, abandonar) e precisam parar o cronômetro: nas regras, viraram 5 testes de comandos sem janela. O resultado lê arquivo e relógio (data do recorde): fica na cena, que é a camada que pode falar com o sistema.
76. **Esc manda Pausa E Voltar no mesmo quadro** (as duas teclas estão mapeadas no Esc). Cada menu precisa tratar a combinação de propósito: na pausa, os dois fecham; na confirmação, Voltar = "Não". O teste aperta os dois juntos, como o teclado faz.
77. **Confirmação destrutiva começa no "Não".** Um Enter a mais (ou o J do combate ainda apertado) não pode jogar a tentativa fora.
78. **Quando o jogador pausa, o piloto tem que ficar parado**, senão o "Atacar" dele confirma opções do menu. Toda pendência nova precisa decidir: o piloto resolve (nível) ou espera (pausa, inventário aberto pela pessoa).
79. **Recorde marcado como "computador" se o piloto jogou QUALQUER trecho**: mais honesto que olhar só o fim, e simples de explicar na defesa.

## Da Etapa 6.5 — piloto v2 e simulação (16/11/2026)

80. **O piloto usa power-up ABRINDO O INVENTÁRIO, não por um atalho só dele.** Custa uma pequena máquina de estados (abrir, aba, cursor, confirmar, fechar), mas prova na defesa que ele não tem nenhum comando que a pessoa não tenha, e testa o menu de graça.
81. **"Escolhe a melhor porta" é inútil se ele só conhece uma.** O teste da nota falhou porque, na sala sem combate, a lanterna revelava uma porta e ele saía por ela. A correção justa é humana: olhar a sala (até 8 s) antes de decidir. Teste de comportamento pega o que teste de função pura não pega.
82. **Medir antes de complicar.** O "recuar com pouca vida" estava no plano; a simulação de 150 tentativas mostrou 1 morte, por desgaste sem cura, que recuar não resolveria. Ficou de fora, com o dado registrado.
83. **Simulação sem janela é a ferramenta de balanceamento**: 50 tentativas (≈ 2,5 h de jogo) em 4 s, com CSV por sala. O primeiro número útil: o bioma 1 dura ~3,5 min, metade da meta. Isso orienta o conteúdo das próximas etapas melhor que qualquer estimativa.
84. **Programa de janela (subsistema WINDOWS) pode escrever no terminal que o chamou** com `AttachConsole(ATTACH_PARENT_PROCESS)` + `WriteConsoleW`; sem terminal, ele falha em silêncio e os arquivos bastam.
85. **`f(x).begin()` e `f(x).end()` são DOIS objetos temporários** (comportamento indefinido em `std::count`). Guardar o resultado numa variável antes; achei isso relendo o teste antes de compilar.

## Da Etapa 6b — itens no personagem e inimigos × itens (16/11/2026)

86. **Aparência como função pura com PRIORIDADE explícita** (dano > cura > coragem): o render não decide nada, e "o que o jogador precisa ler primeiro" vira regra testável, não acidente da ordem de desenho.
87. **Tinta sobre alfa pré-multiplicado: tingir em direção à cor TAMBÉM pré-multiplicada** (`cor × alfa`), senão as bordas semitransparentes viram um halo claro. A conta ficou em `core/pixels` (pura) e tem teste.
88. **Ordem de desenho é regra de jogo quando envolve visibilidade**: inimigo revelado pelos Óculos tem que ser desenhado DEPOIS da escuridão, senão ela o apaga. Mesmo cuidado com avisos de ataque (lição antiga) e, no futuro, com segredos destacados.
89. **Sorteio com efeito de "aproximação" se rola UMA vez, com recarga**: rolar `chanceRoubo` a cada quadro faria 50% virar ~100% em meio segundo. O teste `ladrao_chance_vem_da_dificuldade` mede a taxa (≈ 25% / 80%) e pegaria esse erro.
90. **Limiar de "encostou" precisa contar a célula inteira**: com 30 px, a Sombra colada na caixa (32 px do centro) nunca a quebrava. O teste com a sombra na célula vizinha pegou na primeira execução.
91. **Captura que não mostra o efeito não prova nada**: os inimigos estavam fora da tela (sala de 26 colunas, câmera presa à esquerda). Antes de suspeitar do código, conferir onde os atores estão; mover a câmera com o `--roteiro` resolveu.
92. **Cada item novo pede uma regra no piloto**: com os Óculos no bolso, ele rondava no escuro procurando o último inimigo. Ao ver a captura, virou uma linha na camada `cuidar`.

## Da Etapa 7.1 — menu, perfil e talentos (16/11/2026)

93. **Perfil com leitura tolerante e contagem do que foi ignorado**: um arquivo editado à mão (ou de outra versão) nunca derruba o jogo, e o teste confere que cada valor ruim foi contado. O mesmo padrão dos recordes.
94. **Não vender o que não funciona**: 3 dos 9 talentos dependem de sistemas futuros; ficam visíveis como "em breve" (`disponivel=false`), e `comprarTalento` recusa. Melhor que um talento que cobra Lembranças e não faz nada.
95. **Texto de explicação calculado pela mesma função do jogo**: a página "Pontuação" mostra os exemplos chamando `calcularPontuacao`. Se alguém mudar a fórmula, a explicação muda junto, e o teste `score_exemplos_do_sdd` avisa se os números do SDD deixarem de valer.
96. **Mudar a cena inicial quebra testes e capturas que dependem dela**: o jogo passou a abrir no menu. A saída foi explícita: `--sala`, `--jogar` e `--piloto` pulam o menu, e o `testar_janela` ganhou o caminho do menu (Enter, Enter) com saves próprios e limpos.
97. **Assistir o computador não pode mexer no perfil de quem joga** (nem Lembranças, nem "última dificuldade"): senão o modo de apresentação viraria um jeito de "farmar" o Mural. Verificado pela ausência da pasta de saves depois da captura.
98. **Peças de UI compartilhadas num módulo (`cenas/ui`) assim que a segunda tela precisa delas**, não na quinta: o menu e o resultado usam a MESMA tabela de recordes.

## Da Etapa 7.2 — o Clube dos Perdedores (16/11/2026)

99. **O hub é uma sala, não uma cena nova**: tipo `clube` no mesmo motor da partida. Andar, colidir, luz, pausa e menus vieram de graça, e o piloto e os testes já sabiam "jogar" nele. Uma `cena_hub` separada duplicaria tudo isso.
100. **Regras pedem, quem tem o dado atende**: o Mural e o alçapão só registram `pedidoCompra`/`pedidoTentativa` na partida; `jogo/clube` (que recebe o perfil) atende. As regras continuam sem saber de arquivo, e o teste chama a mesma função que a cena.
101. **Atender os pedidos DENTRO do laço de passos**, não depois: com o assistir em ×4, o alçapão podia ser confirmado no 1º passo e os outros 3 rodariam ainda "no Clube".
102. **Nome de arquivo também é contrato**: as salas carregam em ordem alfabética e `--sala N` usa o índice. `hub_clube.txt` (e não `b0_clube.txt`) fica no fim e não muda o número de nenhuma sala.
103. **Captura revela problema de design que o teste não vê**: uma cadeira decorativa fechava a frente do Stan. O teste de "alcançável" (BFS da validação) passava, porque o enfeite não existe no modelo como parede. Correção no dado (amigo sempre com a frente livre) e a lição: olhar a captura, não só o verde.
104. **Teste de janela por tempo é sensível ao arranque**: logo após recompilar, a 1ª rodada falhou em pontos diferentes e as seguintes passaram. Falha que muda de lugar a cada rodada aponta para tempo, não para regra. *(Corrigido na 7.3: a causa real era foco, ver 110.)*

## Da Etapa 7.3 — diálogos e tutorial (16/11/2026)

105. **Copiar para dentro, recolher para fora**: a partida não conhece o perfil. `prepararVisitaAoClube`/`configurarDicas` copiam o necessário e `sincronizarPerfil` recolhe o que mudou. Recolher ANTES de `iniciarTentativa` (que zera a partida), senão as falas lidas no Clube se perdem.
106. **Pendência resolve "o mundo para"**: o diálogo é só mais uma `Pendencia`. Congelar inimigos, cronômetro e Bill veio de graça, e o teste confere (`stats.tempo` igual depois de 60 quadros lendo).
107. **Quebrar o texto inteiro, mostrar só o prefixo**: se a quebra de linha fosse calculada só sobre as letras já digitadas, a palavra pularia de linha no meio da digitação.
108. **Marcador de botão muda o tamanho do texto**: `{DASH}` vira "Espaço" ou "A". A digitação conta letras no texto original e mostra a parte proporcional do texto trocado, então as duas terminam juntas.
109. **Ordem no arquivo é a prioridade**: "a primeira fala do amigo que vale e não foi vista" deixa o roteirista decidir a prioridade só pela posição (falas de situação especial antes das comuns). Sem campo de prioridade, sem número mágico.
110. **Teste de janela que perde teclas = foco**: o jogo ignora o teclado sem foco (é requisito). Uma janela que rouba o foco no meio do teste derruba teclas soltas, e a falha muda de lugar. Pedir o foco antes de cada bloco de teclas resolveu (6 de 6).

## Da Etapa 7.4 — prólogo, companheiro e teclado/joystick (16/11/2026)

111. **Teste que não testa: o aviso sem dono é cancelado.** O primeiro teste da barreira do Ben passava "por acaso" porque o aviso sem inimigo de origem nunca disparava (nem sem o Ben). O par "com Ben segura / sem Ben fere" é que revelou: todo teste de proteção precisa do caso de controle.
112. **Companheiro que não pode ser escoltado não pode morrer**: invulnerável e com teleporte (longe ou preso). Assim ele nunca vira motivo de derrota nem trava a sala, e o teste de 10 mil passos confere parede e distância.
113. **Habilidade por tabela de ponteiros para função** (`agir`), como o piloto e o menu: acrescentar o 7º amigo é uma linha na tabela, e os testes chamam a mesma função que o jogo.
114. **Jogável por teclado E joystick é regra de TODA a interface, não só das falas.** As falas já trocavam os botões, mas rodapés e "E: abrir" foram escritos para teclado. A correção juntou tudo numa função (`ajuda`) e um teste-guarda lê o código das cenas e reprova tecla fixa: o erro não volta escondido.
115. **CSV de simulação ganha coluna quando o sistema nasce** (`resgates`, `companheiro`): sem ela, "o piloto resgata o Ben?" seria uma suposição; com ela, 150/150 em 3 segundos.

## Da Etapa 8.1 — Ecos (16/11/2026)

116. **Bônus passivo mora onde o recálculo já soma** (`Equipamento::ecoArmadura/ecoHp` dentro de `calcularStats`): assim nenhum `recalcularJogador` (nível, item, power-up) apaga o Eco por esquecimento.
117. **Efeito de evento é uma consulta no ponto do evento** (`valorEco(p, PlantaBaixa)` dentro de `golpeEmInimigo`): o Eco não tem código espalhado; quem lê o golpe vê ali o que muda o dano.
118. **Teste com caso de controle pega a regra que nunca vale**: "Planta baixa" passava compilando e não fazia nada (distância medida até o centro da parede). O teste comparou parede × meio × sem Eco e mostrou 20/20/20.
119. **Número para pessoa, não para máquina**: a regra guarda milissegundos inteiros; a carta mostra "1,5 s". Formatar na descrição, nunca na regra.
120. **Arquivo novo de sala muda os índices de `--sala N`** (ordem alfabética): avisar no HANDOFF e preferir buscar sala pelo id nos testes.

## Da Etapa 8.2 — Loja do Elias, manivela e grade (16/11/2026)

121. **NPC genérico precisa de papel**: o Elias e os amigos usam o mesmo símbolo (`1..9` + `npc=`). Sem perguntar "quem é" (`amigoDoCodigo`), o Elias virava "resgate" e o piloto ficaria andando até ele para sempre. O piloto ganhou `QualNpc`.
122. **Missão lateral nunca bloqueia o caminho**: a grade guarda um bônus (o baú), não a saída, porque a loja é opcional (Loja | Refúgio). A regra do PRD §5.3 ("nunca travar a tentativa") decide onde a grade pode ficar.
123. **Telemetria antes de balancear**: as colunas `manivela` e `compras` mostraram em 3 s que o piloto entrega a manivela ~88% das vezes e quase não compra (pouca moeda). Sem a coluna, a conclusão seria palpite.
124. **Escapes em heredoc de novo** (lição 15/51/64): um patch Python dentro de `bash <<'EOF'` com `\n` falhou em um arquivo e aplicou nos outros. Para C++ com `\n` e aspas, usar o Edit direto ou um script salvo com o Write.

## Da Etapa 8.3 — o primeiro chefe (16/11/2026)

125. **Chefe como tipo de inimigo**: golpes, avisos, XP, piloto e o fim da sala por ondas vieram prontos; o que é de chefe (fases, padrões, barra, B em vez de K) ficou em poucos pontos, cada um com o seu teste.
126. **Padrão de ataque só pode ser escolhido de onde alcança.** Sem essa regra, o chefe "atacava" de longe, nunca se aproximava, e a luta não terminava. Virou um teste de DADOS (`distanciaMax ≤ comprimento + 20`), que pegou mais um caso (a investida) logo depois.
127. **Diagnóstico antes de remendo**: um teste temporário imprimindo vida, estado, distância e objetivo do piloto a cada 2 s mostrou duas causas diferentes em duas seeds. Mexer no piloto teria escondido um defeito de design do chefe.
128. **Chefe precisa respirar**: recuperação (janela de punição) e recarga entre ataques fazem a luta legível para pessoas E para o piloto. Sem isso, o jogador só desvia.
129. **Teste que depende de sorte mente**: a esquiva de 2% fez um golpe do teste "não pegar". O teste repete o golpe (até 5 vezes) e só reprova se nunca acertar; a regra do jogo continua intacta.

## Da Etapa 8.4 — ponte, segredos e a festa (16/11/2026)

130. **Sala de mão única precisa declarar a entrada**: "o jogador sempre entra pela esquerda" era uma suposição; o gerador o fez entrar pela direita, do lado de lá do canal. `entrada=fixa` transforma a suposição em regra.
131. **Validação no ESTADO FINAL** (grade aberta, parede derrubada, ponte baixada): a BFS aplica `ao_resgatar` e trata a parede rachada como passável; o segredo usa a BFS ao contrário (a Lembrancinha NÃO pode ser alcançável sem derrubar a parede).
132. **Item de missão nunca fica para trás**: o PRD diz "nunca travar a tentativa"; um jogador que sai sem pegar a Chave do Medo trava o resgate. Levar a chave junto é invisível para quem joga certo e salva quem não pegou.
133. **Oscilação de alvo tem mais de uma forma**: a 6.5 resolveu dois alvos visíveis; a 8.4 achou um alvo que aparece e some atrás de um pilar a cada passo. A mesma ideia (lembrar onde o viu, por pouco tempo) resolveu, agora valendo também quando há outro visível.
134. **Diagnóstico que chama o piloto duas vezes mente**: a 1ª leitura (alternância a cada quadro) podia ser artefato do próprio diagnóstico. Conferir no código a hipótese antes de acreditar no print.
135. **Telemetria acha "o piloto nunca faz X"**: a festa não aparecia em 150 tentativas não por bug, mas porque a nota da porta era a padrão. Toda porta nova precisa de nota no piloto.

## Da Etapa 8.5 — equilíbrio e ritmo do bioma 1 (17/11/2026)

136. **Medir antes de ajustar, e medir a coisa certa**: "o chefe está fácil" virou números (18 golpes, 45% do tempo em recuperação, 0 de dano em 5 de 6 lutas) só com um diagnóstico DENTRO da luta. O resumo da simulação dizia o sintoma; o diagnóstico disse a causa.
137. **Conteúdo novo muda o equilíbrio de outro lugar**: mais salas deram mais XP, e Bill chegou ao chefe no nível 5. O problema do chefe era também da curva de XP (pensada para 6 biomas sem medir o 1º).
138. **Destino sorteado precisa ser alcançável**: um balão sorteou uma célula do nicho secreto (livre, mas fechada) e ficou parado para sempre. Todo `irPara` que falha precisa de plano B (sortear outro destino).
139. **Desempate de BFS é design**: com a ordem fixa "horizontal primeiro", um perseguidor lento só acompanhava o x de quem anda de um lado para o outro e nunca descia. Desempatar pelo eixo com mais distância deixou o movimento natural.
140. **"Ao alcance" tem que ser verdade**: com a margem de +20 px, a "Mão estendida" (64 px) saía a 72 px, não chegava, e o chefe a repetia parado; do outro lado, o piloto ia "direto" contra a quina de um pilar. Dois impasses, a mesma lição: checar o caminho reto em pixels (`segmentoLivre`), não a distância nem a visão por célula.
141. **Último inimigo no escuro é problema de design, não do piloto**: uma pessoa também sofre procurando o último balão. Os últimos ≤ 2 hostis caçam Bill (como em Hades), devagar e sempre anunciando o ataque.
142. **O piloto não mede o ritmo humano**: ele fecha diálogos e menus na hora. A telemetria da janela separa tempo de jogo e tempo real; a meta de minutos se confirma com gente jogando.
143. **Teste de sala avulsa precisa de condição realista**: o chefe dimensionado para o nível 3 "falhava" no teste com Bill nível 1. O teste passou a dar a Bill o nível de chegada; o critério principal continua sendo a tentativa inteira.
144. **`python - <<EOF` no Git Bash daqui transforma `\n` em quebra de linha real**: três edições quebraram strings C++. Para editar código com escapes, usar a ferramenta Edit ou um script em arquivo.

## Da Etapa 10.1 — os Barrens (17/11/2026)

145. **Sala de teste no banco do gerador é conteúdo de verdade**: a sala de câmera da Etapa 3 tinha 1 porta e virou beco sem saída dentro da tentativa. Toda regra que o gerador SUPÕE (portas suficientes) tem de estar no validador.
146. **Uma regra de render pode depender da textura**: "água funda = líquido escurecido" funcionava na água lisa da galeria e falhava no riacho de pedras. Ao trocar de bioma, rever toda leitura do tipo "isto bloqueia".
147. **Teste que diz "a tentativa acaba no bioma 1" quebra quando o bioma 2 nasce**: os testes passaram a recortar o conteúdo (`limitarAoBioma`) em vez de depender do que existe em data/salas.

## Da Etapa 10.2 — Sanguessuga e Afogado (17/11/2026)

148. **"O aviso termina, o golpe acontece" é um contrato com o jogador**: um golpe que continua ativo depois que o aviso some é perigo invisível. Quebrar o contrato custou 20 pontos de conclusão no Difícil.
149. **Inimigo voador + água funda = intocável**: onde ele termina o ataque importa tanto quanto o ataque. Toda janela de punição precisa acontecer onde Bill alcança.
150. **Quando o teste derruba o processo, a saída mente**: o buffer cortado mostrou outro teste. O executor agora imprime o nome antes de rodar e descarrega a saída.
151. **Auxiliar de teste não pode criar estado que o jogo nunca cria** (inimigo `Perseguindo` sem nunca ter visto Bill). Nascer como na partida e deixar a regra agir.

## Da Etapa 10.3 — o Afogado (17/11/2026)

152. **Impasse de chefe tem uma forma recorrente**: "só um padrão vale nesta distância e o jogador fica nessa distância". A Múmia (8.5) e o Pennywise I+ (10.3) caíram nele. A regra geral "não repete o mesmo seguido; se só ele vale, anda" resolve a classe inteira.
153. **Lugar seguro tem de caber o corpo**: "colado nele é seguro" com um buraco menor que a caixa de dano de Bill é mentira para quem joga. Toda zona segura se mede contra a hurtbox, não contra os pés.
154. **Medir o dano por padrão** (qual ataque tirou vida) apontou o culpado em uma rodada; o "acho que é o piloto" teria levado a mexer no lugar errado.

## Da Etapa 10.4 — o Stan (17/11/2026)

155. **Ponto de evento bem escolhido paga juros**: os 4 Ecos do Stan couberam nos pontos que o Ben já tinha criado (golpe acertou, sala concluída, dash terminou). Um sistema novo de verdade só foi preciso para as cargas do dash.
156. **Limite de teste é estatística, não desejo**: exigir 9/10 numa taxa real de ~92% torna o teste frágil sem pegar regressão nenhuma. Amostra pequena com limite folgado; a meta fina vai para a simulação de 50.
157. **Hitstop também vale nos testes**: golpes seguidos num teste caem juntos depois do congelamento; o auxiliar tem de zerar o hitstop ou esperar.

## Da Etapa 10.5 — ritmo e cartão (17/11/2026)

158. **Determinismo tem fronteira**: a mesma seed dá a mesma partida nas regras, mas a janela abre dicas e diálogos que a simulação não abre; depois de minutos, as duas divergem. Quadro achado sem janela só serve para capturas curtas ou salas avulsas.
159. **Tela que congela a cena congela o piloto também** (ele roda dentro do mesmo laço): cartão, ciclo e resultado nunca precisam de lógica extra no piloto, só de terminar sozinhos.

## Da Etapa 11.1 — armadilhas, o F14 que faltava (17/11/2026)

160. **Conferir os critérios do enunciado contra o CÓDIGO, não contra o plano**: o F14 constava no SDD com módulo e teste previstos, e nada existia. Tile e desenho davam a impressão de pronto.
161. **Dano de terreno se julga pelos pés**: a hurtbox sobe 40 px; julgar a armadilha por ela feria quem estava ao lado. Cada fonte de dano escolhe o volume certo (corpo para golpes, pés para o chão).
162. **Uma política de caminho para todas as camadas**: camadas do piloto com regras diferentes de "o que dá para pisar" brigam a cada quadro. A regra mora num lugar (o padrão da BFS) e as exceções são explícitas.
163. **Medir até a CAIXA, não até o centro**: alcance, visão e "perto demais" medidos de ponto a ponto quebram com atores grandes (a Múmia) ou encostados em parede. Quase todos os impasses desta fatia eram essa conta.
164. **O teste tem de carregar o que o executável carrega**: sem os diálogos, o tutorial não abria dicas e um impasse só existia na simulação real.
165. **Perseguidor lento precisa de compromisso**: reavaliar o alvo a cada passo faz ir e voltar quando o menor caminho troca de lado; decidir e manter por 2 s chega.

## Da Etapa 11.2 — a Neibolt (17/11/2026)

166. **Toda regra de bloqueio da partida tem de existir no validador**: enfeites bloqueavam na partida e não na validação. Sem bug visível até hoje, mas era só uma sala azarada de distância de uma porta isolada.
167. **O mesmo símbolo pode ter leitura diferente por bioma**: `~` é água nos Barrens e terra fofa no porão. A regra (−30%) é a mesma; o que muda é a textura e o nome que a gente dá. Documentar por bioma evita confusão na defesa.

## Da Etapa 11.3 — o Boneco (17/11/2026)

168. **Heurística boa contra um tipo de inimigo é ruim contra outro**: "desviar para longe" é certo contra quem bate de perto e errado contra quem atira. Um inimigo novo pede revisar as heurísticas do piloto que o pressupõem.
169. **Inimigo que foge para sempre é chato, não difícil**: arranque curto com recarga mantém a identidade ("foge quando Bill se aproxima") sem transformar a sala numa perseguição.
170. **Antes de ajustar o inimigo, olhar o que o piloto faz contra ele**: mexi duas vezes nos números do Boneco; o problema de fundo era o desvio do piloto. Diagnóstico primeiro, ajuste depois.

## Da Etapa 11.4 — o Leproso (17/11/2026)

171. **Padrão que vale a qualquer distância domina a escolha**: na fase 2 só "Pela porta" e "Nuvem" eram sempre válidos, e o chefe nunca abria janela. Todo padrão "de longe" precisa terminar numa janela de punição.
172. **Efeito que muda o movimento tem de se ver no escuro**: a nuvem lenta quase sumia na captura. Áreas com efeito pedem forma + borda + contraste, não só uma cor translúcida.
173. **Estender a máquina de estados por dados compensou**: Nuvem e Porta couberam no esqueleto do Mergulho (submerso, aviso, timer) e em `Golpe` (um campo), sem estado novo.

## Da Etapa 11.5 — o Eddie e a progressão (17/11/2026)

174. **Se o herói escala e o inimigo não, todo bioma novo fica mais fácil**: mais inimigos (orçamento) não resolveu; escalar vida e dano por bioma, sim. A primeira pergunta de equilíbrio entre biomas é "quem cresceu desde o anterior?".
175. **Chefe longo e inofensivo pede padrão, não HP**: com os combos ele passou a ameaçar, e aí deu para TIRAR HP e encurtar a luta. Ameaça e duração são botões diferentes.

## Da Etapa 12.1–12.2 — os Porões e o Cabelo (17/11/2026)

176. **Campo de estado compartilhado entre comportamentos é bug esperando acontecer**: a caça (11.1) e a troca de ralo (12.2) reaproveitaram `destinoVagar`/`timer` do passeio aleatório; os dois bugs só apareceram medindo. Cada comportamento tem os seus campos.
177. **Dificuldade de um bioma vem do elenco, não só dos números**: inimigos que avisam devagar não ferem um bom esquivador, nem com reação de 0,25 s. Quem fere são os rápidos. Cada bioma precisa de pelo menos uma fonte de pressão rápida.
178. **Inimigo fixo precisa de regra para não ficar esquecido**: torreta nasce perto da luta e, quando é a última, vem até Bill pela própria ficção (o ralo).
179. **Quando os inimigos caçam, o jogador espera**: a ronda do piloto fazia sentido com inimigos parados; com caçadores, correr é se afastar deles.
180. **Limite de teste também envelhece**: 30 min de simulação cabiam em 2 biomas e estouravam em 4. Limites ligados ao tamanho do conteúdo moram numa constante com o porquê.

## Da Etapa 12.3 — o Ralo de Sangue (17/11/2026)

181. **Reaproveitar a armadilha como efeito do chefe dá tudo de graça**: a inundação troca células por `PocaSangue` temporárias; dano, desenho, regra de terreno e o desvio do piloto já existiam. Uma mecânica nova virou ~40 linhas.
182. **Dois padrões "de qualquer distância" que se alternam anulam a regra de não repetir**: o perseguidor nunca anda. Na fase que persegue, só um padrão pode valer de longe.
183. **Separe o dano por fonte E por fase antes de mexer em números**: 70% do dano de poça vinha da fase 2. O ajuste certo foi no alcance da inundação dessa fase, não na vida do chefe.
184. **Uma correção "óbvia" do piloto pode piorar**: "sair da poça para o seco" levava Bill ao eixo da cruz. Mede antes de manter.
185. **Premissa de teste avulso precisa acompanhar o jogo**: no bioma 4 o teste dava 46 de vida máxima e a tentativa real dá ~77. Quando o avulso e a tentativa discordam, confira a premissa antes de mexer no conteúdo.

## Da Etapa 12.4 — Beverly (17/11/2026)

186. **Nada de inserir no vetor que se está percorrendo**: o Ricochete cria golpes durante a resolução dos golpes; `push_back` em `p.golpes` ali invalidaria a referência do golpe atual. Uma fila (`golpesNovos`) que entra no fim do laço resolve, e o salto ganha um quadro de atraso que lê melhor.
187. **Eco que reusa o golpe de Bill herda as sinergias**: o tiro do Estilingue usa `golpeDoBill`, então rola acerto e crítico, e Mira firme e Tiro certeiro valem para ele sem código a mais.
188. **Teste que passa de primeira merece um segundo olhar**: uma verificação `|| true` tinha ficado no teste do Ricochete. Toda verificação tem de poder falhar.
189. **Uma opção de linha de comando para a defesa paga o custo duas vezes**: `--eco` mostra um Eco na hora da defesa e também serve às capturas determinísticas.

## Da Etapa 12.5 — medir para uma pessoa (17/11/2026)

190. **Antes de mexer no conteúdo, pergunte quem está medindo**: os Porões pareciam fáceis porque o piloto desvia de TODO aviso, e as ameaças do bioma são anunciadas. Com um proxy de pessoa (20% dos avisos despercebidos), o bioma ficou no patamar dos outros. Nada a mudar.
191. **Separe o dano por fonte**: em todos os biomas, quase só os inimigos rápidos feriam o piloto. Sem esse corte, a "solução" seria encher o jogo de ratos.
192. **Reação lenta não é distração**: atrasar a reação em 0,25 s não muda nada contra avisos de 0,7–0,9 s. Pessoas erram por não ver, e o proxy precisa errar assim.
193. **Dado copiado no build fica velho sem aviso**: o exe lê `build/data`. Mudar a sala e simular sem buildar mede o conteúdo antigo. Ferramenta de medição builda antes.
194. **Teste avulso modela a chegada pela telemetria, não por fórmula**: a vida máxima real (Ecos, compras) quase dobrou a de "pontos em Força/Agilidade". A premissa errada fazia o conteúdo parecer difícil demais.

## Da Etapa 13.1 — grelha e esteira (17/11/2026)

195. **Estado deduzido de um temporizador exige que o temporizador comece dentro do ciclo**: a grelha herdou a fase inicial do jato (até 3,7 s) e "parecia" acesa antes de acender pela 1ª vez.
196. **Terreno que move o jogador precisa de um piloto que reage a ele**: andando na diagonal contra a esteira, a componente útil perdia para o empurrão. A resposta é física simples (resolver v·u + b ∥ rumo), não um caso especial por sala.
197. **Guardar referência a um elemento e depois dar push_back no mesmo vetor derruba o programa** (de novo, agora num teste): pegue o id e busque de novo.
198. **Sobreposição de interface só aparece no modo em que se olha**: o título da sala ficava atrás da faixa do piloto, que é exatamente o modo da demonstração de 3,0 pontos.

## Da Etapa 13.2 — o Espectro (17/11/2026)

199. **Temporizador de uma ação em curso não pode depender do estado da IA**: a fumaça do teleporte contava dentro do comportamento, que só roda em alguns estados. Ação começada termina em todo quadro, no código comum.
200. **Visibilidade de um aviso é a do lugar avisado**: o Espectro entrava no desenho pela posição dele. A fumaça do destino, que é o aviso, sumia quando ele vinha do escuro.
201. **Função "corrigida" que às vezes não normaliza é uma armadilha**: devolver o vetor cru fora da esteira multiplicou o passo dos inimigos e quebrou 64 testes. O contrato é "sempre unitário".
202. **Física compartilhada mora na camada de baixo**: a força contra a esteira serve ao piloto (jogo) e à IA (entidades). Por isso vai para `mundo/tile`, não para `jogo/armadilhas`.

## Da Etapa 13.3 — o Pássaro das Cinzas (17/11/2026)

203. **Mecanismo bom se generaliza por um campo, não por uma cópia**: o fogo do Pássaro é a poça do Ralo com outro tile (`Golpe::fogo`). Dano, retorno ao piso original, piloto e desenho vieram quase de graça.
204. **Área de interação se mede pelo que o jogador VÊ**: a coleta pelos pés a 18 px era correta no papel, mas o corpo de 48 px cobria o item antes. "Passei por cima e não pegou" é bug, mesmo com a regra cumprida.
205. **Padrão que vale para outro monitor não vale para o do usuário**: a escala inteira em 90% da tela deu 1280×720 numa tela de 1920×1080. Para apresentar, o padrão é tela cheia; e um perfil antigo que gravou o padrão velho precisa de migração por versão.
206. **Uma versão congelada para apresentar** (pasta separada) deixa o desenvolvimento seguir sem risco de quebrar a demonstração.

## Da Etapa 9b e do dia da apresentação (17/11/2026)

207. **Pedir ao gerador só o que ele acerta; o resto, por construção**: em folhas de 4 direções, esquerda e direita vêm misturadas. Pedir frente, perfil direito e costas e ESPELHAR a esquerda no export é coerente sempre e mais barato.
208. **Uma imagem piloto antes do lote**: o 1º Ben mostrou os dois defeitos (lados misturados, olhos fechados) por 1 geração, não por 16.
209. **Revisar a folha de contato antes de exportar**: um dos 10 saiu outro personagem (Eddie "ninja") e outro com um balão piscando. Rejeitar e refazer só esses.
210. **Um botão com dois comandos precisa que os dois nunca valham na mesma cena**: RB era Interagir e AbaDireita; um atalho de teste esquecido usava AbaDireita no jogo e jogava o jogador em outra sala. Atalho de teste não fica em botão de jogo.
211. **Input descartado parece lag**: toque no hit-stop/dash sumia e o jogador "apertava duas vezes". Buffer curto de entrada (como Hades) resolve sem mudar o ritmo.
212. **Layout e controle andam juntos**: botões lado a lado com navegação só ↑↓ parecem "travados". Se a opção é horizontal, ← → movem.
213. **Antes de mexer numa reclamação, meça**: "não pega moeda" — a varredura mostrou 176/176; a causa eram sistemas de propósito (brasas queimando, rato roubando). Corrigiu-se o que irritava (moeda não queima), não a regra que estava certa.
214. **O padrão de onde se escolhe algo é o que o jogador procura**: a dificuldade no alçapão era temática, mas o professor e o jogador procuram no menu, depois de "Novo jogo".
215. **Sinalização não pode ter a cara de coleta**: o ícone da recompensa da porta, flutuando no chão da sala, era lido como item. O que mostra o futuro tem de estar preso ao que leva a ele (a porta) e ter moldura de placa.
216. **Reproduzir com o conteúdo real antes de supor bug de regra**: a chave e o baú reais funcionavam; o problema era o que o jogador VIA. Um teste com a sala real separa as duas coisas em minutos.
217. **O nome do controle diz as letras impressas**: lendo por posição, o comando está certo, mas "X atacar" num botão escrito Y confunde. A interface fala a língua do controle conectado.
218. **Efeito visual não usa o gerador aleatório do jogo**: as lascas usam hash do golpe; tirar números do Rng da partida mudaria drops, inimigos e o piloto determinístico.

## Da Etapa 13.4 e do fim do dia da apresentação (17/11/2026)

219. **Aviso não precisa de geometria para ser lido**: o chão rachando nos mesmos pontos da área diz ONDE e QUANDO tão bem quanto a hachura, e parece parte do mundo. Forma geométrica na tela soa como ferramenta de desenvolvimento.
220. **Golpe sem efeito próprio é golpe invisível**: o dano aparecia, mas o jogador não via o ataque. Um efeito por atacante (garra, fuligem, cabelo…) espalhado pela área resolve todos os inimigos de uma vez, sem caso por sala.
221. **Hit-stop é tempero, não prato**: 40 ms por acerto é ótimo num rato e vira "travada" num chefe de 45 golpes. Mais curto no chefe, com impacto e tremor no lugar.
222. **Toda ação que pode não sair precisa dizer por quê**: Especial em recarga e lanterna sem carga eram silêncio, que o jogador lê como atraso.
223. **Sinal de estado não pode depender só de uma cor sutil**: moldura cinza × dourada não foi percebida. Estado importante pede forma (cadeado) ou movimento (brilho).
224. **Efeito que nasce de um dano não deve vir do mesmo canal que conta o dano**: a queimadura do Fogo controlado usa `origem` nula para não alimentar Livro de pássaros, Marreta e Prata.
