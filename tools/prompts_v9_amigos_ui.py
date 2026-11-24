"""Gera instructions/Prompts-Assets-IT-v9-amigos-ui.md (P227–P252): armas e poderes dos amigos, o corte do taco
na diagonal e a arte dos menus repaginados.

Pedidos do Diego (24/11/2026):
  * "todas as vezes que tem outro personagem com o Bill, esse personagem terá uma arma e um poder, que têm de ter a
    ver com a lore do personagem" (o Mike é do açougue; a Beverly, fogo; o Ben, construção e ponte). Sprites e efeitos
    das armas e dos poderes gerados pelo Codex.
  * golpes do Bill na diagonal mais bonitos: um rastro do taco desenhado apontando para a DIREITA, que o jogo gira
    para a direção do golpe (qualquer ângulo).
  * "repaginar todo o menu de inventário, itens e árvore de habilidades", inspirado em Resident Evil, Final Fantasy
    e Hades: moldura ornamentada, placas de linha, abas com ícones, fundo ilustrado, quadro de cortiça do Mural com
    polaroides e ícones de talento.

Geração: python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v9-amigos-ui.md [arquivos...]
Uso: python tools/prompts_v9_amigos_ui.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v9-amigos-ui.md"

ESTILO = (
    "Pixel art de RPG com estética de 16 bits, câmera fixa e não isométrica. Contornos legíveis, blocos de cor "
    "definidos, sombras em poucos tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, "
    "cinza pedra, ferrugem, latão envelhecido e vermelho vivo reservado a balões e perigo. O jogo é IT Ecos de Derry, "
    "terror estilizado com crianças protagonistas (Derry, Maine, 1989). "
)
FINAL_ALFA = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, "
    "números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes "
    "cortadas. Entregar somente o asset descrito."
)
FINAL_OPACO = (
    " Imagem opaca, sem bordas pretas de letterbox: a ilustração ocupa o quadro inteiro. Sem letras, números, "
    "legendas, interface, logotipos ou marcas d’água."
)

ARMA = (
    "Crie um PROJÉTIL ARREMESSADO em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: {o_que} Os 4 "
    "frames mostram o objeto GIRANDO no ar (0°, 90°, 180°, 270°), com um leve rastro de movimento. O objeto ocupa "
    "cerca de 60% do frame, centrado; o pivô é o centro. Visto de cima com um pouco da frente. Não desenhar mãos, "
    "personagens, cenário ou interface."
)
PODER = (
    "Crie um EFEITO DE PODER em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: {o_que} Frame 1 o "
    "efeito surgindo, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O "
    "efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame, onde está o inimigo atingido. "
    "Terror estilizado, sem sangue explícito nem gore. Não desenhar personagens, inimigos, cenário ou interface."
)

# (id, título, arquivo, dimensões, referências, prompt)
ASSETS = []


def fx(pid, titulo, arq, molde, o_que, refs):
    dims = "128 × 32 px; 4 colunas × 1 linha; frame 32 × 32" if molde is ARMA else "192 × 48 px; 4 colunas × 1 linha; frame 48 × 48"
    ASSETS.append((pid, titulo, arq, dims, refs, ESTILO + molde.format(o_que=o_que) + FINAL_ALFA))


fx("P227", "Arma do Mike: cutelo do açougue", "fx_arma_mike", ARMA,
   "o CUTELO de açougueiro do pai do Mike (a família dele tem uma fazenda e um açougue): lâmina retangular larga de "
   "aço gasto com um furo no canto, cabo de madeira escura com rebites de latão.", ["item_taco", "fx_impacto"])
fx("P228", "Poder do Mike: pistola de abate", "fx_poder_mike", PODER,
   "o estampido da PISTOLA PNEUMÁTICA DE ABATE do açougue: um anel de choque de ar comprimido branco-acinzentado "
   "explodindo para fora, faíscas de aço e três estrelinhas amarelas de atordoamento girando no auge.",
   ["fx_impacto", "fx_fogo"])
fx("P229", "Arma do Ben: martelo de carpinteiro", "fx_arma_ben", ARMA,
   "o MARTELO DE CARPINTEIRO do Ben, o construtor da represa e do Clube: cabeça de ferro com unha de tirar pregos, "
   "cabo de madeira clara, e dois pregos girando junto.", ["item_taco", "fx_impacto"])
fx("P230", "Poder do Ben: desabamento da ponte", "fx_poder_ben", PODER,
   "TÁBUAS DE MADEIRA e vigas da ponte que o Ben constrói caindo do alto e se empilhando sobre o alvo, com pregos "
   "saltando e uma nuvem de pó de serragem no auge. Madeira castanha clara e escura, pregos prateados.",
   ["fx_impacto", "objetos_barrens"])
fx("P231", "Barreira do Ben: tijolo flutuante", "fx_barreira_ben", ARMA,
   "um TIJOLO vermelho-alaranjado gasto com uma tábua curta pregada nele, a peça da barreira que o Ben ergue em volta "
   "do Bill. Gira devagar flutuando, com um brilho de poeira dourada.", ["item_taco", "fx_impacto"])
fx("P232", "Arma da Beverly: bala de prata do estilingue", "fx_arma_beverly", ARMA,
   "a BALA DE PRATA do estilingue da Beverly (a melhor atiradora dos Perdedores): uma bolinha prateada brilhante, "
   "feita de um dólar de prata derretido, com um rastro curto de luz azulada.", ["item_estilingue", "fx_impacto"])
fx("P233", "Poder da Beverly: labareda", "fx_poder_beverly", PODER,
   "uma LABAREDA de fogo que sobe do chão em espiral, laranja e amarela com o miolo quase branco e faíscas, como a "
   "coragem da Beverly pegando fogo. Fogo estilizado em blocos de cor.", ["fx_fogo", "fx_impacto"])
fx("P234", "Arma do Stan: machadinha de escoteiro", "fx_arma_stan", ARMA,
   "a MACHADINHA DE ESCOTEIRO do Stan (organizado, escoteiro e observador de pássaros): lâmina de aço pequena e limpa, "
   "cabo de madeira com uma tira de couro verde enrolada.", ["item_taco", "fx_impacto"])
fx("P235", "Poder do Stan: revoada", "fx_poder_stan", PODER,
   "uma REVOADA de pássaros do livro de aves do Stan dando um rasante no alvo: seis a oito pássaros pequenos, "
   "azulados e pretos (gaios-azuis e pardais), asas abertas, penas soltas girando no auge.", ["fx_impacto", "fx_fuligem"])
fx("P236", "Arma do Eddie: estaca de cerca", "fx_arma_eddie", ARMA,
   "uma ESTACA DE CERCA de madeira com a ponta de ferro, arrancada da cerca da casa da Rua Neibolt, girando no ar.",
   ["item_taco", "fx_impacto"])
fx("P237", "Poder do Eddie: ácido do inalador", "fx_poder_eddie", PODER,
   "um jato de NÉVOA do inalador de asma do Eddie que vira ÁCIDO DE BATERIA (no livro ele acredita que é ácido e "
   "queima a Coisa): nuvem verde-amarelada borbulhando, com gotas que chiam e fumacinha cinza.", ["fx_nuvem", "fx_impacto"])
fx("P238", "Arma do Richie: fichas do fliperama", "fx_arma_richie", ARMA,
   "três FICHAS DE FLIPERAMA douradas e redondas (o Richie vive no fliperama de Derry), empilhadas e girando em "
   "leque, com um brilho metálico.", ["item_moeda", "fx_impacto"])
fx("P239", "Poder do Richie: as Vozes", "fx_poder_richie", PODER,
   "as VOZES do Richie, o imitador: ondas sonoras em anéis coloridos (amarelo, rosa e azul) saindo do centro, com "
   "estrelinhas e pequenas notas musicais em forma de desenho, empurrando tudo para fora. Divertido e barulhento.",
   ["fx_impacto", "fx_eco"])
ASSETS.append(("P240", "Rastro do taco (girado pelo jogo)", "fx_corte_taco", "192 × 48 px; 4 colunas × 1 linha; frame 48 × 48",
               ["fx_impacto", "item_taco"],
               ESTILO + "Crie o RASTRO DE GOLPE de um taco de beisebol em 4 frames numa única linha, cada frame 48 × 48, "
               "total 192 × 48. O golpe vai SEMPRE da esquerda para a DIREITA da imagem: uma meia-lua de luz "
               "branco-azulada com borda creme, mais grossa no meio e afinando nas pontas, aberta para a esquerda (o "
               "lado do Bill), como a lâmina de ar de um golpe rápido. Frame 1 o arco começando, fino; frame 2 o arco "
               "inteiro e mais forte, com faíscas brancas; frame 3 o arco se desfazendo em fiapos; frame 4 só restos de "
               "brilho. Centrado; o pivô é o centro do frame. Não desenhar o taco, mãos, personagens, cenário ou "
               "interface." + FINAL_ALFA))
# 24/11: o "Olho atento" do Stan deixa de ser um brilho âmbar (Diego não entendeu o que significava) e vira um
# passarinho pousado em cima da armadilha: quem avisou (o Stan, observador de pássaros) e onde.
ASSETS.append(("P252", "Olho atento do Stan: passarinho de aviso", "fx_passaro_stan",
               "128 × 32 px; 4 colunas × 1 linha; frame 32 × 32", ["fx_poder_stan", "fx_impacto"],
               ESTILO + "Crie um PASSARINHO pequeno em 4 frames numa única linha, cada frame 32 × 32, total 128 × 32: "
               "um gaio-azul do livro de aves do Stan (o escoteiro observador de pássaros dos Perdedores), que pousa "
               "em cima de uma armadilha para avisar o Bill. Azul vivo com peito claro, bico e olho bem legíveis, "
               "visto de lado com um pouco da frente, voltado para a direita. Frame 1 pousado, alerta, cabeça erguida "
               "e bico aberto como quem chama; frame 2 pousado, dando um pulinho com a cauda levantada; frame 3 "
               "abrindo as asas para levantar voo; frame 4 em voo, asas para cima. O pássaro ocupa cerca de 55% do "
               "frame; nos frames 1 e 2 os pés tocam a base do frame, centrados. Não desenhar galho, chão, armadilha, "
               "personagens, cenário ou interface." + FINAL_ALFA))

# ---------- menus (inventário, itens, Mural de talentos) ----------
UI = [
    ("P241", "Painel ornamentado (9 fatias)", "ui_painel", "96 × 96 px; imagem individual", ["ui_moldura", "item_kit"],
     "Crie uma MOLDURA DE PAINEL de menu, quadrada, para ser fatiada em 9 partes (cantos de 24 × 24 px no tamanho "
     "final de 96 × 96). Inspiração: os menus de Hades e Final Fantasy, no material do Clube dos Perdedores: borda de "
     "madeira escura envernizada com cantoneiras de LATÃO ENVELHECIDO trabalhadas e um prego em cada canto, um filete "
     "fino vermelho-escuro por dentro da borda. O MIOLO é um azul-noite quase preto, liso e uniforme (sem desenho), "
     "levemente translúcido. As quatro bordas retas são iguais e repetíveis; os quatro cantos são iguais entre si "
     "(espelhados). Sem balão, sem letras."),
    ("P242", "Placas de linha do menu", "ui_placas", "224 × 64 px; 1 coluna × 2 linhas; frame 224 × 32", ["ui_moldura"],
     "Crie DUAS PLACAS horizontais de linha de menu, uma em cima da outra, cada uma 224 × 32. Linha 1 (normal): uma "
     "faixa de madeira escura com acabamento fino de latão nas pontas, discreta e escura. Linha 2 (selecionada): a mesma "
     "faixa, mas com o latão aceso e dourado, um brilho quente de lanterna por trás e uma pequena seta de latão apontando "
     "para a direita na ponta esquerda. As duas têm o miolo liso, sem letras, onde o jogo escreve o texto."),
    ("P243", "Abas do menu", "ui_abas", "224 × 32 px; 2 colunas × 1 linha; frame 112 × 32", ["ui_moldura"],
     "Crie DUAS ABAS de menu lado a lado, cada uma 112 × 32, como as abas de um caderno de escola antigo presas a uma "
     "moldura de latão. Coluna 1 (aba fechada): papel pardo escuro e apagado, borda de latão escura. Coluna 2 (aba "
     "aberta): papel creme iluminado, borda de latão dourado e uma fita vermelha na ponta de cima. Miolo liso sem letras."),
    ("P244", "Ícones das abas", "ui_icones_abas", "160 × 32 px; 5 colunas × 1 linha; frame 32 × 32", ["item_kit", "item_taco"],
     "Crie CINCO ÍCONES de aba lado a lado, cada um 32 × 32, mesmo tamanho e mesmo traço: (1) uma mochila escolar de "
     "lona azul gasta (Itens); (2) um taco de beisebol cruzado com uma tampa de lixeira usada como escudo "
     "(Equipamento); (3) um mapa dobrado de Derry com um X vermelho (Missão); (4) um balão vermelho com um brilho "
     "fantasmagórico azul em volta (Ecos); (5) um caderno de capa de couro com um lápis (Ficha)."),
    ("P245", "Ícones dos atributos", "ui_icones_atributos", "128 × 32 px; 4 colunas × 1 linha; frame 32 × 32",
     ["item_kit", "item_coragem"],
     "Crie QUATRO ÍCONES de atributo lado a lado, cada um 32 × 32, num medalhão redondo de latão envelhecido: (1) Força: "
     "um punho fechado segurando um taco, fundo vermelho-escuro; (2) Foco: um olho atento com a íris dourada, fundo "
     "azul; (3) Agilidade: um tênis de cano alto com riscos de velocidade, fundo verde; (4) Coragem: um coração "
     "vermelho com uma chama por dentro, fundo âmbar."),
    ("P246", "Ícones dos talentos do Mural", "ui_talentos", "144 × 144 px; 3 colunas × 3 linhas; frame 48 × 48",
     ["item_pacto", "item_kit", "item_lembranca"],
     "Crie NOVE ÍCONES de talento em grade 3 × 3, cada um 48 × 48, como desenhos feitos à mão pelas crianças e colados "
     "no mural, na ordem: (1) Pacto de Sangue: duas mãos com um corte na palma se apertando, um brilho vermelho; (2) "
     "Coragem herdada: um coração com um curativo; (3) Lanterna reforçada: uma lanterna amarela acesa com raios; (4) "
     "Bolso fundo: um bolso de calça jeans com um kit de primeiros socorros saindo; (5) Treino no Barrens: um tênis "
     "enlameado sobre uma pedra do riacho; (6) Mão leve: uma mão pegando moedas de prata; (7) Sorte do Richie: os "
     "óculos de lente grossa do Richie com um trevo; (8) Mapa do Ben: um mapa de esgotos desenhado a lápis; (9) Fôlego "
     "extra: um inalador de asma com asas."),
    ("P247", "Polaroides do Mural", "ui_polaroid", "256 × 72 px; 4 colunas × 1 linha; frame 64 × 72", ["ui_moldura"],
     "Crie QUATRO MOLDURAS DE FOTO POLAROIDE lado a lado, cada uma 64 × 72, para os talentos presos num quadro de "
     "cortiça. A janela da foto (onde o jogo desenha o ícone) é um quadrado escuro liso de 48 × 48 no alto, com a faixa "
     "branca mais larga embaixo. (1) Bloqueada: papel amarelado e desbotado, presa com fita adesiva, um cadeado de "
     "ferrugem pequeno no canto. (2) Disponível: papel branco-creme, presa por uma tachinha de latão. (3) Comprada: "
     "papel creme com uma tachinha VERMELHA e um X de caneta vermelha marcando o canto. (4) Selecionada: como a "
     "disponível, mas inclinada levemente e com um brilho quente de lanterna em volta."),
    ("P248", "Fundo do inventário (a mochila)", "tela_fundo_mochila", "640 × 360 px; imagem individual", ["cartao_clube"],
     "Crie a ILUSTRAÇÃO DE FUNDO da tela de inventário, 16:9: vista de cima de um tampo de madeira escura no Clube dos "
     "Perdedores à noite, iluminado só por uma lanterna no canto superior esquerdo. Espalhados nas BORDAS: um mapa "
     "desbotado de Derry, SETE fotos polaroide, uma de cada criança, e só UMA delas é menina: Bill (menino, cabelo "
     "castanho liso), Richie (menino, cabelo preto e óculos de lente grossa), Eddie (menino magro, cabelo castanho "
     "curto), Stan (menino, cabelo loiro cacheado bem penteado), Ben (menino gordinho, cabelo castanho), Mike (menino "
     "negro, cabelo bem curto) e Beverly (a ÚNICA menina, cabelo ruivo CURTO); nenhum outro ruivo. Também um balão "
     "vermelho preso por um barbante, gibis velhos, um "
     "estilingue, pilhas e uma bússola. O CENTRO da imagem fica escuro, calmo e quase vazio (madeira em sombra), porque "
     "os painéis do menu ficam por cima. Clima de mistério acolhedor, não de susto." + FINAL_OPACO),
    ("P249", "Quadro de cortiça do Mural", "tela_mural_cortica", "640 × 360 px; imagem individual", ["cartao_clube"],
     "Crie a ILUSTRAÇÃO DE FUNDO do Mural dos Perdedores, 16:9: um grande QUADRO DE CORTIÇA visto de frente, preso numa "
     "parede de tábuas do Clube, com recortes de jornal de Derry desbotados nas bordas, um mapa dos esgotos, bilhetes "
     "e algumas tachinhas soltas. O centro da cortiça fica LIVRE (só cortiça), porque o jogo prende ali as polaroides "
     "dos talentos e os fios vermelhos entre elas. Luz quente de lanterna vinda de cima, cantos em sombra." + FINAL_OPACO),
    ("P250", "Bill de corpo inteiro (inventário)", "retrato_bill_corpo", "128 × 192 px; imagem individual",
     ["retrato_bill", "bill_referencia"],
     "Crie o BILL DENBROUGH DE CORPO INTEIRO, de frente, em pé, postura determinada, para o painel do personagem do "
     "inventário (como o retrato grande de Hades): menino de 12 anos, cabelo castanho, moletom azul com capuz sobre "
     "camiseta cinza-esverdeada, bermuda cáqui, tênis gastos; segura o taco de beisebol apoiado no ombro direito e uma "
     "lanterna na mão esquerda. Mesma identidade do retrato anexado. A figura ocupa a altura inteira da imagem, pés "
     "na base, centrada." + FINAL_ALFA.replace("Cada figura inteira deve ficar dentro de sua célula", "A figura inteira deve ficar dentro da imagem")),
    ("P251", "Selo de raridade e moldura do item grande", "ui_moldura_item", "192 × 64 px; 3 colunas × 1 linha; frame 64 × 64",
     ["ui_moldura", "item_kit"],
     "Crie TRÊS MOLDURAS QUADRADAS de destaque de item, lado a lado, cada uma 64 × 64, como o quadro que mostra o "
     "item escolhido em grande nos menus de Resident Evil: miolo escuro liso e vazio (o jogo desenha o item ali), "
     "borda de latão trabalhada com cantoneiras. (1) Comum: latão escuro e cinza. (2) Raro: latão com pedras azuis nos "
     "cantos e brilho azul suave. (3) Épico: latão dourado com pedras vermelhas e um brilho vermelho-alaranjado."),
]
for pid, titulo, arq, dims, refs, texto in UI:
    final = "" if texto.endswith(FINAL_OPACO) or texto.endswith("imagem.") or "canal alfa" in texto else FINAL_ALFA
    ASSETS.append((pid, titulo, arq, dims, refs, ESTILO + texto + final))


def main():
    linhas = [
        "# Prompts dos assets v9: armas e poderes dos amigos, corte do taco e menus repaginados (P227–P252)", "",
        "Gerado por `tools/prompts_v9_amigos_ui.py` (24/11/2026). Pedidos do Diego: arma e poder de cada amigo ligados "
        "à lore; golpes na diagonal; inventário, itens e Mural de talentos com visual profissional (Resident Evil, "
        "Final Fantasy, Hades). Gerar com `python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v9-amigos-ui.md`.",
        "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |",
    ]
    for pid, titulo, arq, dims, _, _ in ASSETS:
        linhas.append(f"| {pid} | {titulo} | {arq}.png | {dims} |")
    linhas.append("")
    for pid, titulo, arq, dims, refs, texto in ASSETS:
        linhas += [
            f"## {pid} {titulo}", "",
            f"**Arquivo:** {arq}.png", "",
            f"**Dimensões finais:** {dims}", "",
            f"**Referências para anexar:** {', '.join(r + '.png' for r in refs)}", "",
            "**Prompt completo para copiar:**", "",
            "```text", f"Entregue uma única imagem PNG. Dimensões finais desejadas: {dims}. " + texto, "```", "",
        ]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(ASSETS)} prompts")


if __name__ == "__main__":
    main()
