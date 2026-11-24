"""Gera instructions/Prompts-Assets-IT-v6-cenas.md (P174–P219): quadros das CUT SCENES.

Pedido do Diego (23/11/2026): cenas de história com 5 a 8 imagens e texto — na abertura do Novo jogo
(~1 min 30 s), na chegada a cada bioma depois de um chefe, e uma final de 1 a 2 min. Nenhuma imagem
se repete entre as cenas. Três ilustrações antigas que não aparecem no jogo são reaproveitadas
(cartao_clube, tela_perdedores, referencia_visual); as outras 46 são novas e estão aqui.

O TEXTO de cada quadro fica em data/cenas.txt (o jogo lê de lá); aqui só a imagem.
Uso: python tools/prompts_v6_cenas.py   (edite este script, não o .md)
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SAIDA = ROOT / "instructions" / "Prompts-Assets-IT-v6-cenas.md"

BASE = (
    "Entregue uma única imagem PNG. Dimensões finais desejadas: 640 × 360 px. Pixel art de RPG com estética de "
    "16 bits, contornos legíveis, blocos de cor definidos, sombras em poucos tons, sem suavização fotográfica. "
    "Paleta coerente de azul escuro, verde apagado, cinza pedra, ferrugem e vermelho vivo reservado a balões e "
    "perigo. O jogo é IT Ecos de Derry, uma adaptação de terror estilizado com crianças protagonistas e "
    "manifestações sobrenaturais, ambientada numa cidadezinha americana dos anos 1980. Ilustração em pixel art "
    "640 × 360, composição horizontal 16:9: um quadro de CENA DE HISTÓRIA (cutscene), com enquadramento "
    "cinematográfico ({plano}), mesma direção de arte, paleta e acabamento das imagens de referência anexadas. "
    "Cena: {cena} {elenco}A ilustração ocupa a imagem INTEIRA, de borda a borda, nos quatro lados: sem faixas "
    "pretas em cima ou embaixo, sem letterbox, sem margens, sem vinheta e sem moldura. Os rostos e o assunto "
    "principal ficam na metade de cima e no centro; a parte de baixo é chão ou cenário contínuo, sem rostos. "
    "Terror estilizado e sugestivo, sem sangue "
    "explícito, ferimentos, gore ou violência gráfica. Sem interface, caixa de diálogo, moldura, bordas, texto, "
    "letras legíveis, título, legendas, marcas d’água ou grade desenhada."
)

# Identidades (as mesmas dos sprites aprovados). Entram no prompt de cada quadro em que o personagem aparece.
ID = {
    "bill": "Bill: menino de 12 anos, cabelo castanho liso, jaqueta azul de capuz aberta sobre camiseta verde-clara, "
            "bermuda marrom, tênis; segura uma lanterna",
    "georgie": "Georgie: menino pequeno de 6 anos, capa de chuva amarela com capuz e galochas vermelhas",
    "ben": "Ben: menino robusto de 12 anos, cabelo castanho curto, camiseta bege, colete verde apagado, bermuda azul-escura",
    "stan": "Stan: menino de 12 anos, sério e arrumado, cabelo castanho-claro curto e cacheado, camisa polo verde-oliva "
            "por dentro da calça cáqui, segura um caderno de pássaros",
    "eddie": "Eddie: menino magro de 12 anos, cabelo castanho penteado para o lado, camiseta amarela clara, short azul, "
             "bolsinha marrom a tiracolo",
    "beverly": "Beverly: menina de 12 anos, cabelo ruivo curto, camisa vinho apagado, jardineira azul, tênis claros",
    "mike": "Mike: menino negro de 12 anos, cabelo curto, camisa verde-oliva, calça jeans escura",
    "richie": "Richie: menino magro de 12 anos, óculos grandes de lentes grossas e armação preta, cabelo preto bagunçado, "
              "camisa estampada de cores fortes por cima de camiseta branca, bermuda jeans",
    "pennywise": "Pennywise: palhaço alto e magro de roupa cinza-prateada com babados e pompons vermelhos, rosto branco, "
                 "tufos de cabelo ruivo, olhos amarelos e sorriso estreito (como na referência anexada)",
}
# Sprite aprovado do PRÓPRIO personagem (nunca o de outro: o modelo copia a identidade).
REF_PERSONAGEM = {"bill": "bill_referencia", "georgie": "georgie_idle", "ben": "ben_idle", "stan": "stan_idle",
                  "eddie": "eddie_idle", "beverly": "beverly_idle", "mike": "mike_idle", "richie": "richie_idle",
                  "pennywise": "pennywise_referencia"}
ESTILO = ["referencia_visual", "cartao_clube"]

# (arquivo, título, plano, elenco, cena)
QUADROS = [
    # ---------- Abertura (Novo jogo) ----------
    ("cena_abertura_1", "Derry na chuva", "plano geral aéreo", [],
     "a cidadezinha de Derry ao entardecer sob chuva fina: telhados molhados, uma rua principal com postes acesos, "
     "uma torre d'água ao longe e, em primeiro plano, a tampa de um bueiro com um brilho vermelho fraco saindo das frestas."),
    ("cena_abertura_2", "Georgie e o barquinho", "plano médio na rua", ["georgie"],
     "numa rua residencial alagada pela chuva, Georgie corre pela calçada atrás de um barquinho de papel branco que desce "
     "a enxurrada da sarjeta. Casas de madeira e céu cinzento. Ele está de costas, sozinho."),
    ("cena_abertura_3", "O bueiro", "close baixo, quase no chão", [],
     "a boca escura de um bueiro na guia da calçada molhada. Um barquinho de papel encharcado está parado na entrada. "
     "Lá dentro, no escuro, um único balão vermelho flutua. Ninguém por perto."),
    ("cena_abertura_6", "A noite em que todos sumiram", "plano geral interno", ["bill"],
     "o interior do clube subterrâneo dos Perdedores à noite, com lampião quase apagado, caixotes e mapas na parede. "
     "Bill está sozinho no centro, assustado, olhando em volta. As redes e cadeiras estão vazias, ainda balançando, e "
     "seis rastros de pegadas de lama saem em direções diferentes pela porta. Três balões vermelhos flutuam junto ao teto."),
    ("cena_abertura_7", "Bill desce pelo alçapão", "plano de cima para baixo", ["bill"],
     "dentro do Clube dos Perdedores, Bill abre o alçapão retangular de madeira no chão, ao lado da mesa e do lampião, "
     "e desce por uma escada de madeira apontando a lanterna para a escuridão. Mapas na parede e um balão vermelho "
     "ao fundo. É o alçapão particular do Clube, não um bueiro de rua."),
    # ---------- Bioma 2: O Barrens (depois da Múmia) ----------
    ("cena_b2_2", "A Coisa recua", "plano médio", ["pennywise"],
     "numa galeria de esgoto de tijolos, o palhaço Pennywise se desfaz em névoa escura e fragmentos de balão vermelho, "
     "rindo enquanto recua para dentro de um túnel. Faixas velhas de múmia estão caídas no chão molhado."),
    ("cena_b2_3", "Ben sobe", "contra-plongée, de baixo para cima", ["ben", "bill"],
     "Ben sobe por uma escada de ferro de manutenção em direção a um círculo de luz do luar lá no alto. No pé da escada, "
     "Bill segura um mapa de papel dobrado que Ben acabou de lhe entregar. Ben olha para baixo, preocupado."),
    ("cena_b2_4", "A saída para o Barrens", "plano geral", ["bill"],
     "a saída de um grande cano de concreto que despeja água num riacho no meio da mata do Barrens, à noite. Bill está "
     "pequeno na boca do cano, com a lanterna acesa. Névoa baixa, árvores retorcidas, lua cheia por trás das nuvens."),
    ("cena_b2_5", "O caderno de Stan", "close no chão", [],
     "na margem lamacenta do riacho, um pequeno caderno de pássaros aberto e molhado, com desenhos de pássaros a lápis "
     "(sem letras legíveis). Pegadas pequenas de tênis afundam na lama e seguem direto para a água escura."),
    ("cena_b2_6", "A caixa d'água", "plano geral noturno", [],
     "uma velha caixa d'água de madeira e uma represa abandonada no Barrens à noite. Sob a superfície parada da água, "
     "uma silhueta humana pálida e imóvel, só sugerida. Um balão vermelho está preso nos galhos acima."),
    # ---------- Bioma 3: Casa da Rua Neibolt (depois do Afogado) ----------
    ("cena_b3_1", "A comporta", "plano médio", ["stan", "bill"],
     "Stan gira com força o volante de ferro enferrujado de uma comporta, com Bill ajudando ao lado. A água do riacho "
     "baixa depressa, deixando lama e espuma. Ao fundo, bolhas na água onde algo afundou."),
    ("cena_b3_2", "A página de Stan", "close nas mãos", ["stan"],
     "as mãos de Stan entregando uma página arrancada do caderno com um mapa desenhado a lápis: linhas de túneis que "
     "levam a uma casinha torta desenhada no canto, marcada com um círculo (sem letras legíveis). Luz de lanterna."),
    ("cena_b3_3", "A casa da Rua Neibolt", "plano geral noturno", [],
     "uma casa vitoriana de madeira abandonada e torta na Rua Neibolt, à noite, com tábuas pregadas nas janelas, varanda "
     "afundada, mato alto e uma cerca quebrada com um balão vermelho amarrado. Uma única janela do andar de cima tem luz "
     "esverdeada."),
    ("cena_b3_4", "O inalador", "close no chão", [],
     "um inalador de asma azul e branco caído no assoalho podre de um corredor escuro, com papel de parede florido "
     "descascando nas paredes. Um feixe de lanterna ilumina o inalador."),
    ("cena_b3_5", "A porta do porão", "plano médio, ponto de vista de Bill", [],
     "a porta do porão entreaberta no fim do corredor da casa abandonada; uma escada de madeira desce para o escuro. "
     "Na fresta, a sombra de uma figura curvada com mãos compridas e enfaixadas (só a silhueta). Névoa esverdeada sobe."),
    # ---------- Bioma 4: Porões inundados (depois do Leproso) ----------
    ("cena_b4_1", "Eddie cuida de Bill", "plano médio", ["eddie", "bill"],
     "sentados num degrau de madeira de um porão, Eddie enrola uma atadura de gaze limpa no antebraço de Bill, com um "
     "pequeno kit de primeiros socorros aberto ao lado. Eddie está concentrado e firme; Bill sorri cansado."),
    ("cena_b4_2", "A Coisa aprende", "plano médio, escuro", ["pennywise"],
     "Pennywise, mais forte e mais alto, espia meio escondido atrás do batente de uma porta escura de porão. Olhos "
     "amarelos brilhando, sorriso mais largo, uma sombra roxa discreta colada ao corpo."),
    ("cena_b4_3", "O chão cedeu", "plano de cima para baixo", [],
     "no porão de terra, atrás de uma fornalha velha, o chão desabou num buraco que revela água escura e canos antigos "
     "lá embaixo, com uma escada improvisada de madeira descendo. Raízes saem das paredes de terra."),
    ("cena_b4_4", "O estilingue de Beverly", "close na água", [],
     "boiando na água rasa cor de ferrugem de um porão inundado, um estilingue de madeira e, ao lado, uma pequena esfera "
     "de prata que brilha à luz da lanterna. Canos enferrujados atrás."),
    ("cena_b4_5", "O banheiro antigo", "plano médio interno", [],
     "um banheiro antigo e abandonado no fundo do porão inundado: pia de louça rachada, espelho manchado e quebrado, "
     "azulejos verdes. Do ralo da pia saem fios de cabelo ruivo e escuro, como se algo puxasse lá de dentro. Luz fria."),
    # ---------- Bioma 5: Siderúrgica Kitchener (depois do Ralo de Sangue) ----------
    ("cena_b5_1", "O tiro certeiro", "plano médio dinâmico", ["beverly"],
     "Beverly, de perfil, estica o estilingue e dispara. No fundo, uma passarela de metal enferrujado desaba sobre um "
     "grande ralo no chão, levantando poeira e respingos de água escura."),
    ("cena_b5_2", "Prata", "close nas mãos", ["beverly", "bill"],
     "a mão de Beverly colocando uma pequena esfera de prata brilhante na palma aberta de Bill. Na palma de Bill há uma "
     "cicatriz fina e antiga. Luz quente de lanterna, fundo escuro de porão."),
    ("cena_b5_3", "A Siderúrgica Kitchener", "plano geral noturno", [],
     "as ruínas de uma enorme siderúrgica antiga à noite: chaminés de tijolo, galpões de metal queimados, guindastes "
     "parados, brasas alaranjadas voando no céu escuro. Uma cerca de arame com um balão vermelho preso."),
    ("cena_b5_4", "O caderno de Mike", "close no chão", [],
     "no chão coberto de cinzas de uma fundição, um caderno aberto com recortes de jornal velhos colados (fotos de uma "
     "fábrica e títulos apenas sugeridos, sem letras legíveis), iluminado pelo brilho de brasas. Uma pena preta ao lado."),
    ("cena_b5_5", "O ninho", "contra-plongée", [],
     "no alto de uma fornalha gigante, entre correntes e vigas, um pássaro enorme feito de fuligem e cinzas, com olhos "
     "de carvão em brasa, abre as asas sobre um ninho de ferro retorcido. Brasas caem como chuva."),
    # ---------- Bioma 6: Cisterna das vozes (depois do Pássaro das Cinzas) ----------
    ("cena_b6_1", "O gerador", "plano médio", ["mike", "bill"],
     "Mike puxa com as duas mãos a alavanca de um gerador industrial antigo; uma fileira de lâmpadas acende uma depois da "
     "outra ao longo do galpão da siderúrgica. Bill protege os olhos da luz. Cinzas flutuam e se desfazem no ar."),
    ("cena_b6_2", "O mapa antigo", "close sobre a mesa", ["mike"],
     "as mãos de Mike abrindo um mapa antigo e amarelado da cidade sobre um caixote, com túneis desenhados que descem "
     "todos para um grande círculo no centro (sem letras legíveis). Luz de lâmpada. Mike aponta para o círculo."),
    ("cena_b6_3", "Só falta um", "plano médio, contraste forte", ["pennywise"],
     "Pennywise no alto de uma escada de metal, recortado contra a fumaça, em sua forma mais ameaçadora: sorriso aberto, "
     "contorno de sombra violeta colado ao corpo, segurando um único balão vermelho. Ele olha direto para a câmera."),
    ("cena_b6_4", "A cisterna", "plano geral grandioso", ["bill"],
     "uma cisterna subterrânea gigantesca como uma catedral: colunas de tijolo, arcos, tubulações enormes, água parada "
     "e escura refletindo dezenas de balões vermelhos que não estão no teto. Bill, minúsculo, numa passarela com a lanterna."),
    ("cena_b6_5", "Os óculos de Richie", "close no chão", [],
     "óculos de armação preta com lentes grossas, uma lente rachada, caídos no chão de pedra molhado da cisterna. Ao "
     "lado, pegadas fundas de garras de um animal grande. Luz de lanterna."),
    # ---------- Bioma 7: o Covil (depois do Lobisomem) ----------
    ("cena_b7_1", "Isso aqui é só papelão", "plano médio dinâmico", ["richie"],
     "Richie grita com as mãos em concha na boca, cheio de coragem; à frente dele, paredes de tijolo falsas se desmancham "
     "como cenário de teatro de papelão, rasgando e caindo, revelando um túnel escuro atrás."),
    ("cena_b7_2", "Seis de seis", "plano médio", ["richie", "bill"],
     "Richie, sujo e despenteado, ajeita os óculos colados com fita adesiva e sorri para Bill, com a mão no ombro dele. "
     "Os dois estão num túnel úmido iluminado pela lanterna."),
    ("cena_b7_3", "Juntos lá em cima", "plano de cima, vista aérea", ["ben", "stan", "eddie", "beverly", "mike", "richie"],
     "à noite, na rua molhada, seis crianças de mãos dadas formam um círculo em volta de uma tampa de bueiro aberta, "
     "olhando para baixo. Lanternas e uma lanterna a querosene iluminam o chão."),
    ("cena_b7_4", "A última escada", "plongée vertiginoso", ["bill"],
     "uma escada de pedra em espiral que desce para um abismo; lá no fundo pulsa uma luz morta, laranja pálida e fria. "
     "Bill, pequeno, desce com a lanterna. Balões vermelhos flutuam imóveis no vão."),
    ("cena_b7_5", "A montanha de brinquedos", "plano geral", [],
     "o covil da Coisa: uma montanha enorme de brinquedos velhos e roupas de criança perdidas, bicicletas, ursinhos, "
     "patins, sob luz laranja-pálida. Dezenas de balões vermelhos flutuam parados no ar. No topo da montanha, uma pequena "
     "capa de chuva amarela."),
    ("cena_b7_6", "A cicatriz", "close extremo", ["bill"],
     "a palma da mão de Bill em primeiro plano com uma cicatriz fina e antiga brilhando em vermelho suave, a mão se "
     "fechando em punho. Atrás, desfocado, o covil com balões."),
    # ---------- Final (depois do Pennywise IV) ----------
    ("cena_final_1", "Billy, me ajuda", "plano médio, escuro", ["georgie", "bill"],
     "no covil escuro, uma ilusão de Georgie, de capa amarela, estende a mão para Bill com um sorriso pedinte; seus olhos "
     "têm um brilho amarelado sutil. Bill, de costas para a câmera, segura a lanterna baixa. Balões ao redor."),
    ("cena_final_2", "A ilusão se parte", "plano médio", ["bill"],
     "Bill, firme, ergue a lanterna; à frente dele a figura de capa amarela se racha como vidro quebrado, e pelas "
     "rachaduras aparece a sombra de um palhaço. Cacos de luz voam."),
    ("cena_final_3", "Seis luzes", "plano de cima", ["bill"],
     "no covil quase totalmente escuro, seis círculos de luz quente giram em volta de Bill no chão, cada um com uma cor "
     "suave diferente (bege, verde-oliva, amarela, vinho, verde, colorida). Bill no centro, a lanterna erguida."),
    ("cena_final_4", "A Coisa se quebra", "plano médio", ["pennywise"],
     "Pennywise se quebra em sombra e névoa, a silhueta rachando em partículas escuras; dezenas de balões vermelhos "
     "murcham e caem em silêncio à sua volta. Sem sangue."),
    ("cena_final_5", "Mãos estendidas", "contra-plongée", ["bill"],
     "Bill sobe a escada de ferro de um bueiro; lá no alto, na abertura iluminada pela luz do amanhecer, seis mãos de "
     "crianças se estendem para puxá-lo para cima."),
    ("cena_final_6", "Todos aqui", "plano médio, amanhecer", ["bill", "ben", "stan", "eddie", "beverly", "mike", "richie"],
     "ao amanhecer, numa rua molhada de Derry, as sete crianças se abraçam num grupo, sujas e cansadas, rindo e chorando. "
     "O céu é rosado e dourado; poças refletem a luz."),
    ("cena_final_7", "O último balão", "plano aberto, céu", ["beverly"],
     "Beverly, de costas, solta o último balão vermelho, que sobe sozinho no céu claro da manhã sobre os telhados de "
     "Derry, já pequeno, quase sumindo."),
    ("cena_final_8", "O barquinho", "close baixo, na sarjeta", ["bill"],
     "Bill, agachado no meio-fio ensolarado, coloca com cuidado um barquinho de papel branco na água limpa que corre pela "
     "sarjeta. O bueiro mais adiante está fechado e silencioso. Luz suave da manhã."),
    ("cena_final_9", "O Clube de dia", "plano geral interno", [],
     "o clube subterrâneo dos Perdedores de dia, vazio e tranquilo: a porta aberta deixa entrar um feixe de sol, poeira "
     "dourada no ar, uma rede vazia parada, caixotes, um lampião apagado e sete pares de iniciais rabiscados numa viga "
     "(só rabiscos, sem letras legíveis)."),
    ("cena_final_10", "Derry amanhece", "plano geral aéreo", [],
     "Derry vista do alto ao amanhecer: o rio, uma ponte de ferro, telhados, a torre d'água e o verde do Barrens. Em "
     "primeiro plano, a tampa de um bueiro na calçada, com um último brilho vermelho muito fraco apagando entre as frestas."),
]


def main():
    linhas = [
        "# Prompts de imagens para IT Ecos de Derry — v6 (P174–P219): quadros das cut scenes",
        "",
        "Pedido do Diego (23/11/2026): cenas de história na abertura, entre os biomas e no final. Gerado por "
        "`tools/prompts_v6_cenas.py`; edite o script, não este arquivo. O texto dos quadros mora em `data/cenas.txt`.",
        "",
        "## Catálogo",
        "",
        "| ID | Imagem | Arquivo | Dimensões finais |",
        "| --- | --- | --- | --- |",
    ]
    ids = [f"P{174 + i}" for i in range(len(QUADROS))]
    for pid, (arq, titulo, _, _, _) in zip(ids, QUADROS):
        linhas.append(f"| {pid} | {titulo} | {arq}.png | 640 × 360 px |")
    linhas.append("")
    for pid, (arq, titulo, plano, elenco, cena) in zip(ids, QUADROS):
        texto_elenco = ("Personagens (copie a identidade exatamente): " + "; ".join(ID[p] for p in elenco) + ". ") if elenco else ""
        refs = ESTILO + [REF_PERSONAGEM[p] for p in elenco if len(elenco) <= 2]
        linhas += [
            f"## {pid} {titulo}", "",
            f"**Arquivo:** {arq}.png", "",
            "**Dimensões finais:** 640 × 360 px", "",
            "**Referências para anexar:** " + ", ".join(f"{r}.png" for r in refs), "",
            "**Prompt completo para copiar:**", "",
            "```text", BASE.format(plano=plano, cena=cena, elenco=texto_elenco), "```", "",
        ]
    SAIDA.write_text("\n".join(linhas), encoding="utf-8")
    print(f"{SAIDA.relative_to(ROOT)}: {len(QUADROS)} prompts ({ids[0]}–{ids[-1]})")


if __name__ == "__main__":
    main()
