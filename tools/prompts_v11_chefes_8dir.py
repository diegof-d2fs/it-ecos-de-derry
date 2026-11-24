"""Gera instructions/Prompts-Assets-IT-v11-chefes-8dir.md (P266–P276): os chefes em 8 direções.

Pedido do Diego (24/11/2026): "os chefes ainda ficam só olhando pra tela, não estão se movimentando em 360, tanto os
Pennywise quanto os chefes comuns, exceto o último Pennywise que está top". O IV ganhou o tronco em 8 direções
(P265); aqui o mesmo para os medos (fase 1) e para os Pennywise I a III+ (fase 2).

Cada folha: 5 colunas × 5 linhas. Linhas = direções (baixo, baixo-direita, direita, cima-direita, cima); o jogo espelha
as três da direita para fazer as da esquerda (8 direções). Colunas = parado, passo A, passo B, preparando, atacando.
O Ralo de Sangue não entra: é um ralo fixo no chão.

Geração: python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v11-chefes-8dir.md [arquivos...]
Uso: python tools/prompts_v11_chefes_8dir.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v11-chefes-8dir.md"

ESTILO = (
    "Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera "
    "fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização "
    "fotográfica. O jogo é IT Ecos de Derry, terror estilizado com crianças protagonistas (Derry, Maine, 1989). "
)
FINAL = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, "
    "números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes "
    "cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito."
)


def grade(fw, fh, quem, andar, preparo, ataque):
    return (
        f"O MESMO {quem} das folhas anexadas: mesma silhueta, roupa, cores, rosto e TAMANHO, sem mudar nenhum detalhe "
        f"de identidade. Agora ele aparece em CINCO direções, para virar para qualquer lado no jogo (360 graus). Atlas "
        f"com 5 colunas e 5 linhas, cada frame {fw} × {fh}, total {5 * fw} × {5 * fh}. IMPORTANTE: exatamente 5 linhas "
        "e 5 colunas. Linha 1: virado para BAIXO, de frente para a câmera. Linha 2: virado para BAIXO E DIREITA, três "
        "quartos de frente, o rosto e o peito voltados para o canto de baixo à direita. Linha 3: virado para a "
        "DIREITA, de PERFIL completo, o rosto, o peito e os pés apontando para a DIREITA da imagem. Linha 4: virado "
        "para CIMA E DIREITA, três quartos de costas, vemos as costas e um pouco do perfil direito. Linha 5: virado "
        f"para CIMA, de COSTAS. Colunas em TODAS as linhas: (1) parado, respirando; (2) {andar}, passo A; (3) {andar}, "
        f"passo B (o outro pé ou o outro tempo do movimento); (4) {preparo}; (5) {ataque}, NA DIREÇÃO DA LINHA. Os pés "
        "(ou a sombra, se ele voa) alinhados na mesma base em todos os frames; o corpo centrado na mesma posição em "
        "todas as direções; não simular deslocamento da figura dentro do frame."
    )


# (arquivo, título, frame w, frame h, quem, andar, preparo, ataque, referências)
CHEFES = [
    ("medo_mumia_8dir", "A Múmia (chefe 1, fase 1)", 64, 64, "medo da Múmia (as bandagens soltas, o corpo seco e alto)",
     "andando arrastado", "erguendo os braços, as bandagens se soltando", "o golpe das bandagens lançadas para a frente",
     ["medo_mumia", "medo_mumia_walk"]),
    ("medo_afogado_8dir", "O Afogado (chefe 2, fase 1)", 64, 64, "medo do Afogado: uma figura MAGRA e alta (não é gorda nem careca), de pele azul-esverdeada pálida e meio translúcida, com CABELOS LONGOS, escuros e molhados caindo sobre o rosto, um trapo de vestido cinza encharcado, pingando água, braços finos e compridos",
     "andando arrastado, pingando", "os braços se erguendo, a água subindo em volta dos pés", "o agarrão com os braços finos esticados para a frente, espirrando água",
     ["medo_afogado", "medo_afogado_walk"]),
    ("medo_leproso_8dir", "O Leproso (chefe 3, fase 1)", 64, 64, "medo do Leproso (a pele em feridas, os trapos, curvado)",
     "andando curvado e trôpego", "inflando, a nuvem pútrida saindo do corpo", "o bote com as mãos para a frente",
     ["medo_leproso", "medo_leproso_walk"]),
    ("medo_passaro_8dir", "O Pássaro das Cinzas (chefe 5, fase 1)", 64, 64,
     "medo do Pássaro das Cinzas (a ave gigante de penas negras com brasas, que VOA)",
     "voando, batendo as asas (asas para cima / asas para baixo)", "as brasas acendendo no corpo, asas abertas",
     "o rasante com as garras e o bico para a frente", ["medo_passaro", "medo_passaro_walk"]),
    ("medo_lobisomem_8dir", "O Lobisomem (chefe 6, fase 1)", 64, 64, "medo do Lobisomem (o lobo de pé, enorme, com a jaqueta rasgada)",
     "andando curvado, as garras para baixo", "agachado, pronto para o salto", "a garrada com o braço para a frente",
     ["medo_lobisomem", "medo_lobisomem_walk"]),
    ("pennywise_1_8dir", "Pennywise I (Galeria)", 32, 48, "Pennywise do estágio I (o palhaço de traje cinza clássico)",
     "andando", "erguendo as mãos", "a mão apontada para a frente", ["pennywise_1_actions", "pennywise_1_walk"]),
    ("pennywise_1p_8dir", "Pennywise I+ (Barrens)", 32, 48, "Pennywise do estágio I+ (casaca vinho e dourada, imperioso)",
     "andando ereto e arrogante", "erguendo as mãos", "a mão apontada para a frente", ["pennywise_1p_actions", "pennywise_1p_walk"]),
    ("pennywise_2_8dir", "Pennywise II (Rua Neibolt)", 32, 48, "Pennywise do estágio II (apodrecido, verde-musgo, esguio, braços compridos demais)",
     "andando torto e corcunda", "as garras se erguendo", "a garra apontada para a frente", ["pennywise_2_actions", "pennywise_2_walk"]),
    ("pennywise_2p_8dir", "Pennywise II+ (Porões)", 32, 48, "Pennywise do estágio II+ (encharcado de sangue dos pés até a metade do rosto)",
     "andando curvado e faminto", "as mãos se erguendo, pingando sangue", "a mão apontada para a frente",
     ["pennywise_2p_actions", "pennywise_2p_walk"]),
    ("pennywise_3_8dir", "Pennywise III (Siderúrgica)", 32, 48, "Pennywise do estágio III (carbonizado, brasas nas costuras, cabelo em chamas, gola de penas negras)",
     "andando ereto e soberbo", "as mãos incandescentes se erguendo", "a mão em brasa apontada para a frente",
     ["pennywise_3_actions", "pennywise_3_walk"]),
    ("pennywise_3p_8dir", "Pennywise III+ (Cisterna)", 32, 48, "Pennywise do estágio III+ (traje cinza escurecido e rasgado em tiras, babado violeta, garras, olhos amarelos)",
     "andando encurvado como um predador", "as garras se abrindo", "a garrada para a frente", ["pennywise_3p_actions", "pennywise_3p_walk"]),
]

ASSETS = []
for k, (arq, titulo, fw, fh, quem, andar, preparo, ataque, refs) in enumerate(CHEFES):
    dims = f"{5 * fw} × {5 * fh} px; 5 colunas × 5 linhas; frame {fw} × {fh}"
    ASSETS.append((f"P{266 + k}", f"{titulo}: 8 direções", arq, dims, refs,
                   ESTILO + grade(fw, fh, quem, andar, preparo, ataque) + FINAL))


def main():
    linhas = [
        "# Prompts dos assets v11: os chefes em 8 direções (P266–P276)", "",
        "Gerado por `tools/prompts_v11_chefes_8dir.py` (24/11/2026). Pedido do Diego: os chefes e os Pennywise viram para "
        "qualquer lado (360 graus), como o Pennywise IV. Gerar com "
        "`python tools/codex_sprites.py --md instructions/Prompts-Assets-IT-v11-chefes-8dir.md`.",
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
