"""Avaliação de qualidade: export atual (contrato 1×, paleta 64, alfa binário, ampliado por
vizinho mais próximo) × export HD (4× o contrato, cores completas, alfa suave, reduzido com
filtro de qualidade para o tamanho de tela). Simula um monitor 1080p (escala 3×).

Uso: python tools/avaliar_hd.py <pasta_saida>
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).parent))
import export_final as ef  # noqa: E402

FATOR_HD = 4
ESCALA_TELA = 3   # 1920×1080 / 640×360


def exportar(stem, fator, hd):
    spec = dict(ef.catalog()[stem])
    spec["w"] *= fator
    spec["h"] *= fator
    kind = ef.kind_of(stem)
    opaco = kind in ("tiles", "ilustracao") and stem != "cisterna"
    rgb, alpha = ef.load_rgba(ef.find_master(stem), opaco)
    if kind == "tiles":
        img = ef.export_tiles(stem, spec, rgb, alpha)
    else:
        img = ef.export_frames(stem, spec, kind, rgb, alpha, lambda *_: None)
    if stem in ef.FIX_SIDES:
        img = ef.fix_sides(img, spec)
    return img, spec


def preparar_hd():
    """Desliga as perdas do pipeline: sem quantização e com alfa contínuo."""
    ef.quantize = lambda img, colors: img
    def shrink(rgba_img, size, soft=False):
        return rgba_img.convert("RGBa").resize(size, Image.LANCZOS).convert("RGBA")
    ef.shrink = shrink
    ef.BODY_H = {k: v * FATOR_HD for k, v in ef.BODY_H.items()}
    ef.DEFAULT_BODY_H *= FATOR_HD


def frame(img, spec, col, lin):
    cw, ch = spec["w"] // spec["cols"], spec["h"] // spec["rows"]
    return img.crop((col * cw, lin * ch, (col + 1) * cw, (lin + 1) * ch))


def para_tela(im, alvo_w, alvo_h, nitido):
    if nitido:
        return im.resize((alvo_w, alvo_h), Image.NEAREST)
    return im.convert("RGBa").resize((alvo_w, alvo_h), Image.LANCZOS).convert("RGBA")


def cena(tiles, personagens, nitido, fonte_fator):
    """Monta um recorte de cena 8×4 tiles com personagens, já na resolução da tela."""
    T = 32 * ESCALA_TELA
    W, H = 8 * T, 4 * T
    tela = Image.new("RGBA", (W, H))
    tile_img, tile_spec = tiles
    for lin in range(4):
        for col in range(8):
            idx = [0, 0, 12, 0, 13, 0, 0, 12][(col + lin * 3) % 8]
            t = frame(tile_img, tile_spec, idx % 4, idx // 4)
            tela.alpha_composite(para_tela(t, T, T, nitido), (col * T, lin * T))
    x = int(0.9 * T)
    for img, spec, col, lin in personagens:
        f = frame(img, spec, col, lin)
        w, h = f.width * ESCALA_TELA // fonte_fator, f.height * ESCALA_TELA // fonte_fator
        tela.alpha_composite(para_tela(f, w, h, nitido), (x, int(3.6 * T) - h))
        x += w + T // 3
    return tela


def main():
    saida = Path(sys.argv[1])
    saida.mkdir(parents=True, exist_ok=True)
    nomes = ["galeria", "bill_walk", "beverly_idle", "elias_idle", "pennywise_4_actions", "sombra", "balao"]
    atual = {n: exportar(n, 1, False) for n in nomes}
    preparar_hd()
    hd = {n: exportar(n, FATOR_HD, True) for n in nomes}

    def personagens(base):
        return [(base["bill_walk"][0], base["bill_walk"][1], 0, 0),
                (base["beverly_idle"][0], base["beverly_idle"][1], 0, 0),
                (base["elias_idle"][0], base["elias_idle"][1], 0, 0),
                (base["sombra"][0], base["sombra"][1], 0, 0),
                (base["balao"][0], base["balao"][1], 0, 0),
                (base["pennywise_4_actions"][0], base["pennywise_4_actions"][1], 0, 0)]

    a = cena(atual["galeria"], personagens(atual), True, 1)
    b = cena(hd["galeria"], personagens(hd), False, FATOR_HD)
    folha = Image.new("RGB", (a.width, a.height * 2 + 60), (30, 30, 36))
    d = ImageDraw.Draw(folha)
    d.text((10, 8), "ATUAL: contrato 1x (paleta 64, alfa binario) ampliado 3x por vizinho mais proximo", fill="white")
    folha.paste(a.convert("RGB"), (0, 28))
    d.text((10, a.height + 36), "HD: export 4x (cores completas, alfa suave) reduzido para a tela com filtro", fill="white")
    folha.paste(b.convert("RGB"), (0, a.height + 56))
    folha.save(saida / "comparacao_cena_1080p.png")

    # Close nos rostos: o ganho de detalhe fica evidente.
    closes = []
    for nome in ["bill_walk", "beverly_idle", "pennywise_4_actions"]:
        fa = frame(*atual[nome][:1], atual[nome][1], 0, 0)
        fb = frame(*hd[nome][:1], hd[nome][1], 0, 0)
        za = para_tela(fa, fa.width * 6, fa.height * 6, True)
        zb = para_tela(fb, fa.width * 6, fa.height * 6, False)
        par = Image.new("RGBA", (za.width * 2 + 20, za.height), (60, 62, 74, 255))
        par.alpha_composite(za, (0, 0))
        par.alpha_composite(zb, (za.width + 20, 0))
        closes.append(par)
    W = sum(c.width for c in closes) + 40
    H = max(c.height for c in closes)
    z = Image.new("RGBA", (W, H), (30, 30, 36, 255))
    x = 0
    for c in closes:
        z.alpha_composite(c, (x, 0))
        x += c.width + 20
    z.save(saida / "comparacao_close_6x.png")
    print("ok", saida)


if __name__ == "__main__":
    main()
