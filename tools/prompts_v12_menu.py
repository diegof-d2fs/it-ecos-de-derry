"""Gera instructions/Prompts-Assets-IT-v12-menu.md (P277): a ilustração inédita do menu inicial.

Pedido do Diego (24/11/2026): "quero uma imagem inédita e em altíssima resolução" para o fundo do menu inicial
repaginado. O menu fica à ESQUERDA: a cena tem o foco à direita e o terço esquerdo escuro e calmo.
Nome cena_*: entra no jogo em 4× (2560 × 1440), carregada sob demanda e reduzida para a tela pelo pixel mais
próximo (como as cut scenes), sem borrar o pixel art.

Geração: python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v12-menu.md cena_menu_inicial
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v12-menu.md"

PROMPT = (
    "Ilustração em pixel art de altíssimo detalhe, estética de 16 bits pintada à mão (como as cenas anexadas do mesmo "
    "jogo), formato paisagem 16:9, opaca, ocupando o quadro inteiro, sem bordas pretas. É a TELA DE TÍTULO do jogo IT "
    "Ecos de Derry (Derry, Maine, 1989, terror estilizado com crianças protagonistas). A cena: a Rua Witcham numa noite "
    "de chuva forte. No TERÇO DIREITO, o foco: a boca de lobo (bueiro) no meio-fio, escura como um poço, de onde saem "
    "dois olhos amarelos brilhando e uma mão de luva branca segurando a linha de um BALÃO VERMELHO vivo, que flutua "
    "acima do bueiro (o único vermelho forte da imagem, com um brilho suave). Junto ao bueiro, parado na enxurrada, "
    "um barquinho de papel encharcado. As poças refletem o balão e um poste de luz amarelada. No meio, a rua molhada "
    "brilhando, a enxurrada correndo para o bueiro. Ao fundo, casas de madeira da Nova Inglaterra com janelas acesas "
    "e a silhueta da caixa d'água de Derry contra um céu de tempestade azul-escuro. O TERÇO ESQUERDO é mais escuro, "
    "calmo e com pouco detalhe (céu, casas em sombra, neblina e chuva), porque ali fica o texto do menu: nada de "
    "personagem, rosto ou objeto importante do lado esquerdo. Atmosfera de medo e mistério, luz de cinema, paleta de "
    "azuis e verdes frios com o amarelo dos postes e o vermelho do balão. Sem letras, números, logotipos, legendas, "
    "interface ou marca d'água."
)

ASSETS = [("P277", "Tela de título (fundo do menu inicial)", "cena_menu_inicial",
           "640 × 360 px; imagem individual", ["cena_abertura_1", "cena_abertura_3"], PROMPT)]


def main():
    linhas = ["# Prompts dos assets v12: a ilustração do menu inicial (P277)", "",
              "Gerado por `tools/prompts_v12_menu.py` (24/11/2026). Pedido do Diego: imagem inédita e em altíssima resolução.",
              "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |"]
    for p, titulo, arq, dims, _, _ in ASSETS:
        linhas.append(f"| {p} | {titulo} | {arq}.png | {dims} |")
    linhas.append("")
    for p, titulo, arq, dims, refs, texto in ASSETS:
        linhas += [f"## {p} {titulo}", "", f"**Arquivo:** {arq}.png", "", f"**Dimensões finais:** {dims}", "",
                   f"**Referências para anexar:** {', '.join(r + '.png' for r in refs)}", "", "**Prompt completo para copiar:**", "",
                   "```text", "Entregue uma única imagem PNG, na MAIOR resolução possível. " + texto, "```", ""]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(ASSETS)} prompts")


if __name__ == "__main__":
    main()
