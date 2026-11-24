"""Converte os masters em alta resolução de assets/ nos arquivos finais do contrato.

Uso:
    python tools/export_final.py              # exporta tudo para assets/jogo/
    python tools/export_final.py --preview D  # também salva folhas de conferência em D

Etapas por asset:
1. Fundo: masters sem alfa (JPG) têm o fundo sólido removido por flood fill a
   partir das bordas. Tilesets e ilustrações continuam opacos.
2. Frames: componentes conectados do alfa são agrupados em colunas/linhas pelos
   maiores vãos entre centróides. Isso tolera grades tortas do gerador.
3. Escala: uma escala única por folha (proporção entre frames preservada),
   redução por média de área, alfa binário e paleta reduzida para manter o
   pixel art nítido. Personagens ficam ancorados pelos pés no centro da base.
4. Atlas icones.png (4 × 4 de 16 px) montado a partir dos itens e do HUD.
5. manifesto.txt com dimensões, grade e pivô de cada arquivo.
"""
import argparse
import re
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
OUT = ASSETS / "jogo"
PROMPT_FILES = [ROOT / "instructions" / "Prompts-Assets-IT.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v2.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v3-texturas.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v4-andar.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v5-fx.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v6-cenas.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v7-covil.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v8-pennywise.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v9-amigos-ui.md",   # 24/11: armas/poderes, corte, menus
                ROOT / "instructions" / "Prompts-Assets-IT-v10-pennywise.md",   # 24/11: II e III refeitos, IV aranha
                ROOT / "instructions" / "Prompts-Assets-IT-v11-chefes-8dir.md",   # 24/11: chefes em 8 direções
                ROOT / "instructions" / "Prompts-Assets-IT-v12-menu.md"]   # 24/11: tela de título

# kind: sprite (pivô nos pés), centro (FX/ícones), tiles (opaco, grade regular),
# objetos (alfa, grade regular), modelo (folha inteira), ilustracao (opaca).
KIND = {
    "referencia_visual": "ilustracao", "capa_menu": "ilustracao", "epilogo": "ilustracao",
    "bill_referencia": "modelo", "pennywise_referencia": "modelo",
    "pennywise_4_aranha": "centro",   # 24/11: o corpo de aranha do IV gira em volta do centro
    "galeria": "tiles", "poroes": "tiles", "cisterna": "tiles",
    "objetos": "objetos", "objetos_arena": "objetos",
    # v2
    "rua_derry": "tiles", "clube": "tiles", "barrens": "tiles", "neibolt": "tiles",
    "siderurgica": "tiles", "covil": "tiles",
    "objetos_clube": "objetos", "objetos_barrens": "objetos", "objetos_neibolt": "objetos",
    "objetos_siderurgica": "objetos", "objetos_comuns": "objetos",
}
# Prefixos de ilustração opaca (v2)
ILLU_PREFIX = ("cartao_", "tela_", "cena_")   # cena_: quadros das cut scenes (v6)
# PRD v2: resolução lógica 640 × 360 — ilustrações do v1 re-exportadas em 16:9
SIZE_OVERRIDE = {"capa_menu": (640, 360), "epilogo": (640, 360), "referencia_visual": (640, 360)}
# Folhas do Bill cujo master tem a linha 2 virada para a DIREITA e a linha 3 misturada.
# Contrato: linhas baixo, esquerda, direita, cima → direita = linha 2 do master; esquerda = espelho.
FIX_SIDES = {"bill_walk", "bill_attack", "bill_hurt"}
# altura do corpo em pixels finais, por prefixo
BODY_H = {"medo_": 60, "georgie": 32, "elias": 47, "pennywise_4": 62, "pennywise": 46, "balao": 30, "rato": 30, "sombra": 30}
DEFAULT_BODY_H = 44  # crianças em frame 32 × 48
# folhas cujas partículas se entrelaçam entre frames: grade uniforme sobre a imagem inteira
EVEN_SPLIT = {"fx_piso", "objetos_siderurgica",   # objetos que encostam nas bordas da célula
              "boneco_palhaco"}   # o pino do ataque quase encosta na linha de cima
# Folhas de uma linha só que o gerador não consegue fazer (proporção 8:1, 6:1): o master é
# pedido nesta grade (colunas, linhas) e o export remonta a linha. Mesma tabela em batch_images.py.
MASTER_GRID = {"ui_recompensas": (4, 2), "ui_emblemas": (3, 2)}
# v4 (andar em 4 direções, P151–P166): o master tem 3 linhas — frente, perfil DIREITO, costas. As linhas
# finais são baixo, esquerda, direita, cima: a esquerda é o espelho da direita (coerente por construção;
# pedir as 4 trazia esquerda e direita misturadas, como no bill_walk).
def eh_walk3(stem):
    return stem.endswith("_walk") and stem != "bill_walk"
ICON_ORDER = ["item_kit", "item_bateria", "item_chave_ferro", "item_chave_latao",
              "item_manivela", "item_taco", "item_escudo", "item_lembranca",
              "item_coragem", "hud:0", "hud:1", "hud:2", "hud:3", "hud:4", "hud:5", "hud:6"]


def catalog():
    text = "\n".join(p.read_text(encoding="utf-8") for p in PROMPT_FILES)
    rows = re.findall(r"^\| P\d{2,3} \| .*? \| (\S+)\.png \| (.*?) \|$", text, re.M)
    specs = {}
    for stem, dims in rows:
        w, h = map(int, re.match(r"(\d+) × (\d+)", dims).groups())
        grid = re.search(r"(\d+) colunas? × (\d+) linhas?", dims)
        cols, rows_ = (int(grid[1]), int(grid[2])) if grid else (1, 1)
        if stem in SIZE_OVERRIDE:
            w, h = SIZE_OVERRIDE[stem]
        specs[stem] = {"w": w, "h": h, "cols": cols, "rows": rows_}
    return specs


def kind_of(stem):
    if stem in KIND:
        return KIND[stem]
    if stem.startswith(ILLU_PREFIX):
        return "ilustracao"
    if stem.startswith("tex_"):
        return "textura"
    if stem.startswith(("fx_", "item_", "hud_", "ui_")):
        return "centro"
    if stem.startswith("retrato_"):
        return "retrato"
    return "sprite"


def find_master(stem):
    for ext in (".png", ".jpg", ".jpeg"):
        p = ASSETS / f"{stem}{ext}"
        if p.exists():
            return p
    return None


# ---------- fundo ----------

def remove_flat_background(rgb, tol=24):
    """Flood fill a partir das bordas sobre pixels próximos da cor de fundo."""
    a = rgb.astype(np.int32)
    border = np.concatenate([a[0], a[-1], a[:, 0], a[:, -1]])
    bg = np.median(border, axis=0)
    dist = np.sqrt(((a - bg) ** 2).sum(axis=2))
    near = dist < tol
    labels, _ = ndimage.label(near)
    edge = np.unique(np.concatenate([labels[0], labels[-1], labels[:, 0], labels[:, -1]]))
    bgmask = np.isin(labels, edge[edge > 0])
    bgmask = ndimage.binary_opening(bgmask, iterations=1)
    alpha = np.where(bgmask, 0, 255).astype(np.uint8)
    return alpha


def drop_hollow_frames(alpha):
    """Remove contornos de caixa que o gerador às vezes desenha ao redor dos frames."""
    mask = alpha > 127
    labels, n = ndimage.label(mask, structure=np.ones((3, 3)))
    for i, sl in enumerate(ndimage.find_objects(labels), start=1):
        h, w = sl[0].stop - sl[0].start, sl[1].stop - sl[1].start
        area = (labels[sl] == i).sum()
        thin = min(h, w) <= 6 and max(h, w) >= 40
        hollow = h * w > 4000 and area / (h * w) < 0.08
        if thin or hollow:
            alpha[labels == i] = 0
    return alpha


def trim_flat_border(rgb, tol=18):
    """Corta faixas uniformes (letterbox) que o gerador pinta nas bordas da ilustração."""
    a = rgb.astype(np.int32)
    def flat(line):
        return np.abs(line - np.median(line, axis=0)).max(axis=1).mean() < tol
    top, bot, left, right = 0, a.shape[0], 0, a.shape[1]
    while top < bot // 4 and flat(a[top]): top += 1
    while bot > a.shape[0] * 3 // 4 and flat(a[bot - 1]): bot -= 1
    while left < right // 4 and flat(a[:, left]): left += 1
    while right > a.shape[1] * 3 // 4 and flat(a[:, right - 1]): right -= 1
    return rgb[top:bot, left:right]


def load_rgba(path, opaque):
    im = Image.open(path)
    if opaque:
        return np.array(im.convert("RGB")), None
    if im.mode == "RGBA" and np.array(im.getchannel("A")).min() < 128:
        arr = np.array(im)
        return arr[..., :3], drop_hollow_frames(arr[..., 3].copy())
    rgb = np.array(im.convert("RGB"))
    return rgb, remove_flat_background(rgb)


# ---------- frames ----------

def split_by_gaps(values, weights, n):
    """Divide posições 1D em n grupos pelos n-1 maiores vãos."""
    order = np.argsort(values)
    v = values[order]
    if n <= 1 or len(v) < n:
        return [v.min() - 1] + [v.max() + 1]
    gaps = np.diff(v)
    cut_idx = np.sort(np.argsort(gaps)[-(n - 1):])
    cuts = [(v[i] + v[i + 1]) / 2 for i in cut_idx]
    return [v.min() - 1] + cuts + [v.max() + 1]


def split_by_empty_runs(profile, n, min_run=6):
    """Corta nos n-1 maiores trechos vazios da projeção; None se não houver vãos suficientes."""
    if n <= 1:
        return [-1, len(profile) + 1]
    filled = np.nonzero(profile)[0]
    inner = profile[filled[0]:filled[-1] + 1]
    runs, start = [], None
    for i, v in enumerate(inner):
        if not v and start is None:
            start = i
        elif v and start is not None:
            runs.append((i - start, start + (i - start) / 2))
            start = None
    runs = [r for r in runs if r[0] >= min_run]
    if len(runs) < n - 1:
        return None
    cuts = sorted(filled[0] + c for _, c in sorted(runs, reverse=True)[:n - 1])
    return [filled[0] - 1] + cuts + [filled[-1] + 1]


def find_frames(alpha, cols, rows, even=False):
    mask = alpha > 127
    labels, n = ndimage.label(mask, structure=np.ones((3, 3)))
    if n == 0:
        return None
    idx = np.arange(1, n + 1)
    areas = ndimage.sum(mask, labels, idx)
    cents = np.array(ndimage.center_of_mass(mask, labels, idx))
    big = areas >= max(30, areas.max() * 0.02)
    solid = np.isin(labels, idx[areas >= 30])  # ignora poeira do recorte na projeção
    candidates = []
    if even:
        # Grade uniforme: recorta ESTRITAMENTE pelo retângulo de cada célula. Por componente não
        # serve: uma parte que encosta na célula vizinha (o pino do palhaço nos pés do de cima)
        # juntaria as duas figuras num frame só.
        h, w = alpha.shape
        xs, ys = np.linspace(0, w, cols + 1).astype(int), np.linspace(0, h, rows + 1).astype(int)
        limpo = np.isin(labels, idx[areas >= 4])
        out = {}
        for r in range(rows):
            for c in range(cols):
                m = np.zeros_like(limpo)
                m[ys[r]:ys[r + 1], xs[c]:xs[c + 1]] = limpo[ys[r]:ys[r + 1], xs[c]:xs[c + 1]]
                if m.sum() < 30:
                    continue
                yy, xx = np.nonzero(m)
                out[(r, c)] = (m, (xx.min(), yy.min(), xx.max() + 1, yy.max() + 1))
        return out
    px = split_by_empty_runs(solid.any(axis=0), cols) or split_by_empty_runs(solid.any(axis=0), cols, 2)
    py = split_by_empty_runs(solid.any(axis=1), rows) or split_by_empty_runs(solid.any(axis=1), rows, 2)
    if px and py:
        candidates.append((px, py))
    candidates.append((split_by_gaps(cents[big, 1], areas[big], cols),
                       split_by_gaps(cents[big, 0], areas[big], rows)))
    best = None
    for xs, ys in candidates:
        frames = assign(labels, areas, cents, xs, ys, cols, rows)
        if even:
            return frames
        if best is None or len(frames) > len(best):
            best = frames
    return best


def assign(labels, areas, cents, xs, ys, cols, rows):
    frames = {}
    for i, (cy, cx) in enumerate(cents):
        if areas[i] < 4:
            continue
        c = int(np.clip(np.searchsorted(xs, cx) - 1, 0, cols - 1))
        r = int(np.clip(np.searchsorted(ys, cy) - 1, 0, rows - 1))
        frames.setdefault((r, c), []).append(i + 1)
    out = {}
    for key, labs in frames.items():
        m = np.isin(labels, labs)
        yy, xx = np.nonzero(m)
        out[key] = (m, (xx.min(), yy.min(), xx.max() + 1, yy.max() + 1))
    return out


def shrink(rgba_img, size, soft=False):
    """Redução por área em alfa pré-multiplicado; alfa binário, ou 3 níveis em névoas."""
    small = rgba_img.convert("RGBa").resize(size, Image.BOX).convert("RGBA")
    arr = np.array(small)
    a = arr[..., 3]
    if soft:
        arr[..., 3] = np.select([a >= 200, a >= 110, a >= 40], [255, 170, 85], 0)
    else:
        arr[..., 3] = np.where(a >= 110, 255, 0)
    return Image.fromarray(arr)


def quantize(img, colors):
    arr = np.array(img)
    alpha = arr[..., 3].copy()
    rgb = Image.fromarray(arr[..., :3]).quantize(colors, method=Image.Quantize.MEDIANCUT,
                                                 dither=Image.Dither.NONE).convert("RGB")
    out = np.dstack([np.array(rgb), alpha])
    return Image.fromarray(out.astype(np.uint8))


def export_frames(stem, spec, kind, rgb, alpha, log):
    W, H, cols, rows = spec["w"], spec["h"], spec["cols"], spec["rows"]
    cw, ch = W // cols, H // rows
    if stem in MASTER_GRID:
        # O master veio numa grade mais "quadrada" (o gerador não faz 8:1); a ordem de leitura
        # (esquerda→direita, cima→baixo) vira a ordem na grade final.
        mc, mr = MASTER_GRID[stem]
        lidos = find_frames(alpha, mc, mr) or {}
        frames = {}
        for (r, c), v in lidos.items():
            i = r * mc + c
            if i < cols * rows:
                frames[(i // cols, i % cols)] = v
    elif eh_walk3(stem):
        lidos = find_frames(alpha, cols, 3) or {}
        frames = {({0: 0, 1: 2, 2: 3}[r], c): v for (r, c), v in lidos.items()}
    else:
        frames = find_frames(alpha, cols, rows, stem in EVEN_SPLIT)
    if not frames:
        log(f"  {stem}: nenhum conteúdo após remover o fundo")
        return None
    missing = [(r, c) for r in range(rows) for c in range(cols) if (r, c) not in frames and not (eh_walk3(stem) and r == 1)]
    rgba = np.dstack([rgb, alpha])
    max_w = max(b[2] - b[0] for _, b in frames.values())
    max_h = max(b[3] - b[1] for _, b in frames.values())
    if kind == "sprite":
        body = next((v for k, v in BODY_H.items() if stem.startswith(k)), DEFAULT_BODY_H)
        body = min(body, ch - 1)
        s = min(body / max_h, (cw - 1) / max_w)
    elif kind == "retrato":
        s = min(cw / max_w, ch / max_h)
    else:
        s = min((cw - 1) / max_w, (ch - 1) / max_h)
    sheet = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    for (r, c), (m, (x0, y0, x1, y1)) in frames.items():
        crop = rgba[y0:y1, x0:x1].copy()
        crop[..., 3] = np.where(m[y0:y1, x0:x1], crop[..., 3], 0)
        fw, fh = max(1, round((x1 - x0) * s)), max(1, round((y1 - y0) * s))
        fw, fh = min(fw, cw), min(fh, ch)
        soft = kind == "centro" or re.search(r"disperse|defeat", stem) is not None
        small = shrink(Image.fromarray(crop), (fw, fh), soft)
        ox = c * cw + (cw - fw) // 2
        if kind in ("sprite", "retrato", "objetos"):
            oy = r * ch + ch - fh  # pivô na base
        else:
            oy = r * ch + (ch - fh) // 2
        sheet.alpha_composite(small, (ox, oy))
    if missing:
        # Frame ausente no fim de uma linha (o gerador desenhou 3 em vez de 4): repete o frame
        # anterior da mesma linha — numa animação, equivale a segurar a pose final.
        # Só vale para o caso legítimo: poucos ausentes, todos no FIM da linha. Muitos ausentes =
        # a grade não foi reconhecida; preencher esconderia o defeito, então só avisa.
        legitimo = len(missing) <= 2 and all((r, c + 1) not in frames for r, c in missing)
        repetidos = []
        for (r, c) in (missing if legitimo else []):
            origem = next(((r, k) for k in range(c - 1, -1, -1) if (r, k) in frames), None)
            if origem is None:
                continue
            bloco = sheet.crop((origem[1] * cw, r * ch, origem[1] * cw + cw, r * ch + ch))
            sheet.paste(bloco, (c * cw, r * ch))
            repetidos.append((r, c))
        log(f"  {stem}: ATENÇÃO frames ausentes {missing}; preenchidos repetindo o anterior: {repetidos}")
    return quantize(sheet, 64)


def export_tiles(stem, spec, rgb, alpha):
    W, H, cols, rows = spec["w"], spec["h"], spec["cols"], spec["rows"]
    img = Image.fromarray(np.dstack([rgb, alpha if alpha is not None else np.full(rgb.shape[:2], 255, np.uint8)]))
    if alpha is not None:
        img = img.crop(img.getbbox())
    tw, th = img.width / cols, img.height / rows
    inset = 0.025  # descarta as linhas-guia que o gerador desenha entre tiles
    sheet = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    cw, ch = W // cols, H // rows
    for r in range(rows):
        for c in range(cols):
            box = (round((c + inset) * tw), round((r + inset) * th),
                   round((c + 1 - inset) * tw), round((r + 1 - inset) * th))
            sheet.alpha_composite(shrink(img.crop(box), (cw, ch)), (c * cw, r * ch))
    return sheet


def export_whole(spec, rgb, alpha, opaque, stem=""):
    W, H = spec["w"], spec["h"]
    if opaque:
        # A tela de título tem o terço esquerdo escuro DE PROPÓSITO (o menu fica ali): não é borda de letterbox.
        img = Image.fromarray(rgb if stem == "cena_menu_inicial" else trim_flat_border(rgb))
        src_ratio, dst_ratio = img.width / img.height, W / H
        if src_ratio > dst_ratio:  # recorte central para a proporção final
            nw = round(img.height * dst_ratio)
            img = img.crop(((img.width - nw) // 2, 0, (img.width + nw) // 2, img.height))
        else:
            nh = round(img.width / dst_ratio)
            img = img.crop((0, (img.height - nh) // 2, img.width, (img.height + nh) // 2))
        if stem.startswith("cena_") or stem in ("cartao_clube", "tela_perdedores", "referencia_visual"):
            # 24/11: reduzir por NEAREST (pixel art): o BOX borrava a borda de cada bloco (cenas "embaçadas")
            filtro = Image.Resampling.LANCZOS if W > img.width else Image.Resampling.NEAREST
            quadro = img.resize((W, H), filtro)
            if W > img.width:
                quadro = quadro.filter(ImageFilter.UnsharpMask(radius=1.0, percent=65, threshold=3))
            return quadro.convert("RGBA")
        return img.resize((W, H), Image.BOX).convert("RGBA")
    img = Image.fromarray(np.dstack([rgb, alpha]))
    img = img.crop(img.getbbox())
    s = min(W / img.width, H / img.height)
    small = shrink(img, (round(img.width * s), round(img.height * s)))
    sheet = Image.new("RGBA", (W, H), (0, 0, 0, 0))
    sheet.alpha_composite(small, ((W - small.width) // 2, H - small.height))
    return quantize(sheet, 64)


def espelhar_esquerda(img, spec):
    """Folhas WALK3: a linha 1 (esquerda) é o espelho, quadro a quadro, da linha 2 (direita)."""
    ch = spec["h"] // spec["rows"]
    cw = spec["w"] // spec["cols"]
    out = img.copy()
    for c in range(spec["cols"]):
        frame = img.crop((c * cw, 2 * ch, (c + 1) * cw, 3 * ch)).transpose(Image.FLIP_LEFT_RIGHT)
        out.paste(frame, (c * cw, ch))
    return out


def fix_sides(img, spec):
    """Linha 2 (esquerda) = espelho da linha 2 original; linha 3 (direita) = linha 2 original."""
    ch = spec["h"] // spec["rows"]
    cw = spec["w"] // spec["cols"]
    direita = img.crop((0, ch, spec["w"], 2 * ch))
    esquerda = Image.new("RGBA", direita.size, (0, 0, 0, 0))
    for c in range(spec["cols"]):  # espelha cada frame no lugar, preservando a ordem das colunas
        frame = direita.crop((c * cw, 0, (c + 1) * cw, ch)).transpose(Image.FLIP_LEFT_RIGHT)
        esquerda.alpha_composite(frame, (c * cw, 0))
    out = img.copy()
    out.paste(esquerda, (0, ch))
    out.paste(direita, (0, 2 * ch))
    return out


# ---------- texturas contínuas (v3, 10/11/2026) ----------

def pontuacao_emenda(arr):
    """Quanto a emenda (última → primeira coluna/linha) destoa das transições NORMAIS da textura.
    Pixel art tem transições fortes legítimas (pedra ↔ rejunte), então a referência é o
    percentil 95 das diferenças entre colunas (linhas) vizinhas. É só um ALERTA (tem falsos positivos
    em pixel art com rejunte); quem decide é a montagem 3×3 revisada a olho."""
    a = arr.astype(np.float32)
    col = np.abs(np.diff(a, axis=1)).mean(axis=(0, 2))   # diferença média por par de colunas
    lin = np.abs(np.diff(a, axis=0)).mean(axis=(1, 2))
    emenda_x = np.abs(a[:, 0] - a[:, -1]).mean()
    emenda_y = np.abs(a[0, :] - a[-1, :]).mean()
    return max(emenda_x / max(np.percentile(col, 95), 1e-3), emenda_y / max(np.percentile(lin, 95), 1e-3))


def forcar_continuidade(arr, faixa=0.18):
    """Funde a textura com sua versão deslocada meio período só numa faixa junto às bordas:
    a cópia deslocada tem bordas contínuas entre si, e o centro continua o original."""
    h, w = arr.shape[:2]
    deslocada = np.roll(np.roll(arr, w // 2, axis=1), h // 2, axis=0).astype(np.float32)
    dx = np.minimum(np.arange(w), w - 1 - np.arange(w)) / (w * faixa)
    dy = np.minimum(np.arange(h), h - 1 - np.arange(h)) / (h * faixa)
    peso = np.clip(np.minimum.outer(dy, dx), 0, 1)[..., None]   # 0 na borda → 1 no interior
    return (arr.astype(np.float32) * peso + deslocada * (1 - peso)).round().astype(np.uint8)


def export_textura(stem, spec, rgb, log):
    # Sem trim_flat_border: ele confundiria a linha de rejunte da borda com letterbox e
    # cortaria a textura, CRIANDO uma emenda que não existia.
    lado = min(rgb.shape[0], rgb.shape[1])   # quadrada
    arr = rgb[:lado, :lado]
    nota = pontuacao_emenda(arr)
    if nota > 4.0:
        # Fundir automaticamente borra a pixel art (fantasmas). Melhor gerar a textura de novo;
        # forcar_continuidade() fica disponível para casos em que isso não for possível.
        log(f"  ATENÇÃO {stem}: emenda suspeita ({nota:.2f}); revisar e, se visível, gerar de novo")
    img = Image.fromarray(arr).convert("RGBA")
    return img.resize((spec["w"], spec["h"]), Image.LANCZOS if OUT == OUT_HD else Image.BOX)


def build_icons(specs, fator=1):
    s = 16 * fator
    atlas = Image.new("RGBA", (4 * s, 4 * s), (0, 0, 0, 0))
    hud = Image.open(OUT / "hud_simbolos.png")
    for i, name in enumerate(ICON_ORDER):
        if name.startswith("hud:"):
            k = int(name[4:])
            icon = hud.crop((k * s, 0, k * s + s, s))
        else:
            icon = Image.open(OUT / f"{name}.png")
        atlas.alpha_composite(icon, ((i % 4) * s, (i // 4) * s))
    return atlas


# ---------- modo HD (PRD v2 §19.2, decisão de 10/11/2026) ----------
# O jogo desenha na resolução real da tela (640×360 × escala) e reduz cada imagem uma vez,
# no carregamento. Para isso exportamos em FATOR_HD × o contrato, sem quantizar a paleta e
# com alfa suave. O manifesto continua com as medidas LÓGICAS do contrato + o fator.
OUT_HD = ASSETS / "jogo_hd"
FATOR_HD = 4
FATOR_HD_POR_TIPO = {"ilustracao": 2, "modelo": 2}   # masters de ~1200 px não sustentam 4×
FATOR_HD_CENAS = 4   # quadros das cut scenes: 2560 × 1440 para redução no carregamento


def ativar_modo_hd():
    global OUT, quantize, shrink, BODY_H, DEFAULT_BODY_H
    OUT = OUT_HD
    quantize = lambda img, colors: img   # noqa: E731 — cores completas

    def shrink_hd(rgba_img, size, soft=False):
        # Lanczos em alfa pré-multiplicado: bordas suaves sem halo escuro.
        return rgba_img.convert("RGBa").resize(size, Image.LANCZOS).convert("RGBA")
    shrink = shrink_hd
    BODY_H = {k: v * FATOR_HD for k, v in BODY_H.items()}
    DEFAULT_BODY_H *= FATOR_HD


def pivot_of(kind):
    return {"sprite": "pes", "retrato": "base", "objetos": "base", "centro": "centro"}.get(kind, "-")


def preview(paths, dest, scale=3):
    tiles = []
    for p in paths:
        im = Image.open(p).convert("RGBA")
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
        tiles.append((p.name, im))
    width = 1600
    x = y = rowh = 0
    pos = []
    for name, im in tiles:
        if x + im.width > width:
            x, y, rowh = 0, y + rowh + 8, 0
        pos.append((x, y))
        x += im.width + 8
        rowh = max(rowh, im.height)
    sheet = Image.new("RGBA", (width, y + rowh + 8), (88, 92, 110, 255))
    for (name, im), (px, py) in zip(tiles, pos):
        sheet.alpha_composite(im, (px, py))
    sheet.save(dest)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview", type=Path)
    ap.add_argument("--hd", action="store_true", help="exporta assets/jogo_hd em 4× (usado pelo jogo)")
    args = ap.parse_args()
    if args.hd:
        ativar_modo_hd()
    OUT.mkdir(exist_ok=True)
    specs = catalog()
    manifest = ["# arquivo largura altura frame_largura frame_altura colunas linhas pivo [fator]",
                "# medidas LÓGICAS do contrato; a imagem tem medidas × fator"]
    written = []
    for stem, logico in specs.items():
        master = find_master(stem)
        if not master:
            print(f"  {stem}: sem master, pulado")
            continue
        kind = kind_of(stem)
        fator = FATOR_HD_POR_TIPO.get(kind, FATOR_HD) if args.hd else 1
        # cartao_clube, tela_perdedores e referencia_visual só aparecem nas cut scenes (24/11): também em 4×.
        if args.hd and (stem.startswith("cena_") or stem in ("cartao_clube", "tela_perdedores", "referencia_visual")):   # cut scenes: tela cheia 1080p nítida; o jogo carrega sob demanda
            fator = FATOR_HD_CENAS
        spec = dict(logico, w=logico["w"] * fator, h=logico["h"] * fator)
        opaque = kind in ("tiles", "ilustracao", "textura") and stem != "cisterna"
        rgb, alpha = load_rgba(master, opaque)
        if kind == "textura":
            img = export_textura(stem, spec, rgb, print)
        elif kind == "tiles":
            img = export_tiles(stem, spec, rgb, alpha)
        elif kind in ("ilustracao", "modelo"):
            img = export_whole(spec, rgb, alpha, opaque, stem)
        else:
            img = export_frames(stem, spec, kind, rgb, alpha, print)
        if img is None:
            continue
        if stem in FIX_SIDES:
            img = fix_sides(img, spec)
        if eh_walk3(stem):
            img = espelhar_esquerda(img, spec)
        dest = OUT / f"{stem}.png"
        img.save(dest)
        written.append(dest)
        cw, ch = logico["w"] // logico["cols"], logico["h"] // logico["rows"]
        manifest.append(f"{stem}.png {logico['w']} {logico['h']} {cw} {ch} {logico['cols']} {logico['rows']} "
                        f"{pivot_of(kind)} {fator}")
        print(f"  {stem}.png {img.size} ({master.name}, {kind}, fator {fator})")
    fator_icones = FATOR_HD if args.hd else 1
    icons = build_icons(specs, fator_icones)
    icons.save(OUT / "icones.png")
    written.append(OUT / "icones.png")
    manifest.append(f"icones.png 64 64 16 16 4 4 centro {fator_icones}")
    (OUT / "manifesto.txt").write_text("\n".join(manifest) + "\n", encoding="utf-8")
    print(f"{len(written)} arquivos em {OUT}")
    if args.preview:
        args.preview.mkdir(parents=True, exist_ok=True)
        groups = {"personagens": [p for p in written if kind_of(p.stem) in ("sprite", "modelo")],
                  "retratos_itens_fx": [p for p in written if kind_of(p.stem) in ("retrato", "centro") or p.stem == "icones"],
                  "cenario": [p for p in written if kind_of(p.stem) in ("tiles", "objetos", "ilustracao")]}
        for name, paths in groups.items():
            preview(paths, args.preview / f"{name}.png")


if __name__ == "__main__":
    main()
