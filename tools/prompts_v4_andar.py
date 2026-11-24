"""Gera instructions/Prompts-Assets-IT-v4-andar.md (P151–P166): folhas de ANDAR em 4 direções.

Pedido do Diego (17/11/2026): todo personagem que se move (amigos, NPCs, Pennywise, medos) precisa
"parecer vivo", com sprites de todas as posições, como o Bill. Antes, os amigos tinham só 2 quadros
de frente (`*_idle`) e "deslizavam" olhando para a câmera.

Contrato (o mesmo do bill_walk, P03): linhas de cima para baixo = andando para baixo (de frente),
esquerda (perfil), direita (perfil), cima (de costas); colunas = repouso, passo A, repouso
alternado, passo B. Pés na mesma base em todos os quadros.

Uso: python tools/prompts_v4_andar.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v4-andar.md"

ESTILO = (
    "Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, "
    "câmera fixa e não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem "
    "suavização fotográfica. Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo "
    "reservado a balões e perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças "
    "protagonistas e manifestações sobrenaturais. Respeite a grade e mantenha exatamente a mesma escala, roupa, luz "
    "e proporção entre frames; altere apenas o movimento previsto. A referência determina identidade, não apenas "
    "um clima parecido."
)
FIM = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, "
    "letras, números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem "
    "partes cortadas e com o mesmo pivô dos pés. Entregar somente o asset descrito."
)
# O master vem em 3 LINHAS (frente, perfil DIREITO, costas): no piloto do Ben (P151, 17/11) as linhas de
# perfil vieram com esquerda e direita misturadas, como no bill_walk. A esquerda é o ESPELHO da direita,
# feito no export (WALK3 em export_final.py) — coerente por construção e mais barato.
DIRECOES = (
    "IMPORTANTE: exatamente 3 linhas e 4 colunas (12 frames). Linha 1: andando para BAIXO, de frente para a "
    "câmera. Linha 2: andando para a DIREITA, perfil completo com o nariz, o peito e os pés virados para a "
    "DIREITA da imagem em TODOS os quatro frames da linha (nenhum frame de frente ou virado para a esquerda). "
    "Linha 3: andando para CIMA, de costas, vemos a nuca. Colunas: repouso, passo A (uma perna à frente, braço "
    "oposto balançando), repouso alternado, passo B (a outra perna à frente). As pernas e os braços se movem de "
    "verdade; a cabeça sobe e desce 1 pixel no passo. Olhos SEMPRE abertos e expressão viva. Pés alinhados na "
    "mesma base; não simular deslocamento da figura dentro do frame."
)

CRIANCA = "128 × 192 px; 4 colunas × 4 linhas; frame 32 × 48"
MEDO = "256 × 256 px; 4 colunas × 4 linhas; frame 64 × 64"

# (id, título, arquivo, dimensões, referências, texto específico)
ASSETS = [
    ("P151", "Ben Hanscom andando", "ben_walk", CRIANCA, ["ben_idle", "retrato_ben"],
     "Use exatamente o Ben do sprite anexado: menino de cabelos castanhos curtos, corpo mais robusto, camiseta "
     "bege, colete verde apagado e bermuda azul escura; segura um mapa de papel dobrado numa das mãos."),
    ("P152", "Eddie Kaspbrak andando", "eddie_walk", CRIANCA, ["eddie_idle", "retrato_eddie"],
     "Use exatamente o Eddie do sprite anexado: menino magro, cabelo castanho penteado para o lado, camiseta "
     "amarela suave, short azul e pequena bolsa de cuidados a tiracolo."),
    ("P153", "Beverly Marsh andando", "beverly_walk", CRIANCA, ["beverly_idle", "retrato_beverly"],
     "Use exatamente a Beverly do sprite anexado: menina de cabelo ruivo curto, camisa vinho apagado, jardineira "
     "azul e tênis claros; passo firme."),
    ("P154", "Mike Hanlon andando", "mike_walk", CRIANCA, ["mike_idle", "retrato_mike"],
     "Use exatamente o Mike do sprite anexado: menino negro, cabelo curto, camisa verde oliva, calça jeans escura "
     "e pequeno caderno na mão."),
    ("P155", "Richie Tozier andando", "richie_walk", CRIANCA, ["richie_idle", "retrato_richie"],
     "Use exatamente o Richie do sprite anexado: menino magro, óculos grandes de armação preta e lentes grossas "
     "(visíveis também de perfil), cabelo preto curto e bagunçado, camisa de botão estampada de cores fortes por "
     "cima de camiseta branca, bermuda jeans e tênis."),
    ("P156", "Stan Uris andando", "stan_walk", CRIANCA, ["stan_idle", "retrato_stan"],
     "Use exatamente o Stan do sprite anexado: menino arrumado, cabelo castanho claro curto e cacheado, camisa "
     "polo verde-oliva por dentro da calça cáqui, cinto, sapatos limpos, pequeno caderno de pássaros."),
    ("P157", "Elias, o zelador, andando", "elias_walk", CRIANCA, ["elias_idle"],
     "Use exatamente o Elias do sprite anexado: homem adulto de meia idade, cabelo grisalho curto, bigode "
     "discreto, macacão marrom de manutenção e botas, chave de ferramenta na mão; passo pesado e cansado. Ele é "
     "adulto: ocupa a altura toda do frame."),
    ("P158", "Georgie andando", "georgie_walk", CRIANCA, ["georgie_idle", "retrato_georgie"],
     "Use exatamente o Georgie do sprite anexado: menino pequeno, bem menor que as outras crianças (cerca de dois "
     "terços da altura do frame), capa de chuva amarela com capuz e galochas vermelhas; passinhos curtos."),
    ("P159", "Pennywise I andando", "pennywise_1_walk", CRIANCA, ["pennywise_referencia", "pennywise_1_actions"],
     "Use exatamente o Pennywise de pennywise_1_actions (a primeira manifestação, fraca: corpo contido, ombros "
     "baixos, pompons de vermelho apagado). Ele anda com passos largos e saltitantes de palhaço, braços soltos "
     "balançando, cabeça inclinada; é inquietante, nunca engraçado."),
    ("P160", "Pennywise II andando", "pennywise_2_walk", CRIANCA, ["pennywise_referencia", "pennywise_2_actions"],
     "Use exatamente o Pennywise de pennywise_2_actions (mais forte que o I: postura mais alta, vermelho mais "
     "vivo). Passos largos e saltitantes de palhaço, braços longos balançando, um leve gingado ameaçador."),
    ("P161", "Pennywise III andando", "pennywise_3_walk", CRIANCA, ["pennywise_referencia", "pennywise_3_actions"],
     "Use exatamente o Pennywise de pennywise_3_actions (o mais forte dos três: sorriso largo, olhos acesos). "
     "Anda inclinado para a frente, rápido, com passadas largas e os braços abertos como quem vai agarrar."),
    ("P162", "A Múmia andando", "medo_mumia_walk", MEDO, ["medo_mumia"],
     "Use exatamente a Múmia de medo_mumia: alta e curvada, faixas de linho velhas e amareladas, faixas soltas "
     "balançando, olhos como dois pontos de luz laranja. Andar arrastado, uma perna puxando a outra."),
    ("P163", "O Afogado da Caixa d'Água andando", "medo_afogado_walk", MEDO, ["medo_afogado"],
     "Use exatamente o Afogado de medo_afogado: corpo inchado e encharcado, roupas pingando. Andar pesado e "
     "cambaleante, deixando gotas a cada passo; sem poça, sem chão desenhado."),
    ("P164", "O Leproso andando", "medo_leproso_walk", MEDO, ["medo_leproso"],
     "Use exatamente o Leproso de medo_leproso: andarilho de roupas rasgadas e pele doente estilizada (sem gore). "
     "Andar torto e mancando, braços estendidos à frente."),
    ("P165", "O Lobisomem andando", "medo_lobisomem_walk", MEDO, ["medo_lobisomem"],
     "Use exatamente o Lobisomem de medo_lobisomem: pelagem escura eriçada, jaqueta de colégio rasgada, garras. "
     "Andar curvado e ameaçador, em passadas largas, garras à mostra."),
    ("P166", "O Pássaro das Cinzas voando", "medo_passaro_walk", MEDO, ["medo_passaro"],
     "Use exatamente o Pássaro das Cinzas de medo_passaro: ave enorme de penas negras com brasas laranja entre as "
     "penas, olhos amarelos. Ele VOA, não anda: nas quatro colunas, um ciclo de bater de asas (asas no alto, "
     "descendo, embaixo, subindo); na linha de costas vemos o dorso e a cauda; nos perfis, uma asa na frente do "
     "corpo. Sem chão nem sombra desenhados."),
]


def prompt(dims, especifico, medo):
    escala = ("Criatura de 64 × 64 por frame, em escala de chefe; terror estilizado e sugestivo, sem sangue "
              "explícito, ferimentos, gore ou violência gráfica." if medo else
              "Personagem inteiro, dos pés à cabeça, na mesma escala das outras crianças do jogo (frame 32 × 48).")
    return (f"Entregue uma única imagem PNG. Dimensões finais desejadas: {dims}. {ESTILO} {especifico} "
            f"Crie 12 frames de caminhada, atlas com 4 colunas e 3 linhas. {DIRECOES} {escala}{FIM}")


def main():
    linhas = [
        "# Prompts de imagens para IT Ecos de Derry — v4 (P151–P166): andar em 4 direções",
        "",
        "Pedido do Diego (17/11/2026): todo personagem que se move precisa parecer vivo, com sprites de todas as "
        "posições, como o Bill. Mesmo contrato do `bill_walk` (P03). Gerado por `tools/prompts_v4_andar.py`; "
        "edite o script, não este arquivo.",
        "",
        "Regras herdadas: a referência anexada é sempre o PRÓPRIO personagem (nunca o de outro); fundo verde "
        "#00FF00 na API (o script de lote acrescenta a nota); revisar contact sheets antes de aceitar.",
        "",
        "## Catálogo",
        "",
        "| ID | Imagem | Arquivo | Dimensões finais |",
        "| --- | --- | --- | --- |",
    ]
    for pid, titulo, arq, dims, _, _ in ASSETS:
        linhas.append(f"| {pid} | {titulo} | {arq}.png | {dims} |")
    linhas.append("")
    for pid, titulo, arq, dims, refs, esp in ASSETS:
        linhas += [
            f"## {pid} {titulo}",
            "",
            f"**Arquivo:** {arq}.png",
            "",
            f"**Dimensões finais:** {dims}",
            "",
            f"**Referências para anexar:** {', '.join(r + '.png' for r in refs)}",
            "",
            "**Prompt completo para copiar:**",
            "",
            "```text",
            prompt(dims, esp, dims == MEDO),
            "```",
            "",
        ]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(ASSETS)} prompts")


if __name__ == "__main__":
    main()
