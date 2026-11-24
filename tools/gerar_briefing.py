"""Gera docs/Briefing-Historia-IT-Ecos-de-Derry.pdf — briefing de história e mudanças para apresentar o jogo.

Uso:  python tools/gerar_briefing.py
Fonte do conteúdo: docs/PRD-v2-IT-Ecos-de-Derry.md (mantenha os dois coerentes).
"""
from io import BytesIO
from pathlib import Path

from PIL import Image as PILImage
from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib.utils import ImageReader
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, Image, KeepTogether, NextPageTemplate, PageBreak,
                                PageTemplate, Paragraph, Spacer, Table, TableStyle)

ROOT = Path(__file__).resolve().parent.parent
A = ROOT / "assets"
OUT = ROOT / "docs" / "Briefing-Historia-IT-Ecos-de-Derry.pdf"

F = Path("C:/Windows/Fonts")
pdfmetrics.registerFont(TTFont("Serif", F / "georgia.ttf"))
pdfmetrics.registerFont(TTFont("Serif-B", F / "georgiab.ttf"))
pdfmetrics.registerFont(TTFont("Serif-I", F / "georgiai.ttf"))
pdfmetrics.registerFont(TTFont("Sans", F / "segoeui.ttf"))
pdfmetrics.registerFont(TTFont("Sans-B", F / "segoeuib.ttf"))
pdfmetrics.registerFontFamily("Serif", normal="Serif", bold="Serif-B", italic="Serif-I", boldItalic="Serif-B")

PAPEL = colors.HexColor("#F6F1E7")
TINTA = colors.HexColor("#1E2128")
VERMELHO = colors.HexColor("#B3202A")
CINZA = colors.HexColor("#6B6F78")
LINHA = colors.HexColor("#D9CFBD")
NOITE = colors.HexColor("#12151C")

W, H = A4
M = 20 * mm

st = {
    "h1": ParagraphStyle("h1", fontName="Serif-B", fontSize=22, leading=27, textColor=TINTA, spaceBefore=4, spaceAfter=4),
    "kicker": ParagraphStyle("k", fontName="Sans-B", fontSize=8.5, leading=11, textColor=VERMELHO, spaceAfter=2),
    "h2": ParagraphStyle("h2", fontName="Serif-B", fontSize=14, leading=18, textColor=TINTA, spaceBefore=10, spaceAfter=4),
    "p": ParagraphStyle("p", fontName="Serif", fontSize=10.5, leading=15.5, textColor=TINTA, spaceAfter=6),
    "lead": ParagraphStyle("lead", fontName="Serif-I", fontSize=12.5, leading=18, textColor=TINTA, spaceAfter=8),
    "fala": ParagraphStyle("fala", fontName="Serif-I", fontSize=10.5, leading=15, textColor=TINTA, leftIndent=12,
                           borderPadding=(2, 0, 2, 8), spaceAfter=3),
    "cell": ParagraphStyle("c", fontName="Sans", fontSize=8.8, leading=11.5, textColor=TINTA),
    "cellb": ParagraphStyle("cb", fontName="Sans-B", fontSize=8.8, leading=11.5, textColor=TINTA),
    "th": ParagraphStyle("th", fontName="Sans-B", fontSize=8.5, leading=11, textColor=colors.white),
    "nome": ParagraphStyle("n", fontName="Serif-B", fontSize=11.5, leading=14, textColor=TINTA),
    "papel": ParagraphStyle("pa", fontName="Sans-B", fontSize=8, leading=10, textColor=VERMELHO, spaceAfter=2),
    "bio": ParagraphStyle("b", fontName="Serif", fontSize=9.6, leading=13.2, textColor=TINTA),
    "nota": ParagraphStyle("nota", fontName="Sans", fontSize=8.5, leading=11.5, textColor=CINZA),
}


def P(t, s="p"):
    return Paragraph(t, st[s])


def img(path, largura, pixel=False, recorte=None):
    """Carrega imagem do master, opcionalmente recorta e reduz; retorna Image do reportlab."""
    im = PILImage.open(path).convert("RGB")
    if recorte:
        im = im.crop(recorte)
    alvo = int(largura / mm * 12)  # ~300 dpi
    if im.width > alvo:
        im = im.resize((alvo, int(im.height * alvo / im.width)),
                       PILImage.NEAREST if pixel else PILImage.LANCZOS)
    buf = BytesIO()
    im.save(buf, "JPEG", quality=88)
    buf.seek(0)
    return Image(buf, width=largura, height=largura * im.height / im.width)


def retrato(nome):
    p = A / f"retrato_{nome}.png"
    if p.exists():
        return img(p, 26 * mm)
    t = Table([[P("arte<br/>em produção", "nota")]], colWidths=[26 * mm], rowHeights=[26 * mm])
    t.setStyle(TableStyle([("BOX", (0, 0), (-1, -1), 0.6, LINHA), ("ALIGN", (0, 0), (-1, -1), "CENTER"),
                           ("VALIGN", (0, 0), (-1, -1), "MIDDLE"), ("BACKGROUND", (0, 0), (-1, -1), colors.white)]))
    return t


def tabela(linhas, larguras, cabecalho=True):
    dados = [[P(c, "th" if (cabecalho and i == 0) else ("cellb" if j == 0 else "cell")) for j, c in enumerate(l)]
             for i, l in enumerate(linhas)]
    t = Table(dados, colWidths=larguras, repeatRows=1 if cabecalho else 0)
    estilo = [("VALIGN", (0, 0), (-1, -1), "TOP"), ("TOPPADDING", (0, 0), (-1, -1), 4),
              ("BOTTOMPADDING", (0, 0), (-1, -1), 4), ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINHA)]
    if cabecalho:
        estilo += [("BACKGROUND", (0, 0), (-1, 0), TINTA)]
    for i in range(1 if cabecalho else 0, len(linhas)):
        if i % 2 == 0:
            estilo.append(("BACKGROUND", (0, i), (-1, i), colors.HexColor("#EFE8DA")))
    t.setStyle(TableStyle(estilo))
    return t


def ficha(nome_arq, nome, papel, bio, fala=None):
    texto = [P(nome, "nome"), P(papel, "papel"), P(bio, "bio")]
    if fala:
        texto.append(Spacer(1, 2))
        texto.append(P(f"“{fala}”", "bio"))
    t = Table([[retrato(nome_arq), texto]], colWidths=[30 * mm, W - 2 * M - 30 * mm])
    t.setStyle(TableStyle([("VALIGN", (0, 0), (-1, -1), "TOP"), ("LEFTPADDING", (0, 0), (-1, -1), 0),
                           ("BOTTOMPADDING", (0, 0), (-1, -1), 7), ("LINEBELOW", (0, 0), (-1, -1), 0.4, LINHA)]))
    return KeepTogether(t)


def secao(kicker, titulo):
    return [P(kicker.upper(), "kicker"), P(titulo, "h1")]


# ---------- páginas ----------

def capa(c, doc):
    c.saveState()
    c.setFillColor(NOITE)
    c.rect(0, 0, W, H, fill=1, stroke=0)
    ir = ImageReader(str(A / "capa_menu.png"))
    iw, ih = ir.getSize()
    ah = W * ih / iw
    c.drawImage(ir, 0, H - ah, W, ah)
    c.setFillColor(colors.Color(0.07, 0.08, 0.11, alpha=0.55))
    c.rect(0, H - ah, W, ah, fill=1, stroke=0)
    c.setFillColor(VERMELHO)
    c.rect(M, H - ah - 3 * mm, 40 * mm, 1.6 * mm, fill=1, stroke=0)
    c.setFillColor(colors.white)
    c.setFont("Sans-B", 10)
    c.drawString(M, H - ah - 14 * mm, "BRIEFING DE HISTÓRIA E DIREÇÃO DO JOGO")
    c.setFont("Serif-B", 40)
    c.drawString(M, H - ah - 32 * mm, "IT: Ecos de Derry")
    c.setFont("Serif-I", 14)
    c.setFillColor(colors.HexColor("#D8D2C4"))
    c.drawString(M, H - ah - 42 * mm, "Um roguelike de ação sobre amizade, medo e uma noite que não termina.")
    c.setFont("Sans", 9.5)
    c.setFillColor(colors.HexColor("#A9ADB6"))
    y = 40 * mm
    for linha in ["Versão 2 do conceito — 10 de novembro de 2026",
                  "Projeto acadêmico · Algoritmos e Programação II · UNIVALI",
                  "Base: PRD v2 (docs/PRD-v2-IT-Ecos-de-Derry.md)"]:
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
    c.line(M, H - 12 * mm, M + 14 * mm, H - 12 * mm)
    c.setFont("Sans", 7.5)
    c.setFillColor(CINZA)
    c.drawString(M + 17 * mm, H - 12.8 * mm, "IT: ECOS DE DERRY — BRIEFING DE HISTÓRIA")
    c.drawRightString(W - M, 10 * mm, str(doc.page))
    c.restoreState()


def historia():
    s = []
    s += [NextPageTemplate("interna"), PageBreak()]

    # 1 — o jogo em um minuto
    s += secao("O jogo em um minuto", "Sete amigos, seis medos, uma noite em loop")
    s.append(P("Derry, verão. Balões vermelhos aparecem nos bueiros. Ao cair da noite, seis dos sete amigos do "
               "Clube dos Perdedores ouvem vozes e desaparecem — cada um arrastado para o lugar do seu maior medo. "
               "Só Bill Denbrough resiste. Ele desce pelo alçapão do Clube aos subterrâneos da cidade para trazer todos de volta.", "lead"))
    s.append(P("O jogador controla Bill em um <b>roguelike de ação em tempo real</b>, visto de cima, em pixel art de "
               "16 bits. Cada tentativa atravessa seis ambientes, cada um guardado por um medo — que é, na verdade, a "
               "Coisa disfarçada. Quando Bill cai, ele acorda de novo no Clube, ao entardecer, antes de tudo começar. "
               "Só ele lembra. A cada volta, ele está um pouco mais forte, os amigos dizem coisas novas e a história "
               "avança — até que, numa mesma noite, Bill resgate os seis e enfrente o Pennywise no covil."))
    s.append(Spacer(1, 4))
    s.append(tabela([
        ["Gênero", "Roguelike de ação (estrutura inspirada em Hades 1 e 2), uma noite por tentativa"],
        ["Protagonista", "Bill Denbrough, 12 anos — o único personagem controlável"],
        ["Objetivo", "Resgatar Ben, Stan, Eddie, Beverly, Mike e Richie e derrotar o Pennywise"],
        ["Duração", "45 a 60 minutos para uma tentativa vitoriosa"],
        ["Tom", "Terror pela escuridão, pelo som e pela sugestão; sem sangue explícito nem violência gráfica"],
        ["Plataforma", "Windows, C++17, teclado e joystick"],
    ], [32 * mm, W - 2 * M - 32 * mm], cabecalho=False))

    # 2 — o que mudou
    s += [PageBreak()] + secao("O que mudou", "Da versão 1 para a versão 2")
    s.append(P("O primeiro conceito (PRD v1, outubro de 2026) era um jogo curto e por turnos, com quatro mapas e três "
               "amigos para resgatar. Para chegar à experiência que o grupo quer — algo próximo da dinâmica de "
               "<i>Hades</i> — o projeto foi revisto. Os pontos que mais afetam a história estão em destaque."))
    s.append(tabela([
        ["Tema", "Versão 1", "Versão 2"],
        ["Ritmo do combate", "Por turnos: os inimigos só agiam quando Bill agia", "Tempo real, com esquiva (dash), combos e ataques avisados"],
        ["<b>O que acontece ao morrer</b>", "Fim definitivo; recomeçar do zero", "<b>Bill acorda no Clube — a noite se repete.</b> Ele guarda Lembranças e a história avança"],
        ["<b>Amigos a resgatar</b>", "Três: Ben, Eddie e Beverly", "<b>Seis: entram Stan, Mike e Richie</b>"],
        ["<b>Vilões</b>", "Pennywise em 4 aparições + 3 criaturas", "<b>Seis medos</b> (um por amigo), Pennywise em 3 fases no final e 8 tipos de criaturas"],
        ["Cenários", "Galeria, porões e cisterna", "Rua de Derry, Clube, Galeria, Barrens, Casa da Neibolt, Porões, Siderúrgica, Cisterna e Covil"],
        ["Papel de Mike", "Guia que ficava na superfície", "Guia no prólogo — e depois ele também é levado (pelo medo do fogo)"],
        ["Mapas", "Quatro telas fixas", "Salas desenhadas à mão, sorteadas a cada tentativa"],
        ["Duração", "25 a 40 minutos", "45 a 60 minutos"],
        ["Novidades de interface", "—", "Tutorial em caixas de diálogo, menu de itens em jogo, joystick"],
    ], [34 * mm, 52 * mm, W - 2 * M - 86 * mm]))
    s.append(Spacer(1, 6))
    s.append(P("O que se manteve: Bill como protagonista, Elias (o zelador criado para o jogo), a manivela e a grade "
               "da galeria, a regra de que <b>o Pennywise fica mais forte a cada reencontro</b>, a escuridão que se "
               "revela com a lanterna e o final na câmara dos balões."))

    # 3 — a lore
    s += [PageBreak()] + secao("A história", "A noite que se repete")
    s.append(P("A premissa", "h2"))
    s.append(P("No fim de uma tarde de verão, os sete Perdedores estão reunidos no clube secreto que construíram "
               "debaixo da terra, no Barrens — um terreno baldio de mata e riacho na beira de Derry. Quando escurece, "
               "seis deles escutam uma voz chamando. Cada um ouve uma coisa diferente. Um a um, eles saem correndo e "
               "somem."))
    s.append(P("Bill também ouve uma voz: a do irmão caçula, Georgie, que desapareceu perto de um bueiro correndo "
               "atrás de um barquinho de papel. Mas Bill sabe que não é ele. É a Coisa — a criatura que vive sob a "
               "cidade, se alimenta do medo e costuma aparecer como um palhaço chamado Pennywise."))
    s.append(P("O ciclo", "h2"))
    s.append(P("A Coisa prendeu aquela noite em um <b>ciclo</b>. Toda vez que Bill cai lá embaixo, a cicatriz na palma "
               "da mão — de um juramento de sangue que os amigos fizeram — arde, e ele acorda de novo no Clube, ao "
               "entardecer, antes de tudo começar."))
    s.append(P("Só Bill lembra. Os amigos estão ali, vivos, conversando e rindo, sem saber o que vai acontecer. Mas "
               "carregam <b>ecos</b> do que Bill viveu com eles — como sonhos mal lembrados. É desses ecos que vem a "
               "força dele: lá embaixo, a voz de cada amigo o alcança e lhe dá poderes. Daí o nome do jogo."))
    s.append(P("Para quebrar o ciclo, Bill precisa resgatar os seis <b>na mesma noite</b> e enfrentar a Coisa no covil."))
    s.append(P("Por que isso importa para o jogo", "h2"))
    s.append(tabela([
        ["Na história", "No jogo"],
        ["A noite se repete quando Bill cai", "Morrer não é o fim: Bill volta ao Clube (o hub) e tenta de novo"],
        ["Só Bill lembra", "Ele mantém Lembranças, a moeda de melhorias permanentes no Mural do Clube"],
        ["Os amigos guardam ecos", "As conversas no Clube mudam a cada volta; lá embaixo, os Ecos são poderes escolhidos (1 entre 3)"],
        ["O juramento de sangue", "Os Pactos de Sangue são vidas extras dentro de uma tentativa"],
        ["A Coisa assume a forma do medo", "Cada ambiente é guardado por um chefe que é o medo de um amigo"],
    ], [62 * mm, W - 2 * M - 62 * mm]))

    # 4 — personagens
    s += [PageBreak()] + secao("Personagens", "Os Perdedores e quem cruza o caminho deles")
    for args in [
        ("bill", "Bill Denbrough", "PROTAGONISTA · JOGÁVEL",
         "O líder do grupo. Gagueja quando está nervoso e carrega a culpa pelo desaparecimento de Georgie. Desce com "
         "uma lanterna e um taco de beisebol. É o único que se lembra da noite que se repete.",
         "Eu vou trazer todo mundo de volta."),
        ("ben", "Ben Hanscom", "RESGATE 1 · GALERIA PLUVIAL · MEDO: A MÚMIA",
         "Tímido, leitor e construtor. Conhece as galerias melhor do que ninguém porque desenhou mapas delas. Ao ser "
         "resgatado, baixa uma ponte e abre o caminho.", "Eu desenhei essas galerias. Essa parede não estava no mapa."),
        ("stan", "Stan Uris", "RESGATE 2 · OS BARRENS · MEDO: O AFOGADO DA CAIXA D'ÁGUA",
         "Metódico, organizado, observador de pássaros. Acredita que tudo tem uma ordem — até a água. Ao ser "
         "resgatado, fecha a comporta da represa e o riacho vira lama atravessável.",
         "Vai. Eu sei voltar sozinho, eu anotei o caminho."),
        ("eddie", "Eddie Kaspbrak", "RESGATE 3 · CASA DA RUA NEIBOLT · MEDO: O LEPROSO",
         "Hipocondríaco, nunca larga o inalador, mas é corajoso quando importa. Ao ser resgatado, destranca a porta "
         "por dentro e cuida dos ferimentos de Bill.", "Você tá machucado. Para. Deixa eu ajudar antes de ir."),
        ("beverly", "Beverly Marsh", "RESGATE 4 · PORÕES INUNDADOS · MEDO: O RALO DE SANGUE",
         "A melhor mira do grupo, com seu estilingue. Entende como a Coisa usa as vozes que a gente quer ouvir. Ao "
         "ser resgatada, derruba uma passarela para abrir um atalho e entrega munição de prata.",
         "Ele usa a voz que a gente quer ouvir."),
        ("mike", "Mike Hanlon", "RESGATE 5 · SIDERÚRGICA KITCHENER · MEDO: O PÁSSARO DAS CINZAS",
         "O guardião da história de Derry. É ele quem orienta Bill no prólogo — mas, no meio da noite, também é "
         "levado, para o lugar do seu medo do fogo. Ao ser resgatado, liga um gerador e as luzes revelam tudo.",
         "Tem um gerador. Se a luz voltar, ele não se esconde."),
        ("richie", "Richie Tozier", "RESGATE 6 · CISTERNA DAS VOZES · MEDO: O LOBISOMEM",
         "O piadista, de óculos fundo de garrafa, que imita vozes para disfarçar o medo. Ao ser resgatado, grita "
         "com a Coisa e as paredes falsas da cisterna se desfazem.", "Ei, Coisa! Isso aqui é só papelão!"),
    ]:
        s.append(ficha(*args))
    s.append(Spacer(1, 6))
    extras = [
        ("elias", "Elias", "NPC · CRIADO PARA ESTA ADAPTAÇÃO",
         "O zelador da manutenção dos esgotos. Não enxerga as ilusões como as crianças, mas conhece cada grade. No "
         "início, abre a passagem quando Bill traz a manivela; ao longo da noite, aparece entre as salas vendendo "
         "suprimentos em troca de moedas de prata.", "Se você trouxer a peça, eu abro."),
        ("georgie", "Georgie", "ILUSÃO · O MEDO DE BILL",
         "O irmão caçula de capa de chuva amarela. Não é ele de verdade: é a forma mais cruel que a Coisa encontra "
         "para Bill, usada na batalha final.", "Billy, me ajuda a pegar o barquinho."),
        ("pennywise", "Pennywise, a Coisa", "ANTAGONISTA",
         "A criatura sob Derry. Guarda cada amigo assumindo a forma do medo dele e, a cada encontro, sai de dentro "
         "dessa forma mais forte. No covil, mostra o que é de verdade.", "Vocês vão esquecer."),
    ]
    s.append(KeepTogether([P("Quem mais aparece", "h2"), ficha(*extras[0])]))
    for args in extras[1:]:
        s.append(ficha(*args))

    # 5 — jornada
    s += [PageBreak()] + secao("A jornada", "Uma noite, sete lugares")
    s.append(P("Prólogo — Rua de Derry", "h2"))
    s.append(P("A abertura antecipa Bill descendo pelo alçapão do Clube. O prólogo jogável volta alguns minutos: "
               "uma tempestade termina e Bill procura pistas junto ao bueiro onde Georgie perdeu o barquinho. Mike "
               "chega correndo: os outros sumiram. Ele indica o zelador Elias e a entrada de manutenção. Bill volta "
               "ao Clube para descer pelo alçapão. É aqui que o jogador aprende a andar, interagir e enxergar no escuro."))
    s.append(P("Bill! Os outros… eles saíram correndo, cada um pra um lado. — <b>Mike</b>", "fala"))
    s.append(P("E-eu também ouvi. Era o Georgie. Não era ele. — <b>Bill</b>", "fala"))
    s.append(P("Os seis ambientes", "h2"))
    s.append(P("Cada ambiente segue o mesmo arco: explorar salas no escuro, encontrar a <b>Chave do Medo</b>, abrir a "
               "sala onde o amigo está preso, resgatá-lo e enfrentar o medo dele. No meio da luta, o medo se rasga "
               "como papel molhado e <b>o Pennywise sai de dentro</b> — menor, mais rápido e mais perigoso a cada "
               "ambiente."))
    s.append(tabela([
        ["#", "Ambiente", "Amigo", "O medo", "O que muda com o resgate"],
        ["1", "Galeria pluvial — alvenaria, água e grades", "Ben", "A Múmia: faixas que prendem, andar arrastado", "Ben baixa a ponte (antes, Elias abre a grade)"],
        ["2", "O Barrens — mata, riacho, lama e represa", "Stan", "O Afogado da Caixa d'Água: some na água e emerge por baixo", "A comporta fecha e o riacho vira lama"],
        ["3", "Casa da Rua Neibolt — tábuas podres e papel de parede", "Eddie", "O Leproso: investidas e uma nuvem que deixa lento", "A porta é destrancada por dentro"],
        ["4", "Porões inundados — canos e banheiro antigo", "Beverly", "O Ralo de Sangue: cabelos que chicoteiam do ralo", "A passarela cai e abre um atalho"],
        ["5", "Siderúrgica Kitchener — fornalhas, esteiras, carvão", "Mike", "O Pássaro das Cinzas: voa, mergulha e faz chover brasas", "O gerador liga e a luz revela a sala"],
        ["6", "Cisterna das vozes — tubulações e ilusões", "Richie", "O Lobisomem: garras, saltos e um uivo que chama ratos", "As paredes falsas se desfazem"],
    ], [7 * mm, 40 * mm, 15 * mm, 55 * mm, W - 2 * M - 117 * mm]))
    s.append(Spacer(1, 4))
    s.append(P("Pelo caminho aparecem as criaturas comuns da Coisa: balões perversos que estouram, ratos sombrios, "
               "sombras encharcadas que quebram entulho, sanguessugas aladas, afogados, bonecos de palhaço, cabelos "
               "que saem dos ralos e espectros de fuligem que deixam brasas no chão."))

    s += [PageBreak()]
    s.append(P("O Covil — o confronto final", "h2"))
    s.append(P("Com os seis amigos a salvo, Bill chega ao covil: uma câmara cheia de brinquedos perdidos e balões "
               "imóveis. A entrada se fecha atrás dele. A luta tem três momentos:"))
    s.append(tabela([
        ["Fase", "O que acontece"],
        ["1 · O Palhaço", "O Pennywise em sua forma mais poderosa ocupa o centro da arena. Ataca em linhas e em cruz, "
                          "sempre avisando antes, e solta balões perversos."],
        ["2 · Georgie", "A sala escurece. Georgie aparece chamando Bill para ajudá-lo a pegar o barquinho. A voz puxa "
                        "Bill na direção dele — e encostar nele machuca. Bill precisa resistir: "
                        "<i>“Você não é ele. Você nunca foi ele.”</i>"],
        ["3 · As Luzes Mortas", "A Coisa mostra sua forma verdadeira: luzes que apagam tudo. Só restam seis círculos "
                                "de luz girando pela arena — um para cada amigo resgatado. Fora deles, Bill se "
                                "machuca. <b>A amizade é, literalmente, o que o protege.</b>"],
    ], [34 * mm, W - 2 * M - 34 * mm]))
    s.append(Spacer(1, 6))
    s.append(P("Vocês vão esquecer. — <b>Pennywise</b>", "fala"))
    s.append(P("Hoje, ninguém ficou para trás. — <b>Bill</b>", "fala"))
    s.append(P("Epílogo", "h2"))
    ep = img(A / "epilogo.png", W - 2 * M, recorte=(0, 0, 1200, 665))
    s.append(ep)
    s.append(P("Ilustração da versão 1, com cinco crianças; a versão com os sete Perdedores está na fila de produção.", "nota"))
    s.append(Spacer(1, 4))
    s.append(P("Na superfície, os sete estão juntos. Mike conta todos — “Estamos todos aqui.” Beverly solta o último "
               "balão. O ciclo está quebrado."))
    s.append(P("<b>“Derry ainda guarda segredos. Nesta noite, ninguém ficou para trás.”</b>"))

    # 6 — como a história é contada
    s += [PageBreak()] + secao("Como a história é contada", "Morrer faz parte da narrativa")
    s.append(P("Como em <i>Hades</i>, a história não é contada só em cenas: ela se espalha por cada volta ao Clube."))
    s.append(P("No Clube dos Perdedores (o hub)", "h2"))
    s.append(P("Entre uma tentativa e outra, Bill pode conversar com cada amigo. As falas mudam conforme o que "
               "aconteceu: quantas vezes Bill caiu, qual medo o derrotou, quem ele já conseguiu resgatar. Os amigos "
               "não sabem do ciclo, mas sentem algo estranho:"))
    s.append(P("Sonhei com faixas me apertando. Que esquisito… você tá bem, Bill? — <b>Ben</b>, depois que Bill cai para a Múmia", "fala"))
    s.append(P("Bill, você tá com cara de quem já ouviu essa piada mil vezes. — <b>Richie</b>, depois de várias voltas", "fala"))
    s.append(P("Por que eu sinto que a gente já fez isso antes? — <b>Stan</b>, depois da primeira vitória", "fala"))
    s.append(P("No Clube também ficam o <b>Mural dos Perdedores</b> (melhorias permanentes compradas com Lembranças), "
               "o <b>baú de armas</b> (taco, estilingue da Beverly, cano de ferro), o <b>quadro de recordes</b> e o "
               "<b>diário de Bill</b>, que registra criaturas, itens e falas já vistas."))
    s.append(P("Lá embaixo", "h2"))
    s.append(P("Cada amigo empresta seu jeito de ser a Bill através dos <b>Ecos</b> — Ben dá resistência, Stan dá "
               "precisão, Eddie dá cura e rapidez, Beverly dá mira, Mike dá força e fogo, Richie dá a voz que confunde "
               "os inimigos. Na primeira noite, um tutorial em caixas de diálogo — com Mike falando pelo walkie-talkie "
               "— explica cada coisa no momento em que ela aparece."))
    s.append(P("Tom e limites", "h2"))
    s.append(tabela([
        ["Fazemos", "Evitamos"],
        ["Terror pela escuridão, pelo som e pelo que está fora do lugar", "Sangue explícito, ferimentos e violência gráfica"],
        ["Falas curtas e originais, escritas para o jogo", "Reproduzir trechos do livro ou dos filmes"],
        ["A amizade como força contra o medo", "Temas adultos do livro (a campanha adulta, abusos, rituais longos)"],
        ["Criaturas e medos estilizados, legíveis em pixel art", "Explicar toda a mitologia da Coisa"],
    ], [(W - 2 * M) / 2, (W - 2 * M) / 2]))
    s.append(Spacer(1, 8))
    s.append(P("<i>IT</i>, Derry, os Perdedores e Pennywise pertencem à obra de Stephen King; este é um projeto acadêmico "
               "sem fins comerciais. Elias, os Ecos, o ciclo da noite, as salas, as regras e os diálogos são criação "
               "do grupo. A estrutura de jogo é inspirada em <i>Hades</i> (Supergiant Games), sem uso de nomes, arte "
               "ou textos daquele jogo.", "nota"))

    # 7 — status
    s += [PageBreak()] + secao("Onde estamos", "Status do projeto")
    s.append(tabela([
        ["Frente", "Situação em 10/11/2026"],
        ["Documento de produto (PRD v2)", "Escrito — aguardando aprovação do grupo"],
        ["Documento técnico (SDD v1)", "Escrito — arquitetura, regras, testes e plano em 17 etapas"],
        ["Arte já pronta", "Bill, Ben, Eddie, Beverly, Mike, Elias, Pennywise (4 estágios), 3 criaturas, 3 cenários, "
                           "itens, efeitos, retratos, capa e epílogo"],
        ["Arte a produzir", "Richie, Stan, Georgie, os 6 medos, 5 criaturas, 6 cenários novos, interface e "
                            "ilustrações dos ambientes (50 imagens, prompts prontos)"],
        ["Código", "Começa pela Etapa 1 (janela, desenho, controles) após a aprovação"],
        ["Primeira versão jogável", "Prólogo + Galeria pluvial completa (resgate do Ben e a Múmia)"],
    ], [52 * mm, W - 2 * M - 52 * mm]))
    s.append(Spacer(1, 10))
    ref = img(A / "pennywise_referencia.png", W - 2 * M)
    s.append(ref)
    s.append(P("Referência visual do Pennywise já aprovada para o jogo.", "nota"))
    return s


def main():
    doc = BaseDocTemplate(str(OUT), pagesize=A4, title="IT: Ecos de Derry — Briefing de história",
                          author="Diego Silva", subject="História, personagens e mudanças do PRD v2")
    corpo = Frame(M, 16 * mm, W - 2 * M, H - 36 * mm, id="corpo")
    doc.addPageTemplates([PageTemplate("capa", [Frame(0, 0, W, H)], onPage=capa),
                          PageTemplate("interna", [corpo], onPage=interna)])
    doc.build([Spacer(1, 1)] + historia())
    print(OUT)


if __name__ == "__main__":
    main()
