"""Monta a revisão visual das texturas de um ou mais ambientes: cada material ladrilhado 3×3.
É o critério de aceite das texturas (a métrica de emenda do export é só um alerta).

Uso:  python tools/ver_texturas.py <saida.png> galeria [poroes ...]
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parent.parent
MATERIAIS = ["piso", "piso2", "parede_face", "parede_topo", "liquido"]
LADO = 192   # cada repetição na revisão


def main():
    saida, ambientes = Path(sys.argv[1]), sys.argv[2:]
    bloco = 3 * LADO
    folha = Image.new("RGB", (len(MATERIAIS) * (bloco + 12), len(ambientes) * (bloco + 28)), (24, 24, 30))
    d = ImageDraw.Draw(folha)
    for j, amb in enumerate(ambientes):
        for i, mat in enumerate(MATERIAIS):
            x0, y0 = i * (bloco + 12), j * (bloco + 28)
            d.text((x0 + 4, y0 + 4), f"{amb} / {mat}", fill="white")
            p = ROOT / "assets" / "jogo_hd" / f"tex_{amb}_{mat}.png"
            if not p.exists():
                d.text((x0 + 4, y0 + 30), "(sem textura)", fill="gray")
                continue
            im = Image.open(p).convert("RGB").resize((LADO, LADO), Image.LANCZOS)
            for gx in range(3):
                for gy in range(3):
                    folha.paste(im, (x0 + gx * LADO, y0 + 24 + gy * LADO))
    folha.save(saida)
    print(saida)


if __name__ == "__main__":
    main()
