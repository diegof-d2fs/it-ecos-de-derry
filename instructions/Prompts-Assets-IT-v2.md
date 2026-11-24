# Prompts de imagens para IT Ecos de Derry — v2 (P56–P105)

Complemento do pacote v1 (`Prompts-Assets-IT.md`, P01–P55) para o PRD v2: novos NPCs, chefes-medo, inimigos, biomas, interface, itens, efeitos e ilustrações 16:9. Mesmo texto-base de estilo do v1. Gerado por `tools/prompts_v2.py`; edite o script, não este arquivo.

Regras herdadas do v1: nunca anexar o retrato de um personagem como referência de outro; pedir fundo verde #00FF00 na API (o script de lote acrescenta a nota); revisar contact sheets antes de aceitar.

## Catálogo

| ID | Imagem | Arquivo | Dimensões finais |
| --- | --- | --- | --- |
| P56 | Richie Tozier em espera | richie_idle.png | 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48 |
| P57 | Stan Uris em espera | stan_idle.png | 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48 |
| P58 | Georgie em espera | georgie_idle.png | 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48 |
| P59 | Retrato de Richie | retrato_richie.png | 64 × 64 px; imagem individual |
| P60 | Retrato de Stan | retrato_stan.png | 64 × 64 px; imagem individual |
| P61 | Retrato de Georgie | retrato_georgie.png | 64 × 64 px; imagem individual |
| P62 | Medo de Ben — a Múmia | medo_mumia.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P63 | Medo de Stan — o Afogado da Caixa d'Água | medo_afogado.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P64 | Medo de Eddie — o Leproso | medo_leproso.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P65 | Medo de Beverly — o Ralo de Sangue | medo_ralo.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P66 | Medo de Mike — o Pássaro das Cinzas | medo_passaro.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P67 | Medo de Richie — o Lobisomem | medo_lobisomem.png | 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64 |
| P68 | Sanguessuga alada | sanguessuga.png | 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32 |
| P69 | Afogado comum | afogado.png | 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32 |
| P70 | Boneco palhaço | boneco_palhaco.png | 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32 |
| P71 | Cabelo do ralo | cabelo_ralo.png | 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32 |
| P72 | Espectro de fuligem | espectro_fuligem.png | 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32 |
| P79 | Objetos do Clube | objetos_clube.png | 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32 |
| P80 | Objetos dos Barrens | objetos_barrens.png | 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32 |
| P81 | Objetos da Neibolt | objetos_neibolt.png | 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32 |
| P82 | Objetos da Siderúrgica | objetos_siderurgica.png | 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32 |
| P83 | Objetos comuns | objetos_comuns.png | 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32 |
| P84 | Moldura de diálogo | ui_moldura.png | 96 × 96 px; imagem individual |
| P85 | Ícones de recompensa das portas | ui_recompensas.png | 128 × 16 px; 8 colunas × 1 linha; ícone 16 × 16 |
| P86 | Emblemas dos Ecos | ui_emblemas.png | 192 × 32 px; 6 colunas × 1 linha; ícone 32 × 32 |
| P87 | Estilingue | item_estilingue.png | 16 × 16 px; imagem individual |
| P88 | Cano de ferro | item_cano.png | 16 × 16 px; imagem individual |
| P89 | Moeda de prata | item_moeda.png | 16 × 16 px; imagem individual |
| P90 | Pacto de sangue | item_pacto.png | 16 × 16 px; imagem individual |
| P91 | Melhoria de arma | item_melhoria.png | 16 × 16 px; imagem individual |
| P92 | Fogo no piso | fx_fogo.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P93 | Pilar de Eco | fx_eco.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P94 | Luzes Mortas | fx_luzes_mortas.png | 256 × 64 px; 4 colunas × 1 linha; frame 64 × 64 |
| P95 | Nuvem pútrida | fx_nuvem.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |
| P96 | Cartão Galeria pluvial | cartao_galeria.png | 640 × 360 px |
| P97 | Cartão Os Barrens | cartao_barrens.png | 640 × 360 px |
| P98 | Cartão Casa da Rua Neibolt | cartao_neibolt.png | 640 × 360 px |
| P99 | Cartão Porões inundados | cartao_poroes.png | 640 × 360 px |
| P100 | Cartão Siderúrgica Kitchener | cartao_siderurgica.png | 640 × 360 px |
| P101 | Cartão Cisterna das vozes | cartao_cisterna.png | 640 × 360 px |
| P102 | Cartão Covil | cartao_covil.png | 640 × 360 px |
| P103 | Clube dos Perdedores | cartao_clube.png | 640 × 360 px |
| P104 | Bill acorda no Clube | tela_ciclo.png | 640 × 360 px |
| P105 | Os sete Perdedores | tela_perdedores.png | 640 × 360 px |

## P56 Richie Tozier em espera

**Arquivo:** richie_idle.png

**Dimensões finais:** 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48

**Referências para anexar:** referencia_visual.png, ben_idle.png, eddie_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Dois frames de espera, cada um 32 × 48, lado a lado, total 64 × 48. Frame 1 em pé de frente; frame 2 a mesma pose com respiração sutil. Personagem inteiro, dos pés à cabeça, na mesma escala das outras crianças do jogo. Identidade de Richie nesta adaptação: menino magro de cerca de 12 anos, óculos grandes de armação preta e lentes grossas, cabelo preto curto e bagunçado, sorriso debochado, camisa de botão estampada de cores fortes por cima de camiseta branca, bermuda jeans e tênis. Não é o Bill: sem jaqueta azul. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P57 Stan Uris em espera

**Arquivo:** stan_idle.png

**Dimensões finais:** 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48

**Referências para anexar:** referencia_visual.png, ben_idle.png, eddie_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Dois frames de espera, cada um 32 × 48, lado a lado, total 64 × 48. Frame 1 em pé de frente; frame 2 a mesma pose com respiração sutil. Personagem inteiro, dos pés à cabeça, na mesma escala das outras crianças do jogo. Identidade de Stan nesta adaptação: menino de cerca de 12 anos, arrumado e sério, cabelo castanho claro curto e cacheado bem penteado, camisa polo verde-oliva por dentro da calça cáqui, cinto, sapatos limpos, segura um pequeno caderno de pássaros. Não é o Bill: sem jaqueta azul. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P58 Georgie em espera

**Arquivo:** georgie_idle.png

**Dimensões finais:** 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48

**Referências para anexar:** referencia_visual.png, ben_idle.png, eddie_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 48 px; 2 colunas × 1 linha; frame 32 × 48. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Dois frames de espera, cada um 32 × 48, lado a lado, total 64 × 48. Frame 1 em pé de frente; frame 2 a mesma pose com respiração sutil. Personagem inteiro, dos pés à cabeça, visivelmente menor que as outras crianças, ocupando cerca de dois terços da altura do frame. Identidade de Georgie: menino pequeno de cerca de 6 anos, capa de chuva amarela com capuz, galochas vermelhas, segura um barquinho de papel. Expressão doce, mas os olhos têm um brilho levemente errado, sugerindo uma ilusão. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P59 Retrato de Richie

**Arquivo:** retrato_richie.png

**Dimensões finais:** 64 × 64 px; imagem individual

**Referências para anexar:** richie_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 64 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Retrato de busto 64 × 64 em pixel art: apenas cabeça, pescoço e ombros, sem mãos e sem cintura, olhando levemente para o lado, fundo azul-escuro liso de uma cor só. Mesma identidade, rosto, cabelo e roupa do sprite anexado. Sem moldura, texto ou letras. Richie com óculos de lentes grossas e sorriso debochado.
```

## P60 Retrato de Stan

**Arquivo:** retrato_stan.png

**Dimensões finais:** 64 × 64 px; imagem individual

**Referências para anexar:** stan_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 64 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Retrato de busto 64 × 64 em pixel art: apenas cabeça, pescoço e ombros, sem mãos e sem cintura, olhando levemente para o lado, fundo azul-escuro liso de uma cor só. Mesma identidade, rosto, cabelo e roupa do sprite anexado. Sem moldura, texto ou letras. Stan com expressão séria e atenta.
```

## P61 Retrato de Georgie

**Arquivo:** retrato_georgie.png

**Dimensões finais:** 64 × 64 px; imagem individual

**Referências para anexar:** georgie_idle.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 64 × 64 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Retrato de busto 64 × 64 em pixel art: apenas cabeça, pescoço e ombros, sem mãos e sem cintura, olhando levemente para o lado, fundo azul-escuro liso de uma cor só. Mesma identidade, rosto, cabelo e roupa do sprite anexado. Sem moldura, texto ou letras. Georgie de capa amarela com capuz, sorriso doce com um brilho estranho no olhar.
```

## P62 Medo de Ben — a Múmia

**Arquivo:** medo_mumia.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Múmia alta e curvada envolta em faixas de linho velhas e amareladas, algumas faixas soltas balançando, olhos como dois pontos de luz laranja, andar arrastado. Na preparação, estica as faixas para a frente como chicotes. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P63 Medo de Stan — o Afogado da Caixa d'Água

**Arquivo:** medo_afogado.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Figura encharcada e pálida de roupas antigas e escuras, cabelo molhado cobrindo o rosto, água escorrendo, algas presas aos braços longos. Na preparação, afunda até a cintura numa poça azul-escura; no ataque, emerge jogando água em anel. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P64 Medo de Eddie — o Leproso

**Arquivo:** medo_leproso.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Andarilho magro e corcunda com pele acinzentada e descascada de forma estilizada, roupas rasgadas de vagabundo, capuz, mãos compridas. Na preparação, curva-se e solta uma nuvem esverdeada; no ataque, investe com os braços estendidos. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P65 Medo de Beverly — o Ralo de Sangue

**Arquivo:** medo_ralo.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Ralo redondo de banheiro antigo no chão de ladrilhos, do qual brota uma massa de cabelos longos e escuros molhados de líquido vermelho-escuro, formando tentáculos. Não anda: na linha de andar, os cabelos ondulam no lugar. Na preparação, os cabelos se erguem em cruz; no ataque, chicoteiam para os quatro lados. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P66 Medo de Mike — o Pássaro das Cinzas

**Arquivo:** medo_passaro.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Pássaro gigante de penas cinza-carvão com brasas laranja entre as penas, bico longo, olhos amarelos, asas abertas que quase ocupam o frame. Na linha de andar, voa batendo as asas. Na preparação, encolhe as asas e as brasas brilham; no ataque, mergulha soltando faíscas. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P67 Medo de Richie — o Lobisomem

**Arquivo:** medo_lobisomem.png

**Dimensões finais:** 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64

**Referências para anexar:** pennywise_referencia.png, pennywise_4_actions.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. Lobisomem adolescente de pelo marrom-escuro desgrenhado, jaqueta universitária rasgada nas costas, garras longas, olhos amarelos, postura curvada e ameaçadora. Na preparação, abaixa-se para saltar; no ataque, sequência de garras em arco. Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha 3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. Toda a figura fica dentro de cada frame de 64 × 64. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P68 Sanguessuga alada

**Arquivo:** sanguessuga.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32

**Referências para anexar:** balao.png, rato.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Inimigo comum do jogo. Sanguessuga voadora do tamanho de um gato, corpo segmentado vermelho-amarronzado, pequenas asas de inseto translúcidas, boca circular. Flutua acima do chão com uma pequena sombra oval. No frame de ataque, investe esticada. Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os frames, silhueta legível em 32 pixels, pivô na base. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P69 Afogado comum

**Arquivo:** afogado.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32

**Referências para anexar:** balao.png, rato.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Inimigo comum do jogo. Figura encharcada, pálida e baixa, roupas antigas escuras, cabelo molhado cobrindo o rosto, poça de água sob os pés. No frame de ataque, estende os dois braços para agarrar. Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os frames, silhueta legível em 32 pixels, pivô na base. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P70 Boneco palhaço

**Arquivo:** boneco_palhaco.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32

**Referências para anexar:** balao.png, rato.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Inimigo comum do jogo. Boneco de palhaço de pano e porcelana, rosto branco rachado, sorriso pintado, gola de babados vermelha, movimentos de marionete. No frame de ataque, ergue o braço para arremessar um pino de boliche. Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os frames, silhueta legível em 32 pixels, pivô na base. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P71 Cabelo do ralo

**Arquivo:** cabelo_ralo.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32

**Referências para anexar:** balao.png, rato.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Inimigo comum do jogo. Pequeno ralo redondo no chão com um tufo de cabelos escuros e molhados saindo dele. Fixo no lugar: nas quatro linhas, os cabelos apontam para a direção da linha. Colunas: recolhido, saindo, ondulando, chicote estendido. Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os frames, silhueta legível em 32 pixels, pivô na base. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P72 Espectro de fuligem

**Arquivo:** espectro_fuligem.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32

**Referências para anexar:** balao.png, rato.png, sombra.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Inimigo comum do jogo. Espírito feito de fumaça preta e fuligem, com brasas laranja nos olhos e no peito, forma vagamente humana e pequena, a parte de baixo se desfazendo em fumaça. No frame de ataque, explode em faíscas. Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os frames, silhueta legível em 32 pixels, pivô na base. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica.  Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P79 Objetos do Clube

**Arquivo:** objetos_clube.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Objetos interativos e decorativos do jogo. Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. Ordem exata por linhas: 1 mural de cortiça com desenhos infantis; 2 baú de madeira com armas de brinquedo; 3 rede pendurada; 4 mesa com mapas; 5 alçapão fechado; 6 alçapão aberto com escada; 7 quadro-negro de recordes; 8 lampião aceso; 9 caixa de discos e gibis; 10 rádio walkie-talkie; 11 pilha de livros; 12 pôster rasgado; 13 cadeira velha; 14 caixote; 15 balde; 16 vela acesa. Esses números servem apenas para orientar a ordem e não devem aparecer na imagem. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P80 Objetos dos Barrens

**Arquivo:** objetos_barrens.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Objetos interativos e decorativos do jogo. Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. Ordem exata por linhas: 1 comporta de madeira aberta com água passando; 2 comporta fechada; 3 tronco caído; 4 arbusto espinhoso; 5 pedra grande; 6 represa de galhos; 7 ninho de pássaro; 8 lata enferrujada; 9 tubo de esgoto saindo do barranco; 10 carrinho de mão velho; 11 pneu velho; 12 placa de madeira caída; 13 ferro-velho pequeno; 14 tábua sobre a água; 15 toco de árvore; 16 moita de juncos. Esses números servem apenas para orientar a ordem e não devem aparecer na imagem. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P81 Objetos da Neibolt

**Arquivo:** objetos_neibolt.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Objetos interativos e decorativos do jogo. Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. Ordem exata por linhas: 1 porta de madeira trancada com corrente; 2 a mesma porta aberta; 3 janela tapada com tábuas; 4 poço de pedra no porão; 5 piso podre intacto; 6 piso podre quebrado com buraco; 7 tábua com pregos para cima; 8 armário velho; 9 quadro torto na parede; 10 cadeira de balanço; 11 geladeira antiga; 12 pilha de bonecos de palhaço; 13 escada de madeira; 14 lustre caído; 15 baú com fechadura; 16 vela derretida. Esses números servem apenas para orientar a ordem e não devem aparecer na imagem. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P82 Objetos da Siderúrgica

**Arquivo:** objetos_siderurgica.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Objetos interativos e decorativos do jogo. Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. Ordem exata por linhas: 1 fornalha apagada; 2 fornalha acesa; 3 gerador desligado; 4 gerador ligado com luzes verdes; 5 esteira de metal; 6 grelha de fogo em aviso com brasas; 7 grelha de fogo com chamas; 8 monte de carvão; 9 corrente pendurada com gancho; 10 lâmpada industrial apagada; 11 lâmpada acesa; 12 barril de metal; 13 alavanca para cima; 14 alavanca para baixo; 15 vagonete de minério; 16 engrenagem grande. Esses números servem apenas para orientar a ordem e não devem aparecer na imagem. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P83 Objetos comuns

**Arquivo:** objetos_comuns.png

**Dimensões finais:** 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Objetos interativos e decorativos do jogo. Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. Ordem exata por linhas: 1 baú de madeira fechado; 2 baú aberto vazio; 3 baú dourado fechado; 4 baú dourado aberto; 5 caixa de madeira quebrável; 6 caixa quebrada em tábuas; 7 altar de pedra com chama azul acesa; 8 o mesmo altar apagado; 9 círculo de refúgio no chão com luz quente; 10 porta de sala aberta com moldura de pedra; 11 porta de sala fechada com barras; 12 suporte de recompensa vazio; 13 balcão do zelador com ferramentas; 14 placa de saída; 15 escada de descida em buraco; 16 vaso quebrável. Esses números servem apenas para orientar a ordem e não devem aparecer na imagem. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P84 Moldura de diálogo

**Arquivo:** ui_moldura.png

**Dimensões finais:** 96 × 96 px; imagem individual

**Referências para anexar:** objetos.png, objetos_arena.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 96 × 96 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Moldura de caixa de diálogo de RPG, UMA única moldura contínua de 96 × 96 vista de frente, para ser esticada por 9-slice: borda de madeira escura com pregos de ferro, de espessura constante de cerca de 1/8 da largura, cantos levemente ornamentados e um pequeno balão vermelho preso no canto superior esquerdo; o centro é papel envelhecido azul-acinzentado escuro, liso e uniforme, opaco. Nenhuma linha divisória, grade ou texto. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar somente o asset descrito.
```

## P85 Ícones de recompensa das portas

**Arquivo:** ui_recompensas.png

**Dimensões finais:** 128 × 16 px; 8 colunas × 1 linha; ícone 16 × 16

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 16 px; 8 colunas × 1 linha; ícone 16 × 16. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Oito ícones lado a lado em uma linha, mesmo estilo: 1 chama azul de eco; 2 moeda de prata; 3 pequena estrela de memória dourada; 4 kit de cura; 5 bateria; 6 chave enferrujada; 7 taco com pregos; 8 baú. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P86 Emblemas dos Ecos

**Arquivo:** ui_emblemas.png

**Dimensões finais:** 192 × 32 px; 6 colunas × 1 linha; ícone 32 × 32

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 192 × 32 px; 6 colunas × 1 linha; ícone 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas um clima parecido. Seis emblemas circulares 32 × 32 lado a lado, cada um com uma cor e um símbolo: 1 Ben, tijolo laranja sobre azul; 2 Stan, pássaro branco sobre verde-oliva; 3 Eddie, inalador branco sobre azul-claro; 4 Beverly, estilingue sobre vermelho; 5 Mike, chama sobre marrom; 6 Richie, óculos sobre amarelo. Borda escura, brilho suave. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P87 Estilingue

**Arquivo:** item_estilingue.png

**Dimensões finais:** 16 × 16 px; imagem individual

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 16 × 16 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Estilingue de madeira em forquilha com elástico e uma bolinha de prata. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P88 Cano de ferro

**Arquivo:** item_cano.png

**Dimensões finais:** 16 × 16 px; imagem individual

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 16 × 16 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Pedaço de cano de ferro enferrujado com fita na empunhadura, na diagonal. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P89 Moeda de prata

**Arquivo:** item_moeda.png

**Dimensões finais:** 16 × 16 px; imagem individual

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 16 × 16 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Moeda de prata antiga e brilhante, levemente inclinada. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P90 Pacto de sangue

**Arquivo:** item_pacto.png

**Dimensões finais:** 16 × 16 px; imagem individual

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 16 × 16 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Gota vermelha estilizada sobre uma palma aberta com uma cicatriz em linha, símbolo de juramento, sem sangue explícito. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P91 Melhoria de arma

**Arquivo:** item_melhoria.png

**Dimensões finais:** 16 × 16 px; imagem individual

**Referências para anexar:** referencia_visual.png, item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 16 × 16 px; imagem individual. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em tamanho pequeno, mesmo acabamento dos ícones de item anexados. Punhado de pregos e um martelo pequeno cruzados. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P92 Fogo no piso

**Arquivo:** fx_fogo.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Efeito visual em 4 frames de 32 × 32, lado a lado em uma única linha horizontal, total 128 × 32. Sem personagem, sem cenário. Chamas baixas laranja e amarelas sobre o chão, ciclo de quatro frames em loop. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P93 Pilar de Eco

**Arquivo:** fx_eco.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Efeito visual em 4 frames de 32 × 32, lado a lado em uma única linha horizontal, total 128 × 32. Sem personagem, sem cenário. Coluna de luz azul-clara com partículas subindo, em loop. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P94 Luzes Mortas

**Arquivo:** fx_luzes_mortas.png

**Dimensões finais:** 256 × 64 px; 4 colunas × 1 linha; frame 64 × 64

**Referências para anexar:** item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 256 × 64 px; 4 colunas × 1 linha; frame 64 × 64. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Efeito visual em 4 frames de 64 × 64, lado a lado em uma única linha horizontal, total 256 × 64. Sem personagem, sem cenário. Três globos de luz branco-alaranjada pulsando dentro de uma névoa escura, hipnóticos, crescendo e diminuindo. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P95 Nuvem pútrida

**Arquivo:** fx_nuvem.png

**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32

**Referências para anexar:** item_kit.png, item_bateria.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Efeito visual em 4 frames de 32 × 32, lado a lado em uma única linha horizontal, total 128 × 32. Sem personagem, sem cenário. Nuvem verde-acinzentada baixa se espalhando e desfazendo. Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.
```

## P96 Cartão Galeria pluvial

**Arquivo:** cartao_galeria.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Galeria de esgoto com água rasa, grades de ferro e um balão vermelho ao fundo. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P97 Cartão Os Barrens

**Arquivo:** cartao_barrens.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Os Barrens ao entardecer: riacho, mata pantanosa, uma caixa d'água ao longe. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P98 Cartão Casa da Rua Neibolt

**Arquivo:** cartao_neibolt.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Fachada de casa vitoriana abandonada e torta na Rua Neibolt, à noite. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P99 Cartão Porões inundados

**Arquivo:** cartao_poroes.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Porão inundado com canos antigos e uma pia de banheiro ao fundo, luz azul. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P100 Cartão Siderúrgica Kitchener

**Arquivo:** cartao_siderurgica.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Ruínas de siderúrgica com fornalhas e brasas laranja, fumaça. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P101 Cartão Cisterna das vozes

**Arquivo:** cartao_cisterna.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Cisterna enorme com tubulações e ecos visuais de silhuetas falsas. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P102 Cartão Covil

**Arquivo:** cartao_covil.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Covil com montanha de brinquedos perdidos, balões vermelhos e três luzes no alto. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P103 Clube dos Perdedores

**Arquivo:** cartao_clube.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Interior aconchegante do clube subterrâneo das crianças com lampião e mapas na parede. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P104 Bill acorda no Clube

**Arquivo:** tela_ciclo.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Bill deitado numa rede no clube, acordando assustado, a palma da mão com uma cicatriz brilhando em vermelho suave. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```

## P105 Os sete Perdedores

**Arquivo:** tela_perdedores.png

**Dimensões finais:** 640 × 360 px

**Referências para anexar:** referencia_visual.png, capa_menu.png

**Prompt completo para copiar:**

```text
Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de referência anexada. Sete crianças lado a lado na beira do riacho ao pôr do sol, de costas, de mãos dadas. Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. Sem interface, texto, título, legendas ou grade desenhada.
```
