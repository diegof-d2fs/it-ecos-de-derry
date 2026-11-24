"""Gera os quadros das cut scenes (v6) que faltam com o Codex CLI (gerador de imagens da OpenAI).

Alternativa ao batch_images.py quando o Gemini está indisponível (teto de gastos, 23/11/2026). Usa o
MESMO prompt de instructions/Prompts-Assets-IT-v6-cenas.md e, como referência de estilo, quadros já
aprovados da própria série (mais o sprite do personagem, quando há um ou dois em cena). O original
vem em ~16:9 (1672 × 941); aqui ele é recortado ao centro para 16:9 e vira o master em assets/
(o bruto fica em assets/_raw/<nome>_codex.png).

Uso: python tools/codex_cenas.py [cena_b7_1 ...]   (sem argumentos: todos os cena_* sem master)
Depois: python tools/export_final.py && python tools/export_final.py --hd
"""
import re
import shutil
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
MD = ROOT / "instructions" / "Prompts-Assets-IT-v6-cenas.md"
# Quadros aprovados da série: dizem ao gerador "é esta a arte" melhor que qualquer descrição.
ESTILO = ["cena_b7_4", "cena_b6_4", "referencia_visual"]
GRUPO = ["tela_perdedores"]   # cenas com os sete juntos
WORKERS = 3
# O Codex guarda o original aqui. O sandbox dele não grava fora da própria pasta (23/11: "acesso negado"
# ao copiar para um temporário), então o script acha o arquivo pelo nome que aparece na resposta.
GERADAS = Path.home() / ".codex" / "generated_images"

INSTRUCAO = (
    "Use your image generation tool to create ONE illustration. The attached images are the art-style reference: "
    "frames of the same cutscene series of this game (16-bit RPG pixel art, same palette, lighting and finish). "
    "Match that style exactly; if a character sprite is attached, it defines that character's identity. "
    "Landscape, as wide as possible; keep everything important inside the central 16:9 band. "
    "Do not resize, copy or edit any file: just generate it and reply with the full path of the generated PNG. "
    "Description (Portuguese):\n\n{prompt}"
)


def achar(stem):
    for ext in (".png", ".jpg", ".jpeg"):
        p = ASSETS / f"{stem}{ext}"
        if p.exists():
            return p
    return None


def prompts():
    texto = MD.read_text(encoding="utf-8")
    pat = re.compile(r"\*\*Arquivo:\*\* (cena_\S+)\.png.*?\*\*Referências para anexar:\*\* (.*?)\n.*?```text\n(.*?)```", re.S)
    return {m[1]: {"refs": [r.strip()[:-4] for r in m[2].split(",")], "prompt": m[3].strip()} for m in pat.finditer(texto)}


def recortar_16x9(im):
    w, h = im.size
    alvo = round(w * 9 / 16)
    if alvo <= h:
        topo = (h - alvo) // 2
        return im.crop((0, topo, w, topo + alvo))
    larg = round(h * 16 / 9)
    esq = (w - larg) // 2
    return im.crop((esq, 0, esq + larg, h))


def gerar(stem, dados):
    pessoais = [r for r in dados["refs"] if r not in ("referencia_visual", "cartao_clube")]
    elenco = "Personagens" in dados["prompt"] and not pessoais   # 3+ personagens: o prompt não anexa sprites
    refs = [achar(r) for r in ESTILO + pessoais + (GRUPO if elenco else [])]
    refs = [r for r in refs if r]
    with tempfile.TemporaryDirectory() as tmp:
        cmd = ["codex", "exec", "--skip-git-repo-check", "--sandbox", "read-only", "-C", tmp]
        for r in refs:
            cmd += ["-i", str(r)]
        cmd.append("-")
        r = subprocess.run(cmd, input=INSTRUCAO.format(prompt=dados["prompt"]), capture_output=True, text=True,
                           encoding="utf-8", timeout=900, shell=False)
    nomes = re.findall(r"exec-[0-9a-f-]+\.png", (r.stdout or "") + (r.stderr or ""))
    achados = [p for n in nomes for p in GERADAS.glob(f"*/{n}")]
    if not achados:
        return f"{stem}: FALHOU (codex saiu com {r.returncode}): {(r.stdout or '')[-300:]}"
    im = Image.open(achados[-1]).convert("RGB")
    if im.width < 1200:
        return f"{stem}: REJEITADA (veio reduzida: {im.size})"
    (ASSETS / "_raw").mkdir(exist_ok=True)
    shutil.copy(achados[-1], ASSETS / "_raw" / f"{stem}_codex.png")
    final = recortar_16x9(im)
    final.save(ASSETS / f"{stem}.png")
    return f"{stem}: ok {im.size} -> {final.size}"


def main():
    todos = prompts()
    pedidos = sys.argv[1:] or [s for s in todos if not achar(s)]
    print(f"{len(pedidos)} quadros: {' '.join(pedidos)}", flush=True)
    with ThreadPoolExecutor(WORKERS) as ex:
        for msg in ex.map(lambda s: gerar(s, todos[s]), pedidos):
            print(msg, flush=True)


if __name__ == "__main__":
    main()
