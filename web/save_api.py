"""API privada dos cartões de memória da versão web.

O API Gateway valida o access token do Cognito antes de chamar esta função.
Cada objeto pertence ao `sub` imutável da conta, nunca ao nome de usuário.
"""

import base64
import datetime
import hashlib
import json
import os
import re
import uuid

import boto3
from botocore.exceptions import ClientError


BUCKET = os.environ.get("SAVE_BUCKET", "")
_S3 = None
_BLOCO = re.compile(r"bloco_(0[1-9]|1[0-5])\.txt\Z")
_SUB = re.compile(r"[a-zA-Z0-9-]{1,80}\Z")
_ARQUIVOS = {"perfil.txt", "recordes.txt", "telemetria.csv"}
_LIMITE_CORPO = 1024 * 1024
_FEEDBACK_EMAIL = os.environ.get("FEEDBACK_EMAIL", "")
_ADMIN_EMAIL = os.environ.get("DEMO_ADMIN_EMAIL", "")
_USER_POOL_ID = os.environ.get("USER_POOL_ID", "")
_RANKING = "ranking/mundial.json"
_NICK = re.compile(r"(?=.*[A-Za-z0-9])[A-Za-z0-9._-]{3,20}\Z")
_ETAG = re.compile(r'"[A-Za-z0-9-]{1,100}"\Z')
_PAISES = set("""
AD AE AF AG AI AL AM AO AQ AR AS AT AU AW AX AZ BA BB BD BE BF BG BH BI BJ BL BM BN BO BQ BR BS BT BV BW BY BZ CA CC CD CF CG CH CI CK CL CM CN CO CR CU CV CW CX CY CZ DE DJ DK DM DO DZ EC EE EG EH ER ES ET FI FJ FK FM FO FR GA GB GD GE GF GG GH GI GL GM GN GP GQ GR GS GT GU GW GY HK HM HN HR HT HU ID IE IL IM IN IO IQ IR IS IT JE JM JO JP KE KG KH KI KM KN KP KR KW KY KZ LA LB LC LI LK LR LS LT LU LV LY MA MC MD ME MF MG MH MK ML MM MN MO MP MQ MR MS MT MU MV MW MX MY MZ NA NC NE NF NG NI NL NO NP NR NU NZ OM PA PE PF PG PH PK PL PM PN PR PS PT PW PY QA RE RO RS RU RW SA SB SC SD SE SG SH SI SJ SK SL SM SN SO SR SS ST SV SX SY SZ TC TD TF TG TH TJ TK TL TM TN TO TR TT TV TW TZ UA UG UM US UY UZ VA VC VE VG VI VN VU WF WS YE YT ZA ZM ZW
""".split())


def _s3():
    global _S3
    if _S3 is None:
        _S3 = boto3.client("s3")
    return _S3


def _resposta(status, corpo):
    return {
        "statusCode": status,
        "headers": {"Content-Type": "application/json; charset=utf-8", "Cache-Control": "no-store"},
        "body": json.dumps(corpo, ensure_ascii=False, separators=(",", ":")),
    }


def _codigo_erro(exc):
    return exc.response.get("Error", {}).get("Code", "")


def _ler(chave):
    try:
        obj = _s3().get_object(Bucket=BUCKET, Key=chave)
        return json.loads(obj["Body"].read()), obj["ETag"]
    except ClientError as exc:
        if _codigo_erro(exc) in ("NoSuchKey", "404"):
            return None, None
        raise


def _melhores(texto):
    pontos = []
    for linha in texto.splitlines():
        campos = linha.split(";")
        if len(campos) == 8 and campos[7].strip() == "0" and campos[0].isdigit() and campos[2] in ("facil", "normal", "dificil"):
            valor = int(campos[0])
            if valor <= 1_000_000_000:
                pontos.append({"score": valor, "difficulty": campos[2]})
    return sorted(pontos, key=lambda e: -e["score"])[:10]


def _atualizar_ranking(sub, nick, pais=None, pontos=None):
    if not _NICK.fullmatch(nick):
        return
    for _ in range(10):
        dados, versao = _ler(_RANKING)
        entradas = dados.get("entries", []) if isinstance(dados, dict) else []
        anteriores = {e.get("difficulty"): e for e in entradas if e.get("sub") == sub}
        pais_atual = pais or next((e.get("country") for e in anteriores.values()), None)
        antes = sorted((e.get("difficulty"), e.get("score"), e.get("country"), e.get("nick")) for e in anteriores.values())
        entradas = [e for e in entradas if e.get("sub") != sub]
        if pais_atual in _PAISES:
            for dificuldade in ("facil", "normal", "dificil"):
                valor = max((pontos or {}).get(dificuldade, 0), anteriores.get(dificuldade, {}).get("score", 0))
                if valor > 0:
                    entradas.append({"sub": sub, "nick": nick, "country": pais_atual,
                                     "difficulty": dificuldade, "score": valor})
        depois = sorted((e["difficulty"], e["score"], e["country"], e["nick"]) for e in entradas if e.get("sub") == sub)
        if depois == antes:
            return   # nada novo: evita disputar o arquivo único do ranking a cada salvamento
        entradas.sort(key=lambda e: (e["difficulty"], -e["score"], e["nick"].casefold()))
        entradas = [e for d in ("facil", "normal", "dificil")
                    for e in [x for x in entradas if x["difficulty"] == d][:100]]
        corpo = json.dumps({"entries": entradas}, separators=(",", ":")).encode()
        try:
            _s3().put_object(Bucket=BUCKET, Key=_RANKING, Body=corpo,
                             ContentType="application/json",
                             **({"IfMatch": versao} if versao else {"IfNoneMatch": "*"}))
            return
        except ClientError as exc:
            if _codigo_erro(exc) not in ("PreconditionFailed", "ConditionalRequestConflict", "412", "409"):
                raise
    raise RuntimeError("Ranking concorrido")


def _corpo(event):
    bruto = event.get("body") or ""
    if event.get("isBase64Encoded"):
        bruto = base64.b64decode(bruto, validate=True)
    if isinstance(bruto, str):
        bruto = bruto.encode("utf-8")
    if len(bruto) > _LIMITE_CORPO:
        raise ValueError("Corpo grande")
    return json.loads(bruto)


def _feedback(event, sub, nick):
    try:
        dados = _corpo(event)
        tipo, titulo, mensagem = dados.get("type"), dados.get("title"), dados.get("message")
        email, isca = dados.get("email", ""), dados.get("apelido", "")
        if isca:
            return _resposta(200, {"received": True})
        if (tipo not in {"bug", "elogio", "melhoria", "critica", "outro"} or
            not isinstance(titulo, str) or not 3 <= len(titulo.strip()) <= 100 or
            not isinstance(mensagem, str) or not 5 <= len(mensagem.strip()) <= 5000 or
            not isinstance(email, str) or len(email) > 254 or
            (email and not re.fullmatch(r"[^@\s]+@[^@\s]+\.[^@\s]+", email))):
            return _resposta(400, {"erro": "Preencha o assunto e a mensagem corretamente."})
    except (ValueError, UnicodeError, TypeError, AttributeError):
        return _resposta(400, {"erro": "Feedback inválido."})
    agora = datetime.datetime.now(datetime.timezone.utc)
    limite = hashlib.sha256(f"{sub}:{agora:%Y%m%d%H}".encode()).hexdigest()
    for indice in range(3):
        try:
            _s3().put_object(Bucket=BUCKET, Key=f"feedback/limites/{limite}-{indice}", Body=b"1",
                             IfNoneMatch="*")
            break
        except ClientError as exc:
            if _codigo_erro(exc) not in ("PreconditionFailed", "412"):
                raise
    else:
        return _resposta(429, {"erro": "Limite de três mensagens por hora. Tente mais tarde."})
    protocolo = uuid.uuid4().hex
    registro = {"id": protocolo, "date": agora.isoformat(), "account": sub, "nick": nick, "type": tipo,
                "title": titulo.strip(), "message": mensagem.strip(), "replyEmail": email}
    _s3().put_object(Bucket=BUCKET, Key=f"feedback/mensagens/{agora:%Y/%m/%d}/{protocolo}.json",
                     Body=json.dumps(registro, ensure_ascii=False).encode(), ContentType="application/json")
    enviado = False
    if _FEEDBACK_EMAIL:
        try:
            boto3.client("sesv2").send_email(
                FromEmailAddress=_FEEDBACK_EMAIL,
                Destination={"ToAddresses": [_FEEDBACK_EMAIL]},
                Content={"Simple": {"Subject": {"Data": f"[Ecos de Derry] {tipo}: {titulo.strip()}", "Charset": "UTF-8"},
                                    "Body": {"Text": {"Data": f"Protocolo: {protocolo}\nApelido: {nick}\nContato: {email or 'não informado'}\n\n{mensagem.strip()}",
                                                      "Charset": "UTF-8"}}}})
            enviado = True
        except ClientError:
            pass
    return _resposta(200, {"received": True, "protocol": protocolo, "emailed": enviado})


def _arquivos_validos(arquivos):
    if not isinstance(arquivos, dict) or len(arquivos) > 18:
        return False
    tamanho = 0
    for nome, texto in arquivos.items():
        if nome not in _ARQUIVOS and not _BLOCO.fullmatch(nome):
            return False
        if not isinstance(texto, str):
            return False
        tamanho += len(texto.encode("utf-8"))
        if tamanho > _LIMITE_CORPO:
            return False
    return True


def handler(event, context):
    metodo = ((event.get("requestContext") or {}).get("http") or {}).get("method", "")
    caminho = event.get("rawPath", "")
    if not BUCKET:
        return _resposta(503, {"erro": "Serviço indisponível."})
    claims = (((event.get("requestContext") or {}).get("authorizer") or {}).get("jwt") or {}).get("claims") or {}
    sub = claims.get("sub", "")
    if not isinstance(sub, str) or not _SUB.fullmatch(sub):
        return _resposta(401, {"erro": "Conta não autenticada."})
    nick = claims.get("username") or claims.get("cognito:username") or ""
    if not isinstance(nick, str):
        nick = ""
    if caminho == "/admin" and metodo == "GET":
        if not (_ADMIN_EMAIL and _USER_POOL_ID):
            return _resposta(403, {"allowed": False})
        try:
            usuario = boto3.client("cognito-idp").admin_get_user(UserPoolId=_USER_POOL_ID, Username=nick or sub)
            atributos = {a["Name"]: a["Value"] for a in usuario.get("UserAttributes", [])}
            permitido = (atributos.get("sub") == sub and atributos.get("email", "").casefold() == _ADMIN_EMAIL.casefold()
                         and atributos.get("email_verified") == "true" and usuario.get("Enabled") is True)
        except ClientError:
            permitido = False
        return _resposta(200 if permitido else 403, {"allowed": permitido})
    if caminho == "/ranking" and metodo == "GET":
        dados, _ = _ler(_RANKING)
        entradas = dados.get("entries", []) if isinstance(dados, dict) else []
        # Projeção pública explícita: nunca retorna e-mail, sub nem arquivos salvos.
        publico = {}
        for dificuldade in ("facil", "normal", "dificil"):
            publico[dificuldade] = [
                {"nick": e["nick"], "country": e["country"], "score": e["score"]}
                for e in entradas if isinstance(e, dict) and e.get("difficulty") == dificuldade and
                _NICK.fullmatch(str(e.get("nick", ""))) and e.get("country") in _PAISES and
                isinstance(e.get("score"), int)
            ][:100]
        return _resposta(200, {"ranking": publico})
    if caminho == "/feedback" and metodo == "POST":
        return _feedback(event, sub, nick)
    if caminho == "/profile":
        chave_perfil = f"usuarios/{sub}/perfil_publico.json"
        if metodo == "GET":
            perfil, _ = _ler(chave_perfil)
            cartao, _ = _ler(f"usuarios/{sub}/cartao.json")
            texto = (cartao or {}).get("files", {}).get("recordes.txt", "")
            return _resposta(200, {"nick": nick, "country": (perfil or {}).get("country"),
                                   "top5": _melhores(texto)[:5]})
        if metodo != "PUT":
            return _resposta(405, {"erro": "Método não permitido."})
        try:
            pais = _corpo(event).get("country")
            if pais not in _PAISES:
                return _resposta(400, {"erro": "País inválido."})
        except (ValueError, UnicodeError, TypeError, AttributeError):
            return _resposta(400, {"erro": "Perfil inválido."})
        _s3().put_object(Bucket=BUCKET, Key=chave_perfil,
                         Body=json.dumps({"country": pais}).encode(), ContentType="application/json")
        try:
            _atualizar_ranking(sub, nick, pais=pais)
            pendente = False
        except (ClientError, RuntimeError):
            pendente = True
        return _resposta(200, {"nick": nick, "country": pais, "rankingPending": pendente})

    if caminho != "/saves":
        return _resposta(404, {"erro": "Rota desconhecida."})

    chave = f"usuarios/{sub}/cartao.json"
    if metodo == "GET":
        try:
            objeto = _s3().get_object(Bucket=BUCKET, Key=chave)
            dados = json.loads(objeto["Body"].read())
            if not _arquivos_validos(dados.get("files")):
                return _resposta(503, {"erro": "Memory card salvo inválido. Nenhum dado foi alterado."})
            return _resposta(200, {"files": dados["files"], "version": objeto["ETag"]})
        except ClientError as exc:
            if exc.response.get("Error", {}).get("Code") in ("NoSuchKey", "404"):
                return _resposta(200, {"files": {}, "version": None})
            raise

    if metodo != "PUT":
        return _resposta(405, {"erro": "Método não permitido."})
    try:
        dados = _corpo(event)
        arquivos = dados.get("files")
        versao = dados.get("version")
        if not _arquivos_validos(arquivos) or (versao is not None and not isinstance(versao, str)):
            return _resposta(400, {"erro": "Memory card inválido."})
        if isinstance(versao, str) and not _ETAG.fullmatch(versao):
            return _resposta(400, {"erro": "Versão inválida."})
        corpo = json.dumps({"files": arquivos}, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        opcoes = {"IfMatch": versao} if versao else {"IfNoneMatch": "*"}
        salvo = _s3().put_object(Bucket=BUCKET, Key=chave, Body=corpo, ContentType="application/json", **opcoes)
        perfil, _ = _ler(f"usuarios/{sub}/perfil_publico.json")
        melhores = _melhores(arquivos.get("recordes.txt", ""))
        pendente = False
        if melhores and isinstance(perfil, dict) and perfil.get("country") in _PAISES:
            try:
                por_dificuldade = {d: max((r["score"] for r in melhores if r["difficulty"] == d), default=0)
                                   for d in ("facil", "normal", "dificil")}
                _atualizar_ranking(sub, nick, pais=perfil["country"], pontos=por_dificuldade)
            except (ClientError, RuntimeError):
                pendente = True
        return _resposta(200, {"version": salvo["ETag"], "rankingPending": pendente})
    except (ValueError, UnicodeError, TypeError):
        return _resposta(400, {"erro": "Memory card inválido."})
    except ClientError as exc:
        if exc.response.get("Error", {}).get("Code") in ("PreconditionFailed", "ConditionalRequestConflict", "412", "409"):
            return _resposta(409, {"erro": "O memory card foi alterado em outro dispositivo. Recarregue antes de salvar."})
        raise
