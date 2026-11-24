"""Gera instructions/Prompts-Assets-IT-v8-pennywise.md (P221–P226): folhas PRÓPRIAS dos Pennywise "+".

Pedido do Diego (23–24/11/2026): "o It do bioma 1 e 2 estão muito parecidos, parece até que são o mesmo";
as diferenças entre os estágios devem ser claras em poder, tamanho, prepotência, indumentária e aura. E "um
dos Pennywise deve ser todo encharcado de sangue, do pé até a metade do rosto" (escolhido: o II+, que sai do
Ralo de Sangue nos Porões). Antes, I e I+, II e II+, III e III+ dividiam a mesma folha.

As folhas de I+ foram geradas pelo Codex em 23/11 (masters já em assets/); II+ e III+ podem vir do Gemini
(tools/batch_images.py --only P223 P224 P225 P226) ou do Codex, com estes prompts.
Uso: python tools/prompts_v8_pennywise.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v8-pennywise.md"

ESTILO = (
    "Pixel art de RPG com estética de 16 bits, vista superior com um pouco da frente dos personagens visível, câmera fixa e "
    "não isométrica. Contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. O jogo "
    "é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e manifestações sobrenaturais. "
    "Respeite a grade e mantenha exatamente a mesma escala, roupa, luz e proporção entre frames; altere apenas o movimento "
    "previsto. A referência determina a identidade do Pennywise (rosto branco, cabelo ruivo em tufos, nariz e boca "
    "vermelhos), mas a ROUPA e a postura deste estágio são as descritas aqui, claramente diferentes das outras folhas. "
)
FINAL = (
    " Fundo realmente transparente com canal alfa. Não desenhar padrão quadriculado, fundo branco, legendas, letras, "
    "números, marcas d’água ou linhas de grade. Cada figura inteira deve ficar dentro de sua célula, sem partes cortadas "
    "e com o mesmo pivô dos pés. Entregar somente o asset descrito."
)
ACOES = (
    "Crie um atlas com 4 colunas e 3 linhas, cada frame 32 × 48, total 128 × 144. Linha 1: quatro frames de espera com "
    "respiração. Linha 2: quatro frames de preparação, erguendo as mãos antes de atacar. Linha 3: quatro frames de ataque "
    "com a mão apontada à frente, terminando em recuperação. Ele permanece de frente, voltado levemente para baixo. Não "
    "gerar projéteis junto ao corpo. Mesmo alinhamento dos pés em todas as poses."
)
ANDAR = (
    "Crie 12 frames de caminhada, atlas com 4 colunas e 3 linhas. IMPORTANTE: exatamente 3 linhas e 4 colunas (12 frames). "
    "Linha 1: andando para BAIXO, de frente para a câmera. Linha 2: andando para a DIREITA, perfil completo com o nariz, o "
    "peito e os pés virados para a DIREITA da imagem em TODOS os quatro frames da linha. Linha 3: andando para CIMA, de "
    "costas, vemos a nuca e as costas da roupa. Colunas: repouso, passo A, repouso alternado, passo B. As pernas e os braços "
    "se movem de verdade; a cabeça sobe e desce 1 pixel no passo. A roupa e as manchas são IDÊNTICAS às da folha de ações "
    "anexada, nas três vistas. Pés alinhados na mesma base; não simular deslocamento da figura dentro do frame."
)

ESTAGIOS = {
    "1p": ("I+ (os Barrens)",
           "Segunda manifestação, já arrogante e mais alta: babado vinho em camadas, mangas largas, casaca de painéis vermelhos "
           "com acabamento dourado, quatro pompons vermelhos vivos, olhar zangado e postura ereta e imperiosa.",
           ["pennywise_referencia", "pennywise_1_actions"]),
    "2p": ("II+ (os Porões, o Ralo de Sangue)",
           "Quarta manifestação, a mais repulsiva: o traje cinza-prateado e os babados estão ENCHARCADOS DE SANGUE escuro e "
           "vermelho-vivo, dos PÉS até a METADE DO ROSTO (a boca e o queixo sujos, os olhos e a testa ainda brancos), "
           "escorrendo em fios pelo corpo; sapatos e luvas vermelhos de sangue; cabelo ruivo grudado e úmido. Corpo maior e "
           "mais largo que os estágios anteriores, sorriso largo e cruel, postura curvada para a frente, faminta. Terror "
           "estilizado de pixel art: sangue como cor e manchas, sem ferimentos, sem órgãos, sem gore anatômico.",
           ["pennywise_referencia", "pennywise_2_actions"]),
    "3p": ("III+ (a Cisterna, o Lobisomem)",
           "Sexta manifestação, quase no auge: o maior de todos os estágios humanos, ombros largos e encurvado como um predador; "
           "traje cinza escurecido e RASGADO em tiras nas mangas e na barra, babado negro e violeta, pompons vermelho-sangue, "
           "unhas compridas como garras, olhos amarelos brilhando e uma sombra violeta colada à silhueta. Prepotente e feroz.",
           ["pennywise_referencia", "pennywise_3_actions"]),
}

ITENS = []
pid = 221
for chave, (nome, roupa, refs) in ESTAGIOS.items():
    ITENS.append((f"P{pid}", f"Pennywise {nome}: ações", f"pennywise_{chave}_actions",
                  "128 × 144 px; 4 colunas × 3 linhas; frame 32 × 48", refs, ESTILO + roupa + " " + ACOES + FINAL))
    ITENS.append((f"P{pid + 1}", f"Pennywise {nome}: andando", f"pennywise_{chave}_walk",
                  "128 × 192 px; 4 colunas × 4 linhas; frame 32 × 48", ["pennywise_referencia", f"pennywise_{chave}_actions"],
                  ESTILO + roupa + " " + ANDAR + FINAL))
    pid += 2


def main():
    linhas = ["# Prompts de imagens para IT Ecos de Derry — v8 (P221–P226): Pennywise I+, II+ e III+ com folhas próprias", "",
              "Pedido do Diego (23–24/11/2026). Gerado por `tools/prompts_v8_pennywise.py`; edite o script, não este arquivo.", "",
              "## Catálogo", "", "| ID | Imagem | Arquivo | Dimensões finais |", "| --- | --- | --- | --- |"]
    for p, titulo, arq, dims, _, _ in ITENS:
        linhas.append(f"| {p} | {titulo} | {arq}.png | {dims} |")
    linhas.append("")
    for p, titulo, arq, dims, refs, prompt in ITENS:
        linhas += [f"## {p} {titulo}", "", f"**Arquivo:** {arq}.png", "", f"**Dimensões finais:** {dims}", "",
                   "**Referências para anexar:** " + ", ".join(r + ".png" for r in refs), "", "**Prompt completo para copiar:**", "",
                   "```text", "Entregue uma única imagem PNG. Dimensões finais desejadas: " + dims + ". " + prompt, "```", ""]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(ITENS)} prompts")


if __name__ == "__main__":
    main()
