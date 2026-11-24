"""Gera instructions/Prompts-Assets-IT-v2.md (P56–P105) a partir de uma especificação compacta.

Uso:  python tools/prompts_v2.py

O formato de saída é o mesmo do pacote v1 (catálogo em tabela + seções "## Pnn"),
para que tools/batch_images.py e tools/export_final.py leiam os dois arquivos.
O texto-base de estilo é idêntico ao v1, garantindo a mesma identidade visual.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "instructions" / "Prompts-Assets-IT-v2.md"

STYLE = ("Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens "
         "visível, câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos "
         "tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e "
         "vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado "
         "com crianças protagonistas e manifestações sobrenaturais. ")
GRID = ("Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas "
        "o movimento previsto. A referência determina identidade, não apenas um clima parecido. ")
ALPHA = ("Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, "
         "letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, "
         "sem partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito.")
SAFE = "Terror estilizado e sugestivo, sem sangue explícito, ferimentos, gore ou violência gráfica. "
OPAQUE_END = "Sem interface, texto, título, legendas ou grade desenhada."

TILE_ORDER = ("Cada tile tem 32 × 32, atlas de 4 colunas e 4 linhas, total 128 × 128. Ordem exata por linhas, lendo da "
              "esquerda para a direita: 1 piso seco; 2 piso variante; 3 parede horizontal; 4 parede vertical; 5 canto "
              "superior esquerdo; 6 canto superior direito; 7 canto inferior esquerdo; 8 canto inferior direito; 9 "
              "junção em T; 10 junção em cruz; 11 {t11}; 12 {t12}; 13 piso com detalhe decorativo; 14 {t14}; 15 piso "
              "seguro com marca discreta; 16 {t16}. Esses números servem apenas para orientar a ordem e não devem "
              "aparecer na imagem. Pisos se repetem sem costura; paredes e cantos se encaixam. Iluminação neutra "
              "constante, sem fog of war pintado e sem sombra de personagem. Todos os 16 tiles são totalmente "
              "preenchidos e opacos. Não incluir itens, portas, personagens, interface, texto ou grade.")

NPC_LAYOUT = ("Dois frames de espera, cada um 32 × 48, lado a lado, total 64 × 48. Frame 1 em pé de frente; frame 2 "
              "a mesma pose com respiração sutil. Personagem inteiro, dos pés à cabeça, na mesma escala das outras "
              "crianças do jogo. ")
PORTRAIT = ("Retrato de busto 64 × 64 em pixel art: apenas cabeça, pescoço e ombros, sem mãos e sem cintura, olhando "
            "levemente para o lado, fundo azul-escuro liso de uma cor só. Mesma identidade, rosto, cabelo e roupa do "
            "sprite anexado. Sem moldura, texto ou letras.")
MOB_LAYOUT = ("Atlas 4 colunas × 4 linhas, frame 32 × 32, total 128 × 128. Linhas de cima para baixo: virado para "
              "baixo, esquerda, direita, cima. Colunas: espera, passo A, passo B, ataque. Mesma criatura em todos os "
              "frames, silhueta legível em 32 pixels, pivô na base. ")
BOSS_LAYOUT = ("Atlas 4 colunas × 4 linhas, frame 64 × 64, total 256 × 256, criatura de frente para a câmera em todas as "
               "poses. Linha 1: andar em quatro frames. Linha 2: preparação crescente do ataque em quatro frames. Linha "
               "3: ataque e recuperação em quatro frames. Linha 4: a forma se rasga como papel molhado e revela uma "
               "névoa com detalhes vermelhos e brancos de palhaço, em quatro frames, sem mostrar o palhaço inteiro. "
               "Toda a figura fica dentro de cada frame de 64 × 64. ")
OBJ_NOTE = ("Atlas 4 colunas × 4 linhas, cada objeto ocupa uma célula de 32 × 32, total 128 × 128. Objetos isolados, "
            "centrados, com a base alinhada ao fundo da célula, mesma luz e perspectiva dos tilesets do jogo. "
            "Ordem exata por linhas: {itens}. Esses números servem apenas para orientar a ordem e não devem aparecer "
            "na imagem. ")
ITEM = ("Ícone de item 16 × 16 em pixel art, objeto único centralizado, contorno escuro de 1 pixel, legível em "
        "tamanho pequeno, mesmo acabamento dos ícones de item anexados. {desc} ")
FX = ("Efeito visual em {n} frames de {f} × {f}, lado a lado em uma única linha horizontal, total {w} × {f}. "
      "Sem personagem, sem cenário. {desc} ")
ILLU = ("Ilustração em pixel art 640 × 360, composição horizontal 16:9, mesma direção de arte e paleta da cena de "
        "referência anexada. {desc} " + SAFE + OPAQUE_END)

R_CHILD = ["referencia_visual", "ben_idle", "eddie_idle"]
R_MOB = ["balao", "rato", "sombra"]
R_BOSS = ["pennywise_referencia", "pennywise_4_actions", "sombra"]
R_TILE = ["referencia_visual", "galeria", "cisterna"]
R_OBJ = ["objetos", "objetos_arena"]
R_ITEM = ["referencia_visual", "item_kit", "item_bateria"]
R_FX = ["item_kit", "item_bateria"]
R_ILLU = ["referencia_visual", "capa_menu"]

# (id, título, arquivo, dimensões, referências, corpo do prompt, alfa?)
S = []


def add(pid, titulo, arquivo, dims, refs, corpo, alfa=True, grid=False):
    S.append((pid, titulo, arquivo, dims, refs, corpo, alfa, grid))


# ---------- NPCs (P56–P61) ----------
add("P56", "Richie Tozier em espera", "richie_idle.png", "64 × 48 px; 2 colunas × 1 linha; frame 32 × 48", R_CHILD,
    NPC_LAYOUT + "Identidade de Richie nesta adaptação: menino magro de cerca de 12 anos, óculos grandes de armação "
    "preta e lentes grossas, cabelo preto curto e bagunçado, sorriso debochado, camisa de botão estampada de cores "
    "fortes por cima de camiseta branca, bermuda jeans e tênis. Não é o Bill: sem jaqueta azul.", grid=True)
add("P57", "Stan Uris em espera", "stan_idle.png", "64 × 48 px; 2 colunas × 1 linha; frame 32 × 48", R_CHILD,
    NPC_LAYOUT + "Identidade de Stan nesta adaptação: menino de cerca de 12 anos, arrumado e sério, cabelo castanho "
    "claro curto e cacheado bem penteado, camisa polo verde-oliva por dentro da calça cáqui, cinto, sapatos limpos, "
    "segura um pequeno caderno de pássaros. Não é o Bill: sem jaqueta azul.", grid=True)
add("P58", "Georgie em espera", "georgie_idle.png", "64 × 48 px; 2 colunas × 1 linha; frame 32 × 48", R_CHILD,
    NPC_LAYOUT.replace("na mesma escala das outras crianças do jogo", "visivelmente menor que as outras crianças, "
    "ocupando cerca de dois terços da altura do frame") + "Identidade de Georgie: menino pequeno de cerca de 6 anos, "
    "capa de chuva amarela com capuz, galochas vermelhas, segura um barquinho de papel. Expressão doce, mas os olhos "
    "têm um brilho levemente errado, sugerindo uma ilusão. " + SAFE, grid=True)
add("P59", "Retrato de Richie", "retrato_richie.png", "64 × 64 px; imagem individual", ["richie_idle"],
    PORTRAIT + " Richie com óculos de lentes grossas e sorriso debochado.", alfa=False)
add("P60", "Retrato de Stan", "retrato_stan.png", "64 × 64 px; imagem individual", ["stan_idle"],
    PORTRAIT + " Stan com expressão séria e atenta.", alfa=False)
add("P61", "Retrato de Georgie", "retrato_georgie.png", "64 × 64 px; imagem individual", ["georgie_idle"],
    PORTRAIT + " Georgie de capa amarela com capuz, sorriso doce com um brilho estranho no olhar.", alfa=False)

# ---------- Chefes-medo (P62–P67) ----------
BOSSES = [
    ("P62", "Medo de Ben — a Múmia", "medo_mumia.png",
     "Múmia alta e curvada envolta em faixas de linho velhas e amareladas, algumas faixas soltas balançando, olhos como "
     "dois pontos de luz laranja, andar arrastado. Na preparação, estica as faixas para a frente como chicotes."),
    ("P63", "Medo de Stan — o Afogado da Caixa d'Água", "medo_afogado.png",
     "Figura encharcada e pálida de roupas antigas e escuras, cabelo molhado cobrindo o rosto, água escorrendo, "
     "algas presas aos braços longos. Na preparação, afunda até a cintura numa poça azul-escura; no ataque, emerge "
     "jogando água em anel."),
    ("P64", "Medo de Eddie — o Leproso", "medo_leproso.png",
     "Andarilho magro e corcunda com pele acinzentada e descascada de forma estilizada, roupas rasgadas de "
     "vagabundo, capuz, mãos compridas. Na preparação, curva-se e solta uma nuvem esverdeada; no ataque, investe com os "
     "braços estendidos."),
    ("P65", "Medo de Beverly — o Ralo de Sangue", "medo_ralo.png",
     "Ralo redondo de banheiro antigo no chão de ladrilhos, do qual brota uma massa de cabelos longos e escuros "
     "molhados de líquido vermelho-escuro, formando tentáculos. Não anda: na linha de andar, os cabelos ondulam no "
     "lugar. Na preparação, os cabelos se erguem em cruz; no ataque, chicoteiam para os quatro lados."),
    ("P66", "Medo de Mike — o Pássaro das Cinzas", "medo_passaro.png",
     "Pássaro gigante de penas cinza-carvão com brasas laranja entre as penas, bico longo, olhos amarelos, asas "
     "abertas que quase ocupam o frame. Na linha de andar, voa batendo as asas. Na preparação, encolhe as asas e as "
     "brasas brilham; no ataque, mergulha soltando faíscas."),
    ("P67", "Medo de Richie — o Lobisomem", "medo_lobisomem.png",
     "Lobisomem adolescente de pelo marrom-escuro desgrenhado, jaqueta universitária rasgada nas costas, garras "
     "longas, olhos amarelos, postura curvada e ameaçadora. Na preparação, abaixa-se para saltar; no ataque, "
     "sequência de garras em arco."),
]
for pid, titulo, arq, desc in BOSSES:
    add(pid, titulo, arq, "256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64", R_BOSS,
        "Chefe do jogo: uma forma que Pennywise assume a partir do maior medo de uma das crianças. " + desc + " "
        + BOSS_LAYOUT + SAFE, grid=True)

# ---------- Inimigos comuns novos (P68–P72) ----------
MOBS = [
    ("P68", "Sanguessuga alada", "sanguessuga.png",
     "Sanguessuga voadora do tamanho de um gato, corpo segmentado vermelho-amarronzado, pequenas asas de inseto "
     "translúcidas, boca circular. Flutua acima do chão com uma pequena sombra oval. No frame de ataque, investe esticada."),
    ("P69", "Afogado comum", "afogado.png",
     "Figura encharcada, pálida e baixa, roupas antigas escuras, cabelo molhado cobrindo o rosto, poça de água sob os "
     "pés. No frame de ataque, estende os dois braços para agarrar."),
    ("P70", "Boneco palhaço", "boneco_palhaco.png",
     "Boneco de palhaço de pano e porcelana, rosto branco rachado, sorriso pintado, gola de babados vermelha, "
     "movimentos de marionete. No frame de ataque, ergue o braço para arremessar um pino de boliche."),
    ("P71", "Cabelo do ralo", "cabelo_ralo.png",
     "Pequeno ralo redondo no chão com um tufo de cabelos escuros e molhados saindo dele. Fixo no lugar: nas quatro "
     "linhas, os cabelos apontam para a direção da linha. Colunas: recolhido, saindo, ondulando, chicote estendido."),
    ("P72", "Espectro de fuligem", "espectro_fuligem.png",
     "Espírito feito de fumaça preta e fuligem, com brasas laranja nos olhos e no peito, forma vagamente humana e "
     "pequena, a parte de baixo se desfazendo em fumaça. No frame de ataque, explode em faíscas."),
]
for pid, titulo, arq, desc in MOBS:
    add(pid, titulo, arq, "128 × 128 px; 4 colunas × 4 linhas; frame 32 × 32", R_MOB,
        "Inimigo comum do jogo. " + desc + " " + MOB_LAYOUT + SAFE, grid=True)

# ---------- Tilesets (P73–P78) ----------
TILES = [
    ("P73", "Tileset Rua de Derry", "rua_derry.png",
     "Rua residencial de cidade pequena americana à noite depois da chuva: asfalto molhado, calçada de concreto, "
     "meio-fio, muros baixos de tijolo e cercas de madeira como paredes, poças refletindo luz de poste.",
     "poça de chuva rasa", "bueiro com grade no meio-fio", "calçada com folhas", "borda da calçada com meio-fio"),
    ("P74", "Tileset Clube dos Perdedores", "clube.png",
     "Interior de um clube secreto subterrâneo construído por crianças: piso de tábuas de madeira, paredes de terra "
     "escorada com tábuas e vigas, tapete velho, iluminação quente de lampião.",
     "tapete gasto", "alçapão fechado de madeira", "piso de tábuas com tapete", "borda de tábua com terra"),
    ("P75", "Tileset Os Barrens", "barrens.png",
     "Mata pantanosa e selvagem à beira de um riacho: terra úmida, grama alta, lama, raízes, barrancos de terra e "
     "pedra com arbustos como paredes, riacho raso de água esverdeada.",
     "água rasa do riacho", "lama funda", "grama com flores silvestres", "margem do riacho"),
    ("P76", "Tileset Casa da Rua Neibolt", "neibolt.png",
     "Interior de casa abandonada e apodrecida: tábuas de assoalho escuras e empenadas, papel de parede descascado "
     "verde e sépia nas paredes, rodapés, poeira, teias, porão de terra batida.",
     "terra batida do porão", "tábuas podres rachadas", "tábuas com poeira e teias", "borda do assoalho com rodapé"),
    ("P77", "Tileset Siderúrgica Kitchener", "siderurgica.png",
     "Ruínas de siderúrgica antiga: chapas de ferro rebitadas e enferrujadas, piso de concreto com fuligem, paredes "
     "de tijolo enegrecido e vigas de aço, brilho laranja de brasas nas frestas.",
     "piso de carvão solto", "grelha de ferro sobre brasas", "concreto com fuligem", "borda de chapa de ferro"),
    ("P78", "Tileset Covil", "covil.png",
     "Covil subterrâneo da criatura: chão de pedra escura e úmida coberto de brinquedos velhos espalhados, paredes de "
     "rocha com montanhas de objetos perdidos, fios de balões vermelhos, luz fria e esbranquiçada vinda do alto.",
     "água escura parada", "pilha de brinquedos baixa", "pedra com brinquedos", "borda de água escura"),
]
# OBSOLETO desde 10/11/2026: os tilesets em atlas (P73–P78) foram substituídos pelas texturas
# contínuas de tools/prompts_texturas.py (P106–P150). Os dados ficam acima só como referência.
for pid, titulo, arq, desc, t11, t12, t14, t16 in []:
    add(pid, titulo, arq, "128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32", R_TILE,
        "Crie um tileset para um novo ambiente do mesmo jogo, com o mesmo acabamento e encaixe dos tilesets "
        "anexados. Tema: " + desc + " " + TILE_ORDER.format(t11=t11, t12=t12, t14=t14, t16=t16), alfa=False)

# ---------- Objetos (P79–P83) ----------
OBJS = [
    ("P79", "Objetos do Clube", "objetos_clube.png",
     "1 mural de cortiça com desenhos infantis; 2 baú de madeira com armas de brinquedo; 3 rede pendurada; 4 mesa com "
     "mapas; 5 alçapão fechado; 6 alçapão aberto com escada; 7 quadro-negro de recordes; 8 lampião aceso; 9 caixa de "
     "discos e gibis; 10 rádio walkie-talkie; 11 pilha de livros; 12 pôster rasgado; 13 cadeira velha; 14 caixote; "
     "15 balde; 16 vela acesa"),
    ("P80", "Objetos dos Barrens", "objetos_barrens.png",
     "1 comporta de madeira aberta com água passando; 2 comporta fechada; 3 tronco caído; 4 arbusto espinhoso; 5 pedra "
     "grande; 6 represa de galhos; 7 ninho de pássaro; 8 lata enferrujada; 9 tubo de esgoto saindo do barranco; 10 "
     "carrinho de mão velho; 11 pneu velho; 12 placa de madeira caída; 13 ferro-velho pequeno; 14 tábua sobre a água; "
     "15 toco de árvore; 16 moita de juncos"),
    ("P81", "Objetos da Neibolt", "objetos_neibolt.png",
     "1 porta de madeira trancada com corrente; 2 a mesma porta aberta; 3 janela tapada com tábuas; 4 poço de pedra "
     "no porão; 5 piso podre intacto; 6 piso podre quebrado com buraco; 7 tábua com pregos para cima; 8 armário velho; "
     "9 quadro torto na parede; 10 cadeira de balanço; 11 geladeira antiga; 12 pilha de bonecos de palhaço; 13 "
     "escada de madeira; 14 lustre caído; 15 baú com fechadura; 16 vela derretida"),
    ("P82", "Objetos da Siderúrgica", "objetos_siderurgica.png",
     "1 fornalha apagada; 2 fornalha acesa; 3 gerador desligado; 4 gerador ligado com luzes verdes; 5 esteira de metal; "
     "6 grelha de fogo em aviso com brasas; 7 grelha de fogo com chamas; 8 monte de carvão; 9 corrente pendurada com "
     "gancho; 10 lâmpada industrial apagada; 11 lâmpada acesa; 12 barril de metal; 13 alavanca para cima; 14 "
     "alavanca para baixo; 15 vagonete de minério; 16 engrenagem grande"),
    ("P83", "Objetos comuns", "objetos_comuns.png",
     "1 baú de madeira fechado; 2 baú aberto vazio; 3 baú dourado fechado; 4 baú dourado aberto; 5 caixa de madeira "
     "quebrável; 6 caixa quebrada em tábuas; 7 altar de pedra com chama azul acesa; 8 o mesmo altar apagado; 9 "
     "círculo de refúgio no chão com luz quente; 10 porta de sala aberta com moldura de pedra; 11 porta de sala "
     "fechada com barras; 12 suporte de recompensa vazio; 13 balcão do zelador com ferramentas; 14 placa de saída; "
     "15 escada de descida em buraco; 16 vaso quebrável"),
]
for pid, titulo, arq, itens in OBJS:
    add(pid, titulo, arq, "128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32", R_OBJ,
        "Objetos interativos e decorativos do jogo. " + OBJ_NOTE.format(itens=itens) + ALPHA.replace(
            " e com o mesmo pivô dos pés", ""), grid=False)

# ---------- Interface (P84–P86) ----------
add("P84", "Moldura de diálogo", "ui_moldura.png", "96 × 96 px; imagem individual", R_OBJ,
    "Moldura de caixa de diálogo de RPG, UMA única moldura contínua de 96 × 96 vista de frente, para ser esticada "
    "por 9-slice: borda de madeira escura com pregos de ferro, de espessura constante de cerca de 1/8 da largura, "
    "cantos levemente ornamentados e um pequeno balão vermelho preso no canto superior esquerdo; o centro é papel "
    "envelhecido azul-acinzentado escuro, liso e uniforme, opaco. Nenhuma linha divisória, grade ou texto. " + ALPHA.replace(
        " e com o mesmo pivô dos pés", ""))
add("P85", "Ícones de recompensa das portas", "ui_recompensas.png", "128 × 16 px; 8 colunas × 1 linha; ícone 16 × 16",
    R_ITEM, ITEM.format(desc="Oito ícones lado a lado em uma linha, mesmo estilo: 1 chama azul de eco; 2 moeda de prata; "
    "3 pequena estrela de memória dourada; 4 kit de cura; 5 bateria; 6 chave enferrujada; 7 taco com pregos; 8 baú.")
    + ALPHA, grid=True)
add("P86", "Emblemas dos Ecos", "ui_emblemas.png", "192 × 32 px; 6 colunas × 1 linha; ícone 32 × 32", R_ITEM,
    "Seis emblemas circulares 32 × 32 lado a lado, cada um com uma cor e um símbolo: 1 Ben, tijolo laranja sobre "
    "azul; 2 Stan, pássaro branco sobre verde-oliva; 3 Eddie, inalador branco sobre azul-claro; 4 Beverly, estilingue "
    "sobre vermelho; 5 Mike, chama sobre marrom; 6 Richie, óculos sobre amarelo. Borda escura, brilho suave. " + ALPHA,
    grid=True)

# ---------- Itens novos (P87–P91) ----------
for pid, titulo, arq, desc in [
    ("P87", "Estilingue", "item_estilingue.png", "Estilingue de madeira em forquilha com elástico e uma bolinha de prata."),
    ("P88", "Cano de ferro", "item_cano.png", "Pedaço de cano de ferro enferrujado com fita na empunhadura, na diagonal."),
    ("P89", "Moeda de prata", "item_moeda.png", "Moeda de prata antiga e brilhante, levemente inclinada."),
    ("P90", "Pacto de sangue", "item_pacto.png", "Gota vermelha estilizada sobre uma palma aberta com uma cicatriz em "
     "linha, símbolo de juramento, sem sangue explícito."),
    ("P91", "Melhoria de arma", "item_melhoria.png", "Punhado de pregos e um martelo pequeno cruzados."),
]:
    add(pid, titulo, arq, "16 × 16 px; imagem individual", R_ITEM, ITEM.format(desc=desc) + ALPHA)

# ---------- Efeitos novos (P92–P95) ----------
for pid, titulo, arq, n, f, desc in [
    ("P92", "Fogo no piso", "fx_fogo.png", 4, 32, "Chamas baixas laranja e amarelas sobre o chão, ciclo de quatro frames em loop."),
    ("P93", "Pilar de Eco", "fx_eco.png", 4, 32, "Coluna de luz azul-clara com partículas subindo, em loop."),
    ("P94", "Luzes Mortas", "fx_luzes_mortas.png", 4, 64, "Três globos de luz branco-alaranjada pulsando dentro de uma névoa "
     "escura, hipnóticos, crescendo e diminuindo."),
    ("P95", "Nuvem pútrida", "fx_nuvem.png", 4, 32, "Nuvem verde-acinzentada baixa se espalhando e desfazendo."),
]:
    add(pid, titulo, arq, f"{n * f} × {f} px; {n} colunas × 1 linha; frame {f} × {f}", R_FX,
        FX.format(n=n, f=f, w=n * f, desc=desc) + ALPHA)

# ---------- Ilustrações (P96–P105) ----------
ILLUS = [
    ("P96", "Cartão Galeria pluvial", "cartao_galeria.png", "Galeria de esgoto com água rasa, grades de ferro e um balão vermelho ao fundo."),
    ("P97", "Cartão Os Barrens", "cartao_barrens.png", "Os Barrens ao entardecer: riacho, mata pantanosa, uma caixa d'água ao longe."),
    ("P98", "Cartão Casa da Rua Neibolt", "cartao_neibolt.png", "Fachada de casa vitoriana abandonada e torta na Rua Neibolt, à noite."),
    ("P99", "Cartão Porões inundados", "cartao_poroes.png", "Porão inundado com canos antigos e uma pia de banheiro ao fundo, luz azul."),
    ("P100", "Cartão Siderúrgica Kitchener", "cartao_siderurgica.png", "Ruínas de siderúrgica com fornalhas e brasas laranja, fumaça."),
    ("P101", "Cartão Cisterna das vozes", "cartao_cisterna.png", "Cisterna enorme com tubulações e ecos visuais de silhuetas falsas."),
    ("P102", "Cartão Covil", "cartao_covil.png", "Covil com montanha de brinquedos perdidos, balões vermelhos e três luzes no alto."),
    ("P103", "Clube dos Perdedores", "cartao_clube.png", "Interior aconchegante do clube subterrâneo das crianças com lampião e mapas na parede."),
    ("P104", "Bill acorda no Clube", "tela_ciclo.png", "Bill deitado numa rede no clube, acordando assustado, a palma da mão com uma cicatriz brilhando em vermelho suave."),
    ("P105", "Os sete Perdedores", "tela_perdedores.png", "Sete crianças lado a lado na beira do riacho ao pôr do sol, de costas, de mãos dadas."),
]
for pid, titulo, arq, desc in ILLUS:
    add(pid, titulo, arq, "640 × 360 px", R_ILLU, ILLU.format(desc=desc), alfa=False)


def render():
    out = ["# Prompts de imagens para IT Ecos de Derry — v2 (P56–P105)", "",
           "Complemento do pacote v1 (`Prompts-Assets-IT.md`, P01–P55) para o PRD v2: novos NPCs, chefes-medo, "
           "inimigos, biomas, interface, itens, efeitos e ilustrações 16:9. Mesmo texto-base de estilo do v1. "
           "Gerado por `tools/prompts_v2.py`; edite o script, não este arquivo.", "",
           "Regras herdadas do v1: nunca anexar o retrato de um personagem como referência de outro; pedir fundo verde "
           "#00FF00 na API (o script de lote acrescenta a nota); revisar contact sheets antes de aceitar.", "",
           "## Catálogo", "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |"]
    for pid, titulo, arq, dims, *_ in S:
        out.append(f"| {pid} | {titulo} | {arq} | {dims} |")
    out.append("")
    for pid, titulo, arq, dims, refs, corpo, alfa, grid in S:
        prompt = f"Entregue uma única imagem PNG. Dimensões finais desejadas: {dims}. " + STYLE
        if grid:
            prompt += GRID
        prompt += corpo
        if alfa and "Fundo realmente transparente" not in prompt:
            prompt += " " + ALPHA
        out += [f"## {pid} {titulo}", "", f"**Arquivo:** {arq}", "", f"**Dimensões finais:** {dims}", "",
                f"**Referências para anexar:** {', '.join(r + '.png' for r in refs)}", "",
                "**Prompt completo para copiar:**", "", "```text", prompt.strip(), "```", ""]
    OUT.write_text("\n".join(out), encoding="utf-8")
    print(f"{len(S)} prompts → {OUT}")


if __name__ == "__main__":
    render()
