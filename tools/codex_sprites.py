"""Gera FOLHAS DE SPRITES que faltam com o Codex CLI (gerador de imagens da OpenAI), sem usar o Gemini.

Irmão de tools/codex_cenas.py, para folhas com fundo transparente (personagens): lê o MESMO prompt e as
referências de um pacote de prompts (padrão: instructions/Prompts-Assets-IT-v8-pennywise.md) e grava o master
em assets/<nome>.png. As folhas de AÇÕES vêm antes das de ANDAR (o andar usa as ações como referência).
O Codex não grava fora da pasta dele: o arquivo é achado em ~/.codex/generated_images pelo nome exec-*.png
que ele devolve na resposta (lição de 23/11).

Uso: python tools/codex_sprites.py [--md instructions/OUTRO.md] [pennywise_2p_actions ...]
     (sem nomes: todos os do pacote sem master). Desde 24/11 também gera efeitos (fx_*), peças de interface
     (ui_*) e ilustrações opacas de fundo (tela_*), cada tipo com a sua instrução.
Depois: python tools/export_final.py --hd
"""
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
MD = ROOT / "instructions" / "Prompts-Assets-IT-v8-pennywise.md"
GERADAS = Path.home() / ".codex" / "generated_images"

INSTRUCAO = (
    "Use your image generation tool to create ONE 2D game sprite sheet with a TRANSPARENT background (real alpha "
    "channel; no checkerboard, no solid color). The attached images are the approved art of this game: match their "
    "16-bit pixel-art finish exactly. Follow the grid in the description EXACTLY (columns x rows), one full-body figure "
    "per cell, equal cells, clear transparent gutters, feet aligned at the bottom of each cell, nothing cut off. "
    "Do not resize, copy or edit any file: just generate it and reply with the full path of the generated PNG. "
    "Description (Portuguese):\n\n{prompt}"
)
# Efeitos e peças de interface: não são figuras de corpo inteiro (24/11).
INSTRUCAO_ASSET = (
    "Use your image generation tool to create ONE 2D game asset with a TRANSPARENT background (real alpha channel; no "
    "checkerboard, no solid color). The attached images are the approved art of this game: match their 16-bit "
    "pixel-art finish exactly. Follow the grid in the description EXACTLY (columns x rows), equal cells, one element "
    "per cell, clear transparent gutters, nothing cut off. Do not resize, copy or edit any file: just generate it and "
    "reply with the full path of the generated PNG. Description (Portuguese):\n\n{prompt}"
)
# Ilustrações de fundo (tela_*): opacas, 16:9, sem transparência.
INSTRUCAO_TELA = (
    "Use your image generation tool to create ONE full-bleed 16:9 landscape illustration (opaque, no letterbox, no "
    "text). The attached images are the approved art of this game: match their painterly 16-bit pixel-art finish and "
    "palette. Do not resize, copy or edit any file: just generate it and reply with the full path of the generated "
    "PNG. Description (Portuguese):\n\n{prompt}"
)


def instrucao(stem):
    if stem.startswith(("tela_", "cena_")):   # ilustrações opacas (cena_menu_inicial, 24/11)
        return INSTRUCAO_TELA
    if stem.startswith(("fx_", "ui_")) or stem.endswith("_aranha"):   # o corpo de aranha do IV: centrado, sem pés
        return INSTRUCAO_ASSET
    return INSTRUCAO


def achar(stem):
    for ext in (".png", ".jpg", ".jpeg"):
        p = ASSETS / f"{stem}{ext}"
        if p.exists():
            return p
    return None


def prompts():
    texto = MD.read_text(encoding="utf-8")
    pat = re.compile(r"\*\*Arquivo:\*\* (\S+)\.png.*?\*\*Referências para anexar:\*\* (.*?)\n.*?```text\n(.*?)```", re.S)
    return {m[1]: {"refs": [r.strip()[:-4] for r in m[2].split(",")], "prompt": m[3].strip()} for m in pat.finditer(texto)}


def gerar(stem, dados):
    refs = [r for r in (achar(x) for x in dados["refs"]) if r]
    with tempfile.TemporaryDirectory() as tmp:
        cmd = ["codex", "exec", "--skip-git-repo-check", "--sandbox", "read-only", "-C", tmp]
        for r in refs:
            cmd += ["-i", str(r)]
        cmd.append("-")
        r = subprocess.run(cmd, input=instrucao(stem).format(prompt=dados["prompt"]), capture_output=True, text=True,
                           encoding="utf-8", timeout=900, shell=False)
    nomes = re.findall(r"exec-[0-9a-f-]+\.png", (r.stdout or "") + (r.stderr or ""))
    achados = [p for n in nomes for p in GERADAS.glob(f"*/{n}")]
    if not achados:
        return f"{stem}: FALHOU (codex saiu com {r.returncode}): {(r.stdout or '')[-300:]}"
    im = Image.open(achados[-1])
    if not stem.startswith(("tela_", "cena_")) and (im.mode != "RGBA" or im.getextrema()[-1][0] > 0):
        return f"{stem}: REJEITADA (sem transparência: {im.mode})"
    (ASSETS / "_raw").mkdir(exist_ok=True)
    shutil.copy(achados[-1], ASSETS / "_raw" / f"{stem}_codex.png")
    shutil.copy(achados[-1], ASSETS / f"{stem}.png")
    return f"{stem}: ok {im.size}"


def main():
    global MD
    args = sys.argv[1:]
    if len(args) >= 2 and args[0] == "--md":
        MD = ROOT / args[1]
        args = args[2:]
    todos = prompts()
    pedidos = args or [s for s in todos if not achar(s)]
    pedidos.sort(key=lambda s: s.endswith("_walk"))   # ações primeiro: o andar usa a folha de ações como referência
    print(f"{len(pedidos)} folhas: {' '.join(pedidos)}", flush=True)
    for s in pedidos:
        print(gerar(s, todos[s]), flush=True)


if __name__ == "__main__":
    main()
