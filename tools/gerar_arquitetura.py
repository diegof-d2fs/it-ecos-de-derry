"""Gera docs/imagens/arquitetura_aws.svg: o diagrama da versão web na AWS (README, seção "Arquitetura na AWS").

Desenhado à mão em SVG (sem IA): caixas em grupos tracejados, setas com rótulo. Rodar de novo depois de mudar a
arquitetura (web/infra.yaml, tools/publicar_web.sh).

Uso: python tools/gerar_arquitetura.py
"""
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parent.parent
DESTINO = ROOT / "docs" / "imagens" / "arquitetura_aws.svg"
L, A = 1400, 860
partes = []


def grupo(x, y, w, h, titulo):
    partes.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" class="grupo"/>')
    partes.append(f'<text x="{x + 14}" y="{y + 22}" class="titulo-grupo">{escape(titulo)}</text>')


def caixa(x, y, w, h, titulo, linhas, destaque=False):
    partes.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="6" class="{"destaque" if destaque else "caixa"}"/>')
    cx = x + w / 2
    ty = y + 26 if linhas else y + h / 2 + 5
    partes.append(f'<text x="{cx}" y="{ty}" class="titulo">{escape(titulo)}</text>')
    for i, linha in enumerate(linhas):
        partes.append(f'<text x="{cx}" y="{ty + 20 + 17 * i}" class="texto">{escape(linha)}</text>')


def seta(pontos, rotulo=None, pos=None, tracejada=False, ancora="middle"):
    d = " ".join(f"{'M' if i == 0 else 'L'}{x},{y}" for i, (x, y) in enumerate(pontos))
    partes.append(f'<path d="{d}" class="{"seta tracejada" if tracejada else "seta"}" marker-end="url(#ponta)"/>')
    if rotulo:
        x, y = pos
        partes.append(f'<text x="{x}" y="{y}" class="rotulo" text-anchor="{ancora}">{escape(rotulo)}</text>')


# ---------- topo ----------
partes.append(f'<text x="{L / 2}" y="38" class="principal">Arquitetura da versão web na AWS (us-east-1)</text>')
caixa(530, 62, 340, 62, "Jogador: navegador", ["teclado ou controle · computador, celular, tablet"])
caixa(1030, 62, 320, 62, "Wiki ilustrada", ["GitHub Pages, publicada pelo GitHub Actions"])
seta([(870, 93), (1026, 93)], "Ler lore (nova aba)", (948, 84))

# ---------- borda ----------
grupo(40, 150, 1320, 140, "Borda")
caixa(70, 190, 300, 78, "Route 53 e ACM", ["ecosdederry.click: domínio, DNS", "e certificado TLS"])
caixa(550, 190, 300, 78, "CloudFront", ["HTTPS, HTTP/2 e 3, compressão,", "cabeçalhos de segurança"])
caixa(1030, 190, 300, 78, "S3: site do jogo", ["bucket privado (OAC): HTML, WASM,", "dados do jogo e músicas"])
seta([(700, 124), (700, 186)], "HTTPS", (712, 160), ancora="start")
seta([(370, 229), (546, 229)])
seta([(850, 229), (1026, 229)], "arquivos do jogo", (938, 220))

# ---------- contas ----------
grupo(40, 330, 620, 150, "Contas")
caixa(70, 372, 290, 86, "Amazon Cognito", ["login gerenciado em pt-BR,", "apelido ou e-mail, código por e-mail"])
caixa(390, 372, 240, 86, "Lambda PreSignUp", ["valida o apelido e recusa", "e-mail já cadastrado"])
seta([(360, 415), (386, 415)])

# ---------- API ----------
grupo(700, 330, 660, 150, "API (dados de cada conta)")
caixa(730, 372, 280, 86, "API Gateway (HTTP API)", ["exige o token JWT do Cognito,", "até 20 pedidos por segundo"])
caixa(1050, 372, 280, 86, "Lambda da API (Python 3.13)", ["memory card, perfil, ranking,", "feedback e demonstrações"], destaque=True)
seta([(1010, 415), (1046, 415)])

# jogador → login e → API (caminhos que saem do jogador pela esquerda e pela direita do CloudFront)
seta([(530, 104), (215, 104), (215, 368)], "login: OAuth 2.0 com PKCE", (225, 140), ancora="start")
seta([(870, 112), (980, 112), (980, 136), (1380, 136), (1380, 310), (870, 310), (870, 368)],
     "pedidos com o token (fetch)", (1120, 304))

# ---------- dados ----------
grupo(700, 520, 660, 150, "Dados")
caixa(730, 562, 280, 86, "S3: saves (privado)", ["memory card por conta (ETag contra", "conflito), ranking e feedbacks"])
caixa(1050, 562, 280, 86, "Amazon SES", ["avisa cada feedback", "por e-mail"])
seta([(1130, 458), (1130, 520), (870, 520), (870, 558)], "lê e grava", (1000, 512))
seta([(1250, 458), (1250, 558)])

# ---------- operação e custos ----------
grupo(40, 520, 620, 150, "Operação e custos")
caixa(70, 562, 290, 86, "CloudWatch e SNS", ["logs por 30 dias, 4 alarmes", "de capacidade, alerta por e-mail"])
caixa(390, 562, 240, 86, "AWS Budgets", ["US$ 12,50 por mês, só o que", "tem a etiqueta do projeto"])
seta([(1090, 458), (1090, 500), (215, 500), (215, 558)], "logs e métricas", (650, 492), tracejada=True)

# ---------- responsável ----------
caixa(530, 730, 340, 52, "Responsável (e-mail)", [])
seta([(215, 648), (215, 756), (526, 756)])
seta([(510, 648), (510, 742), (526, 742)])
seta([(1190, 648), (1190, 756), (874, 756)], "feedbacks", (1030, 748))

partes.append(f'<text x="{L / 2}" y="{A - 22}" class="nota">Implantação: CloudFormation (web/infra.yaml) com tools/configurar_nuvem.sh; '
              f'site com tools/publicar_web.sh (S3 + invalidação do CloudFront). Todo recurso leva a etiqueta projeto=ecos-de-derry.</text>')

svg = f"""<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {L} {A}" width="{L}" height="{A}" font-family="Arial, Helvetica, sans-serif">
<defs>
  <marker id="ponta" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="8" markerHeight="8" orient="auto-start-reverse">
    <path d="M0,0 L10,5 L0,10 z" fill="#444"/>
  </marker>
  <style>
    .grupo {{ fill: none; stroke: #8a8a8a; stroke-width: 1.4; stroke-dasharray: 7 5; }}
    .titulo-grupo {{ font-size: 15px; font-weight: bold; fill: #333; }}
    .caixa {{ fill: #f1f1f1; stroke: #6b6b6b; stroke-width: 1.3; }}
    .destaque {{ fill: #e3ecf7; stroke: #4a6a94; stroke-width: 1.5; }}
    .titulo {{ font-size: 16px; font-weight: bold; fill: #111; text-anchor: middle; }}
    .texto {{ font-size: 13.5px; fill: #333; text-anchor: middle; }}
    .seta {{ fill: none; stroke: #444; stroke-width: 1.4; }}
    .tracejada {{ stroke-dasharray: 5 4; }}
    .rotulo {{ font-size: 13px; fill: #333; paint-order: stroke; stroke: #fff; stroke-width: 5px; }}
    .principal {{ font-size: 21px; font-weight: bold; fill: #111; text-anchor: middle; }}
    .nota {{ font-size: 13px; fill: #555; text-anchor: middle; }}
  </style>
</defs>
<rect width="{L}" height="{A}" fill="#fff"/>
{chr(10).join(partes)}
</svg>
"""
DESTINO.parent.mkdir(parents=True, exist_ok=True)
DESTINO.write_text(svg, encoding="utf-8")
print(f"{DESTINO.relative_to(ROOT)}: {len(svg)} bytes")
