"""Geração em lote dos assets faltantes de IT Ecos de Derry via Gemini API.

Uso:
    python tools/batch_images.py --dry-run          # planeja, não chama a API
    python tools/batch_images.py --only P38         # piloto de uma imagem
    python tools/batch_images.py                    # gera tudo que falta (idempotente)

Decisões:
- Modelo: gemini-3-pro-image (Nano Banana Pro), o mesmo do app Gemini Pro usado
  nas imagens já aprovadas; melhor aderência a imagens de referência.
- Limites Tier 1 do modelo: 20 RPM, 100K TPM, 250 RPD. O limitador usa 75% de
  folga (15 RPM, 75K TPM) e um ledger diário em disco para o RPD.
- Ondas por dependência: retratos e HUD dependem de assets gerados na onda 1.
- A API não devolve canal alfa: pedimos fundo verde chroma sólido e recortamos
  localmente. O bruto da API fica em assets/_raw/.
"""
import argparse
import base64
import io
import json
import random
import re
import sys
import threading
import time
from collections import deque
from concurrent.futures import ThreadPoolExecutor
from datetime import date
from pathlib import Path

import requests
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / "assets"
RAW = ASSETS / "_raw"
# Pacotes de prompts (v1: P01–P55; v2: P56–P105). Ficam em instructions/.
PROMPT_FILES = [ROOT / "instructions" / "Prompts-Assets-IT.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v2.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v3-texturas.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v4-andar.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v5-fx.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v6-cenas.md",
                ROOT / "instructions" / "Prompts-Assets-IT-v8-pennywise.md"]
STATE = ROOT / "tools" / "batch_state.json"

MODEL = "gemini-3-pro-image"
ENDPOINT = f"https://generativelanguage.googleapis.com/v1beta/models/{MODEL}:generateContent"
RPM_LIMIT, TPM_LIMIT, RPD_LIMIT = 20, 100_000, 250
SAFETY = 0.75
WORKERS = 3
MAX_RETRIES = 6
REF_MAX_SIDE = 1024
CHROMA = (0, 255, 0)

# Referências por asset (nomes lógicos; resolvemos .png/.jpg no disco).
# "Um ícone/retrato aprovado" do prompt original vira um asset concreto aqui.
ITEM_REFS = ["referencia_visual", "item_kit", "item_bateria"]
# FX e HUD usam ícones isolados como referência: a cena completa fazia o modelo
# devolver colagens e molduras em vez de frames soltos sobre o fundo.
FX_REFS = ["item_kit", "item_bateria"]
REFS = {
    "P25": ["referencia_visual", "galeria"],
    **{f"P{n}": ITEM_REFS for n in range(30, 37)},
    "P37": ["item_kit", "item_bateria", "item_chave_ferro", "item_taco", "item_escudo"],
    "P38": ["bill_referencia", "bill_walk"],
    "P39": ["ben_idle"],
    "P40": ["eddie_idle"],
    "P41": ["beverly_idle"],
    "P42": ["mike_idle"],
    "P43": ["elias_idle"],
    "P44": ["pennywise_referencia"],
    **{f"P{n}": FX_REFS for n in (45, 46, 47, 48, 51, 52)},
    "P49": FX_REFS + ["pennywise_4_actions"],
    "P50": FX_REFS + ["pennywise_4_actions"],
    "P53": ["referencia_visual", "bill_referencia", "pennywise_referencia"],
    # beverly_idle.jpg e mike_idle.jpg originais saíram corrompidos: identidade vem
    # do retrato aprovado e escala/layout vêm do sprite de espera do Ben.
    "P09": ["ben_idle", "mike_idle", "retrato_beverly"],
    "P10": ["referencia_visual", "ben_idle", "_rejeitadas/retrato_mike"],
    "P14": ["pennywise_referencia", "pennywise_1_actions", "pennywise_3_actions"],
    "P54": ["objetos"],
    "P55": ["capa_menu", "bill_referencia", "ben_idle", "eddie_idle", "beverly_idle", "mike_idle"],
}

CHROMA_NOTE = (
    " Observação técnica para esta geração: como a saída não suporta canal alfa, "
    "pinte TODO o fundo com verde chroma puro e uniforme #00FF00, sem gradiente, sem "
    "sombra projetada no fundo e sem xadrez; não use esse verde no asset. O recorte do "
    "fundo será feito depois. As imagens anexadas definem identidade visual, paleta e "
    "acabamento de pixel art; siga-as fielmente. Nunca pinte água, chão, ladrilhos, painéis, quadrados "
    "coloridos, faixas ou cenário atrás das figuras: cada figura fica isolada sobre o verde puro, com "
    "espaço verde entre as células."
)
FX_NOTE = (
    " A imagem inteira é o fundo verde: não desenhe moldura, faixas pretas ou brancas, "
    "painel, caixa, xadrez, cenário ou tiles. Apenas os frames do efeito, soltos, lado a lado "
    "em uma única linha horizontal, com espaçamento uniforme."
)
OPAQUE_NOTE = (
    " As imagens anexadas definem identidade visual, paleta e acabamento de pixel art; "
    "siga-as fielmente."
)


# Notas extras por asset, aplicadas por cima do prompt original.
_SO_ESCALA = (" Os sprites anexados de outras crianças servem SOMENTE para escala, enquadramento e acabamento "
              "de pixel art: NÃO copie rosto, cabelo, pele nem roupa deles. Personagem inteiro, dos pés à "
              "cabeça, nos dois frames, sem cenário e sem chão.")
EXTRA = {
    "P58": _SO_ESCALA + " Este é o Georgie, bem menor que os anexos: capa de chuva amarela com capuz e galochas vermelhas."
           " Imagem simples: apenas os dois sprites de Georgie sobre o verde; nada de fotos, cenas de filme, molduras ou outros personagens.",
    "P61": " Pele com tom saudável e normal de criança; apenas um brilho levemente amarelado nos olhos sugere a ilusão."
           " Sem pele verde, sem olhos vermelhos, sem aparência de monstro.",
    "P63": " Sem faixa de água no fundo: apenas uma pequena poça oval sob os pés em cada frame, isolada sobre o verde.",
    "P65": " Sem chão de ladrilhos: em cada célula, apenas o ralo redondo e os cabelos, isolados sobre o verde.",
    "P68": " Uma única sanguessuga GRANDE por célula, ocupando cerca de 70% da célula, 16 células em 4 × 4; sem cenário, sem chão.",
    "P70": " Exatamente 16 bonecos no total: UM único boneco por célula (nunca dois ou três juntos), grande,"
           " ocupando cerca de 80% da altura da célula, de corpo inteiro. O pino de boliche só aparece NA MÃO do"
           " boneco nos frames de ataque, nunca solto numa célula. Sem quadrados coloridos atrás; isolados sobre o verde.",
    "P79": " Nenhum texto, letra, palavra ou número em nenhum objeto. Item 7: quadro-negro com apenas RABISCOS e"
           " desenhos de giz (bonequinhos, setas, um balão), sem nenhuma letra. Item 12: pôster rasgado com apenas"
           " uma estrela desenhada, sem palavras. Sem placas, quadrados ou chão de fundo atrás dos objetos.",
    "P80": " Sem quadrados de fundo, água ou chão ao redor dos objetos (exceto a água que faz parte da comporta). Objetos grandes, ocupando a célula.",
    # P85/P86: o master vem em GRADE (MASTER_GRID no export_final.py remonta a linha única).
    "P85": " IGNORE a instrução de uma linha: desenhe os 8 ícones numa GRADE de 4 colunas × 2 linhas, na ordem"
           " de leitura (linha de cima: 1 a 4; linha de baixo: 5 a 8). Cada célula tem UM ícone grande e"
           " centralizado, todos diferentes entre si; nenhum ícone repetido, nada empilhado na mesma célula,"
           " bastante verde entre as células. Nenhum ícone tem quadrado, placa, moldura ou fundo escuro atrás: a chama azul e a estrela ficam DIRETO sobre o verde, só com contorno escuro de 1 pixel.",
    "P86": " IGNORE a instrução de uma linha: desenhe os 6 emblemas numa GRADE de 3 colunas × 2 linhas, na ordem"
           " de leitura (linha de cima: Ben, Stan, Eddie; linha de baixo: Beverly, Mike, Richie). Cada célula tem"
           " exatamente UM emblema circular grande, do mesmo tamanho dos outros; nenhum emblema sobreposto,"
           " duplicado ou menor; bastante verde entre eles.",
    "P93": " A coluna de luz é um objeto com contorno: núcleo branco-azulado, borda azul média e contorno azul-escuro"
           " de 1 pixel em volta da forma. FORA desse contorno, todo pixel é verde #00FF00: não existe céu, noite,"
           " retângulo ou fundo escuro atrás da luz. As partículas também têm contorno escuro.",
    "P94": " A névoa escura é uma NUVEM de bordas recortadas e irregulares, menor que o frame (cerca de 70%), com"
           " verde #00FF00 em volta por todos os lados: nunca um retângulo ou quadro escuro. Os três globos ficam"
           " dentro da nuvem.",
    # v4: no 1º lote o Eddie saiu como um ninja de anime (faixa vermelha na testa, roupa escura): rejeitado.
    "P152": " IDENTIDADE OBRIGATÓRIA: o Eddie do sprite anexado — camiseta AMARELA clara, short AZUL, meias brancas,"
            " tênis, bolsinha marrom a tiracolo, cabelo castanho curto penteado para o lado. Sem faixa na testa, sem"
            " roupa escura, sem estilo de anime ou ninja: o mesmo acabamento de pixel art do sprite anexado.",
    "P159": " SEM balão em nenhum quadro: as duas mãos livres e iguais nos 12 frames (no 1º lote o balão aparecia"
            " e sumia de um quadro para o outro).",
    "P166": " Nenhuma linha, borda ou divisória entre as células: só o verde puro entre os pássaros (no 1º lote"
            " vieram linhas pretas de grade). O mesmo tamanho de pássaro nos 12 frames.",
    # v5 (efeitos): no 1º lote, 4 vieram com fundo cinza/xadrez, painéis coloridos ou faixa de paleta.
    "P169": " A IMAGEM INTEIRA tem fundo verde #00FF00 puro e uniforme, do primeiro ao último pixel: sem cinza, sem xadrez, sem painéis ou quadrados de cor atrás dos frames, sem faixa de paleta, sem linhas divisórias. A água é verde-azulada clara com espuma branca, bem diferente do fundo verde-limão #00FF00.",
    "P171": " A IMAGEM INTEIRA tem fundo verde #00FF00 puro e uniforme, do primeiro ao último pixel: sem cinza, sem xadrez, sem painéis ou quadrados de cor atrás dos frames, sem faixa de paleta, sem linhas divisórias.",
    "P172": " A IMAGEM INTEIRA tem fundo verde #00FF00 puro e uniforme, do primeiro ao último pixel: sem cinza, sem xadrez, sem painéis ou quadrados de cor atrás dos frames, sem faixa de paleta, sem linhas divisórias. Cada frame é só o estouro, solto sobre o verde: sem retângulo escuro, sem quadro, sem fundo azul ou cinza atrás da explosão.",
    "P173": " A IMAGEM INTEIRA tem fundo verde #00FF00 puro e uniforme, do primeiro ao último pixel: sem cinza, sem xadrez, sem painéis ou quadrados de cor atrás dos frames, sem faixa de paleta, sem linhas divisórias.",
    "P56": _SO_ESCALA + " Este é o Richie: óculos grandes de lentes grossas são obrigatórios.",
    "P57": _SO_ESCALA + " Este é o Stan: cabelo castanho-claro cacheado, camisa polo verde-oliva por dentro da calça cáqui.",
    "P25": " Todos os 16 tiles são totalmente preenchidos e opacos; nenhuma área branca, vazia ou recortada.",
    "P37": " Exatamente UMA linha com sete símbolos, sem repetir a linha.",
    "P45": " Exatamente seis frames em UMA única linha horizontal, em ordem: ponto de luz, feixe curto, "
           "feixe médio, feixe forte, feixe enfraquecendo, últimas faíscas.",
    "P49": " Exatamente quatro frames, sem linhas divisórias verticais entre eles.",
    "P51": " Cada frame fica direto sobre o fundo verde, sem quadrado ou painel escuro atrás.",
    "P09": " Personagem inteiro, dos pés à cabeça, nos dois frames, como nos sprites de espera anexados. "
           "Sem cenário, sem chão, sem paredes. Esta é a Beverly do retrato anexado, não o Ben nem o Mike.",
    "P54": " Sem nenhum texto ou legenda. Itens 5, 6 e 7 (balões) flutuam isolados sobre o fundo verde puro, SEM laje, SEM piso, SEM pedra embaixo; apenas uma pequena sombra oval. Itens 1 a 4 seguem o encaixe das portas de objetos.png.",
    "P55": " Exatamente cinco crianças, nem mais nem menos. Não desenhe caixa de diálogo nem moldura: "
           "a faixa inferior é só cenário calmo, como calçada molhada.",
    "P10": " Personagem inteiro, dos pés à cabeça, nos dois frames. Este é o Mike do retrato anexado, não o Ben.",
    "P14": " Siga o layout de pennywise_1_actions: 12 poses de corpo inteiro em grade de 4 colunas e 3 linhas, "
           "não uma folha de modelo com vistas.",
    "P40": " Enquadramento de busto: apenas cabeça, pescoço e ombros, sem mãos e sem cintura.",
    "P42": " Enquadramento de busto: apenas cabeça, pescoço e ombros, sem mãos e sem cintura.",
}
# Proporção pedida ao gerador quando o master usa outra grade (ver MASTER_GRID no export).
ASPECT_OVERRIDE = {"P85": "16:9", "P86": "3:2",
                   # v4 (andar): o master tem 3 linhas (frente, perfil direito, costas); o export espelha a esquerda
                   **{f"P{n}": "4:5" for n in range(151, 162)}, **{f"P{n}": "4:3" for n in range(162, 167)},
                   # v8 (Pennywise +): andar em 3 linhas, como as folhas de andar aprovadas
                   "P222": "4:5", "P224": "4:5", "P226": "4:5"}
PORTRAITS = {"P39": "P07", "P40": "P08", "P41": "P09", "P42": "P10", "P43": "P11"}


def identity(prompts, pid):
    m = re.search(r"desta adaptação: (.*?)\. Dois frames", prompts[PORTRAITS[pid]]["prompt"])
    return (f" IDENTIDADE OBRIGATÓRIA: {m[1]}. Este personagem NÃO é o Bill: não use jaqueta azul "
            "nem o rosto do Bill. Copie rosto, cabelo, pele e roupa do sprite anexado.")


def load_key():
    for line in (ROOT / ".env").read_text(encoding="utf-8").splitlines():
        if line.startswith("API_GEMINI="):
            return line.split("=", 1)[1].strip().strip("\"'")
    sys.exit("API_GEMINI não encontrada no .env")


def parse_prompts():
    """Lê todos os pacotes de prompts. IDs têm 2 ou 3 dígitos (P01…P105)."""
    pat = re.compile(
        r"## (P\d{2,3}) (.*?)\n\n\*\*Arquivo:\*\* (\S+).*?\*\*Dimensões finais:\*\* (.*?)\n"
        r"(?:\n\*\*Referências para anexar:\*\* (.*?)\n)?.*?```text\n(.*?)```",
        re.S,
    )
    prompts = {}
    for path in PROMPT_FILES:
        if not path.exists():
            sys.exit(f"Pacote de prompts não encontrado: {path}")
        for m in pat.finditer(path.read_text(encoding="utf-8")):
            refs = [r.strip()[:-4] for r in (m[5] or "").split(",") if r.strip().endswith(".png")]
            prompts[m[1]] = {"id": m[1], "title": m[2], "file": m[3], "dims": m[4],
                             "refs_md": refs, "prompt": m[6].strip()}
    return prompts


def find_asset(stem):
    for ext in (".png", ".jpg", ".jpeg"):
        p = ASSETS / f"{stem}{ext}"
        if p.exists():
            return p
    return None


def aspect_ratio(dims):
    w, h = map(int, re.match(r"(\d+) × (\d+)", dims).groups())
    options = {"1:1": 1, "4:3": 4 / 3, "3:4": 3 / 4, "16:9": 16 / 9, "9:16": 9 / 16,
               "21:9": 21 / 9, "3:2": 1.5, "2:3": 2 / 3, "5:4": 1.25, "4:5": 0.8}
    return min(options, key=lambda k: abs(options[k] - w / h))


def needs_alpha(prompt):
    return "Fundo realmente transparente" in prompt


# ---------- rate limiting ----------

class Limiter:
    """Janela deslizante de 60s para RPM e TPM, mais ledger diário para RPD."""

    def __init__(self, state):
        self.rpm = int(RPM_LIMIT * SAFETY)
        self.tpm = int(TPM_LIMIT * SAFETY)
        self.events = deque()  # (ts, tokens)
        self.lock = threading.Lock()
        self.state = state

    def _used_today(self):
        return self.state["daily"].get(str(date.today()), 0)

    def acquire(self, est_tokens):
        while True:
            with self.lock:
                if self._used_today() >= RPD_LIMIT * SAFETY:
                    raise RuntimeError("Cota diária (com folga) atingida; retome amanhã.")
                now = time.time()
                while self.events and now - self.events[0][0] > 60:
                    self.events.popleft()
                tokens = sum(t for _, t in self.events)
                if len(self.events) < self.rpm and tokens + est_tokens <= self.tpm:
                    entry = [now, est_tokens]
                    self.events.append(entry)
                    day = str(date.today())
                    self.state["daily"][day] = self._used_today() + 1
                    return entry
                wait = 60 - (now - self.events[0][0]) + 0.5
            time.sleep(max(wait, 1))

    def settle(self, entry, actual_tokens):
        with self.lock:
            entry[1] = actual_tokens


# ---------- API ----------

def encode_ref(path):
    im = Image.open(path)
    im = im.convert("RGBA") if im.mode in ("RGBA", "LA", "P") else im.convert("RGB")
    if im.mode == "RGBA":  # achata transparência sobre o chroma para coerência com o pedido
        bg = Image.new("RGBA", im.size, CHROMA + (255,))
        im = Image.alpha_composite(bg, im).convert("RGB")
    im.thumbnail((REF_MAX_SIDE, REF_MAX_SIDE), Image.NEAREST)
    buf = io.BytesIO()
    im.save(buf, "PNG")
    return {"inline_data": {"mime_type": "image/png", "data": base64.b64encode(buf.getvalue()).decode()}}


def call_api(key, job, log):
    parts = [encode_ref(p) for p in job["ref_paths"]]
    parts.append({"text": job["full_prompt"]})
    body = {
        "contents": [{"role": "user", "parts": parts}],
        "generationConfig": {
            "responseModalities": ["IMAGE"],
            # Cut scenes (v6): 2K, mesmo preço do 1K; a ilustração ocupa a tela inteira no jogo.
            "imageConfig": {"aspectRatio": job["aspect"], "imageSize": "2K" if job["stem"].startswith("cena_") else "1K"},
        },
    }
    for attempt in range(MAX_RETRIES):
        try:
            r = requests.post(ENDPOINT, headers={"x-goog-api-key": key}, json=body, timeout=300)
        except requests.RequestException as e:
            delay = min(60, 2 ** attempt * 3) + random.random() * 2
            log(f"{job['id']} rede: {e!r}; nova tentativa em {delay:.0f}s")
            time.sleep(delay)
            continue
        if r.status_code == 200:
            data = r.json()
            for cand in data.get("candidates", []):
                for part in cand.get("content", {}).get("parts", []):
                    blob = part.get("inline_data") or part.get("inlineData")
                    if blob:
                        return base64.b64decode(blob["data"]), data.get("usageMetadata", {})
            reason = [c.get("finishReason") for c in data.get("candidates", [])] or data.get("promptFeedback")
            log(f"{job['id']} sem imagem na resposta ({reason}); tentativa {attempt + 1}")
            if attempt >= 2:
                raise RuntimeError(f"sem imagem: {reason}")
            continue
        if r.status_code == 429 and "spending cap" in r.text:
            # Teto de gastos mensal do projeto: retentar não adianta.
            raise SystemExit("HTTP 429: teto de gastos mensal atingido. Aumente em https://ai.studio/spend")
        if r.status_code in (429, 500, 502, 503, 504):
            delay = min(120, 2 ** attempt * 5) + random.random() * 3
            m = re.search(r'"retryDelay":\s*"(\d+)', r.text)
            if m:
                delay = max(delay, int(m[1]) + 1)
            msg = re.search(r'"message":\s*"([^"]{0,200})', r.text)
            quota = re.findall(r'"quotaId":\s*"([^"]+)', r.text)
            detalhe = (msg[1] if msg else r.text[:200]).replace("\\n", " ")
            if quota:
                detalhe += f" | quotaId={','.join(sorted(set(quota)))}"
            log(f"{job['id']} HTTP {r.status_code}; aguardando {delay:.0f}s (tentativa {attempt + 1}) — {detalhe}")
            time.sleep(delay)
            continue
        raise RuntimeError(f"HTTP {r.status_code}: {r.text[:500]}")
    raise RuntimeError("tentativas esgotadas")


# ---------- pós-processamento ----------

def chroma_key(im, tol=110):
    """Remove o fundo verde chroma. A cor real do fundo é estimada pelas bordas."""
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()
    border = [px[x, y][:3] for x in range(0, w, 4) for y in (0, h - 1)]
    border += [px[x, y][:3] for y in range(0, h, 4) for x in (0, w - 1)]
    greenish = [c for c in border if c[1] > 150 and c[1] - max(c[0], c[2]) > 80] or [CHROMA]
    bg = tuple(sorted(c[i] for c in greenish)[len(greenish) // 2] for i in range(3))
    out = []
    for r, g, b, a in im.get_flattened_data():
        d = ((r - bg[0]) ** 2 + (g - bg[1]) ** 2 + (b - bg[2]) ** 2) ** 0.5
        if d < tol or (g > 180 and g - max(r, b) > 120):
            out.append((0, 0, 0, 0))
        else:
            if g > max(r, b):  # tira o halo verde das bordas
                g = max(r, b)
            out.append((r, g, b, a))
    im.putdata(out)
    return im


# ---------- orquestração ----------

def load_state():
    if STATE.exists():
        return json.loads(STATE.read_text(encoding="utf-8"))
    return {"daily": {}, "jobs": {}}


def save_state(state, lock):
    with lock:
        STATE.write_text(json.dumps(state, indent=2, ensure_ascii=False), encoding="utf-8")


def build_jobs(prompts, only, force=False):
    jobs = []
    for pid, p in prompts.items():
        if only and pid not in only:
            continue
        stem = Path(p["file"]).stem
        existing = find_asset(stem)
        # --force: o original só vai para _rejeitadas DEPOIS que a imagem nova chegar (em work()).
        # Antes ele era movido aqui e, se a API falhasse, o asset sumia do jogo.
        if existing and not (force and only):
            continue
        alpha = needs_alpha(p["prompt"])
        jobs.append({
            "id": pid, "stem": stem, "title": p["title"], "dims": p["dims"],
            "refs": REFS.get(pid) or p["refs_md"] or ["referencia_visual"],
            "aspect": ASPECT_OVERRIDE.get(pid) or aspect_ratio(p["dims"]), "alpha": alpha,
            "full_prompt": p["prompt"] + (identity(prompts, pid) if pid in PORTRAITS else "")
            + EXTRA.get(pid, "") + (CHROMA_NOTE if alpha else OPAQUE_NOTE)
            + (FX_NOTE if re.match(r"fx_|hud_", stem) else ""),
        })
    return jobs


def run(args):
    prompts = parse_prompts()
    jobs = build_jobs(prompts, set(args.only) if args.only else None, args.force)
    pending_stems = {j["stem"] for j in jobs}
    # onda = profundidade de dependência entre jobs pendentes
    for j in jobs:
        j["deps"] = [r for r in j["refs"] if r in pending_stems]
        j["wave"] = 2 if j["deps"] else 1

    print(f"Modelo {MODEL} | limites efetivos {int(RPM_LIMIT*SAFETY)} RPM, "
          f"{int(TPM_LIMIT*SAFETY)} TPM, {int(RPD_LIMIT*SAFETY)} RPD | {WORKERS} workers")
    for j in sorted(jobs, key=lambda j: (j["wave"], j["id"])):
        print(f"  onda {j['wave']} {j['id']} {j['stem']:<22} {j['aspect']:<5} "
              f"alpha={'sim' if j['alpha'] else 'não'} refs={','.join(j['refs'])}")
    if args.dry_run or not jobs:
        return

    key = load_key()
    state = load_state()
    limiter = Limiter(state)
    lock = threading.Lock()
    RAW.mkdir(exist_ok=True)

    def log(msg):
        print(time.strftime("%H:%M:%S"), msg, flush=True)

    def work(job):
        missing = [r for r in job["refs"] if not find_asset(r)]
        if missing:
            log(f"{job['id']} pulado: referências ausentes {missing}")
            return False
        job["ref_paths"] = [find_asset(r) for r in job["refs"]]
        # estimativa conservadora até calibrar pelo usageMetadata
        est = state.get("tok_per_ref", 1300) * len(job["ref_paths"]) + 2500
        entry = limiter.acquire(est)
        t0 = time.time()
        try:
            img_bytes, usage = call_api(key, job, log)
        except Exception as e:
            limiter.settle(entry, est)
            state["jobs"][job["id"]] = {"status": "erro", "erro": str(e)[:300]}
            save_state(state, lock)
            log(f"{job['id']} FALHOU: {e}")
            return False
        total = usage.get("totalTokenCount", est)
        limiter.settle(entry, total)
        raw_path = RAW / f"{job['stem']}.png"
        raw_path.write_bytes(img_bytes)
        im = Image.open(io.BytesIO(img_bytes))
        if job["alpha"]:
            im = chroma_key(im)
        anterior = find_asset(job["stem"])
        if anterior:   # só agora, com a imagem nova em mãos, arquiva a versão anterior
            rejected = ASSETS / "_rejeitadas"
            rejected.mkdir(exist_ok=True)
            anterior.replace(rejected / anterior.name)
        im.save(ASSETS / f"{job['stem']}.png")
        state["jobs"][job["id"]] = {
            "status": "ok", "arquivo": f"{job['stem']}.png", "tamanho": im.size,
            "segundos": round(time.time() - t0, 1), "uso": usage,
        }
        save_state(state, lock)
        log(f"{job['id']} ok -> {job['stem']}.png {im.size} {total} tokens {time.time() - t0:.0f}s")
        return True

    for wave in (1, 2):
        batch = [j for j in jobs if j["wave"] == wave]
        if not batch:
            continue
        log(f"--- onda {wave}: {len(batch)} imagens ---")
        with ThreadPoolExecutor(WORKERS) as pool:
            list(pool.map(work, batch))
    save_state(state, lock)
    ok = sum(1 for j in jobs if state["jobs"].get(j["id"], {}).get("status") == "ok")
    log(f"Concluído: {ok}/{len(jobs)} geradas. Requisições hoje: {state['daily'].get(str(date.today()))}")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--only", nargs="*", help="IDs, ex.: P38 P25")
    ap.add_argument("--force", action="store_true",
                    help="com --only: move o asset atual para assets/_rejeitadas e regera")
    run(ap.parse_args())
