"""Gera instructions/Prompts-Assets-IT-v5-fx.md (P167–P173): efeitos de ATAQUE dos inimigos.

Pedido do Diego (17/11/2026): os ataques dos inimigos apareciam só como áreas vermelhas tracejadas
("parece coisa de desenvolvimento"). O aviso continua (todo ataque é anunciado), mas quando o golpe
sai, o ataque de verdade aparece: garra, fuligem, água, cabelo, bandagem, estouro de balão, sangue.
Cada efeito é uma tira de 4 quadros 32 × 32 com pivô no centro, desenhada ao longo da área do golpe.

Uso: python tools/prompts_v5_fx.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v5-fx.md"

BASE = (
    "Entregue uma única imagem PNG. Dimensões finais desejadas: 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32. "
    "Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente visível, câmera fixa e não "
    "isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. "
    "Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e "
    "perigo. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas. Crie uma sequência de efeito "
    "visual de ATAQUE: {efeito} São 4 frames em uma única linha, cada frame 32 × 32, total 128 × 32: frame 1 o golpe "
    "começando, frame 2 no auge (o maior e mais forte), frame 3 se desfazendo, frame 4 os últimos restos. O efeito "
    "ocupa cerca de 80% do frame no auge, centrado; o pivô é o centro do frame. Terror estilizado, sem sangue "
    "explícito nem gore. Não desenhar personagens, cenário, interface, moldura ou aviso de área. Fundo realmente "
    "transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, números, marcas "
    "d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas. Entregar "
    "somente o asset descrito."
)

FX = [
    ("P167", "Garras (rato, sombra, lobisomem)", "fx_garra",
     "três riscos de garra em diagonal, escuros com borda vermelha viva e faíscas brancas, como um arranhão no ar."),
    ("P168", "Rajada de fuligem (Espectro)", "fx_fuligem",
     "nuvem de fuligem negra e cinza que explode para fora, com brasas laranja e fagulhas no meio."),
    ("P169", "Pancada d'água (Afogado, sanguessuga, ondas)", "fx_agua",
     "respingo de água esverdeada e escura que sobe e cai em gotas, com espuma clara na borda."),
    ("P170", "Chicote de cabelo (Cabelo do ralo, Ralo de Sangue)", "fx_cabelo",
     "mechas de cabelo negro e molhado chicoteando em S, com respingos vermelho-escuros nas pontas."),
    ("P171", "Bandagens (a Múmia)", "fx_bandagem",
     "faixas de linho velho e amarelado estalando como chicote, se desenrolando em espiral, com poeira."),
    ("P172", "Estouro de balão (balão, balões vermelhos do Pennywise)", "fx_estouro",
     "um balão vermelho estourando: pedaços de borracha vermelha voando, confete claro e uma onda de choque branca."),
    ("P173", "Golpe sombrio (Pennywise e medos)", "fx_golpe_sombrio",
     "um corte em arco de energia vermelha e preta, com faíscas de luz morta (laranja pálido), como uma garra de sombra."),
]


def main():
    linhas = [
        "# Prompts de imagens para IT Ecos de Derry — v5 (P167–P173): efeitos de ataque dos inimigos",
        "",
        "Pedido do Diego (17/11/2026): o golpe dos inimigos precisa aparecer como um ataque de verdade, não só como a "
        "área vermelha do aviso. Gerado por `tools/prompts_v5_fx.py`; edite o script, não este arquivo.",
        "",
        "## Catálogo",
        "",
        "| ID | Imagem | Arquivo | Dimensões finais |",
        "| --- | --- | --- | --- |",
    ]
    for pid, titulo, arq, _ in FX:
        linhas.append(f"| {pid} | {titulo} | {arq}.png | 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32 |")
    linhas.append("")
    for pid, titulo, arq, efeito in FX:
        linhas += [
            f"## {pid} {titulo}", "",
            f"**Arquivo:** {arq}.png", "",
            "**Dimensões finais:** 128 × 32 px; 4 colunas × 1 linha; frame 32 × 32", "",
            "**Referências para anexar:** fx_impacto.png, fx_fogo.png", "",
            "**Prompt completo para copiar:**", "",
            "```text", BASE.format(efeito=efeito), "```", "",
        ]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(FX)} prompts")


if __name__ == "__main__":
    main()
