"""Gera a wiki do GitHub Pages a partir das cenas atuais do jogo.

As imagens são cópias da arte já existente. Este script não gera imagens novas.
"""
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ORIGEM = ROOT / "assets" / "jogo_web"
DESTINO = ROOT / "wiki" / "assets"
DESTINO.mkdir(parents=True, exist_ok=True)
for fonte in ("Gelasio.ttf", "NotoSans.ttf", "OFL-Gelasio.txt", "OFL-NotoSans.txt"):
    shutil.copyfile(ROOT / "third_party" / "fontes" / fonte, DESTINO / fonte)


def imagem(nome):
    fonte = ORIGEM / f"{nome}.png"
    if not fonte.exists():
        return None
    formato = ".jpg" if fonte.read_bytes()[:3] == b"\xff\xd8\xff" else ".png"
    alvo = DESTINO / f"{nome}{formato}"
    shutil.copyfile(fonte, alvo)
    return f"assets/{alvo.name}"


capitulos = []
atual = None
for linha in (ROOT / "data" / "cenas.txt").read_text(encoding="utf-8-sig").splitlines():
    linha = linha.strip()
    if not linha or linha.startswith("#"):
        continue
    if linha.startswith("["):
        ident, titulo = linha.split("]", 1)
        atual = {"id": ident[1:], "titulo": titulo.strip(), "cenas": []}
        capitulos.append(atual)
        continue
    campos = linha.split(";", 3)
    if atual and len(campos) == 4 and campos[0] != "-":
        atual["cenas"].append({"imagem": imagem(campos[0]), "texto": campos[3]})

locais = [
    ("clube", "Clube dos Perdedores", "O refúgio onde a noite recomeça e o Mural guarda as Lembranças."),
    ("galeria", "Galeria pluvial", "Grades, água escura e as faixas da Múmia. Ben espera aqui."),
    ("barrens", "Barrens", "Riacho, lama e a caixa d'água que aprisiona Stan."),
    ("neibolt", "Rua Neibolt", "Uma casa abandonada, o inalador de Eddie e a voz do Leproso."),
    ("poroes", "Porões inundados", "Canos, passarelas e o ralo que chama Beverly."),
    ("siderurgica", "Siderúrgica Kitchener", "Cinzas, esteiras e o ninho do Pássaro que guarda Mike."),
    ("cisterna", "Cisterna das vozes", "Paredes falsas, ecos e o uivo que imita Richie."),
    ("covil", "O Covil", "Brinquedos perdidos, a capa amarela e a última forma da Coisa."),
]
locais = [{"nome": titulo, "descricao": desc, "imagem": imagem("cartao_" + chave)}
          for chave, titulo, desc in locais]

personagens = [
    ("bill", "Bill Denbrough", "O único que se lembra do ciclo. Desce com uma lanterna e um taco para trazer os amigos de volta."),
    ("ben", "Ben Hanscom", "Leitor e construtor, conhece as galerias e deixa para Bill seus mapas e sua resistência."),
    ("stan", "Stan Uris", "Observador de pássaros, procura ordem até na água parada do Barrens."),
    ("eddie", "Eddie Kaspbrak", "Carrega o inalador, cuida dos ferimentos de Bill e encontra coragem na casa da Rua Neibolt."),
    ("beverly", "Beverly Marsh", "Atira com o estilingue e sabe reconhecer as vozes que a Coisa usa para enganar."),
    ("mike", "Mike Hanlon", "Guarda a história de Derry, orienta Bill e acende a luz da siderúrgica."),
    ("richie", "Richie Tozier", "Usa a voz e o humor para enfrentar o medo. Seu grito desfaz as paredes falsas."),
    ("georgie", "Georgie", "O irmão desaparecido de Bill. No covil, a Coisa usa seu rosto e sua voz como armadilha."),
    ("pennywise", "Pennywise", "A Coisa que se alimenta do medo e assume a forma dos pesadelos de cada amigo."),
    ("elias", "Elias", "O zelador dos esgotos. Conhece as grades e vende suprimentos para atravessar a noite."),
]
personagens = [{"nome": titulo, "descricao": desc, "imagem": imagem("retrato_" + chave)}
               for chave, titulo, desc in personagens]

dados = {"capa": imagem("cena_menu_inicial"), "capitulos": capitulos,
         "locais": locais, "personagens": personagens,
         "grupo": imagem("tela_perdedores"), "final": imagem("cena_final_10")}
(ROOT / "wiki" / "lore.json").write_text(json.dumps(dados, ensure_ascii=False, indent=2), encoding="utf-8")
print(f"Wiki: {len(capitulos)} capítulos, {sum(len(c['cenas']) for c in capitulos)} cenas, "
      f"{len(personagens)} personagens, "
      f"{sum(p.suffix in ('.jpg', '.png') for p in DESTINO.iterdir())} imagens.")
