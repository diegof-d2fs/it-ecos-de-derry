"""Gera assets/jogo_web (Etapa 18): as imagens do jogo em 2× para o navegador.

O jogo no Windows usa assets/jogo_hd (4×, ~300 MB): grande demais para baixar numa página. Na web o canvas vai até
a escala 3 (1080p), e 2× basta: cada imagem de fator 4 vira fator 2 (metade da largura e da altura, pelo PIXEL MAIS
PRÓXIMO: é pixel art, a média borraria), com o manifesto ajustado. O que já é 2× ou 1× fica igual.

Uso: python tools/export_web.py   (depois de tools/export_final.py --hd)
"""
import shutil
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ORIGEM = ROOT / "assets" / "jogo_hd"
DESTINO = ROOT / "assets" / "jogo_web"


def main():
    DESTINO.mkdir(parents=True, exist_ok=True)
    for velho in DESTINO.iterdir():   # limpa os arquivos (a pasta pode estar aberta em outro programa)
        velho.unlink()
    linhas = []
    for linha in (ORIGEM / "manifesto.txt").read_text(encoding="utf-8").splitlines():
        partes = linha.split()
        if not partes or linha.startswith("#") or len(partes) < 9:
            linhas.append(linha)
            if partes and not linha.startswith("#") and (ORIGEM / partes[0]).exists():   # linha sem fator: copia
                shutil.copy(ORIGEM / partes[0], DESTINO / partes[0])
            continue
        arquivo, fator = partes[0], int(partes[8])
        im = Image.open(ORIGEM / arquivo)
        if fator >= 4:
            novo = fator // 2
            im = im.resize((im.width * novo // fator, im.height * novo // fator), Image.NEAREST)
            partes[8] = str(novo)
        # Ilustrações OPACAS grandes (cenas, cartões, telas): JPEG de alta qualidade com o MESMO nome (o stb_image
        # reconhece o formato pelo conteúdo). ~4× menores; os sprites com transparência seguem em PNG sem perda.
        opaca = im.mode in ("RGB", "RGBA") and (im.mode == "RGB" or im.getextrema()[3][0] == 255)
        if opaca and im.width * im.height >= 640 * 360:
            im.convert("RGB").save(DESTINO / arquivo, format="JPEG", quality=90, subsampling=0, optimize=True)
        else:
            im.save(DESTINO / arquivo, optimize=True)
        linhas.append(" ".join(partes))
    (DESTINO / "manifesto.txt").write_text("\n".join(linhas) + "\n", encoding="utf-8")
    total = sum(p.stat().st_size for p in DESTINO.iterdir())
    print(f"{DESTINO.relative_to(ROOT)}: {len(list(DESTINO.iterdir()))} arquivos, {total / 1e6:.1f} MB")


if __name__ == "__main__":
    main()
