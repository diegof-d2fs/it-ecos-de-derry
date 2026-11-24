# IT: Ecos de Derry — PRD v2

**Documento de requisitos de produto** · versão 2.0 · 10/11/2026
Substitui o PRD v1.0 (27/10/2026, `instructions/PRD-IT-Ecos-de-Derry.docx`). O v1 continua como histórico; em caso de conflito vale este documento.
Documento técnico correspondente: `docs/SDD-v1-IT-Ecos-de-Derry.md`.

---

## 0. O que mudou do v1 para o v2

| Tema | PRD v1 | PRD v2 | Motivo |
|---|---|---|---|
| Combate | Por turnos, com energia | **Tempo real** a 60 FPS: ataque, especial, dash com invulnerabilidade, clarão | Pedido do Diego: dinâmica de Hades; combina com joystick |
| Estrutura | 4 telas fixas de 20×13 | **Salas desenhadas à mão, sorteadas por tentativa**, 6 biomas + covil, câmera que acompanha | Duração de 45–60 min e rejogabilidade |
| Morte | Permanente pura | **Hub estilo Hades**: a tentativa acaba e mostra a pontuação; Bill volta ao Clube dos Perdedores com Lembranças para melhorias permanentes | Pedido do Diego; o PDF continua atendido (HP zero = derrota da partida) |
| Vidas | Uma por tentativa | **Pactos de Sangue**: revivem dentro da tentativa (padrão 1, até 3) e entram no score como "vidas utilizadas" | Atende ao "vidas utilizadas" do PDF |
| Elenco | Bill + Ben, Eddie, Beverly, Mike, Elias | **Os 7 Perdedores** (entram Richie e Stan) + Elias + Georgie | Pedido: o Pennywise vira o medo de cada um |
| Vilões | 3 inimigos comuns + Pennywise I–IV | **8 inimigos comuns + 6 chefes-medo** (Pennywise disfarçado) + Pennywise final em 3 fases | Pedido: mais vilões, os medos |
| Cenários | Galeria, porões, cisterna | **8 ambientes**: Rua de Derry, Clube, Galeria, Barrens, Neibolt, Porões, Siderúrgica, Cisterna e Covil | Pedido: cenários mais diversos |
| Duração | 25–40 min | **45–60 min** numa tentativa vitoriosa | Pedido do Diego |
| Joystick | P2 (depois) | **P0** (SDL2 GameController + XInput) | Pedido do Diego |
| Tutorial | Telas do menu | **Caixas de diálogo contextuais estilo RPG** na primeira tentativa, com retrato | Pedido do Diego |
| Inventário | Tecla I simples | **Menu de itens em jogo** com abas (itens, equipamento, missão, Ecos, ficha) | Pedido do Diego |
| Resolução lógica | 640×480 com HUD embaixo | **640×360 (16:9)** com HUD sobreposto; escala inteira ×2/×3 | Monitores 16:9 (1280×720 e 1920×1080 exatos) |

O que **não** mudou: C++17, Win32 + GDI/GDI+, sem engine; pixel art de 16 bits; tiles de 32 px; sprites 32×48; atributos Força/Foco/Agilidade/Coragem; Elias e a manivela; Pennywise cada vez mais forte; os 20 critérios funcionais e as 4 técnicas do professor são P0.

---

## 1. Visão do produto

*IT: Ecos de Derry* é um **roguelike de ação em tempo real**, visto de cima, inspirado na estrutura de *Hades* (Supergiant). Bill Denbrough desce pelos subterrâneos de Derry para resgatar os seis amigos levados pelas ilusões da Coisa. Cada amigo está preso no seu **maior medo**, e o próprio Pennywise assume essa forma para guardá-lo. Morrer faz parte: Bill acorda no Clube dos Perdedores, conversa com os amigos, fica mais forte e tenta de novo, até quebrar o ciclo.

Projeto acadêmico de Algoritmos e Programação II (UNIVALI), avaliado pelo enunciado `instructions/T3 - Roguelike - 2026.pdf`. A prioridade é cumprir **todos** os critérios com um jogo bonito, fluido e explicável na defesa.

### 1.1 Pilares de design

1. **Movimento é defesa.** Como em Hades, o dash com quadros de invulnerabilidade é a principal ferramenta. Todo ataque inimigo tem **aviso visível** (contorno, símbolo e cor) e dá tempo de reação.
2. **Escolhas a cada sala.** As portas mostram a recompensa da próxima sala. Os Ecos oferecem 1 de 3 melhorias. Cada tentativa monta um Bill diferente.
3. **Morrer conta história.** Cada volta ao Clube libera falas novas, melhorias permanentes e pistas. A derrota nunca é tempo perdido.
4. **Terror pela escuridão.** O mapa começa escuro e a lanterna revela. Inimigos fora da luz não aparecem. O vermelho fica reservado para balões e perigo.
5. **Amizade é poder.** Cada resgate fortalece Bill. No final, a luz dos seis amigos é o que protege Bill das Luzes Mortas.

### 1.2 Referências de design (inspiração, não cópia)

| Mecânica em Hades / Hades II | Equivalente em Ecos de Derry |
|---|---|
| Câmaras com recompensa anunciada na porta | Salas com ícone de recompensa na porta |
| Bênçãos dos deuses (1 de 3) | **Ecos** dos amigos (1 de 3) |
| Cast com munição recuperável | **Clarão da lanterna** com cargas (bateria) |
| Death Defiance | **Pacto de Sangue** |
| Espelho da Noite (Darkness) | **Mural dos Perdedores** (Lembranças) |
| Caronte e Óbolos | **Elias** e Moedas de prata |
| Fonte (cura) | **Refúgio** (área segura) |
| Armas Infernais | Armas no **Baú do Clube** (taco, estilingue, cano) |
| Keepsakes | **Lembrancinhas** dos amigos (P1) |
| God Mode | **Modo Coragem** (acessibilidade) |
| Movimentos Ω de Hades II | Golpe carregado com Determinação (P1) |

Nenhum nome, arte, texto ou som de Hades é usado. Derry, os Perdedores e Pennywise vêm da obra de Stephen King. Elias, os Ecos, as salas, as regras e os diálogos são criações desta adaptação.

---

## 2. Decisões de escopo

| Decisão | Escopo da versão de entrega |
|---|---|
| Plataforma | Windows 10/11, C++17, Win32 + GDI (render por software) + GDI+ (só para carregar PNG). Sem engine. |
| Jogadores | 1 jogador. Bill é o único personagem controlável. |
| Entrada | Teclado (P0), joystick (P0), mouse (P2) |
| Estrutura | Prólogo (só na 1ª vez) → Clube (hub) → 6 biomas → Covil → epílogo |
| Conteúdo | 8 inimigos comuns, elites (P1), 6 chefes-medo, Pennywise final em 3 fases, 9 NPCs |
| Duração | 45–60 min por tentativa vitoriosa; tentativas que terminam em derrota são naturalmente mais curtas |
| Derrota | HP chega a 0 sem Pacto de Sangue restante: a tentativa termina, mostra a pontuação e Bill volta ao Clube |
| Vitória | Derrotar o Pennywise no último nível (Covil) com Bill vivo |
| Salvamento | Perfil (meta-progressão) salvo no Clube. Sair no meio da tentativa a abandona, com confirmação. |
| Dificuldade | **Fácil / Normal / Difícil** (escolhida ao sair do Clube; alteram as probabilidades do jogo e o multiplicador da pontuação: SDD §24.2), mais o **Modo Coragem** opcional de acessibilidade (dano recebido −20%, e mais −2% por morte, até −60%) |
| Companheiro | Um amigo acompanha Bill em cada tentativa, com uma habilidade de apoio (Mike no bioma 1; depois, o último resgatado): SDD §24.6 |
| Computador joga | Opção **"Assistir o computador jogar"** e tecla F10: o piloto automático joga com os mesmos comandos do jogador e vence o jogo (SDD §24.7) |

---

## 3. Loop de jogo

```
            ┌──────────────── primeira vez ────────────────┐
 Menu ──► Prólogo (Rua de Derry, tutorial) ──► Clube dos Perdedores (hub)
                                                    │  conversar, Mural, Baú de armas,
                                                    │  Lembrancinha, quadro de recordes
                                                    ▼
                                 Alçapão ──► TENTATIVA (uma noite)
                                              Bioma 1..6: salas sorteadas
                                              (combate, Eco, loja, refúgio,
                                               tesouro, resgate, chefe-medo)
                                              Covil: Pennywise final
                                         ┌────────┴─────────┐
                                    HP 0 sem Pacto        Pennywise derrotado
                                         │                       │
                              Tela "Fim da tentativa"     Vitória + epílogo
                              (pontuação e estatísticas)   + créditos
                                         │                       │
                                         └──────► Clube ◄────────┘
                                   (Lembranças guardadas; falas novas)
```

**Dentro de uma sala de combate:** a sala começa escura → Bill entra e as portas se fecham → os inimigos surgem em ondas, em pontos aleatórios → limpar a sala → as portas abrem e a recompensa aparece → Bill escolhe a próxima porta pelo ícone.

---

## 4. História

### 4.1 Premissa e moldura do ciclo

Verão, Derry. Balões vermelhos aparecem nos bueiros. Ao entardecer, os sete Perdedores estão reunidos no clube subterrâneo que construíram no Barrens. Quando a noite cai, cada um dos seis amigos ouve uma voz diferente e some, puxado para o lugar do seu maior medo. Bill fica, porque a voz que ele ouve é a do irmão Georgie, e ele sabe que é mentira.

A Coisa prendeu aquela noite num **ciclo**. Sempre que Bill cai, a cicatriz do pacto de sangue na palma da mão arde e ele acorda de novo no Clube, ao entardecer, antes de tudo começar. Só Bill lembra. Os amigos estão lá, vivos, conversando, sem saber o que vai acontecer — mas guardam *ecos*, como sonhos, do que Bill viveu com eles. Para quebrar o ciclo, Bill precisa resgatar os seis na mesma noite e enfrentar a Coisa no covil.

Essa moldura justifica o hub, as conversas que evoluem a cada morte, as melhorias permanentes e o nome do jogo.

### 4.2 Elenco

| Personagem | Papel | Medo / forma da Coisa | Onde |
|---|---|---|---|
| **Bill Denbrough** | Jogável. Gagueja quando está nervoso; é o líder. | **Georgie** (a culpa pelo irmão) | Todo o jogo |
| **Ben Hanscom** | Resgate 1. Construtor, leitor, tímido. | **A Múmia** | Galeria pluvial |
| **Stan Uris** | Resgate 2. Metódico, observador de pássaros. | **O Afogado da Caixa d'Água** | O Barrens |
| **Eddie Kaspbrak** | Resgate 3. Hipocondríaco, inalador, corajoso quando importa. | **O Leproso** | Casa da Rua Neibolt |
| **Beverly Marsh** | Resgate 4. Mira certeira com o estilingue. | **O Ralo de Sangue** | Porões inundados |
| **Mike Hanlon** | Resgate 5. Guarda a história de Derry; está no Clube no prólogo. | **O Pássaro das Cinzas** (fogo) | Siderúrgica Kitchener |
| **Richie Tozier** | Resgate 6. Piadista, imita vozes, óculos fundo de garrafa. | **O Lobisomem** | Cisterna das vozes |
| **Elias** | Zelador da manutenção (criado para o jogo). Lojista entre as salas. | — | Galeria (grade) e lojas |
| **Georgie** | Ilusão: a forma mais cruel da Coisa para Bill. | — | Covil (fase 2) |
| **Pennywise** | Antagonista. Guarda cada medo e é cada vez mais forte. | — | Todos os chefes e o Covil |

Os NPCs são **invulneráveis** e nunca precisam ser escoltados. O amigo resgatado sai por uma rota segura indicada na cena.

### 4.3 Prólogo (primeira tentativa) — Rua de Derry e Clube

Uma tempestade termina. A abertura antecipa Bill descendo pelo alçapão do Clube. O prólogo jogável volta alguns minutos: Bill está na Rua Witcham, junto ao bueiro onde Georgie perdeu o barquinho de papel, procurando pistas antes de voltar ao Clube. A sala ensina a andar, a interagir e a enxergar no escuro. Um balão vermelho sobe contra o vento.

> **Mike:** "Bill! Os outros… eles saíram correndo, cada um pra um lado. Disseram que ouviram alguém chamando."
> **Bill:** "E-eu também ouvi. Era o Georgie. Não era ele."
> **Mike:** "O Elias, o zelador, conhece as grades de manutenção. Eu fico aqui e mantenho a saída livre."
> **Bill:** "Eu vou trazer todo mundo de volta."

O prólogo termina na entrada da Galeria; a rota subterrânea parte do alçapão do Clube. Na primeira morte, Bill acorda no Clube e o ciclo é revelado:

> **Mike:** "Bill? Você tá pálido. A gente acabou de chegar, ainda nem escureceu."
> **Bill (pensando):** "Eu já vivi essa tarde. Eles ainda estão aqui… e vão sumir de novo."

### 4.4 Roteiro por bioma (resumo; diálogos completos ficam em `data/dialogos.txt`)

Cada bioma segue o mesmo arco: **entrada → salas → chave do medo → sala de resgate trancada → chefe-medo → descida**. Na fase 2 de cada chefe, o medo se rasga e o Pennywise sai de dentro — **menor, mais rápido e mais forte a cada bioma**.

| # | Bioma | Amigo | Medo (fase 1) | Pennywise (fase 2) | Alteração de mapa no resgate |
|---|---|---|---|---|---|
| 1 | **Galeria pluvial** — alvenaria, água, grades | Ben | **Múmia**: faixas que prendem, avanço lento | Estágio I | Ben **baixa a ponte**; antes, **Elias abre a grade** com a manivela |
| 2 | **O Barrens** — mata, riacho, lama, represa | Stan | **Afogado da Caixa d'Água**: some na água e emerge embaixo de Bill | Estágio I+ | Stan **fecha a comporta**: a água vira lama caminhável |
| 3 | **Casa da Rua Neibolt** — tábuas podres, papel de parede, porão de terra | Eddie | **Leproso**: investidas longas e nuvem pútrida que deixa lento | Estágio II | Eddie **destranca a porta por dentro** e cura Bill uma vez |
| 4 | **Porões inundados** — fundação, canos, banheiro antigo | Beverly | **Ralo de Sangue**: fixo, cabelos em cruz, poças que ferem | Estágio II+ | Beverly **derruba a passarela** com o estilingue, abrindo um atalho |
| 5 | **Siderúrgica Kitchener** — fornalhas, esteiras, carvão | Mike | **Pássaro das Cinzas**: voa, mergulha e faz chover brasas | Estágio III | Mike **liga o gerador**: as luzes revelam a sala inteira |
| 6 | **Cisterna das vozes** — tubulações, ilusões | Richie | **Lobisomem**: combo de garras, salto e uivo que invoca ratos | Estágio III+ | Richie **grita e desfaz as paredes ilusórias** |
| 7 | **Covil** — montanha de brinquedos, balões, luz morta | — | **Georgie** (ilusão, fase 2) | Estágio IV (2×2) e **Luzes Mortas** | A entrada se fecha durante a luta |

Falas de exemplo, uma por resgate (o tom é curto, original e sem violência gráfica):

- **Ben:** "Eu desenhei essas galerias. Essa parede não estava no mapa." → "Deixa eu baixar a ponte. A descida fica do outro lado."
- **Stan:** "Tudo aqui tem uma ordem. Até a água. Se eu fechar a comporta…" → "Vai. Eu sei voltar sozinho, eu anotei o caminho."
- **Eddie:** "Ele fica dizendo que eu tô doente. Eu não tô doente!" → "Você tá machucado. Para. Deixa eu ajudar antes de ir."
- **Beverly:** "Ele usa a voz que a gente quer ouvir." → "Segura essa. Prata. Se ele voltar, mira no meio."
- **Mike:** "O fogo… foi aqui que tudo queimou, Bill. Eu li sobre isso." → "Tem um gerador. Se a luz voltar, ele não se esconde."
- **Richie:** "Bill! Finalmente! Tava ensaiando minha voz de lobisomem." → "Ei, Coisa! Isso aqui é só papelão!" (as paredes falsas somem)

### 4.5 Confronto final e epílogo

**Entrada do Covil.** A sala está cheia de balões imóveis. A entrada se fecha atrás de Bill. "Pennywise IV" aparece no HUD com a barra de vida visível.

- **Fase 1 (100%–60%) — O Palhaço.** O corpo ocupa 2×2 células. Ele alterna aproximação, ataque em linha de 3 células, cruz de raio 2 na posição registrada de Bill e a invocação de 2 balões. Todo ataque é anunciado.
- **Fase 2 (60%–25%) — Georgie.** A sala escurece. Georgie aparece de capa amarela, chamando: *"Billy, me ajuda a pegar o barquinho."* A voz **puxa Bill de leve** na direção dele, e tocar Georgie causa dano e Medo. O Pennywise ataca do escuro, com pulsos. Bill responde: *"Você não é ele. Você nunca foi ele."*
- **Fase 3 (25%–0%) — Luzes Mortas.** A tela vai ficando sem luz. **Seis círculos de luz**, um por amigo resgatado, giram pela arena. Fora deles, Bill sofre dano contínuo. O Pennywise só fica vulnerável logo depois de cada ataque. É a tradução mecânica do pilar "amizade é poder".
- **Derrota da Coisa.** A sombra se quebra e os balões murcham. *Pennywise:* "Vocês vão esquecer." *Bill:* "Hoje, ninguém ficou para trás."

**Epílogo** (até 4 falas, ilustração `epilogo`): na superfície, os sete estão juntos. *Mike:* "Eu contei todos. Estamos todos aqui." *Beverly solta o último balão.* Texto final: **"Derry ainda guarda segredos. Nesta noite, ninguém ficou para trás."** Depois vêm a pontuação, os créditos e a volta ao Clube, com o ciclo quebrado e a opção de jogar de novo pelo score.

### 4.6 Conversas no Clube (hub)

Cada amigo tem um conjunto de falas com **condições**: número de mortes, quem derrotou Bill, quais resgates já ocorreram, vitórias. A cada visita, cada amigo oferece **no máximo 1 fala nova** (indicada por um balão "!"). As falas já vistas ficam no perfil. Meta P0: pelo menos 6 falas por amigo e 10 para o Mike, ou seja, 40 falas curtas. Exemplos:

- (após morrer para a Múmia) **Ben:** "Sonhei com faixas me apertando. Que esquisito… você tá bem, Bill?"
- (após 5 mortes) **Richie:** "Bill, você tá com cara de quem já ouviu essa piada mil vezes."
- (após a 1ª vitória) **Stan:** "Por que eu sinto que a gente já fez isso antes?"

---

## 5. Estrutura da tentativa

### 5.1 Sequência de salas por bioma

Cada bioma tem um **conjunto de salas desenhadas à mão** (arquivos de texto). O gerador sorteia, com seed registrada, quais salas aparecem e em que ordem, e as recompensas das portas:

```
Entrada (segura) → Combate A → [escolha de 2 portas] → Combate B ou Especial
→ Combate C (sempre contém a Chave do Medo) → [escolha: Loja do Elias | Refúgio]
→ Sala de Resgate (porta trancada: exige a Chave do Medo) → Chefe-medo → Descida
```

- Por bioma: ~7 salas (3 de combate, 1 especial, 1 de loja ou refúgio, resgate, chefe). O total vem dos dados e pode ser ajustado sem recompilar.
- **Banco mínimo por bioma (P0):** 6 salas de combate, 1 entrada, 1 loja, 1 refúgio, 1 tesouro, 1 de Eco, 1 de resgate e 1 arena de chefe = **13 salas**. Seis biomas mais prólogo, Clube e Covil dão **81 arquivos de sala**.
- Cada sala ocupa de 20×12 a 40×24 células de 32 px. Salas maiores que a tela usam câmera que acompanha Bill.

### 5.2 Tipos de sala

| Tipo | Conteúdo | Ícone na porta |
|---|---|---|
| Combate | 1 a 3 ondas de inimigos; armadilhas; recompensa ao limpar | ícone da recompensa |
| Eco | Altar de luz: escolher 1 de 3 Ecos de um amigo | emblema do amigo |
| Tesouro | Baú trancado (Chave enferrujada) com item valioso; sem inimigos ou com poucos | baú |
| Loja do Elias | Comprar com Moedas de prata: kits, baterias, Ecos, melhoria de arma, escudo | moeda |
| Refúgio (**área segura**) | Cura de 30% do HP; sem spawn; reduz o Medo | coração |
| Resgate | Porta trancada pela Chave do Medo, cena com o amigo, Eco lendário, alteração de mapa | rosto do amigo |
| Chefe | Arena do medo; portas fechadas até a vitória | caveira de balão |

**Recompensas possíveis de uma sala de combate:** Eco · Moedas de prata (40–60, cerca de meio item da loja; além disso, cada inimigo comum pode soltar 1–3 moedas pela chance da dificuldade, e Bill chega à loja do bioma 1 com ~60–75) · Lembranças (meta) · Kit de cura · Bateria · Chave enferrujada · Melhoria de arma · Ponto de atributo extra (raro).

### 5.3 Garantias de consistência (nunca travar a tentativa)

- Cada sala passa por validação ao carregar: **BFS de conectividade** entre a entrada, todas as portas, a chave e os NPCs. Uma sala inválida aborta no carregamento em modo debug e é recusada pelos testes.
- A Chave do Medo fica **sempre antes** da porta que ela abre. A sala de resgate só é sorteada depois da sala da chave.
- Os spawns não ocupam paredes, portas, itens, NPCs nem um raio de 4 células da entrada de Bill.
- As posições vêm de uma lista finita embaralhada; se faltar espaço, reduz-se a onda, sem laço de ressorteio infinito.
- Inimigos só destroem **detritos frágeis** e **caixas** marcados e podem **pegar ou queimar itens comuns no chão** (Rato ladrão, Espectro: SDD §24.5); paredes estruturais são indestrutíveis; **itens de missão nunca são pegos nem destruídos**.

---

## 6. Bill: controles e ações

### 6.1 Ações

| Ação | Descrição |
|---|---|
| **Mover** | 8 direções, com movimento contínuo e colisão por caixa nos pés; o sprite usa a direção dominante (4 direções do atlas). Diagonal normalizada. |
| **Atacar** | Combo de até 3 golpes com a arma equipada; o 3º golpe empurra o inimigo |
| **Especial** | Golpe da arma com recarga (ex.: taco = giro 360°) |
| **Clarão** | A lanterna dispara um feixe de até 4 células: dano, atordoa inimigos comuns por 1,5 s e revela o escuro. Consome 1 carga. |
| **Dash** | 96 px em 0,15 s, **invulnerável** durante o dash, recarga de 0,70 − 0,03×AGI s. Atravessa inimigos e projéteis, mas **não** paredes nem buracos largos. |
| **Interagir** | Conversar, abrir porta, baú ou válvula, pegar item, usar alavanca |
| **Usar kit** | Atalho de cura (só gasta se HP < máx) |
| **Inventário / Ficha / Diário / Pausa** | Menus que **pausam** o jogo |

### 6.2 Mapeamento padrão

| Ação | Teclado | Joystick (posição Xbox; o 8BitDo segue a mesma posição física) |
|---|---|---|
| Mover | WASD ou setas | Analógico esquerdo ou direcional |
| Atacar | **J** | **X** (oeste) |
| Especial | **K** | **Y** (norte) |
| Clarão | **L** | **B** (leste) |
| Dash | **Espaço** | **A** (sul) |
| Interagir | **E** | **RB** |
| Usar kit | **Q** | **LB** |
| Inventário | **I** ou **Tab** | **Back/View** |
| Ficha de atributos | **C** | direcional para cima, segurando LT |
| Diário / minimapa | **M** | **LT** (segurar) |
| Pausa | **Esc** ou **P** | **Start** |
| Confirmar / Voltar (menus) | Enter / Esc | A / B |

O mesmo mapeamento aparece no menu **Controles**, e os ícones de botão no tutorial mudam conforme o dispositivo usado por último. Remapeamento completo é P2.

---

## 7. Atributos, experiência e níveis

### 7.1 Atributos (1 a 9; todos começam em 1)

| Atributo | Afeta | Fórmula |
|---|---|---|
| **Força** | Dano | Dano base = 6 + 2×FOR + bônus da arma + melhorias |
| **Foco** | Acerto, crítico, clarão | Acerto = limitar(80 + 3×FOC − 2×AGI_alvo, 60, 98)% · Crítico = 2×FOC % (dano ×1,5) · Clarão = 15 + 5×FOC |
| **Agilidade** | Esquiva, velocidade, dash | Esquiva = mín(30, 2 + 3×AGI)% · Velocidade = 120 + 8×AGI px/s · Recarga do dash = 0,70 − 0,03×AGI s |
| **Coragem** | Vida, armadura, Medo | HP máx = 40 + 6×COR · Armadura = ⌊COR/2⌋ + escudo · Ganho de Medo −5% por ponto (P1) |

O PDF pede impacto em **armadura, acerto, dano, esquiva, vida e velocidade de movimento**: os seis estão cobertos (ver F17).

### 7.2 Experiência

- XP vem de inimigos (6–20; elites ×2), chefes-medo (60, 80, 100, 120, 140, 160), resgates (+25) e salas concluídas (+5). Diálogos repetidos, invocações e andar em círculos **não** dão XP.
- Os limiares são **acumulados** e o XP não é zerado: N2 = 70 · N3 = 180 · N4 = 400 · N5 = 700 · N6 = 1050 · N7 = 1450 · N8 = 1950 · N9 = 2550 · N10 = 3300 (máximo). *(Rebaseado na Etapa 8.5: com a curva antiga, N10 = 1380, só o bioma 1 levava Bill ao nível 5. Agora ele chega ao chefe 1 no nível ~3, como pede o §10.2, e ao N10 perto do fim do bioma 6.)*
- Cada nível dá **2 pontos de atributo** e restaura 10 HP. Uma recompensa que atravessa vários limiares concede todos os níveis. O menu de nível abre assim que a sala é limpa (não durante a luta) e **pausa o jogo**.
- O nível e o XP são **da tentativa**: zeram a cada nova noite. O que persiste são os talentos do Mural.

### 7.3 Exemplo verificável (entra nos testes)

Bill com FOR 3, FOC 2, AGI 2, COR 3, taco (+2) e escudo (+2): dano base 6+6+2 = **14** · HP máx 40+18 = **58** · armadura 1+2 = **3** · esquiva 2+6 = **8%** · velocidade 120+16 = **136 px/s** · recarga do dash 0,64 s. Contra um alvo com AGI 1 e armadura 1: acerto 80+6−2 = **84%**, dano efetivo 14−1 = **13** (crítico 4%: ⌊14×1,5⌋−1 = 20).

---

## 8. Combate

### 8.1 Resolução de um golpe (igual para Bill e inimigos)

1. A hitbox do golpe sobrepõe a hurtbox do alvo, e o alvo não está invulnerável nem já foi atingido por **este mesmo golpe** (cada golpe tem um ID).
2. Rolar **acerto** do atacante. Se falhar: texto "Errou".
3. Rolar **esquiva** do alvo. Se esquivar: texto "Esquivou", dano 0.
4. Rolar **crítico** (só Bill e elites).
5. **Dano efetivo = máx(1, dano − armadura do alvo)**, com modificadores dos Ecos aplicados na ordem documentada no SDD.
6. Aplicar: HP, recuo (knockback), **hit-stop** de 50 ms, piscar branco de **1 quadro** (sem piscar contínuo), número de dano flutuante.
7. Se o HP do alvo ≤ 0: morte, XP, drop e score, **uma única vez** por entidade.

Áreas, projéteis e armadilhas pulam o passo 2 (não erram), mas respeitam a esquiva, exceto armadilhas de terreno.

### 8.2 Bill ferido

- 0,8 s de invulnerabilidade após um dano; o sprite fica **translúcido**, sem piscar.
- **HP ≤ 0:** se houver Pacto de Sangue, consome 1, Bill revive com 50% do HP e todos os inimigos próximos são empurrados (anel vermelho). Sem Pacto: **derrota da tentativa**.

### 8.3 Avisos de ataque (legibilidade, regra do PRD v1 mantida)

- Todo ataque inimigo com dano ≥ 8 ou área tem uma fase de **preparação de pelo menos 0,5 s** (chefes: 0,6–1,2 s), com contorno, padrão hachurado e símbolo, e não só cor.
- **O alvo é registrado na preparação**: um ataque em área não segue Bill depois do aviso.
- Abrir menus pausa tudo, inclusive os avisos.

### 8.4 Armas (escolhidas no Baú do Clube antes da tentativa)

| Arma | Ataque | Especial | Desbloqueio |
|---|---|---|---|
| **Taco de beisebol** | Combo de 3 em arco frontal; dano +2 | Giro 360° (recarga 3 s) | Inicial |
| **Estilingue de Beverly** | Projétil de prata rápido (dano +0, alcance 7 células) | Tiro carregado perfurante | Resgatar Beverly 1× |
| **Cano de ferro** (P1) | Golpe lento e largo; dano +5 | Pancada no chão (onda) | Resgatar Mike 1× |

Dentro da tentativa, armas e escudos **também são encontrados**: Melhoria de arma ("pregos no taco", +3 de dano), **Escudo** (tampa de lixeira, +2 de armadura) e Ficha de coragem (power-up temporário).

---

## 9. Inimigos comuns

Todos compartilham movimento, colisão, dano e estados. Muda apenas a tabela de parâmetros e uma decisão de comportamento. **Todos vagam aleatoriamente** até detectar Bill (distância + linha de visão; não enxergam através de paredes ou portas fechadas) e então **perseguem** com BFS na grade. Perdendo a visão, vão até a última posição conhecida por 3 s e voltam a vagar. Nunca atravessam paredes, nunca se sobrepõem e nunca se teleportam para encurtar caminho (exceto o Espectro, que faz isso com aviso).

| # | Inimigo | HP | Dano | Arm | AGI | Vel. (px/s) | XP | Det. | Comportamento distinto | Biomas |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | **Balão perverso** | 20 | 10 (estouro) | 0 | 1 | 70 | 6 | 6 | Flutua; perto de Bill infla por 0,8 s (aviso de círculo) e estoura em raio de 1,5 célula, **ferindo também inimigos** | 1, 4, 6, 7 |
| 2 | **Rato sombrio** | 26 | 6 | 0 | 3 | 140 | 8 | 5 | Zigue-zague; morde e recua; foge por 2 s com HP < 30% | 1, 2, 3, **4** (Etapa 12.2: a pressão rápida que os Porões não tinham) |
| 3 | **Sombra encharcada** | 60 | 12 | 2 | 1 | 80 | 15 | 7 | BFS; golpe com aviso de 0,6 s; **quebra detritos frágeis** para abrir caminho | 1, 4, 6 |
| 4 | **Sanguessuga alada** | 18 | 5 | 0 | 4 | 110 (investida 300) | 7 | 7 | Voa (ignora água, buracos e armadilhas de chão); investida em linha anunciada; ao acertar, recupera HP | 2, 3, 5 |
| 5 | **Afogado** | 70 | 10 | 1 | 1 | 60 | 16 | 6 | Surge de poças; agarrão deixa Bill 50% mais lento por 1,5 s; **deixa poças** por onde passa | 2, 4 |
| 6 | **Boneco palhaço** | 34 | 7 | 0 | 2 | 90 | 12 | 8 | À distância: mantém 4–6 células e arremessa pinos em arco (aviso no chão); foge quando Bill se aproxima | 3, 5, 6 |
| 7 | **Cabelo do ralo** | 45 | 9 | 1 | 0 | 0 (fixo) | 12 | 4 | Torreta presa a um ralo; chicote em cruz anunciado; recolhe-se após atacar | 4 |
| 8 | **Espectro de fuligem** | 40 | 11 | 0 | 3 | 100 | 14 | 8 | Teleporte curto com aviso de fumaça (0,5 s; reaparece a 2 células de Bill, só se estiver a 4+; recarga 4 s); **deixa brasas** (6 de dano/s a Bill, por 4 s) | 5, 6 |

- **Ondas:** cada sala de combate define um orçamento de pontos por onda; o gerador sorteia inimigos do bioma até gastar o orçamento, em pontos de spawn sorteados. Limite de **12 hostis ativos** por sala.
- **Elites "Assombrados" (P1):** a partir do bioma 2, até 1 por sala; +60% HP, +25% dano, aura violeta e XP ×2.
- **Escala por bioma (Etapa 11.5):** os inimigos comuns ganham +15% de vida e de dano por bioma depois do primeiro (Bill cresce de bioma em bioma; sem isso a Neibolt ficava mais fácil que o Barrens). A legibilidade e os avisos não mudam.
- Inimigos derrotados não reaparecem na mesma sala. Uma morte causada por armadilha também dá XP e K.

---

## 10. Chefes: os medos e o Pennywise

### 10.1 Regra de progressão do Pennywise (mantida do v1, ampliada)

- Há **7 encontros** com o Pennywise: 6 disfarçados de medo e 1 definitivo. Cada um tem ID e flag próprios; o HP de um não afeta o seguinte.
- **Fase 1** = a forma do medo (sprite 64×64, lenta, padrões amplos). **Fase 2** (≤ 50% do HP) = o medo se rasga e o Pennywise do estágio correspondente luta (sprite 32×48: menor, rápido, ataques em linha). Esse contraste é intencional.
- Vencer os encontros 1 a 6 **dispersa** a Coisa (a interface diz "Manifestação dispersa", não "Vitória") e libera a descida. **Somente o Covil concede a vitória.**
- Todos os ataques são anunciados. Dificuldade maior **não** significa ataque instantâneo, invulnerabilidade arbitrária ou atravessar paredes.

| Encontro | HP total | Dano | Arm | Esquiva | Fase 1: o medo | Fase 2: Pennywise | XP |
|---|---|---|---|---|---|---|---|
| 1 Múmia | **650** | 9 | **2** | 0% | Faixa em linha de 3 células (prende 1 s); leque de 3 bandagens; invoca 2 ratos; **redemoinho** (anel em volta dela) | **I**: linha de 2, aproximação **seguida da linha**, **gargalhada** (anel curto), **balões vermelhos** (ferem todos, até ele) | 60 |
| 2 Afogado | **900** | 10 | **2** | 2% | Mergulha e emerge sob a posição registrada (círculo de 1,0 s); ondas em anel (**2 anéis, centro seguro**); respingo; chama sanguessugas | **I+**: linha de 3 + ondas, aproximação seguida da linha, gargalhada | 80 |
| 3 Leproso | **850** | 12 | **3** | 5% | Investida longa (linha anunciada) **seguida do golpe podre**; nuvem pútrida (área de lentidão por 5 s); chama bonecos | **II**: some e reaparece por uma porta (**bote anunciado + gargalhada**), linha de 3, nuvem | 100 |
| 4 Ralo de Sangue | **1200** | **13** | **3** | 5% | Fixo no centro; cabelos em **cruz e em xis** (o lugar seguro muda); inunda **quadrantes opostos** alternados (poças de sangue por 5 s; os eixos ficam secos); **golfada** (poça onde Bill está); chama **Cabelos do ralo** pelos ralos | **II+**: sai do ralo e persegue (linha de 3, bote → gargalhada, golfada); o ralo sangra e ainda transborda em volta | 120 |
| 5 Pássaro das Cinzas | 720 | 14 | 3 | 6% | Voo (imune a terreno); mergulho em linha; chuva de brasas (círculos) que acende fogo no piso | **III**: linha de 3 + cruz | 140 |
| 6 Lobisomem | 860 | 16 | 3 | 8% | Combo de 3 garras (aviso curto); salto na posição registrada; uivo invoca 2 ratos | **III+**: linha + salto | 160 |
| 7 **Pennywise IV** | 1300 | 18 | 3 | 10% | Ver §4.5: Palhaço (2×2) → Georgie → Luzes Mortas | — | V = 1 |

Os números são o ponto de partida do balanceamento e ficam numa única tabela de dados (ver SDD).

**Rebaseamento da Etapa 8.5 (telemetria de 150 tentativas do piloto):** com 300 de HP e armadura 1, a Múmia caía com **18 golpes em 26 s**, e o piloto levava 0 de dano em 5 de 6 lutas. Seguindo o §10.2, primeiro vieram padrões (redemoinho, gargalhada, balões, combo) e armadura (1 → 2); como 18 golpes não comportam uma luta de duas fases, o HP subiu para 650 (~45 golpes). Resultado: luta de ~55 s para o piloto (estimativa de 1,5–2 min para uma pessoa), com ~25% da vida perdida no Normal. Os chefes 2–6 devem ser escalados a partir da Múmia **medindo da mesma forma**, não pelos números desta tabela.

**Chefe 4 (Etapa 12.3):** fixo na fase 1, Bill bate o tempo todo; com 900 de vida caía em 49 s com 17% de dano. Com 1200/13: **60 s e 19–24%** no Normal. A inundação da fase 2 cobre só 5 células em volta do ralo: com a arena inteira, a perseguição acontecia toda dentro do sangue.

### 10.2 Critério de balanceamento

O chefe 1 deve ser vencível com Bill nível 2–3 e um Eco. Cada chefe seguinte deve exigir mais **leitura e deslocamento**, não só mais HP. Se a progressão de Bill tornar um chefe trivial, ajustar padrões e armadura antes de somar HP.

---

## 11. Itens, inventário e economia

### 11.1 Itens

| Item | Tipo | Efeito | Limite | Ícone (asset) |
|---|---|---|---|---|
| **Kit de cura** | Consumível (poção de vida) | +35% do HP máx | 3 (+talento) | `item_kit` |
| **Bateria** | Consumível | +1 carga da lanterna | 5 | `item_bateria` |
| **Ficha de coragem** | Power-up | +25% de dano por 30 s (aura laranja no Bill) | 2 | `item_coragem` |
| **Tônico do Eddie** | Power-up | +30% de velocidade por 20 s (rastro azul) | 2 | `item_tonico` (novo) |
| **Óculos do Richie** | Power-up | Revela inimigos e paredes rachadas por 30 s (contorno branco) | 1 | `item_oculos` (novo) |
| **Lembrancinha** | Segredo | 1 escondida por bioma; vai para a estante do Clube e dá Lembranças | 1 por bioma | `item_lembrancinha_*` (novo) |
| **Barquinho de papel** | Consumível | −30 de Medo e +10 HP (P0 sem Medo: só HP) | 3 | `item_lembranca` |
| **Escudo** (tampa de lixeira) | Equipamento | +2 de armadura | 1 equipado | `item_escudo` |
| **Melhoria de arma** | Permanente na tentativa | +3 de dano | 3 | `item_melhoria` (novo) |
| **Chave enferrujada** | Chave comum | Abre baús e portas laterais de tesouro | 3 | `item_chave_ferro` |
| **Chave do Medo** | Missão | Abre a sala de resgate do bioma | 1 por bioma | `item_chave_latao` |
| **Manivela** | Missão (bioma 1) | Entregar a Elias para abrir a grade | 1 | `item_manivela` |
| **Moeda de prata** | Moeda da tentativa | Gastar na Loja do Elias; **perdida** ao morrer | — | `item_moeda` (novo) |
| **Lembrança** | Moeda meta | Gastar no Mural; **guardada** ao morrer | — | `hud_simbolos` |
| **Pacto de Sangue** | Vida extra | Revive com 50% do HP | 1 + talentos (máx. 3) | `item_pacto` (novo) |

Itens de missão não podem ser descartados nem vendidos e não ocupam espaço comum. Itens comuns acima do limite **ficam no chão**. Tentar curar com HP cheio não gasta o kit.

### 11.2 Menu de itens durante o jogo (Inventário)

Abre com **I/Tab** ou **Back**, **pausa** o jogo e tem abas navegáveis pelo direcional ou pelo teclado:

1. **Itens:** consumíveis com quantidade, descrição e ação "Usar" (gasta o item; o tempo continua pausado).
2. **Equipamento:** arma (nome, dano, melhorias), escudo e lembrancinha (P1).
3. **Missão:** chaves, manivela e objetivo atual do bioma.
4. **Ecos:** os Ecos ativos, com ícone, raridade e efeito.
5. **Ficha:** atributos, valores derivados (dano, armadura, acerto, esquiva, HP, velocidade, crítico) e distribuição dos pontos pendentes.

### 11.3 Loja do Elias

3 a 5 ofertas sorteadas: kit (30), bateria (20), Eco aleatório (80), melhoria de arma (100), escudo (60), chave enferrujada (25), barquinho (25). Os preços vêm dos dados. Elias tem falas curtas e muda de comentário conforme a morte anterior.

---

## 12. Ecos (bênçãos dos amigos)

Uma sala de Eco oferece **3 opções de um mesmo amigo**. A raridade (Comum, Raro, Épico) muda os números. O resgate de cada amigo dá **1 Eco Lendário** dele. P0: **4 Ecos por amigo (24)** mais 6 lendários. P1: Ecos duplos (combinação de dois amigos).

| Amigo | Tema | Ecos P0 (Comum / Raro / Épico) | Lendário (no resgate) |
|---|---|---|---|
| **Ben** | Estrutura, defesa | Tijolo a tijolo (+1/2/3 armadura) · Coração pesado (+10/15/20 HP máx) · Planta baixa (golpe contra parede: ×1,5/1,75/2) · Ponte firme (o dash deixa uma barreira que bloqueia projéteis por 1/1,5/2 s) | **Poema**: ao chegar a 0 HP pela primeira vez, sobrevive com 1 HP (1× por bioma) |
| **Stan** | Ordem, precisão | Livro de pássaros (3 golpes marcam o alvo: +10/15/20 de dano) · Escoteiro (sala sem dano: +10/15/25 moedas) · Calma (+0,05/0,1/0,15 s de invulnerabilidade no dash) · Ordem (+8/12/16% de crítico em alvos atordoados) | **Asas**: dash 2 cargas |
| **Eddie** | Cura, rapidez | Inalador (o dash solta uma névoa que deixa lento por 1/1,5/2 s) · Remédio (kits +20/35/50%) · Gesso (1×/sala, reduz um golpe em 30/45/60%) · Pressa (+6/9/12% de velocidade) | **Cuidado**: ao resgatar, +1 kit e cura total |
| **Beverly** | Mira, prata | Mira firme (+5/8/12 acerto) · Prata (+20/30/40% contra chefes) · Ricochete (o clarão salta para +1/2/3 alvos, até 4 células do atingido) · Estilingue (o especial dispara projétil de prata de 10/15/20, até 7 células, para no 1º inimigo) | **Tiro certeiro**: críticos atordoam por 0,8 s (chefes, 0,4 s) |
| **Mike** | Força, fogo | Marreta (3º golpe +30/45/60% e mais recuo) · Cronista (+10/15/20% de XP) · Fogo controlado (o clarão queima: 3/4/5 de dano/s por 3 s) · Resistência (+1 armadura e imunidade a fogo no piso) | **Luz do gerador**: raio de visão +2 |
| **Richie** | Voz, confusão | Voz do locutor (o especial atordoa em área por 0,6/0,9/1,2 s) · Óculos (inimigos em células exploradas aparecem como contorno: 3/4/5 células além da luz) · Piada ruim (inimigos −5/8/12 de acerto) · Bip-bip (o dash causa 8/12/16 de dano) | **Imitação**: 1×/sala, um inimigo luta do lado de Bill por 6 s |

---

## 13. Meta-progressão (persistente entre tentativas)

### 13.1 Mural dos Perdedores (gastar Lembranças)

| Talento | Efeito por nível | Níveis | Custo |
|---|---|---|---|
| Pacto de Sangue | +1 vida extra (base 1) | 2 | 40, 120 |
| Coragem herdada | +5 HP máx | 5 | 10, 20, 30, 40, 50 |
| Lanterna reforçada | +1 carga máxima | 2 | 25, 60 |
| Bolso fundo | Começa com +1 kit | 2 | 20, 50 |
| Treino no Barrens | +1 ponto de atributo inicial | 3 | 30, 60, 90 |
| Sorte do Richie | +1 re-sorteio de Eco por tentativa | 3 | 20, 40, 60 |
| Mão leve | +20% de moedas | 3 | 15, 30, 45 |
| Mapa do Ben | Revela o minimapa do bioma ao entrar | 1 | 50 |
| Fôlego extra | +1 carga de dash | 1 | 100 |

Lembranças por tentativa: ~3 por sala concluída, 10 por chefe e 5 por resgate. Uma tentativa até o bioma 2 rende ~30; uma vitória, ~120. O Mural deve ser **completável em ~8–10 tentativas**: é um bônus de conforto, não uma exigência.

### 13.2 Outros elementos do Clube

- **Baú de armas:** escolher a arma da noite.
- **Quadro de recordes:** as 10 melhores pontuações, com data, bioma alcançado e vitória ou derrota.
- **Diário de Bill:** bestiário (inimigos vistos e derrotados), itens, Ecos descobertos e falas.
- **Lembrancinhas (P1):** cada amigo dá um presente equipável depois de 3 conversas (ex.: Inalador do Eddie = +1 kit).
- **Alçapão:** começa a tentativa.

---

## 14. Mapa, escuridão, armadilhas e alterações

### 14.1 Escuridão e exploração (critério F11)

- Cada célula tem `explorada` (persistente na sala) e `visivelAgora`.
- **No início, toda a sala está coberta** (preto). A lanterna ilumina um raio de **6 células** (+Ecos/talentos; −até 3 com Medo alto, P1), com linha de visão que **para em paredes e portas fechadas**.
- **Visível agora:** cor plena, com gradiente suave de luz. **Explorada e fora da visão:** só o terreno, escurecido a ~35%. **Desconhecida:** preto.
- Inimigos, itens e projéteis **fora da visão atual não são desenhados**.
- O **minimapa** do bioma (M/LT) mostra as salas visitadas, a sala atual e as portas conhecidas.
- Abrir porta, baixar ponte, quebrar detrito ou ligar gerador **recalcula a visão** na hora.

### 14.2 Armadilhas (critério F14)

| Armadilha | Aviso | Efeito | Atinge | Biomas |
|---|---|---|---|---|
| Piso cedendo | Rachaduras visíveis quando reveladas | 8 de dano, depois vira buraco (bloqueia a passagem) | Bill e inimigos terrestres | 1, 3 |
| Jato da tubulação | Gotejamento (0,8 s) → jato → recarga | 10 de dano por disparo, linha de 3 | Todos os terrestres | 1, 4, 6 |
| Lama / areia movediça | Textura própria | −40% de velocidade | Terrestres | 2 |
| Espinheiro / pregos | Textura própria | 6 de dano ao entrar (1×/s) | Terrestres | 2, 3 |
| Poça de sangue | Brilho ritmado | 4 de dano/s | Bill e inimigos | 4 |
| Grelha de fogo | Brasas (0,8 s) → chamas | 12 de dano | Todos | 5 |
| Esteira | Setas animadas | Empurra 60 px/s | Terrestres | 5 |
| Brasas do Espectro | Faíscas | 6 de dano/s por 4 s | Bill | 5, 6 |

A mesma função de dano de terreno atende a qualquer criatura elegível, e um ator não sofre o mesmo disparo duas vezes por ativação. NPCs e chefes não acionam armadilhas.

### 14.3 Alterações do mapa por origem (critério F19)

| Origem | Evento | Mudança real de colisão / visão |
|---|---|---|
| **NPC** | Elias recebe a manivela | Grade → aberta (deixa de bloquear a passagem e a visão) |
| **NPC** | Ben, Stan, Beverly, Mike, Richie no resgate | Ponte baixada · água → lama · passarela derrubada · gerador liga (sala inteira explorada) · paredes ilusórias somem |
| **Mob** | Sombra quebra detrito | Detrito → chão (abre rota) |
| **Mob** | Afogado deixa poças; Espectro deixa brasas | Água rasa / armadilha temporária criadas |
| **Mob** | Balão estoura perto de detrito | Detrito → chão |
| **Jogador** | Chave em porta, válvula, alavanca, baú, caixa quebrável | Porta abre · jato desligado para sempre · ponte · item · caixa vira chão |
| **Chefe** | Pássaro acende fogo; Ralo inunda | Fogo / poça temporários |

---

## 15. Interface

### 15.1 Menu principal (critério F04)

`Continuar` (se existir perfil) · `Novo jogo` (confirma que apaga o perfil) · **`Como jogar`** · **`Itens e efeitos`** · **`Pontuação`** · `Controles` · `Opções` · `Recordes` · `Créditos` · `Sair`.

- **Como jogar** (páginas com ilustração): objetivo e história; movimento e dash; combate e avisos; escuridão e lanterna; salas, portas e recompensas; Ecos; XP, níveis e atributos (com a tabela de efeitos); Pactos de Sangue, morte e o Clube; vitória.
- **Itens e efeitos:** cada item com ícone, efeito, limite e onde aparece; as armadilhas; os tipos de sala.
- **Pontuação:** a fórmula, a tabela dos símbolos e os **dois exemplos** de §16, com a explicação de "movimentos" e "vidas utilizadas".
- **Controles:** as tabelas de teclado e joystick, o dispositivo detectado e um teste de botões.
- **Opções:** volume (P1), escala (Auto/1×/2×/3×), tela cheia (F11), dicas de tutorial ligadas/desligadas, Modo Coragem, mostrar cronômetro.
- O menu é navegável por teclado e joystick.

### 15.2 HUD (sobreposto, 640×360)

- **Superior esquerdo:** barra de HP (atual/máx), Pactos de Sangue (gotas vermelhas), cargas da lanterna, Medo (P1).
- **Superior direito:** **pontuação atual**, moedas de prata e Lembranças.
- **Inferior esquerdo:** nível, barra de XP e indicador "+2 pontos" pulsando quando houver pontos pendentes.
- **Inferior direito:** atalho do kit (Q/LB) com quantidade e a fileira de ícones dos Ecos ativos.
- **Centro superior:** nome do bioma e da sala (some após 2 s); barra grande do chefe ("A Múmia — Pennywise I").
- **Centro inferior:** as 3 mensagens mais recentes (dano, coleta, bloqueio, missão), que somem com o tempo.
- **Cronômetro sempre visível** (centro superior, `mm:ss`; só conta o tempo de jogo, sem pausas nem menus). Entra na pontuação (SDD §24.3).
- **Faixa de itens pegos** (inferior direito) e notificação "+ item" com ícone a cada coleta.
- Com o piloto ligado: faixa "COMPUTADOR JOGANDO".

### 15.3 Caixa de diálogo estilo RPG

Moldura 9-slice na parte de baixo da tela, retrato 64×64 à esquerda (ou à direita para o interlocutor), nome do falante, texto com efeito de digitação (Confirmar completa o texto; Confirmar de novo avança) e indicador ▼. **O jogo fica pausado** enquanto a caixa está aberta. Falas com escolha (2–3 opções) só existem no Clube.

### 15.4 Tutorial contextual (primeira tentativa)

Dicas em caixa de diálogo, ditas por **Mike** (pelo walkie-talkie) ou **Bill** (pensando). Cada uma aparece **uma vez** (flag no perfil), pode ser desligada em Opções e fica disponível em "Como jogar". Os botões citados aparecem conforme o dispositivo atual.

**Teclado E joystick, sempre.** O jogo é jogável inteiro pelos dois, e quem joga de controle nunca deve ler "aperte E". Vale para **todo** texto que cita um botão: falas e dicas, rodapés de atalhos do HUD, "E: abrir o baú" sobre objetos, rodapés dos menus (pausa, inventário, Mural, alçapão, resultado, menu principal). Cada texto mostra o botão do **último dispositivo usado** (mexeu no controle, os textos passam a dizer "RB", "A", "B"; voltou ao teclado, "E", "Enter", "Esc"). Exceção de propósito: a página **Controles**, que mostra as duas colunas lado a lado.

| ID | Gatilho | Ensina |
|---|---|---|
| D01 | Início do prólogo | Mover (WASD/analógico) |
| D02 | Perto de Mike | Interagir (E/RB) e caixa de diálogo |
| D03 | Primeira sala escura | Escuridão, lanterna e mapa explorado |
| D04 | Primeiro inimigo visível | Atacar e combo |
| D05 | Primeiro aviso de ataque inimigo | Dash e invulnerabilidade |
| D06 | Primeira bateria ou 1ª sala do bioma 1 | Clarão e cargas |
| D07 | Primeiro item coletado | Inventário (I/Tab) |
| D08 | HP < 50% pela 1ª vez | Kit de cura (Q/LB) |
| D09 | Primeira sala limpa | Portas com recompensa |
| D10 | Primeiro Eco | Ecos e escolha de 1 de 3 |
| D11 | Primeira subida de nível | XP, níveis e Ficha de atributos |
| D12 | Primeira armadilha revelada | Armadilhas |
| D13 | Primeira porta trancada | Chaves (enferrujada e do Medo) |
| D14 | Primeira loja | Moedas e Elias |
| D15 | Primeiro refúgio | Área segura |
| D16 | Primeiro chefe | Fases, avisos e barra do chefe |
| D17 | Primeiro resgate | Resgates e Eco lendário |
| D18 | Primeiro Pacto consumido | Vidas e pontuação |
| D19 | Primeira volta ao Clube | O ciclo, as Lembranças e o Mural |
| D20 | Primeira vez no minimapa | Diário e mapa (M/LT) |

### 15.5 Outras telas

Pausa (continuar, inventário, como jogar, opções, abandonar a tentativa com confirmação) · Subida de nível (distribuir pontos com pré-visualização dos valores derivados) · Escolha de Eco (3 cartas) · Loja · Fim da tentativa (resultado, score detalhado por símbolo, estatísticas, seed e tempo) · Vitória e epílogo · Créditos.

### 15.6 Regras de legibilidade

Textos em português brasileiro com acentos. Avisos de ataque com contorno, padrão e símbolo. Nenhum piscar contínuo, inversão de controles ou tremor que esconda avisos; o tremor de tela é leve (≤ 2 px, ≤ 150 ms) e pode ser desligado. Ao perder o foco, a janela **pausa e limpa as entradas pendentes**.

---

## 16. Pontuação (critérios F04 e F13)

```
S = máx(0, 100K + 300E + 50I + 500R + 1000B + 250C + 5000V − ⌊M/2⌋ − 1000U)
```

| Símbolo | Significado (sem dupla contagem) |
|---|---|
| **K** | Inimigos comuns derrotados (não conta chefes nem invocações) |
| **E** | Elites derrotados (P1; sem elites, E = 0) |
| **I** | Itens coletados do chão ou de baús (cada coleta conta 1×; compras na loja não contam) |
| **R** | Amigos resgatados (máx. 6) |
| **B** | Chefes-medo dispersados (máx. 6) |
| **C** | Salas concluídas |
| **V** | 1 se derrotou o Pennywise no Covil |
| **M** | **Movimentos**: células percorridas por Bill (cada vez que o centro dos pés entra numa célula nova). Andar contra a parede não conta. |
| **U** | **Vidas utilizadas**: Pactos de Sangue consumidos, mais 1 se a tentativa terminou em morte |

- **Exemplo de vitória:** K=140, E=10, I=60, R=6, B=6, C=24, V=1, M=8000, U=1 → 14000 + 3000 + 3000 + 3000 + 6000 + 6000 + 5000 − 4000 − 1000 = **35.000**
- **Exemplo de derrota no bioma 2:** K=25, E=1, I=12, R=1, B=1, C=5, V=0, M=1500, U=2 → 2500 + 300 + 600 + 500 + 1000 + 1250 − 750 − 2000 = **3.400**

**Atualização da parte 2 (13/11/2026):** a fórmula passa a considerar o **tempo T** (bônus de vitória rápida `V·máx(0, 9000 − 2T)` e penalidade `−⌊T/5⌋`) e o **multiplicador da dificuldade D** (×0,75 / ×1 / ×1,5). Definição completa e novos exemplos (37.400 · 3.220 · 56.100) no SDD §24.3. Os recordes guardam dificuldade, tempo e se a partida foi do computador.

A fórmula existe numa **única função** usada pelo HUD, pela tela de resultado, pelo menu de explicação e pelos testes. A pontuação pode cair quando Bill anda, conforme a fórmula. Diálogos, curas e menus não dão pontos.

---

## 17. Joystick (P0)

- **Caminho 1: SDL2 GameController**, com `SDL2.dll` oficial (2.30.x, licença zlib) **ao lado do executável**, carregada dinamicamente. Funciona com Xbox, PlayStation, Switch Pro e 8BitDo, inclusive no modo "Pro Controller", que o DirectInput lia com entradas fantasmas no Bomberman.
- **Caminho 2: XInput** (`xinput1_4.dll`), se a SDL2 não estiver presente.
- **Sem DirectInput.**
- Os botões são mapeados por **posição** (sul/leste/oeste/norte), não por letra impressa.
- Zona morta radial de 25% no analógico, com reescala; direcional digital com prioridade.
- Conectar e desconectar durante o jogo: desconectar **pausa** a partida com o aviso "Controle desconectado". A busca por controle novo é feita no máximo a cada 2 s e só quando nenhum está conectado (aprendizado do Bomberman: buscar todo segundo travava o jogo).
- Teclado e joystick funcionam ao mesmo tempo; a última entrada usada define os ícones mostrados.
- Sem foco na janela, a entrada do joystick é ignorada.

---

## 18. Duração e ritmo (meta: 45–60 min por vitória)

| Trecho | Salas | Tempo estimado |
|---|---|---|
| Prólogo (só na 1ª vez) | 1–2 | 3 min |
| Bioma (×6) | 3–4 combates (~70 s) + especial (~30 s) + loja/refúgio (~40 s) + resgate (~60 s) + chefe (~120 s) + transições | ~7,5 min → **45 min** |
| Covil | 1 arena, 3 fases | 4–5 min |
| Epílogo | — | 1 min |
| **Total de uma tentativa vitoriosa** | ~44 salas | **~50–55 min** |

- A quantidade de salas por bioma, o número de ondas e o HP dos chefes vêm dos dados, para ajustar a duração sem tocar no código.
- **Telemetria local** (`saves/telemetria.csv`, **feita na Etapa 8.5**): uma linha por sala de cada tentativa jogada na janela (pessoa ou computador, coluna `quem`), com o **tempo de jogo** e o **tempo real** (diálogos, loja e menus congelam o cronômetro, mas contam para quem joga). A meta é validada em pelo menos 3 tentativas completas antes da entrega.
- **Medição do piloto (17/11):** bioma 1 em ~4,6 min de jogo no Normal (combates ~55 s, chefe ~55 s). Uma pessoa leva mais (lê diálogos, decide na loja, erra desvios); a meta de ~7,5 min por bioma se confirma com a telemetria de tentativas reais.
- **Modo Defesa** (`--defesa` ou Ctrl+F12): pular para qualquer bioma, invencibilidade, +XP, revelar o mapa e mostrar hitboxes. Assim o professor vê tudo em 10 minutos.

---

## 19. Direção de arte e assets

### 19.1 Contrato visual (mantido do v1)

Pixel art de RPG de 16 bits, vista superior com a frente visível, sem perspectiva isométrica. Tiles de 32×32; personagens de 32×48 com pivô nos pés; inimigos comuns de 32×32; chefes-medo e Pennywise IV de 64×64; ícones de 16×16; retratos de 64×64. Paleta: azuis quase pretos, verdes apagados, cinza pedra, ferrugem e pele, com **vermelho vivo reservado a balões e perigo**. Cada bioma tem uma **cor dominante** para a variedade ser legível: Galeria verde-cinza, Barrens verde-musgo e marrom, Neibolt madeira e sépia, Porões azul-água, Siderúrgica laranja-brasa, Cisterna azul-violeta, Covil preto, vermelho e luz morta.

### 19.2 Resolução e qualidade (atualizado em 10/11/2026: render HD)

- **Lógica de 640×360** (tiles de 32, sprites de 32×48): é nela que ficam as regras, a colisão e a câmera.
- **Render HD:** o jogo desenha na resolução real da tela, 640×360 × escala inteira (2× em 720p, 3× em 1080p, até 6× em 4K). O texto e os sprites ficam nítidos, e o movimento fica suave.
- **Assets em 4×** (`assets/jogo_hd/`, gerados por `tools/export_final.py --hd`): cores completas, alfa suave e manifesto com as medidas lógicas + `fator`. Cada imagem é reduzida **uma vez** para a escala da tela, por média de área, frame a frame. As ilustrações usam fator 2 (1280×720).
- Motivo: no contrato antigo (32×48, 64 cores, alfa binário), os rostos viravam borrões. A comparação lado a lado está em `tools/avaliar_hd.py`.
- `assets/jogo/` (1×) continua sendo gerado, como reserva e para ferramentas.

### 19.3 Assets existentes (P01–P55) — todos aprovados e exportados em `assets/jogo/`

Bill (andar, ataque, dano, derrota), Ben, Eddie, Beverly, Mike, Elias, Pennywise I–IV (ações, dispersão, derrota), balão, rato, sombra, tilesets da galeria, dos porões e da cisterna, objetos, objetos da arena, 9 itens, símbolos do HUD, 7 retratos, 8 efeitos, capa e epílogo.

**Pendência conhecida:** conferir a linha "direita" de `bill_walk` (no contact sheet ela parece virada para a esquerda). Se confirmado, espelhar a linha "esquerda" na exportação.

### 19.4 Assets novos (P56–P105) — prompts em `instructions/Prompts-Assets-IT-v2.md`

| Grupo | Assets | Prio |
|---|---|---|
| Novos NPCs | Richie, Stan e Georgie (espera 64×48) + 3 retratos | P0 |
| Chefes-medo | Múmia, Afogado, Leproso, Ralo de Sangue, Pássaro das Cinzas, Lobisomem — atlas 256×256 (4×4 de 64×64: andar, preparar, atacar, rasgar) | P0 |
| Inimigos novos | Sanguessuga, Afogado comum, Boneco palhaço, Cabelo do ralo, Espectro de fuligem — atlas 128×128 | P0 |
| **Cenários (texturas contínuas, v3)** | **Prontos em 10/11/2026.** 9 ambientes × 5 materiais (piso, piso secundário, face de parede, topo de parede, líquido/terreno), prompts P106–P150. Substituem os tilesets em atlas (P24–P26 ficam só como reserva; P73–P78 foram cancelados). O motor desenha a célula (c, l) com o pedaço (c mod 4, l mod 4), e as transições (sombra da parede, borda da água) são feitas por código. | P0 ✅ |
| Objetos novos | Clube, Barrens, Neibolt, Siderúrgica e objetos comuns (baús, caixa, altar de Eco, refúgio, portas de recompensa) | P0 |
| Interface | Moldura 9-slice, ícones de recompensa, emblemas dos 6 Ecos | P0 |
| Itens novos | Estilingue, cano, moeda de prata, pacto de sangue, melhoria de arma | P0 |
| Efeitos novos | Fogo no piso, Eco (pilar de luz), Luzes Mortas, nuvem pútrida | P0/P1 |
| Ilustrações | Cartões dos 7 biomas, tela "Bill acorda no Clube" | P1 |

---

## 20. Áudio (P0 desde a parte 2 do enunciado)

Música em loop por bioma, no hub e no chefe (WAV/MP3 via MCI); efeitos curtos (passo, golpe, acerto, dash, clarão, porta, item, aviso, nível) por WinMM. O áudio não participa da lógica. Volume em Opções. Os arquivos precisam ter licença livre (CC0/CC-BY), registrada nos créditos. **Os sons são providenciados pelo Diego**; o motor (mixer próprio com várias vozes) e a lista de IDs estão no SDD §24.8.

---

## 21. Rastreabilidade com o enunciado (PDF T3)

### 21.1 Funcionalidades (F01–F20 = ordem da tabela do professor)

| ID | Critério do PDF | Como o v2 entrega | Verificação |
|---|---|---|---|
| F01 | Otimizado, sem atrasos; jogador se move sem bugs | Passo de lógica fixo de 60 Hz, render por software em 640×360, assets pré-convertidos | p95 do quadro ≤ 16,7 ms medido no modo Release; teste de colisão em quinas |
| F02 | Não pisca nem desloca blocos de forma incoerente | Buffer duplo (DIB) + apresentação por escala inteira; o piscar de dano é de 1 quadro | Jogar e redimensionar sem apagões |
| F03 | Design caprichado e consistente | Contrato de arte §19, paleta por bioma, HUD sobreposto legível | Revisão de capturas em ×2 e ×3 |
| F04 | Menu com como jogar, itens e pontuação | §15.1 | Acessar as 3 explicações antes de jogar |
| F05 | Mover em todas as direções | 8 direções (inclui as 4 exigidas) por teclado e joystick | Teste manual + teste de movimento |
| F06 | Bloqueado por qualquer parede | Colisão AABB × tiles com deslizamento; portas fechadas, grades, NPCs e chefes bloqueiam | Testes automatizados de colisão |
| F07 | Inimigos diferentes, aleatórios, que perseguem e respeitam paredes | 8 tipos (§9) com vagar/perseguir/BFS | Testes de IA + observação |
| F08 | Vence ao derrotar o chefe final no último nível | Somente o Pennywise IV no Covil dá V = 1 | Teste do estado de vitória |
| F09 | Itens, chaves que abrem portas, poções de vida | Kits, chave enferrujada (baús/portas), Chave do Medo (resgate), manivela | Teste de chave correta e incorreta |
| F10 | Perde se a vida chegar a zero | HP ≤ 0 sem Pacto → fim da tentativa e resultado | Teste de dano letal com e sem Pacto |
| F11 | Mapa começa escuro e abre ao explorar | §14.1 | Teste de visão e memória da exploração |
| F12 | Sistema de batalha | §8: acerto, esquiva, crítico, armadura, recuo, avisos | Testes das fórmulas |
| F13 | Pontuação e vida na tela | HUD §15.2 + resultado §16 | Mesma função nos dois |
| F14 | Armadilhas | §14.2 | Armadilha atinge Bill e inimigo |
| F15 | XP e subida de nível | §7.2 | Teste de XP atravessando 2 limiares |
| F16 | Atributos distribuídos ao subir de nível | 2 pontos por nível, tela de nível e Ficha | Teste de aplicação e limite 9 |
| F17 | Atributos afetam armadura, acerto, dano, esquiva, vida, velocidade | Tabela §7.1 | Exemplo verificável §7.3 |
| F18 | História e NPCs | §4: 9 NPCs, prólogo, 6 resgates, Clube, epílogo | Jogar o prólogo e um resgate |
| F19 | Mapa muda por NPCs, mobs e jogador | §14.3 | Teste de colisão antes e depois de cada evento |
| F20 | Boss final | Pennywise IV em 3 fases (§4.5) | Teste das transições de fase |

### 21.1b Funcionalidades da parte 2 (M01–M15)

A cobertura completa, com a situação anterior de cada item e a solução, está no **SDD §24.1**. Em resumo: já atendidos e mantidos sem redução: 3+ fases (M02), recordes em arquivo (M04), 7+ itens (M05), aleatoriedade (M12), farmar XP (M14) e pausa (M15). Novos: 3 dificuldades com probabilidades próprias (M01, M06), tempo na tela e na pontuação (M03), som (M07), itens mudando o visual do Bill (M08), itens pegos na interface (M09), inimigos que pegam/destroem itens (M10), companheiro (M11), sala secreta, Lembrancinhas e Festa dos Balões (M13).

### 21.2 Técnicas de programação (T01–T04)

| ID | Técnica | Demonstração (detalhes no SDD) |
|---|---|---|
| T01 | Struct | `Vec2`, `Tile`, `Sala`, `Jogador`, `Atributos`, `Inimigo`, `Chefe`, `Projetil`, `Aviso`, `Item`, `Inventario`, `Eco`, `EstadoPartida`, `Perfil`… |
| T02 | Sub-rotinas com parâmetros e referências corretos | Consultas por `const&`, alterações por `&` explícitas, funções pequenas |
| T03 | Ponteiros e referências | Busca por ID que retorna ponteiro (pode ser `nullptr`); **tabela de cenas com ponteiros para função**; canvas e imagens passados por ponteiro/referência; nenhum ponteiro guardado após alterar um `vector` |
| T04 | Boa segmentação | Módulos separados: regras (sem janela) × plataforma × render; funções de validação, efeito, combate, IA, visão, missões, score |
| T05 | **Computador joga sozinho e vence** (3,0) | Piloto automático (SDD §24.7): mesmos comandos do jogador, decisão em camadas, BFS, desvio de avisos, ficha por chefe; simulação sem janela prova a vitória em várias seeds |
| T06 | Uso dos quesitos anteriores (1,0) | T01–T04 aplicados também ao piloto, à dificuldade e ao companheiro |

---

## 22. Prioridades

- **P0 (obrigatório para entregar):** tudo de F01–F20 e T01–T04; tempo real com dash; joystick; menu completo; tutorial em diálogos; inventário em jogo; hub + Mural + Pacto; 6 biomas + Covil; 8 inimigos; 6 chefes-medo + Pennywise IV; 24 Ecos + 6 lendários; Taco + Estilingue; telemetria; Modo Defesa.
- **P0 acrescentado pela parte 2 (13/11/2026):** 3 dificuldades; cronômetro + tempo na pontuação; áudio; aparência por item; inimigos × itens; companheiro; sala secreta, Lembrancinhas e Festa dos Balões; **piloto automático que vence o jogo**.
- **P1 (polimento):** Medo; elites; Cano de ferro; Ecos duplos; cartões de bioma; golpe carregado; vibração do controle.
- **P2 (depois da entrega):** mouse; remapeamento; "noites mais escuras" (modificadores de dificuldade); mais salas por bioma.

**Escopo completo confirmado pelo Diego (10/11/2026):** não há regra de corte. Os 6 biomas, as ~81 salas, os chefes e todo o conteúdo P0 serão entregues. A ordem das etapas garante que o jogo esteja sempre jogável de ponta a ponta com o que já existe (cada bioma pronto entra na tentativa).

---

## 23. Riscos

| Risco | Impacto | Mitigação |
|---|---|---|
| Escopo grande (6 biomas, ~81 salas) — **aceito** | Volume de trabalho | Conteúdo orientado a dados (salas em .txt, inimigos e chefes em tabelas); fatia vertical (bioma 1) primeiro; os biomas seguintes reutilizam os sistemas e só acrescentam dados, arte e um comportamento por inimigo ou chefe; validador automático de salas |
| Defesa individual com código grande | Nota | Código em structs e funções livres (sem herança), comentários em PT-BR, módulos pequenos, ensaio de alterações por pessoa |
| Desempenho do GDI+ | F01 | GDI+ só para carregar PNG; render por software num buffer de 640×360; uma única apresentação por quadro |
| 8BitDo e controles diversos | Joystick falha | SDL2 GameController (comprovado no Bomberman) + XInput; sem DirectInput |
| Arte por IA inconsistente | F03 | Mesmo pipeline do v1 (chroma, referências, contact sheet, export); nunca anexar o retrato de um personagem como referência de outro |
| Duração fora de 45–60 min | Requisito do Diego | Telemetria + parâmetros em dados |

---

## 24. Definição de concluído

1. Os 20 critérios funcionais, os 15 da parte 2 (M01–M15) e as técnicas T01–T06 têm evidência jogável **e** teste quando aplicável.
1b. **O computador vence uma tentativa completa ao vivo** e o `t_piloto` passa (≥ 8/10 seeds no Normal, 10/10 no Fácil).
2. Uma tentativa completa, do prólogo ao epílogo, **foi jogada e medida** entre 45 e 60 min.
3. O caminho de derrota (com e sem Pacto) foi verificado, a volta ao Clube preserva as Lembranças e o reinício limpa as flags da tentativa.
4. O jogo funciona com teclado e com joystick (Xbox/XInput e 8BitDo via SDL2).
5. A pasta de entrega roda numa máquina limpa: `ecos_de_derry.exe` + `SDL2.dll` + `assets/` + `data/`, sem caminhos absolutos.
6. Cada integrante sabe explicar e alterar dano, inimigo, regra de XP, encontro de chefe, sala e item sem refazer o programa.
