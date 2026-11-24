"""Testes locais da fronteira privada e dos recordes na API AWS."""
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import types
import unittest


class ClientError(Exception):
    def __init__(self, code):
        self.response = {"Error": {"Code": code}}


class S3Falso:
    def __init__(self):
        self.objetos = {}

    def get_object(self, Bucket, Key):
        if Key not in self.objetos:
            raise ClientError("NoSuchKey")
        corpo, etag = self.objetos[Key]
        return {"Body": io.BytesIO(corpo), "ETag": etag}

    def put_object(self, Bucket, Key, Body, **opcoes):
        anterior = self.objetos.get(Key)
        if opcoes.get("IfNoneMatch") == "*" and anterior:
            raise ClientError("PreconditionFailed")
        if opcoes.get("IfMatch") and (not anterior or anterior[1] != opcoes["IfMatch"]):
            raise ClientError("PreconditionFailed")
        etag = '"' + hashlib.md5(Body).hexdigest() + '"'
        self.objetos[Key] = Body, etag
        return {"ETag": etag}


sys.modules["boto3"] = types.SimpleNamespace(client=lambda nome: None)
sys.modules["botocore"] = types.ModuleType("botocore")
excecoes = types.ModuleType("botocore.exceptions")
excecoes.ClientError = ClientError
sys.modules["botocore.exceptions"] = excecoes
os.environ["SAVE_BUCKET"] = "privado-teste"
spec = importlib.util.spec_from_file_location("save_api", Path(__file__).with_name("save_api.py"))
api = importlib.util.module_from_spec(spec)
spec.loader.exec_module(api)


def evento(metodo, rota, sub=None, nick="Jogador", body=None):
    contexto = {"http": {"method": metodo, "sourceIp": "127.0.0.1"}}
    if sub:
        contexto["authorizer"] = {"jwt": {"claims": {"sub": sub, "username": nick}}}
    return {"rawPath": rota, "requestContext": contexto,
            "body": json.dumps(body, ensure_ascii=False) if body is not None else None}


def resposta(ev):
    bruto = api.handler(ev, None)
    return bruto["statusCode"], json.loads(bruto["body"])


class TesteApi(unittest.TestCase):
    def setUp(self):
        api._S3 = S3Falso()

    def test_sem_login_nao_acessa_rotas(self):
        for metodo, rota in [("GET", "/ranking"), ("GET", "/saves"),
                             ("GET", "/profile"), ("POST", "/feedback"), ("GET", "/admin")]:
            self.assertEqual(401, resposta(evento(metodo, rota))[0])

    def test_cartao_privado_e_conflito(self):
        a = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
        b = "bbbbbbbb-bbbb-bbbb-bbbb-bbbbbbbbbbbb"
        status, salvo = resposta(evento("PUT", "/saves", a, body={"files": {"perfil.txt": "A"}, "version": None}))
        self.assertEqual(200, status)
        self.assertEqual({"files": {}, "version": None}, resposta(evento("GET", "/saves", b))[1])
        self.assertEqual("A", resposta(evento("GET", "/saves", a))[1]["files"]["perfil.txt"])
        self.assertEqual(409, resposta(evento("PUT", "/saves", a, body={"files": {}, "version": '"deadbeef"'}))[0])
        self.assertEqual(salvo["version"], resposta(evento("GET", "/saves", a))[1]["version"])

    def test_tres_rankings_so_mostram_nick_bandeira_pontos(self):
        sub = "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa"
        self.assertEqual(200, resposta(evento("PUT", "/profile", sub, "Diego", {"country": "BR"}))[0])
        linhas = [f"{p};2026-11-24 12:00;{d};100;7;1;123;0"
                  for p, d in [(4500, "facil"), (5000, "normal"), (9000, "dificil")]]
        status, _ = resposta(evento("PUT", "/saves", sub, "Diego",
                                     {"files": {"recordes.txt": "\n".join(linhas)}, "version": None}))
        self.assertEqual(200, status)
        ranking = resposta(evento("GET", "/ranking", sub))[1]["ranking"]
        self.assertEqual({"facil", "normal", "dificil"}, set(ranking))
        for nivel, score in [("facil", 4500), ("normal", 5000), ("dificil", 9000)]:
            self.assertEqual([{"nick": "Diego", "country": "BR", "score": score}], ranking[nivel])
        self.assertNotIn("sub", json.dumps(ranking))
        self.assertNotIn("email", json.dumps(ranking))
        top = resposta(evento("GET", "/profile", sub))[1]["top5"]
        self.assertEqual("dificil", top[0]["difficulty"])


if __name__ == "__main__":
    unittest.main()
