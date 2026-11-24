"""Gera instructions/Prompts-Assets-IT-v10-pennywise.md (P253–P265): Pennywise II e III refeitos e o IV aranha.

Pedido do Diego (24/11/2026, olhando as folhas uma a uma):
  * I fica como está.
  * II (Rua Neibolt) e III (Siderúrgica) "estão muito parecidos com o do bioma 1, tem que melhorar".
  * IV: "se é o chefe final quero algo grandioso, corpo de Pennywise e bunda com pernas de aranha, algo grande que
    se move 360 graus e tem poderes fodas".

Como o IV fica grande e gira em 360 graus: o corpo de aranha (abdome e 8 patas) é uma folha VISTA DE CIMA apontando
para a DIREITA, que o jogo gira para o rumo exato em que ele anda; o TRONCO do Pennywise (da cintura para cima) é
outra folha, sempre em pé, encaixada na frente do corpo e virada para o Bill. Os poderes novos têm efeitos próprios
(teia, patas cravando no chão, as Luzes Mortas).

Geração: python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v10-pennywise.md [arquivos...]
Uso: python tools/prompts_v10_pennywise.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v10-pennywise.md"

ESTILO = (
    "Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera "
    "fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização "
    "fotográfica. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). "
    "Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o "
    "movimento previsto. A referência do Pennywise define só a IDENTIDADE do rosto (rosto branco, olhos amarelos, "
    "nariz e marcas vermelhas, cabelo ruivo); a SILHUETA, a ROUPA, as CORES e a postura deste estágio são as descritas "
    "aqui e precisam ser CLARAMENTE DIFERENTES do palhaço cinza comum de babado creme (o estágio I): quem olhar as "
    "duas folhas lado a lado tem de ver outro monstro. "
)
FINAL = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, "
    "números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes "
    "cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito."
)
FINAL_FX = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, "
    "números, marcas d’água ou linhas de grade. Cada quadro inteiro dentro da sua célula, sem partes cortadas. "
    "Entregar somente o asset descrito."
)
ACOES = (
    "Crie um atlas com 4 colunas e 3 linhas, cada frame 32 × 48, total 128 × 144. Linha 1: quatro frames de espera "
    "com respiração, com o tique próprio deste estágio. Linha 2: quatro frames de preparação do ataque. Linha 3: quatro "
    "frames de ataque com a mão ou garra apontada à frente, terminando em recuperação. Ele permanece de frente, "
    "voltado levemente para baixo. Não gerar projéteis junto ao corpo. Mesmo alinhamento dos pés em todas as poses."
)
ANDAR = (
    "Crie 12 frames de caminhada, atlas com 4 colunas e 3 linhas. IMPORTANTE: exatamente 3 linhas e 4 colunas (12 "
    "frames). Linha 1: andando para BAIXO, de frente para a câmera. Linha 2: andando para a DIREITA, perfil completo "
    "com o nariz, o peito e os pés virados para a DIREITA da imagem em TODOS os quatro frames da linha. Linha 3: "
    "andando para CIMA, de costas, vemos a nuca e as costas da roupa. Colunas: repouso, passo A, repouso alternado, "
    "passo B. As pernas e os braços se movem de verdade, com o jeito de andar próprio deste estágio. A roupa, as cores "
    "e a silhueta são IDÊNTICAS às da folha de ações anexada, nas três vistas. Pés alinhados na mesma base; não "
    "simular deslocamento da figura dentro do frame."
)
DISPERSAR = (
    "Crie quatro frames de dispersão numa única linha, cada frame 32 × 48, total 128 × 48, da esquerda para a direita: "
    "corpo intacto surpreendido; contorno se desfazendo {como}; o corpo quase todo desfeito em {em_que} e um balão "
    "vermelho escapando; os últimos fragmentos desaparecendo (o último frame pode estar quase transparente). Não é morte "
    "definitiva: é um recuo sobrenatural para voltar mais forte. A figura é IDÊNTICA à da folha de ações anexada. Sem "
    "cenário e sem ferimentos."
)

ESTAGIOS = {
    "2": ("II (Rua Neibolt, sai do Leproso)",
          "Terceira manifestação, a da casa da Rua Neibolt: um palhaço APODRECIDO e esguio, bem mais alto e magro que o "
          "estágio I, com os braços compridos demais (as mãos passam dos joelhos) e a cabeça inclinada de lado. O traje "
          "é VERDE-MUSGO e amarelo-doentio, manchado de mofo e de umidade, com remendos de saco de estopa e rasgos nas "
          "mangas; o babado é cinza-esverdeado, murcho e caído; pompons verde-escuros; luvas sujas de terra com as "
          "pontas dos dedos rasgadas. A pele do rosto é rachada como gesso velho, com manchas verdes nas bochechas, e "
          "a boca se abre demais mostrando fileiras de dentes finos. Uma aura de mosquitos e de esporos verdes em volta. "
          "Postura corcunda e torta, os ombros em alturas diferentes.",
          ["pennywise_referencia", "medo_leproso"],
          "em pó de mofo verde, esporos e folhas secas", "em esporos e mosquitos"),
    "3": ("III (Siderúrgica, sai do Pássaro das Cinzas)",
          "Quinta manifestação, a da Siderúrgica Kitchener (que explodiu e queimou): um palhaço CARBONIZADO e "
          "imponente, ombros largos e peito estufado, o mais alto até aqui. O traje é PRETO-CARVÃO e cinza de fuligem, "
          "com as costuras e as rachaduras brilhando em BRASA laranja como metal derretido; o babado do pescoço virou "
          "um colar de PENAS NEGRAS chamuscadas, e penas de cinza caem das mangas como uma capa curta de asas. O cabelo "
          "ruivo arde em CHAMAS de verdade (laranja e amarelo), os olhos são brasas acesas, pompons como carvões em "
          "brasa, luvas pretas de fuligem com as pontas incandescentes. Faíscas e cinzas sobem dele o tempo todo. "
          "Postura ereta e soberba, queixo erguido.",
          ["pennywise_referencia", "medo_passaro"],
          "em cinzas, brasas e penas negras queimando", "em cinzas e brasas"),
}

ASSETS = []
pid = 253
for chave, (nome, roupa, refs, como, em_que) in ESTAGIOS.items():
    ASSETS.append((f"P{pid}", f"Pennywise {nome}: ações", f"pennywise_{chave}_actions",
                   "128 × 144 px; 4 colunas × 3 linhas; frame 32 × 48", refs, ESTILO + roupa + " " + ACOES + FINAL))
    ASSETS.append((f"P{pid + 1}", f"Pennywise {nome}: andando", f"pennywise_{chave}_walk",
                   "128 × 192 px; 4 colunas × 4 linhas; frame 32 × 48", [f"pennywise_{chave}_actions", "pennywise_referencia"],
                   ESTILO + roupa + " " + ANDAR + FINAL))
    ASSETS.append((f"P{pid + 2}", f"Pennywise {nome}: se dispersando", f"pennywise_{chave}_disperse",
                   "128 × 48 px; 4 colunas × 1 linha; frame 32 × 48", [f"pennywise_{chave}_actions", "pennywise_referencia"],
                   ESTILO + roupa + " " + DISPERSAR.format(como=como, em_que=em_que) + FINAL))
    pid += 3

# ---------- O Pennywise IV: o palhaço-aranha do Covil ----------
TRONCO_IV = (
    "O Pennywise IV, a forma verdadeira do Covil: só o TRONCO, da cintura para cima, de um Pennywise enorme e "
    "majestoso, o mais assustador de todos. Traje de seda branco-perolado e prateado com detalhes dourados e "
    "vermelho-sangue, o maior babado de todos (em camadas, como uma coroa em volta do pescoço), pompons vermelhos "
    "vivos, cabelo ruivo armado como chifres, sorriso enorme cheio de dentes afiados, olhos com o brilho alaranjado das "
    "Luzes Mortas. Na altura da cintura o traje termina num saiote de babados de seda que se abre em leque: é ali que "
    "ele se encaixa no corpo de aranha (o corpo de aranha NÃO aparece nesta folha). Braços longos com luvas brancas e "
    "dedos compridos como garras."
)
ASSETS.append(("P259", "Pennywise IV: tronco (ações)", "pennywise_4_actions",
               "256 × 192 px; 4 colunas × 3 linhas; frame 64 × 64", ["pennywise_referencia", "pennywise_1_actions"],
               ESTILO + TRONCO_IV + " Crie um atlas com 4 colunas e 3 linhas, cada frame 64 × 64, total 256 × 192. O tronco "
               "ocupa a parte de cima do frame, com a barra do saiote de babados encostada na BASE do frame em todos os "
               "quadros (é o ponto de encaixe). Linha 1: quatro frames de espera, respirando, os ombros e o babado "
               "subindo e descendo, a cabeça se inclinando de leve. Linha 2: quatro frames de preparação: os braços se "
               "abrem e se erguem, as mãos em garra, a boca se abre cada vez mais. Linha 3: quatro frames de ataque: os "
               "braços avançam para a frente, a boca escancarada mostrando as fileiras de dentes e o brilho das Luzes "
               "Mortas no fundo da garganta, e voltam à recuperação. Sempre de frente para a câmera." + FINAL))
ASSETS.append(("P260", "Pennywise IV: corpo de aranha (girado pelo jogo)", "pennywise_4_aranha",
               "512 × 256 px; 4 colunas × 2 linhas; frame 128 × 128", ["pennywise_4_actions", "pennywise_referencia"],
               ESTILO.replace("vista superior com um pouco da frente dos personagens visível", "vista de cima")
               + "O CORPO DE ARANHA GIGANTE do Pennywise IV (a forma verdadeira da Coisa no Covil), visto DE CIMA, com o "
               "corpo apontando para a DIREITA da imagem em TODOS os quadros (o jogo gira a imagem para qualquer "
               "direção). À esquerda, um ABDOME enorme e bulboso, listrado como a roupa de palhaço (branco-perolado e "
               "prata com faixas vermelho-sangue e pompons vermelhos em fila no dorso), coberto de pelos curtos e de "
               "pequenas luzes alaranjadas; à direita, a parte da frente do corpo termina num grande babado de seda em "
               "leque, branco e vermelho, VAZIO no meio (é onde o jogo encaixa o tronco do palhaço, que NÃO aparece "
               "nesta folha). Oito patas longas, peludas e articuladas, quatro de cada lado, pretas com pontas de osso "
               "e listras vermelhas, abertas em volta do corpo como as de uma aranha caranguejeira. Atlas com 4 colunas "
               "e 2 linhas, cada frame 128 × 128, total 512 × 256, o corpo centrado em cada frame (o centro do frame é "
               "o centro do corpo) e ocupando cerca de 80% do frame com as patas. Linha 1: quatro frames de caminhada, "
               "as patas se alternando como numa aranha de verdade (as patas 1 e 3 de um lado com a 2 e a 4 do outro). "
               "Linha 2: frame 1 agachado, as patas recolhidas junto ao corpo, preparando o bote; frame 2 no salto, as "
               "patas esticadas para trás; frame 3 aterrissando, as patas abertas ao máximo e cravadas; frame 4 "
               "atingido, as patas se encolhendo de dor." + FINAL_FX))
# 24/11, Diego: "cuidar para que o Pennywise esteja virado para onde a parte aranha está" e "todos têm que ter
# movimentação 360". O tronco em 8 direções: o Codex desenha 5 (baixo, baixo-direita, direita, cima-direita, cima) e
# o jogo espelha as 3 da direita para fazer as da esquerda. Em cada direção: respirar (2), preparar e atacar.
ASSETS.append(("P265", "Pennywise IV: tronco em 8 direções", "pennywise_4_tronco",
               "256 × 320 px; 4 colunas × 5 linhas; frame 64 × 64", ["pennywise_4_actions", "pennywise_4_aranha"],
               ESTILO + TRONCO_IV + " O MESMO tronco da folha de ações anexada (mesma roupa, cores, babado, cabelo e "
               "tamanho), agora visto em CINCO direções, para girar junto com o corpo de aranha anexado. Atlas com 4 "
               "colunas e 5 linhas, cada frame 64 × 64, total 256 × 320. IMPORTANTE: exatamente 5 linhas e 4 colunas. "
               "Linha 1: virado para BAIXO, de frente para a câmera. Linha 2: virado para BAIXO E DIREITA (três quartos "
               "de frente, o rosto e o peito voltados para o canto de baixo à direita). Linha 3: virado para a DIREITA, "
               "de PERFIL completo, o nariz, o peito e as mãos apontando para a DIREITA da imagem. Linha 4: virado para "
               "CIMA E DIREITA (três quartos de costas: vemos a nuca, as costas e um pouco do perfil direito). Linha 5: "
               "virado para CIMA, de COSTAS: a nuca com o cabelo ruivo, as costas do traje e o babado por trás. Colunas "
               "em todas as linhas: (1) e (2) respirando, os ombros e o babado subindo e descendo; (3) preparando o "
               "golpe, os braços erguidos em garra; (4) atacando, os braços esticados PARA A DIREÇÃO DA LINHA. A barra "
               "do saiote de babados encosta na BASE do frame em todos os quadros (é o ponto de encaixe no corpo de "
               "aranha) e o tronco fica centrado na mesma posição em todas as direções." + FINAL))
ASSETS.append(("P261", "Pennywise IV: derrota final", "pennywise_4_defeat",
               "512 × 128 px; 4 colunas × 1 linha; frame 128 × 128", ["pennywise_4_actions", "pennywise_4_aranha"],
               ESTILO + TRONCO_IV + " Agora o palhaço-aranha INTEIRO, como aparece no jogo: o tronco de pé saindo da "
               "frente do corpo de aranha anexado, o abdome listrado atrás e as oito patas abertas em volta, visto de "
               "cima com um pouco da frente. Crie quatro frames numa única linha, cada frame 128 × 128, total 512 × 128, "
               "a DERROTA FINAL no Covil: (1) o palhaço-aranha gritando, a cabeça para trás e os braços abertos; (2) as "
               "patas se dobrando e se encolhendo sobre o corpo, como uma aranha morrendo, o traje rachando com luz "
               "alaranjada vazando pelas rachaduras; (3) o corpo desmoronando em cinzas, fios de teia e luzes "
               "alaranjadas, um balão vermelho escapando; (4) só restam cinzas no chão, as últimas Luzes Mortas se "
               "apagando e o balão vermelho murcho. A base da figura encosta na base do frame." + FINAL))

FX = [
    ("P262", "Pennywise IV: teia", "fx_teia",
     "a TEIA que o palhaço-aranha cospe e que gruda o Bill no chão: frame 1 uma bola de teia branca voando e se abrindo; "
     "frame 2 a teia se espalhando no chão como uma rede redonda de fios grossos, brancos e brilhantes, com gotas "
     "pegajosas; frame 3 a rede inteira, esticada e firme (o maior e mais forte); frame 4 os fios se soltando e se "
     "desfazendo. Vista de cima."),
    ("P263", "Pennywise IV: patas cravando no chão", "fx_patas",
     "as PONTAS DE PATA DE ARANHA do Pennywise IV cravando no chão: frame 1 três pontas pretas e afiadas, com listras "
     "vermelhas, descendo; frame 2 as pontas cravadas no chão, rachando o piso, com poeira e lascas de pedra subindo; "
     "frame 3 as pontas se erguendo e deixando buracos rachados; frame 4 só a poeira assentando. Vista de cima com um "
     "pouco da frente."),
    ("P264", "Pennywise IV: Luzes Mortas", "fx_luzes_mortas",
     "as LUZES MORTAS (deadlights) do Pennywise: frame 1 três pequenas luzes alaranjadas e amarelas surgindo, "
     "hipnóticas; frame 2 elas se juntam num clarão giratório de luz alaranjada com um miolo branco, ondulando como "
     "fogo frio, com faíscas (o maior e mais forte); frame 3 o clarão se partindo em fiapos de luz; frame 4 as últimas "
     "faíscas se apagando. Bonito e assustador ao mesmo tempo."),
]
for fid, titulo, arq, o_que in FX:
    ASSETS.append((fid, titulo, arq, "192 × 48 px; 4 colunas × 1 linha; frame 48 × 48", ["fx_penny_pulso", "fx_impacto"],
                   ESTILO + "Crie um EFEITO DE ATAQUE em 4 frames numa única linha, cada frame 48 × 48, total 192 × 48: "
                   + o_que + " O efeito ocupa cerca de 85% do frame no auge, centrado; o pivô é o centro do frame. Terror "
                   "estilizado, sem sangue explícito nem gore. Não desenhar personagens, cenário ou interface." + FINAL_FX))


def main():
    linhas = [
        "# Prompts dos assets v10: Pennywise II e III refeitos e o IV palhaço-aranha (P253–P265)", "",
        "Gerado por `tools/prompts_v10_pennywise.py` (24/11/2026). Pedido do Diego: II e III muito parecidos com o I; "
        "o IV grandioso, com corpo de Pennywise sobre um corpo de aranha, que se move em 360 graus e tem poderes novos. "
        "Gerar com `python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v10-pennywise.md`.",
        "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |",
    ]
    for p, titulo, arq, dims, _, _ in ASSETS:
        linhas.append(f"| {p} | {titulo} | {arq}.png | {dims} |")
    linhas.append("")
    for p, titulo, arq, dims, refs, texto in ASSETS:
        linhas += [
            f"## {p} {titulo}", "",
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
