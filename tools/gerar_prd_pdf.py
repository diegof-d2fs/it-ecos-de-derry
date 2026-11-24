"""Gera instructions/PRD-IT-Ecos-de-Derry.pdf: Parte I = a história e a lore do jogo (ilustrada com os
assets aprovados); Parte II = o PRD v2 (convertido de docs/PRD-v2-IT-Ecos-de-Derry.md); Apêndice = os
requisitos da parte 2 do enunciado (SDD §24).

Uso:  python -X utf8 tools/gerar_prd_pdf.py
Fontes do conteúdo: o texto da Parte I está aqui (mantenha coerente com o PRD §4); as Partes II e o
apêndice são lidos dos .md a cada geração, então basta editar os .md e rodar de novo.
"""
import re
from io import BytesIO
from pathlib import Path

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, CondPageBreak, Frame, Image, KeepTogether, NextPageTemplate,
                                PageBreak, PageTemplate, Paragraph, Preformatted, Spacer, Table, TableStyle)

ROOT = Path(__file__).resolve().parent.parent
A = ROOT / "assets"
HD = A / "jogo_hd"
PRD_MD = ROOT / "docs" / "PRD-v2-IT-Ecos-de-Derry.md"
SDD_MD = ROOT / "docs" / "SDD-v1-IT-Ecos-de-Derry.md"
OUT = ROOT / "instructions" / "PRD-IT-Ecos-de-Derry.pdf"

F = Path("C:/Windows/Fonts")
for nome, arq in [("Serif", "georgia.ttf"), ("Serif-B", "georgiab.ttf"), ("Serif-I", "georgiai.ttf"),
                  ("Sans", "segoeui.ttf"), ("Sans-B", "segoeuib.ttf"), ("Sans-I", "segoeuii.ttf"),
                  ("Mono", "consola.ttf"), ("Simbolo", "seguisym.ttf")]:
    pdfmetrics.registerFont(TTFont(nome, F / arq))
pdfmetrics.registerFontFamily("Serif", normal="Serif", bold="Serif-B", italic="Serif-I", boldItalic="Serif-B")
pdfmetrics.registerFontFamily("Sans", normal="Sans", bold="Sans-B", italic="Sans-I", boldItalic="Sans-B")

PAPEL = colors.HexColor("#F6F1E7")
TINTA = colors.HexColor("#1E2128")
VERMELHO = colors.HexColor("#B3202A")
CINZA = colors.HexColor("#6B6F78")
LINHA = colors.HexColor("#D9CFBD")
NOITE = colors.HexColor("#12151C")
ZEBRA = colors.HexColor("#EFE8DA")
PAPEL_RGB = (246, 241, 231)

W, H = A4
M = 18 * mm
LARG = W - 2 * M

st = {
    "parte": ParagraphStyle("parte", fontName="Serif-B", fontSize=30, leading=36, textColor=colors.white),
    "h1": ParagraphStyle("h1", fontName="Serif-B", fontSize=21, leading=26, textColor=TINTA, spaceBefore=2, spaceAfter=5),
    "kicker": ParagraphStyle("k", fontName="Sans-B", fontSize=8.5, leading=11, textColor=VERMELHO, spaceAfter=2),
    "h2": ParagraphStyle("h2", fontName="Serif-B", fontSize=14, leading=18, textColor=TINTA, spaceBefore=9, spaceAfter=4),
    "h3": ParagraphStyle("h3", fontName="Sans-B", fontSize=10.5, leading=14, textColor=VERMELHO, spaceBefore=7, spaceAfter=3),
    "p": ParagraphStyle("p", fontName="Serif", fontSize=10.5, leading=15.5, textColor=TINTA, spaceAfter=6),
    "ps": ParagraphStyle("ps", fontName="Sans", fontSize=9.4, leading=13.6, textColor=TINTA, spaceAfter=5),
    "item": ParagraphStyle("item", fontName="Sans", fontSize=9.4, leading=13.4, textColor=TINTA, leftIndent=12,
                           bulletIndent=2, spaceAfter=2.5),
    "lead": ParagraphStyle("lead", fontName="Serif-I", fontSize=12.5, leading=18.5, textColor=TINTA, spaceAfter=8),
    "fala": ParagraphStyle("fala", fontName="Serif-I", fontSize=10.5, leading=15, textColor=TINTA, leftIndent=14, spaceAfter=3),
    "citacao": ParagraphStyle("cit", fontName="Serif-I", fontSize=9.8, leading=14, textColor=TINTA, leftIndent=12,
                              spaceAfter=5, borderPadding=(3, 0, 3, 6)),
    "cell": ParagraphStyle("c", fontName="Sans", fontSize=8.3, leading=10.8, textColor=TINTA),
    "cellb": ParagraphStyle("cb", fontName="Sans-B", fontSize=8.3, leading=10.8, textColor=TINTA),
    "th": ParagraphStyle("th", fontName="Sans-B", fontSize=8.2, leading=10.5, textColor=colors.white),
    "nome": ParagraphStyle("n", fontName="Serif-B", fontSize=12, leading=15, textColor=TINTA),
    "papel": ParagraphStyle("pa", fontName="Sans-B", fontSize=7.8, leading=10, textColor=VERMELHO, spaceAfter=2),
    "bio": ParagraphStyle("b", fontName="Serif", fontSize=9.7, leading=13.4, textColor=TINTA),
    "nota": ParagraphStyle("nota", fontName="Sans-I", fontSize=8.2, leading=11, textColor=CINZA, spaceAfter=4),
    "legenda": ParagraphStyle("leg", fontName="Sans-B", fontSize=7.8, leading=10, textColor=TINTA, alignment=1),
    "sumario": ParagraphStyle("sum", fontName="Serif", fontSize=11, leading=17, textColor=TINTA),
    "mono": ParagraphStyle("mono", fontName="Mono", fontSize=7.8, leading=10.2, textColor=TINTA, backColor=ZEBRA,
                           borderPadding=5, leftIndent=4, spaceAfter=6, spaceBefore=2),
}


def P(t, s="p"):
    return Paragraph(t, st[s])


# ---------- imagens ----------

def _para_flowable(im, largura, pixel):
    if im.mode in ("RGBA", "LA", "P"):
        im = im.convert("RGBA")
        fundo = PILImage.new("RGBA", im.size, PAPEL_RGB + (255,))
        fundo.alpha_composite(im)
        im = fundo
    im = im.convert("RGB")
    alvo = int(largura / mm * 10)   # ~250 dpi
    if pixel:
        fator = max(1, alvo // max(1, im.width))
        im = im.resize((im.width * fator, im.height * fator), PILImage.NEAREST)
    elif im.width > alvo:
        im = im.resize((alvo, int(im.height * alvo / im.width)), PILImage.LANCZOS)
    buf = BytesIO()
    im.save(buf, "PNG" if pixel else "JPEG", **({} if pixel else {"quality": 86}))
    buf.seek(0)
    return Image(buf, width=largura, height=largura * im.height / im.width)


def img(caminho, largura, pixel=False, recorte=None):
    im = PILImage.open(caminho)
    if recorte:
        im = im.crop(recorte)
    return _para_flowable(im, largura, pixel)


MANIFESTO = {}
for linha in (HD / "manifesto.txt").read_text(encoding="utf-8").splitlines():
    p = linha.split()
    if len(p) >= 8 and p[0].endswith(".png"):
        MANIFESTO[p[0][:-4]] = {"fw": int(p[3]), "fh": int(p[4]), "fator": int(p[8]) if len(p) > 8 else 1}


def frame(nome, col, lin, largura):
    """Um frame de um atlas exportado (assets/jogo_hd), em pixel art nítida sobre o papel."""
    m = MANIFESTO[nome]
    fw, fh = m["fw"] * m["fator"], m["fh"] * m["fator"]
    im = PILImage.open(HD / f"{nome}.png").convert("RGBA").crop((col * fw, lin * fh, (col + 1) * fw, (lin + 1) * fh))
    return _para_flowable(im, largura, pixel=True)


def galeria(itens, colunas, largura_img):
    """Grade de figuras com legenda: itens = [(flowable, legenda)]."""
    celulas, linha = [], []
    for fl, leg in itens:
        linha.append([fl, Spacer(1, 2), P(leg, "legenda")])
        if len(linha) == colunas:
            celulas.append(linha)
            linha = []
    if linha:
        celulas.append(linha + [""] * (colunas - len(linha)))
    t = Table(celulas, colWidths=[LARG / colunas] * colunas)
    t.setStyle(TableStyle([("ALIGN", (0, 0), (-1, -1), "CENTER"), ("VALIGN", (0, 0), (-1, -1), "BOTTOM"),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 8)]))
    return t


def tabela(linhas, larguras, cabecalho=True, fonte="cell"):
    dados = [[c if not isinstance(c, str) else
              P(c, "th" if (cabecalho and i == 0) else ("cellb" if j == 0 and fonte == "cell" else fonte))
              for j, c in enumerate(l)] for i, l in enumerate(linhas)]
    t = Table(dados, colWidths=larguras, repeatRows=1 if cabecalho else 0)
    estilo = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("TOPPADDING", (0, 0), (-1, -1), 3.5),
              ("BOTTOMPADDING", (0, 0), (-1, -1), 3.5), ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINHA)]
    if cabecalho:
        estilo.append(("BACKGROUND", (0, 0), (-1, 0), TINTA))
    for i in range(1 if cabecalho else 0, len(linhas)):
        if i % 2 == 0:
            estilo.append(("BACKGROUND", (0, i), (-1, i), ZEBRA))
    t.setStyle(TableStyle(estilo))
    return t


def ficha(retrato_arq, nome, papel, bio, fala=None, sprite=None):
    """Retrato + sprite do jogo + texto."""
    texto = [P(nome, "nome"), P(papel, "papel"), P(bio, "bio")]
    if fala:
        texto += [Spacer(1, 3), P(f"“{fala}”", "fala")]
    figura = [img(A / f"retrato_{retrato_arq}.png", 27 * mm)]
    if sprite:
        figura += [Spacer(1, 2), frame(sprite, 0, 0, 12 * mm)]
    t = Table([[figura, texto]], colWidths=[32 * mm, LARG - 32 * mm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                           ("ALIGN", (0, 0), (0, 0), "CENTER"),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 8), ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINHA)]))
    return KeepTogether(t)


def secao(kicker, titulo):
    return [P(kicker.upper(), "kicker"), P(titulo, "h1")]


def ilustracao(nome, legenda):
    return KeepTogether([img(A / f"{nome}.png", LARG), Spacer(1, 2), P(legenda, "nota")])


# ---------- páginas ----------

def capa(c, doc):
    c.saveState()
    c.setFillColor(NOITE)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    ir = ImageReader(str(A / "capa_menu.png"))
    iw, ih = ir.getSize()
    ah = W * ih / iw
    c.drawImage(ir, 0, H - ah, W, ah)
    c.setFillColor(colors.Color(0.07, 0.08, 0.11, alpha=0.45))
    c.rect(0, H - ah, W, ah, fill=1, stroke=0)
    c.setFillColor(VERMELHO)
    c.rect(M, H - ah - 3 * mm, 40 * mm, 1.6 * mm, fill=1, stroke=0)
    c.setFillColor(colors.white)
    c.setFont("Sans-B", 10)
    c.drawString(M, H - ah - 14 * mm, "HISTÓRIA, LORE E DOCUMENTO DE REQUISITOS DO PRODUTO")
    c.setFont("Serif-B", 40)
    c.drawString(M, H - ah - 32 * mm, "IT: Ecos de Derry")
    c.setFont("Serif-I", 14)
    c.setFillColor(colors.HexColor("#D8D2C4"))
    c.drawString(M, H - ah - 42 * mm, "Um roguelike de ação sobre amizade, medo e uma noite que não termina.")
    c.setFont("Sans", 9.5)
    c.setFillColor(colors.HexColor("#A9ADB6"))
    y = 40 * mm
    for linha in ["PRD v2.1 — 13 de novembro de 2026 (inclui a parte 2 do enunciado)",
                  "Projeto acadêmico · Algoritmos e Programação II · UNIVALI",
                  "Fontes: docs/PRD-v2-IT-Ecos-de-Derry.md e docs/SDD-v1-IT-Ecos-de-Derry.md (§24)"]:
        c.drawString(M, y, linha)
        y -= 5.5 * mm
    c.setFont("Sans", 7.5)
    c.drawString(M, 18 * mm, "Adaptação livre e não oficial da obra IT, de Stephen King, para fins educacionais.")
    c.restoreState()


def interna(c, doc):
    c.saveState()
    c.setFillColor(PAPEL)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    c.setStrokeColor(VERMELHO)
    c.setLineWidth(1.2)
    c.line(M, H - 11 * mm, M + 14 * mm, H - 11 * mm)
    c.setFont("Sans", 7.5)
    c.setFillColor(CINZA)
    c.drawString(M + 17 * mm, H - 11.8 * mm, f"IT: ECOS DE DERRY — {getattr(doc, 'cabecalho', '')}")
    c.drawRightString(W - M, 9 * mm, str(doc.page))
    c.restoreState()


def abertura_parte(c, doc):
    """Página de abertura de parte: fundo noturno com uma ilustração."""
    c.saveState()
    c.setFillColor(NOITE)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    arte = getattr(doc, "arte_parte", "tela_ciclo")
    ir = ImageReader(str(A / f"{arte}.png"))
    iw, ih = ir.getSize()
    ah = W * ih / iw
    c.drawImage(ir, 0, H / 2 - ah / 2 + 20 * mm, W, ah)
    c.setFillColor(colors.Color(0.07, 0.08, 0.11, alpha=0.35))
    c.rect(0, H / 2 - ah / 2 + 20 * mm, W, ah, fill=1, stroke=0)
    c.restoreState()


class Doc(BaseDocTemplate):
    """Guarda o cabeçalho e a arte da abertura da parte atual (mudam por marcadores no fluxo)."""

    def handle_flowable(self, flowables):
        f = flowables[0] if flowables else None
        if isinstance(f, Marcador):
            for k, v in f.valores.items():
                setattr(self, k, v)
        super().handle_flowable(flowables)


class Marcador(Spacer):
    def __init__(self, **valores):
        super().__init__(0, 0)
        self.valores = valores


def pagina_parte(numero, titulo, subtitulo, arte):
    texto = Table([[P(f"PARTE {numero}", "kicker")], [P(titulo, "parte")],
                   [P(f"<font color='#D8D2C4'>{subtitulo}</font>", "lead")]], colWidths=[LARG])
    return [Marcador(arte_parte=arte), NextPageTemplate("parte"), PageBreak(), Spacer(1, H * 0.62), texto,
            NextPageTemplate("interna")]


# ======================================================================================
# PARTE I — A HISTÓRIA
# ======================================================================================

def parte_historia():
    s = pagina_parte("I", "A história de Derry", "A lore do jogo: o mundo, os Perdedores, a Coisa e a noite que não termina.",
                     "tela_ciclo")
    s += [Marcador(cabecalho="PARTE I · A HISTÓRIA"), PageBreak()]

    # 1.1 Derry
    s += secao("Capítulo 1 · O mundo", "Derry, verão")
    s.append(P("Derry é uma cidade pequena do Maine, cortada por um canal e assentada sobre um labirinto de galerias "
               "pluviais mais antigo do que qualquer mapa da prefeitura. Os adultos de Derry têm um talento estranho: "
               "esquecer. Esquecem as crianças que somem, os incêndios, as enchentes, os balões vermelhos presos nas "
               "grades dos bueiros. A cidade inteira parece concordar em não olhar para baixo.", "lead"))
    s.append(P("Debaixo das ruas vive <b>a Coisa</b>. Ninguém sabe o que ela é. Sabe-se o que ela faz: acorda, se "
               "alimenta do medo e volta a dormir. Para se aproximar das crianças, ela prefere a forma de um palhaço "
               "de roupa prateada que se apresenta como <b>Pennywise</b>. Mas a Coisa não tem uma forma só: ela "
               "veste o que cada um mais teme. Para um, é uma múmia; para outro, uma voz no ralo da pia."))
    s.append(P("Neste verão, sete crianças que a escola chama de “Perdedores” descobriram juntas que não estavam "
               "loucas. Elas viram a Coisa. E ela as viu. Em vez de fugir, construíram um clube secreto debaixo da "
               "terra, nos Barrens, e fizeram um juramento: se a Coisa voltasse, eles voltariam também. Cortaram a "
               "palma da mão com um caco de vidro e apertaram as mãos em roda. A cicatriz de Bill nunca fechou direito."))
    s.append(Spacer(1, 4))
    s.append(ilustracao("cartao_clube", "O Clube dos Perdedores: o esconderijo nos Barrens que serve de base (hub) entre as tentativas."))

    # 1.2 A noite
    s += [PageBreak()] + secao("Capítulo 2 · A premissa", "A noite que se repete")
    s.append(P("No fim de uma tarde de tempestade, os sete estão no Clube. Quando escurece, seis deles escutam uma voz "
               "chamando pelo nome, cada um uma voz diferente: a mãe, um amigo, uma música de rádio. Um a um, eles "
               "saem correndo para a noite e não voltam. Ficam presos, cada um no lugar do seu maior medo."))
    s.append(P("Bill também ouve. É a voz de <b>Georgie</b>, o irmão caçula que sumiu num dia de chuva, perto de um "
               "bueiro, correndo atrás de um barquinho de papel que Bill tinha dobrado para ele. Bill sabe que não é "
               "Georgie. É a Coisa usando a culpa dele como isca. Por isso ele não corre: ele desce."))
    s.append(P("O ciclo", "h2"))
    s.append(P("A Coisa fez algo com aquela noite. Toda vez que Bill cai lá embaixo, a cicatriz do juramento arde e ele "
               "acorda de novo no Clube, ao entardecer, antes de tudo começar. Os amigos estão ali, rindo, discutindo "
               "gibis, sem saber de nada. <b>Só Bill lembra.</b>"))
    s.append(P("Mas os amigos guardam <b>ecos</b>: sonhos mal lembrados, uma frase que sai sem querer, uma sensação de "
               "déjà-vu. Lá embaixo, esses ecos alcançam Bill como vozes e lhe dão força. É por isso que o jogo se "
               "chama <i>Ecos de Derry</i>. A cada volta Bill sabe um pouco mais, os amigos dizem coisas novas e a "
               "história avança, até a noite em que ele consegue trazer os seis de volta e encarar a Coisa no covil."))
    s.append(tabela([
        ["Na história", "No jogo"],
        ["A noite se repete quando Bill cai", "Morrer não é o fim: Bill volta ao Clube (o hub) e tenta de novo"],
        ["Só Bill lembra", "Ele guarda Lembranças, a moeda das melhorias permanentes do Mural"],
        ["Os amigos guardam ecos", "As conversas no Clube mudam a cada volta; lá embaixo, os Ecos são poderes (1 entre 3)"],
        ["O juramento de sangue", "Os Pactos de Sangue são as vidas extras de uma tentativa"],
        ["A Coisa veste o medo de cada um", "Cada ambiente é guardado por um chefe que é o medo de um amigo"],
        ["Um amigo sempre por perto", "O companheiro de tentativa (Mike no início; depois, o último resgatado)"],
    ], [62 * mm, LARG - 62 * mm]))
    s.append(Spacer(1, 6))
    s.append(ilustracao("tela_ciclo", "Bill acorda no Clube: a noite começou de novo, e só ele se lembra."))

    # 1.3 Os Perdedores
    s += [PageBreak()] + secao("Capítulo 3 · Os Perdedores", "Sete amigos")
    s.append(ilustracao("tela_perdedores", "Os sete Perdedores. Juntos, são a única coisa de que a Coisa tem medo."))
    s.append(Spacer(1, 4))
    for args in [
        ("bill", "Bill Denbrough", "PROTAGONISTA · JOGÁVEL · O MEDO DELE: GEORGIE",
         "O líder do grupo. Gagueja quando está nervoso e carrega a culpa pelo desaparecimento do irmão. Desce com uma "
         "lanterna e um taco de beisebol. É o único que se lembra da noite que se repete, e o único que pode quebrá-la.",
         "Eu vou trazer todo mundo de volta.", "bill_walk"),
        ("ben", "Ben Hanscom", "RESGATE 1 · GALERIA PLUVIAL · MEDO: A MÚMIA",
         "Tímido, leitor de livros de história e construtor. Conhece as galerias porque desenhou mapas delas na "
         "biblioteca. A Coisa o prende numa galeria que não existe em mapa nenhum, guardada por uma múmia cujas faixas "
         "apertam como abraços que não soltam.", "Eu desenhei essas galerias. Essa parede não estava no mapa.", "ben_idle"),
        ("stan", "Stan Uris", "RESGATE 2 · OS BARRENS · MEDO: O AFOGADO DA CAIXA D'ÁGUA",
         "Metódico, observador de pássaros, acredita que tudo tem uma ordem. O que o apavora é o que não tem: a água "
         "parada da represa, de onde algo afogado sobe sem fazer barulho.",
         "Tudo aqui tem uma ordem. Até a água. Se eu fechar a comporta…", "stan_idle"),
        ("eddie", "Eddie Kaspbrak", "RESGATE 3 · CASA DA RUA NEIBOLT · MEDO: O LEPROSO",
         "Hipocondríaco, nunca larga o inalador. A mãe o convenceu de que ele é frágil; a Coisa sabe disso e o "
         "espera na casa abandonada da Neibolt, na pele de um mendigo doente que oferece “só um beijinho”.",
         "Ele fica dizendo que eu tô doente. Eu não tô doente!", "eddie_idle"),
        ("beverly", "Beverly Marsh", "RESGATE 4 · PORÕES INUNDADOS · MEDO: O RALO DE SANGUE",
         "A mira mais certeira do grupo, com o estilingue sempre no bolso de trás. Entende melhor que ninguém como a "
         "Coisa usa as vozes que a gente quer ouvir. Nos porões, as vozes vêm do ralo, junto com os cabelos.",
         "Ele usa a voz que a gente quer ouvir.", "beverly_idle"),
        ("mike", "Mike Hanlon", "RESGATE 5 · SIDERÚRGICA KITCHENER · MEDO: O PÁSSARO DAS CINZAS",
         "O guardião da história de Derry. É ele quem encontra Bill no bueiro e o acompanha no começo da noite, até "
         "voltar para manter a saída livre. Mais tarde, a Coisa o leva para a siderúrgica onde, anos atrás, um "
         "incêndio matou muita gente, guardada por um pássaro feito de fuligem e brasa.",
         "O fogo… foi aqui que tudo queimou, Bill. Eu li sobre isso.", "mike_idle"),
        ("richie", "Richie Tozier", "RESGATE 6 · CISTERNA DAS VOZES · MEDO: O LOBISOMEM",
         "O piadista do grupo, de óculos fundo de garrafa, que imita vozes para não deixar o medo aparecer. Na "
         "cisterna, onde as paredes repetem tudo o que se diz, a Coisa o espera vestida de lobisomem de filme B.",
         "Bill! Finalmente! Tava ensaiando minha voz de lobisomem.", "richie_idle"),
    ]:
        s.append(ficha(*args))
    s.append(Spacer(1, 4))
    s += [CondPageBreak(70 * mm), P("Quem mais cruza o caminho deles", "h2")]
    for args in [
        ("elias", "Elias", "NPC · CRIADO PARA ESTA ADAPTAÇÃO",
         "O zelador da manutenção dos esgotos, um homem de poucas palavras que não enxerga as ilusões como as crianças, "
         "mas conhece cada grade e cada válvula. Abre a passagem da galeria quando Bill traz a manivela e, ao longo da "
         "noite, aparece entre as salas vendendo suprimentos em troca de moedas de prata.", "Se você trouxer a peça, eu abro.",
         "elias_idle"),
        ("georgie", "Georgie", "ILUSÃO · O MEDO DE BILL",
         "O irmão caçula de capa de chuva amarela e galochas vermelhas, com o barquinho de papel nas mãos. Não é ele: "
         "é a forma mais cruel que a Coisa encontra para Bill, guardada para o fim.", "Billy, me ajuda a pegar o barquinho.",
         "georgie_idle"),
        ("pennywise", "Pennywise, a Coisa", "ANTAGONISTA",
         "A criatura sob Derry. Guarda cada amigo vestindo o medo dele e, a cada encontro, sai de dentro desse medo "
         "um pouco mais forte, como se aprendesse com Bill. No covil, mostra o que é de verdade: luzes que apagam tudo.",
         "Vocês vão esquecer.", None),
    ]:
        s.append(ficha(*args))

    # 1.4 A Coisa e os medos
    s += [PageBreak()] + secao("Capítulo 4 · A Coisa", "Os seis medos e o palhaço por dentro")
    s.append(P("Cada amigo está preso no lugar do seu medo, e esse medo é a Coisa disfarçada. Na metade de cada luta, "
               "o disfarce se rasga como papel molhado e o <b>Pennywise sai de dentro</b>. A cada ambiente ele sai "
               "menor, mais rápido e mais perigoso: a Coisa está aprendendo com Bill. É uma regra da história e "
               "também uma regra do jogo (a dificuldade dos chefes cresce bioma a bioma)."))
    s.append(galeria([
        (frame("medo_mumia", 0, 0, 30 * mm), "A Múmia · Galeria"),
        (frame("medo_afogado", 0, 0, 30 * mm), "O Afogado · Barrens"),
        (frame("medo_leproso", 0, 0, 30 * mm), "O Leproso · Neibolt"),
        (frame("medo_ralo", 0, 0, 30 * mm), "O Ralo de Sangue · Porões"),
        (frame("medo_passaro", 0, 0, 30 * mm), "O Pássaro das Cinzas · Siderúrgica"),
        (frame("medo_lobisomem", 0, 0, 30 * mm), "O Lobisomem · Cisterna"),
    ], 3, 30 * mm))
    s.append(P("O palhaço, cada vez mais forte", "h2"))
    s.append(galeria([
        (frame("pennywise_1_actions", 0, 0, 22 * mm), "Estágio I · biomas 1–2"),
        (frame("pennywise_2_actions", 0, 0, 22 * mm), "Estágio II · biomas 3–4"),
        (frame("pennywise_3_actions", 0, 0, 22 * mm), "Estágio III · biomas 5–6"),
        (frame("pennywise_4_actions", 0, 0, 34 * mm), "Estágio IV · o Covil"),
    ], 4, 22 * mm))
    s.append(P("As crias da Coisa", "h2"))
    s.append(P("Pelos corredores, a Coisa espalha formas menores, que vigiam, perseguem e roubam. O <b>rato</b> "
               "arrasta para a toca qualquer coisa que brilhe no chão; o <b>espectro de fuligem</b> queima o que "
               "encontra pelo caminho; a <b>sombra encharcada</b> arrebenta entulho e caixas."))
    s.append(galeria([
        (frame("balao", 0, 0, 16 * mm), "Balão perverso"), (frame("rato", 0, 0, 16 * mm), "Rato sombrio"),
        (frame("sombra", 0, 0, 16 * mm), "Sombra encharcada"), (frame("sanguessuga", 0, 0, 16 * mm), "Sanguessuga alada"),
        (frame("afogado", 0, 0, 16 * mm), "Afogado"), (frame("boneco_palhaco", 0, 0, 16 * mm), "Boneco palhaço"),
        (frame("cabelo_ralo", 0, 0, 16 * mm), "Cabelo do ralo"), (frame("espectro_fuligem", 0, 0, 16 * mm), "Espectro de fuligem"),
    ], 4, 16 * mm))

    # 1.5 A jornada
    s += [PageBreak()] + secao("Capítulo 5 · A jornada", "Uma noite, oito lugares")
    s.append(P("Prólogo · Rua Witcham", "h2"))
    s.append(P("A tempestade acaba de passar. Bill está ao lado do bueiro onde Georgie sumiu. A água ainda escorre "
               "pela grade, e um balão vermelho sobe contra o vento. Mike chega correndo, sem fôlego: os outros "
               "sumiram, cada um para um lado. Ele não deixa Bill descer sozinho. É aqui que se aprende a andar, a "
               "falar com as pessoas e a enxergar no escuro."))
    s.append(P("Bill! Os outros… eles saíram correndo, cada um pra um lado. Disseram que ouviram alguém chamando. — <b>Mike</b>", "fala"))
    s.append(P("E-eu também ouvi. Era o Georgie. Não era ele. — <b>Bill</b>", "fala"))
    s.append(P("Eu vou com você até achar o Ben. Depois volto e seguro a saída. — <b>Mike</b>", "fala"))
    biomas = [
        ("cartao_galeria", "1 · Galeria pluvial", "Ben",
         "Alvenaria antiga, água pela canela e grades de ferro. Elias, o zelador, abre a grade principal se Bill "
         "encontrar a manivela perdida. No fundo, Ben está encolhido atrás de uma parede que não existia no mapa, e a "
         "Múmia arrasta as faixas pelo corredor. Libertado, Ben baixa a ponte de manutenção para a descida."),
        ("cartao_barrens", "2 · Os Barrens", "Stan",
         "Mata fechada, riacho e a represa que os Perdedores construíram num verão. A água ficou escura e parada. O "
         "Afogado da Caixa d'Água some nela e emerge embaixo de quem estiver perto. Stan, livre, fecha a comporta: o "
         "riacho vira lama, e a lama dá passagem."),
        ("cartao_neibolt", "3 · Casa da Rua Neibolt", "Eddie",
         "Tábuas podres, papel de parede descascando e um porão de terra batida. O Leproso avança em investidas longas "
         "e deixa uma nuvem que pesa nos pulmões. Eddie destranca a porta por dentro e, antes de ir, cuida dos "
         "machucados de Bill."),
        ("cartao_poroes", "4 · Porões inundados", "Beverly",
         "Fundações alagadas, canos que gotejam e um banheiro antigo onde o ralo fala. Cabelos saem em cruz e o chão "
         "fica escorregadio de vermelho. Beverly derruba uma passarela com o estilingue, abre um atalho e entrega a Bill "
         "uma bolinha de prata: “Se ele voltar, mira no meio.”"),
        ("cartao_siderurgica", "5 · Siderúrgica Kitchener", "Mike",
         "Fornalhas, esteiras rolantes e montes de carvão. O Pássaro das Cinzas voa baixo, mergulha e faz chover brasa. "
         "Mike, resgatado, liga o gerador: as lâmpadas acendem uma a uma e a Coisa não tem mais onde se esconder."),
        ("cartao_cisterna", "6 · Cisterna das vozes", "Richie",
         "Tubulações gigantes e paredes que repetem tudo. Algumas paredes nem são paredes. O Lobisomem ataca com "
         "garras e saltos e uiva para chamar ratos. Richie grita para a Coisa que aquilo tudo é papelão, e as paredes "
         "falsas se desfazem."),
    ]
    for arte, titulo, amigo, texto in biomas:
        s.append(CondPageBreak(95 * mm))
        s.append(KeepTogether([P(titulo, "h2"), img(A / f"{arte}.png", LARG), Spacer(1, 3),
                               P(f"<b>Quem está preso:</b> {amigo}. {texto}")]))
    s.append(P("Em cada ambiente o arco é o mesmo: explorar as salas no escuro, achar a <b>Chave do Medo</b>, abrir a "
               "sala trancada onde o amigo está, enfrentar o medo dele e descer. O amigo libertado acompanha Bill até o "
               "próximo resgate, e o anterior volta para o Clube por uma rota segura."))

    # 1.6 Covil e epílogo
    s += [PageBreak()] + secao("Capítulo 6 · O fim da noite", "O Covil")
    s.append(img(A / "cartao_covil.png", LARG))
    s.append(Spacer(1, 4))
    s.append(P("Com os seis amigos a salvo, Bill desce até o covil: uma câmara enorme feita de brinquedos perdidos, "
               "bicicletas tortas e balões imóveis no ar. A entrada se fecha atrás dele. A luta tem três momentos:"))
    s.append(tabela([
        ["Fase", "O que acontece"],
        ["1 · O Palhaço", "O Pennywise no seu estágio mais forte ocupa o centro da arena. Ataca em linhas e em cruz, sempre "
                          "avisando antes, e solta balões perversos."],
        ["2 · Georgie", "A sala escurece. Georgie aparece pedindo ajuda para pegar o barquinho. A voz puxa Bill na direção "
                        "dele, e encostar nele machuca. Bill precisa resistir: <i>“Você não é ele. Você nunca foi ele.”</i>"],
        ["3 · As Luzes Mortas", "A Coisa mostra sua forma verdadeira: três luzes que apagam tudo. Só restam seis círculos "
                                "de luz girando pela arena, um para cada amigo resgatado. Fora deles, Bill se machuca. "
                                "<b>A amizade é, literalmente, o que o protege.</b>"],
    ], [34 * mm, LARG - 34 * mm]))
    s.append(Spacer(1, 4))
    s.append(galeria([(frame("fx_luzes_mortas", 2, 0, 40 * mm), "As Luzes Mortas"),
                      (frame("georgie_idle", 0, 0, 22 * mm), "A ilusão de Georgie"),
                      (frame("pennywise_4_defeat", 0, 0, 34 * mm), "A queda da Coisa")], 3, 30 * mm))
    s.append(P("Vocês vão esquecer. — <b>Pennywise</b>", "fala"))
    s.append(P("Hoje, ninguém ficou para trás. — <b>Bill</b>", "fala"))
    s.append(KeepTogether([P("Epílogo", "h2"), img(A / "epilogo.png", LARG, recorte=(0, 0, 1200, 665))]))
    s.append(Spacer(1, 3))
    s.append(P("Na superfície, o dia está nascendo. Os sete estão juntos na beira do canal. Mike conta todo mundo, duas "
               "vezes: “Estamos todos aqui.” Beverly solta o último balão, que desta vez só sobe. A cicatriz na mão de "
               "Bill para de arder. O ciclo está quebrado."))
    s.append(P("<b>“Derry ainda guarda segredos. Nesta noite, ninguém ficou para trás.”</b>"))

    # 1.7 Como a história é contada
    s += [PageBreak()] + secao("Capítulo 7 · Narrativa", "Morrer faz parte da história")
    s.append(P("Como em <i>Hades</i>, a história não fica só nas cenas: ela se espalha por cada volta ao Clube. Cada "
               "derrota acrescenta uma conversa, uma pista ou uma melhoria, e o jogador sente que avançou mesmo sem vencer."))
    s.append(P("No Clube (o hub)", "h2"))
    s.append(P("Entre uma tentativa e outra, Bill conversa com os amigos. As falas mudam conforme o que aconteceu: quantas "
               "vezes Bill caiu, qual medo o derrotou, quem ele já conseguiu resgatar. Eles não sabem do ciclo, mas sentem:"))
    s.append(P("Sonhei com faixas me apertando. Que esquisito… você tá bem, Bill? — <b>Ben</b>, depois que Bill cai para a Múmia", "fala"))
    s.append(P("Bill, você tá com cara de quem já ouviu essa piada mil vezes. — <b>Richie</b>, depois de várias voltas", "fala"))
    s.append(P("Por que eu sinto que a gente já fez isso antes? — <b>Stan</b>, depois da primeira vitória", "fala"))
    s.append(P("No Clube ficam também o <b>Mural dos Perdedores</b> (melhorias permanentes compradas com Lembranças), o "
               "<b>baú de armas</b> (taco, estilingue da Beverly, cano de ferro), o <b>quadro de recordes</b>, a "
               "<b>estante das Lembrancinhas</b> (os segredos achados em cada bioma) e o <b>diário de Bill</b>."))
    s.append(P("Lá embaixo", "h2"))
    s.append(P("Cada amigo empresta a Bill o seu jeito de ser por meio dos <b>Ecos</b>: Ben dá resistência, Stan dá "
               "precisão, Eddie dá cura e rapidez, Beverly dá mira, Mike dá força e fogo, Richie dá a voz que confunde "
               "os inimigos. O amigo que acompanha Bill (o <b>companheiro</b>) ajuda do seu jeito: Ben ergue uma "
               "barreira, Beverly atira com o estilingue, Eddie cura uma vez por sala, Richie provoca a Coisa."))
    s.append(galeria([(frame("ui_emblemas", i, 0, 14 * mm), n) for i, n in
                      enumerate(["Ben", "Stan", "Eddie", "Beverly", "Mike", "Richie"])], 6, 14 * mm))
    s.append(P("Segredos", "h2"))
    s.append(P("Derry esconde coisas atrás das paredes. Rachaduras discretas levam a <b>salas secretas</b> com tesouros "
               "e uma <b>Lembrancinha</b> por bioma (o barquinho, a figurinha, o anel de Beverly…). Às vezes, uma porta "
               "rara leva à <b>Festa dos Balões</b>, uma fase bônus em que a Coisa, por um instante, brinca de festa."))
    s.append(P("Tom e limites", "h2"))
    s.append(tabela([
        ["Fazemos", "Evitamos"],
        ["Terror pela escuridão, pelo som e pelo que está fora do lugar", "Sangue explícito, ferimentos e violência gráfica"],
        ["Falas curtas e originais, escritas para o jogo", "Reproduzir trechos do livro ou dos filmes"],
        ["A amizade como força contra o medo", "Temas adultos do livro"],
        ["Criaturas e medos estilizados, legíveis em pixel art", "Explicar toda a mitologia da Coisa"],
    ], [LARG / 2, LARG / 2]))
    s.append(Spacer(1, 6))
    s.append(P("<i>IT</i>, Derry, os Perdedores e Pennywise pertencem à obra de Stephen King; este é um projeto acadêmico "
               "sem fins comerciais. Elias, os Ecos, o ciclo da noite, as salas, as regras e os diálogos são criação do "
               "grupo. A estrutura de jogo é inspirada em <i>Hades</i> (Supergiant Games), sem uso de nomes, arte ou "
               "textos daquele jogo. Toda a arte deste documento é a arte do próprio jogo.", "nota"))
    return s


# ======================================================================================
# Markdown → flowables (PRD e SDD)
# ======================================================================================

EMOJIS = {"✅": "<font name='Simbolo' color='#2E7D32'>✔</font>", "❌": "<font name='Simbolo' color='#B3202A'>✘</font>",
          "🟡": "<font name='Simbolo' color='#C08A00'>◐</font>", "⏭": "→", "⚠️": "<b>(!)</b>", "⚠": "<b>(!)</b>"}


def inline(t):
    """Markdown em linha → marcação do reportlab (negrito, itálico, código, links)."""
    codigos = []

    def guarda(m):
        codigos.append(m.group(1))
        return f"\x00{len(codigos) - 1}\x00"

    t = re.sub(r"`([^`]+)`", guarda, t)
    t = t.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    t = re.sub(r"\[([^\]]+)\]\(([^)]+)\)", r"\1", t)
    t = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", t)
    t = re.sub(r"(?<![\w*])\*(?!\s)(.+?)(?<!\s)\*(?![\w*])", r"<i>\1</i>", t)
    for k, v in EMOJIS.items():
        t = t.replace(k, v)

    def devolve(m):
        c = codigos[int(m.group(1))].replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
        return f"<font name='Mono' size='8'>{c}</font>"

    return re.sub(r"\x00(\d+)\x00", devolve, t)


def tabela_md(linhas):
    celulas = [[c.strip() for c in l.strip().strip("|").split("|")] for l in linhas]
    celulas = [c for c in celulas if not all(re.fullmatch(r":?-{2,}:?", x or "--") for x in c)]
    n = max(len(c) for c in celulas)
    celulas = [c + [""] * (n - len(c)) for c in celulas]
    # Largura mínima de cada coluna = a maior PALAVRA dela (palavra curta nunca quebra no meio);
    # o que sobra da largura útil é dividido pelo volume de texto de cada coluna.
    limpo = [[re.sub(r"[*`]", "", c) for c in r] for r in celulas]
    def maior_palavra(texto):
        return max((len(w) for w in texto.split()), default=0)

    minimo = [min(38 * mm, max(max(maior_palavra(r[j]) for r in limpo[1:]) * 1.75 * mm if len(limpo) > 1 else 0,
                               maior_palavra(limpo[0][j]) * 2.25 * mm) + 4.5 * mm)   # cabeçalho é negrito
              for j in range(n)]
    if sum(minimo) > LARG:
        minimo = [x * LARG / sum(minimo) for x in minimo]
    volume = [sum(len(r[j]) for r in limpo) + 1 for j in range(n)]
    sobra = LARG - sum(minimo)
    larg = [minimo[j] + sobra * volume[j] / sum(volume) for j in range(n)]
    dados = [[inline(c) for c in r] for r in celulas]
    return tabela(dados, larg)


def markdown(texto, estilo_p="ps"):
    s, par, lista, i = [], [], [], 0
    linhas = texto.splitlines()

    def fecha_par():
        if par:
            s.append(P(inline(" ".join(par)), estilo_p))
            par.clear()

    while i < len(linhas):
        l = linhas[i]
        if l.startswith("```"):
            fecha_par()
            bloco = []
            i += 1
            while i < len(linhas) and not linhas[i].startswith("```"):
                bloco.append(linhas[i])
                i += 1
            s.append(Preformatted("\n".join(bloco), st["mono"], maxLineLength=118))
        elif l.startswith("|"):
            fecha_par()
            bloco = []
            while i < len(linhas) and linhas[i].startswith("|"):
                bloco.append(linhas[i])
                i += 1
            s += [tabela_md(bloco), Spacer(1, 5)]
            continue
        elif l.startswith("### "):
            fecha_par()
            s += [CondPageBreak(30 * mm), P(inline(l[4:]), "h3")]
        elif l.startswith("## "):
            fecha_par()
            s += [CondPageBreak(45 * mm), P(inline(l[3:]), "h2")]
        elif re.match(r"^\s*([-*]|\d+[.)]|\d+b\.)\s+", l):
            fecha_par()
            m = re.match(r"^(\s*)([-*]|\d+[.)]|\d+b\.)\s+(.*)$", l)
            recuo = len(m.group(1)) // 2
            marca = "•" if m.group(2) in "-*" else m.group(2)
            corpo = [m.group(3)]
            while i + 1 < len(linhas) and linhas[i + 1].startswith("  ") and not re.match(r"^\s*([-*]|\d+[.)])\s+", linhas[i + 1]):
                i += 1
                corpo.append(linhas[i].strip())
            estilo = ParagraphStyle("li", parent=st["item"], leftIndent=12 + 12 * recuo, bulletIndent=2 + 12 * recuo)
            s.append(Paragraph(inline(" ".join(corpo)), estilo, bulletText=marca))
        elif l.startswith(">"):
            fecha_par()
            s.append(P(inline(l.lstrip("> ")), "citacao"))
        elif l.strip() in ("---", "***"):
            fecha_par()
            s.append(Spacer(1, 4))
        elif not l.strip():
            fecha_par()
        else:
            par.append(l.strip())
        i += 1
    fecha_par()
    return s


def secoes_md(texto):
    """Divide um .md em [(titulo_h2, corpo)] pelos cabeçalhos '## '."""
    partes = re.split(r"^## ", texto, flags=re.M)
    return [(p.split("\n", 1)[0].strip(), p.split("\n", 1)[1] if "\n" in p else "") for p in partes[1:]]


# ======================================================================================
# PARTE II — PRD   e   APÊNDICE — parte 2 do enunciado
# ======================================================================================

def parte_prd():
    s = pagina_parte("II", "Documento de requisitos do produto", "PRD v2.1: o que o jogo é, como se joga e o que precisa ser entregue.",
                     "cartao_clube")
    s += [Marcador(cabecalho="PARTE II · PRD"), PageBreak()]
    texto = PRD_MD.read_text(encoding="utf-8")
    for titulo, corpo in secoes_md(texto):
        numero = titulo.split(".")[0]
        s += [PageBreak()] if s and not isinstance(s[-1], PageBreak) and numero in ("0", "5", "8", "11", "14", "16", "19", "21") else []
        s += [CondPageBreak(70 * mm), P(f"PRD § {numero}", "kicker"), P(inline(titulo.split(". ", 1)[-1]), "h1")]
        if numero == "4":   # a história está inteira na Parte I
            s.append(P("A história, o elenco, o prólogo, o roteiro por bioma, o confronto final e as conversas do Clube "
                       "estão na <b>Parte I</b> deste documento, em versão completa e ilustrada. As falas definitivas "
                       "ficam em <font name='Mono' size='8'>data/dialogos.txt</font>.", "ps"))
            continue
        s += markdown(corpo)
    return s


def apendice_parte2():
    s = pagina_parte("III", "Apêndice: parte 2 do enunciado", "Os 15 critérios novos e o piloto automático (SDD §24).",
                     "cartao_covil")
    s += [Marcador(cabecalho="APÊNDICE · PARTE 2 DO ENUNCIADO"), PageBreak()]
    texto = SDD_MD.read_text(encoding="utf-8")
    m = re.search(r"^## 24\..*?$(.*)", texto, flags=re.M | re.S)
    corpo = m.group(1) if m else ""
    corpo = re.sub(r"^#### ", "### ", corpo, flags=re.M)
    corpo = re.sub(r"^### ", "## ", corpo, flags=re.M)
    s += secao("SDD § 24", "Requisitos da parte 2 (13/11/2026)")
    s += markdown(corpo)
    return s


def sumario():
    s = [Marcador(cabecalho="SUMÁRIO"), NextPageTemplate("interna"), PageBreak()]
    s += secao("Como ler este documento", "Sumário")
    s.append(P("A <b>Parte I</b> conta a história e a lore do jogo, ilustrada com a arte do próprio jogo. As partes "
               "seguintes são o documento de requisitos de fato: a <b>Parte II</b> é o PRD v2.1 (produto, regras, "
               "interface, pontuação, prioridades) e a <b>Parte III</b> reúne os requisitos da parte 2 do enunciado, "
               "com a tabela de cobertura e o desenho do piloto automático.", "lead"))
    itens = [("I", "A história de Derry", ["1 · Derry, verão", "2 · A noite que se repete", "3 · Os Perdedores",
                                           "4 · A Coisa: os seis medos e o palhaço", "5 · A jornada: oito lugares",
                                           "6 · O Covil e o epílogo", "7 · Como a história é contada"]),
             ("II", "Documento de requisitos do produto (PRD v2.1)",
              [t for t, _ in secoes_md(PRD_MD.read_text(encoding="utf-8"))]),
             ("III", "Apêndice: parte 2 do enunciado (SDD §24)",
              ["Tabela de cobertura M01–M15 e T05", "Dificuldade", "Tempo e pontuação", "Itens visíveis",
               "Inimigos × itens e segredos", "Companheiro", "Piloto automático", "Áudio", "Entrega e defesa"])]
    for num, titulo, subs in itens:
        s.append(P(f"<b>Parte {num} — {titulo}</b>", "sumario"))
        s.append(P(" · ".join(inline(x) for x in subs), "nota"))
        s.append(Spacer(1, 4))
    return s


def main():
    doc = Doc(str(OUT), pagesize=A4, title="IT: Ecos de Derry — História e PRD",
              author="Diego Silva", subject="Lore, PRD v2.1 e requisitos da parte 2",
              leftMargin=M, rightMargin=M, topMargin=18 * mm, bottomMargin=16 * mm)
    corpo = Frame(M, 15 * mm, LARG, H - 32 * mm, id="corpo")
    doc.addPageTemplates([PageTemplate("capa", [Frame(0, 0, W, H)], onPage=capa),
                          PageTemplate("interna", [corpo], onPage=interna),
                          PageTemplate("parte", [Frame(M, 15 * mm, LARG, H - 32 * mm)], onPage=abertura_parte)])
    historia = [Spacer(1, 1)] + sumario() + parte_historia() + parte_prd() + apendice_parte2()
    doc.build(historia)
    print(OUT, f"{OUT.stat().st_size / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
