"""Monta folhas pequenas para revisar elenco e continuidade das cut scenes."""
from pathlib import Path

from PIL import Image, ImageDraw

raiz = Path(__file__).resolve().parent.parent
fontes = sorted((raiz / "assets").glob("cena_*.png"))
fontes += [raiz / "assets" / nome for nome in ("cartao_clube.png", "tela_perdedores.png")]
fontes += list((raiz / "assets").glob("referencia_visual.*"))
destino = raiz / "build" / "conferencia_cenas"
destino.mkdir(parents=True, exist_ok=True)

for pagina, inicio in enumerate(range(0, len(fontes), 12), 1):
    folha = Image.new("RGB", (1200, 936), "#171821")
    desenho = ImageDraw.Draw(folha)
    for indice, caminho in enumerate(fontes[inicio:inicio + 12]):
        x, y = (indice % 3) * 400, (indice // 3) * 234
        arte = Image.open(caminho).convert("RGB")
        arte.thumbnail((392, 210), Image.Resampling.LANCZOS)
        folha.paste(arte, (x + (400 - arte.width) // 2, y))
        desenho.text((x + 6, y + 211), caminho.stem, fill="white")
    folha.save(destino / f"pagina_{pagina:02d}.jpg", quality=90)
print(f"{len(fontes)} imagens em {pagina} folhas: {destino}")
