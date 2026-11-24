# Prompts dos assets v9: armas e poderes dos amigos, corte do taco e menus repaginados (P227–P252)

Gerado por `tools/prompts_v9_amigos_ui.py` (24/11/2026). Pedidos do Diego: arma e poder de cada amigo ligados à lore; golpes na diagonal; inventário, itens e Mural de talentos com visual profissional (Resident Evil, Final Fantasy, Hades). Gerar com `python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v9-amigos-ui.md`.

| ID | Imagem | Arquivo | Dimensões finais |
| --- | --- | --- | --- |
| P227 | Arma do Mike: cutelo do açougue | fx_arma_mike.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P228 | Poder do Mike: pistola de abate | fx_poder_mike.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P229 | Arma do Ben: martelo de carpinteiro | fx_arma_ben.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P230 | Poder do Ben: desabamento da ponte | fx_poder_ben.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P231 | Barreira do Ben: tijolo flutuante | fx_barreira_ben.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P232 | Arma da Beverly: bala de prata do estilingue | fx_arma_beverly.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P233 | Poder da Beverly: labareda | fx_poder_beverly.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P234 | Arma do Stan: machadinha de escoteiro | fx_arma_stan.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P235 | Poder do Stan: revoada | fx_poder_stan.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P236 | Arma do Eddie: estaca de cerca | fx_arma_eddie.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P237 | Poder do Eddie: ácido do inalador | fx_poder_eddie.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P238 | Arma do Richie: fichas do fliperama | fx_arma_richie.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P239 | Poder do Richie: as Vozes | fx_poder_richie.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P240 | Rastro do taco (girado pelo jogo) | fx_corte_taco.png | 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48 |
| P252 | Olho atento do Stan: passarinho de aviso | fx_passaro_stan.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P241 | Painel ornamentado (9 fatias) | ui_painel.png | 96 × 96 px; imagem individual |
| P242 | Placas de linha do menu | ui_placas.png | 224 × 64 px; 1 coluna × 2 linhas; frame 224 × 32 |
| P243 | Abas do menu | ui_abas.png | 224 × 32 px; 2 colunas × 1 linha; frame 112 × 32 |
| P244 | Ícones das abas | ui_icones_abas.png | 160 × 32 px; 5 colunas × 1 linha; frame 32 × 32 |
| P245 | Ícones dos atributos | ui_icones_atributos.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P246 | Ícones dos talentos do Mural | ui_talentos.png | 144 × 144 px; 3 colunas × 3 linhas; frame 48 × 48 |
| P247 | Polaroides do Mural | ui_polaroid.png | 256 × 72 px; 4 colunas × 1 linha; frame 64 × 72 |
| P248 | Fundo do inventário (a mochila) | tela_fundo_mochila.png | 640 × 360 px; imagem individual |
| P249 | Quadro de cortiça do Mural | tela_mural_cortica.png | 640 × 360 px; imagem individual |
| P250 | Bill de corpo inteiro (inventário) | retrato_bill_corpo.png | 128 × 192 px; imagem individual |
| P251 | Selo de raridade e moldura do item grande | ui_moldura_item.png | 192 × 64 px; 3 colunas × 1 linha; frame 64 × 64 |

## P227 Arma do Mike: cutelo do açougue

**Arquivo:** fx_arma_mike.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_taco.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: o CUTELO de açougueiro do pai do Mike (a família dele tem uma fazenda e um açougue): lâmina retangular larga de aço gasto com um furo no canto, cabo de madeira escura com rebites de latão. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P228 Poder do Mike: pistola de abate

**Arquivo:** fx_poder_mike.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_impacto.png, fx_fogo.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: o estampido da PISTOLA PNEUMÁTICA DE ABATE do açougue: um anel de choque de ar comprimido branco-acinzentado explodindo para fora, faíscas de aço e três estrelinhas amarelas de atordoamento girando no auge. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P229 Arma do Ben: martelo de carpinteiro

**Arquivo:** fx_arma_ben.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_taco.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: o MARTELO DE CARPINTEIRO do Ben, o construtor da represa e do Clube: cabeça de ferro com unha de tirar pregos, cabo de madeira clara, e dois pregos girando junto. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P230 Poder do Ben: desabamento da ponte

**Arquivo:** fx_poder_ben.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_impacto.png, objetos_barrens.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: TÁBUAS DE MADEIRA e vigas da ponte que o Ben constrói caindo do alto e se empilhando sobre o alvo, com pregos saltando e uma nuvem de pó de serragem no auge. Madeira castanha clara e escura, pregos prateados. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P231 Barreira do Ben: tijolo flutuante

**Arquivo:** fx_barreira_ben.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_taco.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: um TIJOLO vermelho-alaranjado gasto com uma tábua curta pregada nele, a peça da barreira que o Ben ergue em volta do Bill. Gira devagar flutuando, com um brilho de poeira dourada. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P232 Arma da Beverly: bala de prata do estilingue

**Arquivo:** fx_arma_beverly.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_estilingue.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: a BALA DE PRATA do estilingue da Beverly (a melhor atiradora dos Perdedores): uma bolinha prateada brilhante, feita de um dólar de prata derretido, com um rastro curto de luz azulada. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P233 Poder da Beverly: labareda

**Arquivo:** fx_poder_beverly.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_fogo.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: uma LABAREDA de fogo que sobe do chão em espiral, laranja e amarela com o miolo quase branco e faíscas, como a coragem da Beverly pegando fogo. Fogo estilizado em blocos de cor. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P234 Arma do Stan: machadinha de escoteiro

**Arquivo:** fx_arma_stan.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_taco.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: a MACHADINHA DE ESCOTEIRO do Stan (organizado, escoteiro e observador de pássaros): lâmina de aço pequena e limpa, cabo de madeira com uma tira de couro verde enrolada. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P235 Poder do Stan: revoada

**Arquivo:** fx_poder_stan.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_impacto.png, fx_fuligem.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: uma REVOADA de pássaros do livro de aves do Stan dando um rasante no alvo: seis a oito pássaros pequenos, azulados e pretos (gaios-azuis e pardais), asas abertas, penas soltas girando no auge. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P236 Arma do Eddie: estaca de cerca

**Arquivo:** fx_arma_eddie.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_taco.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: uma ESTACA DE CERCA de madeira com a ponta de ferro, arrancada da cerca da casa da Rua Neibolt, girando no ar. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P237 Poder do Eddie: ácido do inalador

**Arquivo:** fx_poder_eddie.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_nuvem.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: um jato de NÉVOA do inalador de asma do Eddie que vira ÁCIDO DE BATERIA (no livro ele acredita que é ácido e queima a Coisa): nuvem verde-amarelada borbulhando, com gotas que chiam e fumacinha cinza. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P238 Arma do Richie: fichas do fliperama

**Arquivo:** fx_arma_richie.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_moeda.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: três FICHAS DE FLIPERAMA douradas e redondas (o Richie vive no fliperama de Derry), empilhadas e girando em leque, com um brilho metálico. Os 4 frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P239 Poder do Richie: as Vozes

**Arquivo:** fx_poder_richie.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_impacto.png, fx_eco.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: as VOZES do Richie, o imitador: ondas sonoras em anéis coloridos (amarelo, rosa e azul) saindo do centro, com estrelinhas e pequenas notas musicais em forma de desenho, empurrando tudo para fora. Divertido e barulhento. Frame 1 o efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P240 Rastro do taco (girado pelo jogo)

**Arquivo:** fx_corte_taco.png

**Dimensões finais:** 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48

**Referências para anexar:** fx_impacto.png, item_taco.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 48 px; 4 colunas × 1 linha; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie o RASTRO DE GOLPE de um taco de beisebol em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48. O golpe vai SEMPRE da esquerda para a DIREITA da imagem: uma meia-lua de luz branco-azulada com borda creme, mais grossa no meio e afinando nas pontas, aberta para a esquerda (o lado do Bill), como a lâmina de ar de um golpe rápido. Frame 1 o arco começando, fino; frame 2 o arco inteiro e mais forte, com faíscas brancas; frame 3 o arco se desfazendo em fiapos; frame 4 só restos de brilho. Centrado; o pivô é o centro do frame. Não desenhar o taco, mãos, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P252 Olho atento do Stan: passarinho de aviso

**Arquivo:** fx_passaro_stan.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** fx_poder_stan.png, fx_impacto.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie um PASSARINHO pequeno em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: um gaio-azul do livro de aves do Stan (o escoteiro observador de pássaros dos Perdedores), que pousa em cima de uma armadilha para avisar o Bill. Azul vivo com peito claro, bico e olho bem legíveis, visto de lado com um pouco da frente, voltado para a direita. Frame 1 pousado, alerta, cabeça erguida e bico aberto como quem chama; frame 2 pousado, dando um pulinho com a cauda levantada; frame 3 abrindo as asas para levantar voo; frame 4 em voo, asas para cima. O pássaro ocupa cerca de 55% do frame; nos frames 1 e 2 os pés tocam a base do frame, centrados. Não desenhar galho, chão, armadilha, personagens, cenário ou interface. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P241 Painel ornamentado (9 fatias)

**Arquivo:** ui_painel.png

**Dimensões finais:** 96 × 96 px; imagem individual

**Referências para anexar:** ui_moldura.png, item_kit.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 96 × 96 px; imagem individual. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie uma MOLDURA DE PAINEL de menu, quadrada, para ser fatiada em 9 partes (cantos de 24 × 24 px no tamanho final de 96 × 96). Inspiração: os menus de Hades e Final Fantasy, no material do Clube dos Perdedores: borda de madeira escura envernizada com cantoneiras de LATÃO ENVELHECIDO trabalhadas e um prego em cada canto, um filete fino vermelho-escuro por dentro da borda. O MIOLO é um azul-noite quase preto, liso e uniforme (sem desenho), levemente translúcido. As quatro bordas retas são iguais e repetíveis; os quatro cantos são iguais entre si (espelhados). Sem balão, sem letras. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P242 Placas de linha do menu

**Arquivo:** ui_placas.png

**Dimensões finais:** 224 × 64 px; 1 coluna × 2 linhas; frame 224 × 32

**Referências para anexar:** ui_moldura.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 224 × 64 px; 1 coluna × 2 linhas; frame 224 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie DUAS PLACAS horizontais de linha de menu, uma em cima da outra, cada uma 224 × 32. Linha 1 (normal): uma faixa de madeira escura com acabamento fino de latão nas pontas, discreta e escura. Linha 2 (selecionada): a mesma faixa, mas com o latão aceso e dourado, um brilho quente de lanterna por trás e uma pequena seta de latão apontando para a direita na ponta esquerda. As duas têm o miolo liso, sem letras, onde o jogo escreve o texto. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P243 Abas do menu

**Arquivo:** ui_abas.png

**Dimensões finais:** 224 × 32 px; 2 colunas × 1 linha; frame 112 × 32

**Referências para anexar:** ui_moldura.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 224 × 32 px; 2 colunas × 1 linha; frame 112 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie DUAS ABAS de menu lado a lado, cada uma 112 × 32, como as abas de um caderno de escola antigo presas a uma moldura de latão. Coluna 1 (aba fechada): papel pardo escuro e apagado, borda de latão escura. Coluna 2 (aba aberta): papel creme iluminado, borda de latão dourado e uma fita vermelha na ponta de cima. Miolo liso sem letras. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P244 Ícones das abas

**Arquivo:** ui_icones_abas.png

**Dimensões finais:** 160 × 32 px; 5 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_kit.png, item_taco.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 160 × 32 px; 5 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie CINCO ÍCONES de aba lado a lado, cada um 32 × 32, mesmo tamanho e mesmo traço: (1) uma mochila escolar de lona azul gasta (Itens); (2) um taco de beisebol cruzado com uma tampa de lixeira usada como escudo (Equipamento); (3) um mapa dobrado de Derry com um X vermelho (Missão); (4) um balão vermelho com um brilho fantasmagórico azul em volta (Ecos); (5) um caderno de capa de couro com um lápis (Ficha). Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P245 Ícones dos atributos

**Arquivo:** ui_icones_atributos.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_kit.png, item_coragem.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie QUATRO ÍCONES de atributo lado a lado, cada um 32 × 32, num medalhão redondo de latão envelhecido: (1) Força: um punho fechado segurando um taco, fundo vermelho-escuro; (2) Foco: um olho atento com a íris dourada, fundo azul; (3) Agilidade: um tênis de cano alto com riscos de velocidade, fundo verde; (4) Coragem: um coração vermelho com uma chama por dentro, fundo âmbar. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P246 Ícones dos talentos do Mural

**Arquivo:** ui_talentos.png

**Dimensões finais:** 144 × 144 px; 3 colunas × 3 linhas; frame 48 × 48

**Referências para anexar:** item_pacto.png, item_kit.png, item_lembranca.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 144 × 144 px; 3 colunas × 3 linhas; frame 48 × 48. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie NOVE ÍCONES de talento em grade 3 × 3, cada um 48 × 48, como desenhos feitos à mão pelas crianças e colados no mural, na ordem: (1) Pacto de Sangue: duas mãos com um corte na palma se apertando, um brilho vermelho; (2) Coragem herdada: um coração com um curativo; (3) Lanterna reforçada: uma lanterna amarela acesa com raios; (4) Bolso fundo: um bolso de calça jeans com um kit de primeiros socorros saindo; (5) Treino no Barrens: um tênis enlameado sobre uma pedra do riacho; (6) Mão leve: uma mão pegando moedas de prata; (7) Sorte do Richie: os óculos de lente grossa do Richie com um trevo; (8) Mapa do Ben: um mapa de esgotos desenhado a lápis; (9) Fôlego extra: um inalador de asma com asas. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P247 Polaroides do Mural

**Arquivo:** ui_polaroid.png

**Dimensões finais:** 256 × 72 px; 4 colunas × 1 linha; frame 64 × 72

**Referências para anexar:** ui_moldura.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 72 px; 4 colunas × 1 linha; frame 64 × 72. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie QUATRO MOLDURAS DE FOTO POLAROIDE lado a lado, cada uma 64 × 72, para os talentos presos num quadro de cortiça. A janela da foto (onde o jogo desenha o ícone) é um quadrado escuro liso de 48 × 48 no alto, com a faixa branca mais larga embaixo. (1) Bloqueada: papel amarelado e desbotado, presa com fita adesiva, um cadeado de ferrugem pequeno no canto. (2) Disponível: papel branco-creme, presa por uma tachinha de latão. (3) Comprada: papel creme com uma tachinha VERMELHA e um X de caneta vermelha marcando o canto. (4) Selecionada: como a disponível, mas inclinada levemente e com um brilho quente de lanterna em volta. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P248 Fundo do inventário (a mochila)

**Arquivo:** tela_fundo_mochila.png

**Dimensões finais:** 640 × 360 px; imagem individual

**Referências para anexar:** cartao_clube.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px; imagem individual. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie a ILUSTRAÇÃO DE FUNDO da tela de inventário, 16:9: vista de cima de um tampo de madeira escura no Clube dos Perdedores à noite, iluminado só por uma lanterna no canto superior esquerdo. Espalhados nas BORDAS: um mapa desbotado de Derry, SETE fotos polaroide, uma de cada criança, e só UMA delas é menina: Bill (menino, cabelo castanho liso), Richie (menino, cabelo preto e óculos de lente grossa), Eddie (menino magro, cabelo castanho curto), Stan (menino, cabelo loiro cacheado bem penteado), Ben (menino gordinho, cabelo castanho), Mike (menino negro, cabelo bem curto) e Beverly (a ÚNICA menina, cabelo ruivo CURTO); nenhum outro ruivo. Também um balão vermelho preso por um barbante, gibis velhos, um estilingue, pilhas e uma bússola. O CENTRO da imagem fica escuro, calmo e quase vazio (madeira em sombra), porque os painéis do menu ficam por cima. Clima de mistério acolhedor, não de susto. Imagem opaca, sem bordas pretas de letterbox: a ilustração ocupa o quadro inteiro. Sem letras, números, legendas, interface, logotipos ou marcas d’água.
```

## P249 Quadro de cortiça do Mural

**Arquivo:** tela_mural_cortica.png

**Dimensões finais:** 640 × 360 px; imagem individual

**Referências para anexar:** cartao_clube.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px; imagem individual. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie a ILUSTRAÇÃO DE FUNDO do Mural dos Perdedores, 16:9: um grande QUADRO DE CORTIÇA visto de frente, preso numa parede de tábuas do Clube, com recortes de jornal de Derry desbotados nas bordas, um mapa dos esgotos, bilhetes e algumas tachinhas soltas. O centro da cortiça fica LIVRE (só cortiça), porque o jogo prende ali as polaroides dos talentos e os fios vermelhos entre elas. Luz quente de lanterna vinda de cima, cantos em sombra. Imagem opaca, sem bordas pretas de letterbox: a ilustração ocupa o quadro inteiro. Sem letras, números, legendas, interface, logotipos ou marcas d’água.
```

## P250 Bill de corpo inteiro (inventário)

**Arquivo:** retrato_bill_corpo.png

**Dimensões finais:** 128 × 192 px; imagem individual

**Referências para anexar:** retrato_bill.png, bill_referencia.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 192 px; imagem individual. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie o BILL DENBROUGH DE CORPO INTEIRO, de frente, em pé, postura determinada, para o painel do personagem do inventário (como o retrato grande de Hades): menino de 12 anos, cabelo castanho, moletom azul com capuz sobre camiseta cinza-esverdeada, bermuda cáqui, tênis gastos; segura o taco de beisebol apoiado no ombro direito e uma lanterna na mão esquerda. Mesma identidade do retrato anexado. A figura ocupa a altura inteira da imagem, pés na base, centrada. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. A figura inteira deve ficar dentro da imagem, sem partes cortadas. Entregar somente o asset descrito.
```

## P251 Selo de raridade e moldura do item grande

**Arquivo:** ui_moldura_item.png

**Dimensões finais:** 192 × 64 px; 3 colunas × 1 linha; frame 64 × 64

**Referências para anexar:** ui_moldura.png, item_kit.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 64 px; 3 colunas × 1 linha; frame 64 × 64. Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). Crie TRÊS MOLDURAS QUADRADAS de destaque de item, lado a lado, cada uma 64 × 64, como o quadro que mostra o item escolhido em grande nos menus de Resident Evil: miolo escuro liso e vazio (o jogo desenha o item ali), borda de latão trabalhada com cantoneiras. (1) Comum: latão escuro e cinza. (2) Raro: latão com pedras azuis nos cantos e brilho azul suave. (3) Épico: latão dourado com pedras vermelhas e um brilho vermelho-alaranjado. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```
