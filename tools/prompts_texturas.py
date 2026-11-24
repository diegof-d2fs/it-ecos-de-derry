"""Gera instructions/Prompts-Assets-IT-v3-texturas.md (P106–P150): texturas CONTÍNUAS por ambiente.

Uso:  python tools/prompts_texturas.py

Por quê (10/11/2026): pedir um atlas 4×4 de tiles diferentes faz o modelo desenhar 16
"quadrinhos" com moldura própria, que não emendam. Agora cada material é UMA textura
contínua; o export_final.py força a emenda (seamless) e o motor corta em 4×4, desenhando a
célula (c, l) com o pedaço (c mod 4, l mod 4). Transições (sombra da parede, borda da água)
são desenhadas pelo código.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "instructions" / "Prompts-Assets-IT-v3-texturas.md"

ESTILO = ("Pixel art de RPG com estética de 16 bits, acabamento detalhado, blocos de cor definidos, sombras em poucos "
          "tons, sem suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem; "
          "vermelho vivo só em perigo. Jogo IT Ecos de Derry, terror estilizado. ")
CONTINUA = ("Esta imagem é UMA ÚNICA TEXTURA CONTÍNUA (seamless, tileable) que preenche todo o quadro, vista "
            "diretamente de cima. Não é uma folha de tiles: NÃO divida em quadrados, NÃO desenhe moldura, borda, "
            "bisel, rejunte em grade regular, vinheta ou sombra nas bordas. A borda esquerda continua exatamente na "
            "direita e a de cima continua na de baixo, para repetir sem emenda visível. Iluminação plana e uniforme em "
            "toda a área, sem fog of war, sem foco de luz, sem objetos grandes isolados, sem personagens, sem texto. "
            "A escala dos detalhes deve combinar com personagens de 32 × 48 pixels: cada pedra ou tábua ocupa de "
            "1/8 a 1/3 da largura da imagem. Distribua manchas, musgo, rachaduras e variações de cor de forma "
            "IRREGULAR e orgânica pelo quadro inteiro: nada de padrão xadrez, blocos repetidos ou manchas grandes "
            "isoladas que denunciem a repetição quando a imagem for ladrilhada. SEM REPETIÇÃO INTERNA: a imagem "
            "inteira é um único trecho do material, sem copiar o mesmo motivo em grade, em fileiras ou em 2 × 2 "
            "dentro dela. Use VERMELHO apenas se o material pedir explicitamente: no jogo, vermelho vivo significa perigo. ")

MATERIAIS = [
    ("piso", "Piso principal"),
    ("piso2", "Piso secundário"),
    ("parede_face", "Face frontal da parede"),
    ("parede_topo", "Topo da parede"),
    ("liquido", "Líquido ou terreno especial"),
]

# Ambiente → descrição de cada material (mesma ordem de MATERIAIS).
AMBIENTES = {
    "galeria": ("Galeria pluvial de esgoto antigo", [
        "piso de lajes de pedra cinza-esverdeada irregulares e gastas, juntas escuras finas, umidade, pequenas manchas de musgo",
        "lajes de pedra cinza-esverdeada parcialmente cobertas por musgo e lodo verde-escuro que se espalham em manchas orgânicas irregulares, sem formar quadrados",
        "parede de tijolos e blocos de pedra cinza-esverdeada vista de frente, fileiras horizontais, manchas de umidade escorrendo, um pouco de ferrugem",
        "topo de muro maciço visto de cima: pedras escuras compactas quase pretas com leve tom azul-esverdeado, superfície rugosa",
        "superfície de água de esgoto azul-escura esverdeada vista de cima, só a superfície: ondulações finas e reflexos discretos espalhados por igual, sem fundo visível, sem pedras, tijolos ou objetos submersos",
    ]),
    "poroes": ("Porões inundados de casa antiga", [
        "piso liso de cimento velho cinza-bege quase uniforme, textura granulada fina, poucas rachaduras finas que atravessam o quadro em direções diferentes e uma ou duas manchas de umidade suaves e grandes, sem faixas",
        "piso de ladrilhos hexagonais antigos de banheiro, brancos e verde-água, alguns quebrados e sujos",
        "parede de blocos de concreto e reboco descascado bege-acinzentado vista de frente, com canos finos de ferro enferrujado na horizontal",
        "topo de muro de concreto visto de cima, cinza-escuro quase preto, áspero",
        "superfície de água turva de porão azul-acinzentada vista de cima, ondulações finas e sujeira miúda espalhadas por igual, sem objetos grandes",
    ]),
    "cisterna": ("Cisterna subterrânea das vozes", [
        "piso de grandes blocos de pedra azul-acinzentada escura, molhados, juntas profundas",
        "piso de pedra azul com grades e tampas metálicas de drenagem incrustadas e ferrugem",
        "parede de pedra azul-violeta escura em blocos grandes vista de frente, com veios de umidade e tubos antigos embutidos",
        "topo de muro de pedra quase preta azul-violeta visto de cima",
        "água escura e profunda azul-violeta, quase preta, com brilhos frios discretos",
    ]),
    "rua_derry": ("Rua residencial de Derry à noite após a chuva", [
        "asfalto escuro molhado com pedrinhas, pequenas rachaduras e reflexos de poças",
        "calçada de placas de concreto cinza com folhas caídas e juntas retas",
        "muro baixo de tijolos vermelhos escuros visto de frente, com musgo nas juntas",
        "coroamento de muro de concreto cinza-escuro visto de cima, liso e gasto, sem tijolos aparentes, poucas manchas de musgo discretas e irregulares",
        "poça rasa sobre asfalto escuro vista de cima: superfície escura com reflexos alongados e irregulares de luz fria, sem círculos nem manchas repetidas",
    ]),
    "clube": ("Clube secreto subterrâneo das crianças", [
        "assoalho de tábuas de madeira marrom quente, gastas, pregos visíveis, tábuas em direção horizontal",
        "tapete de retalhos velho e desbotado em vermelho, mostarda e azul",
        "parede de terra escorada com tábuas verticais e vigas, vista de frente, raízes aparecendo",
        "topo de parede de terra escura com raízes vista de cima",
        "terra batida marrom-escura úmida com pedrinhas",
    ]),
    "barrens": ("Os Barrens, mata pantanosa à beira do riacho", [
        "chão de terra úmida marrom-esverdeada com tufos de grama baixa em manchas grandes e suaves, poucas folhas secas; formas largas e calmas, pouco ruído miúdo, para personagens se destacarem por cima",
        "grama alta e mato denso verde-escuro com flores silvestres miúdas",
        "barranco vertical de terra marrom compacta visto de frente, com estratos horizontais e pedras e raízes espalhadas irregularmente",
        "topo de mata fechada vista de cima: copas de arbustos densos verde-escuros quase pretos",
        "água rasa do riacho verde-acastanhada com pedras no fundo e correnteza suave",
    ]),
    "neibolt": ("Interior da casa abandonada da Rua Neibolt", [
        "assoalho de tábuas escuras compridas na horizontal, de comprimentos variados, empenadas, com veios, nós e poeira irregular",
        "tábuas escuras quebradas e soltas com poucos buracos irregulares mostrando terra do porão, formas grandes e legíveis, pouco detalhe miúdo",
        "parede com papel de parede descascado verde e sépia de padrão floral antigo, rodapé de madeira escura, vista de frente",
        "topo de parede de madeira apodrecida vista de cima, escuro",
        "terra batida de porão marrom-acinzentada, úmida",
    ]),
    "siderurgica": ("Ruínas da Siderúrgica Kitchener", [
        "piso de concreto industrial cinza-escuro com fuligem preta difusa e algumas manchas de óleo de formas variadas, sem verde e sem manchas alinhadas em diagonal; superfície calma",
        "chapas de ferro rebitadas enferrujadas com textura antiderrapante",
        "parede de tijolos enegrecidos pela fumaça com vigas de aço rebitadas, vista de frente",
        "topo de muro de tijolos queimados visto de cima, preto com brasas mínimas nas frestas",
        "leito de carvão em brasa visto de cima: pedaços de carvão preto de tamanhos variados espalhados irregularmente, frestas laranja finas e esparsas",
    ]),
    "covil": ("Covil subterrâneo da Coisa", [
        "chão de pedra escura, úmida e irregular em tons de cinza-azulado e verde apagado, com raros objetos pequenos desbotados (sem vermelho); superfície calma, pouco ruído",
        "entulho contínuo e denso de objetos perdidos vistos de cima, como um cascalho grande de brinquedos, sapatos e peças desbotadas, misturados ao acaso em toda a área, sem caminhos, linhas ou grade",
        "parede de rocha escura com camadas de objetos perdidos incrustados, vista de frente",
        "topo de rocha quase preta vista de cima",
        "superfície de água escura parada vista de cima, reflexos frios esparsos e alguns fios finos vermelhos boiando espalhados por igual",
    ]),
}


def render():
    entradas = []
    n = 106
    for amb, (titulo_amb, descricoes) in AMBIENTES.items():
        for (mat, titulo_mat), desc in zip(MATERIAIS, descricoes):
            entradas.append((f"P{n}", f"{titulo_mat} — {titulo_amb}", f"tex_{amb}_{mat}.png", desc))
            n += 1
    dims = "128 × 128 px; 4 colunas × 4 linhas; tile 32 × 32"
    out = ["# Prompts de texturas contínuas — v3 (P106–P150)", "",
           "Substituem os tilesets em atlas (P24–P26, P73–P78) para pisos, paredes e líquidos. "
           "Gerado por `tools/prompts_texturas.py`; edite o script, não este arquivo.", "",
           "## Catálogo", "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |"]
    out += [f"| {pid} | {tit} | {arq} | {dims} |" for pid, tit, arq, _ in entradas]
    out.append("")
    for pid, tit, arq, desc in entradas:
        prompt = (f"Entregue uma única imagem PNG quadrada. Dimensões finais desejadas: {dims}. " + ESTILO + CONTINUA
                  + f"Material: {desc}.")
        out += [f"## {pid} {tit}", "", f"**Arquivo:** {arq}", "", f"**Dimensões finais:** {dims}", "",
                "**Referências para anexar:** referencia_visual.png", "", "**Prompt completo para copiar:**", "",
                "```text", prompt, "```", ""]
    OUT.write_text("\n".join(out), encoding="utf-8")
    print(f"{len(entradas)} prompts → {OUT}")


if __name__ == "__main__":
    render()
