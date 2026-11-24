"""Monta o estilo do login Cognito com a arte e as cores do menu do jogo."""
import base64
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
outputs = {item["OutputKey"]: item["OutputValue"]
           for item in json.loads((root / "web/outputs.local.json").read_text())}
arte = (root / "assets/jogo_web/cena_menu_inicial.png").read_bytes()
if not arte.startswith(b"\xff\xd8\xff"):
    raise SystemExit("A imagem do menu precisa ser JPEG para o branding.")

pedido = {
    "UserPoolId": outputs["UserPoolId"],
    # O !Ref do CloudFormation devolve "pool|id"; a API quer só o id.
    "ManagedLoginBrandingId": outputs["BrandingId"].split("|")[-1],
    "Settings": {
        "categories": {"global": {"colorSchemeMode": "DARK"}},
        "components": {
            "pageBackground": {"image": {"enabled": True}, "darkMode": {"color": "04050aff"}},
            "form": {"borderRadius": 2.0,
                     "darkMode": {"backgroundColor": "080a12ee", "borderColor": "8c6b3dff"}},
            "pageText": {"darkMode": {"bodyColor": "d6c8afff", "descriptionColor": "c7b799ff",
                                      "headingColor": "f4ead9ff"}},
            "primaryButton": {"darkMode": {
                "defaults": {"backgroundColor": "7c1824ff", "textColor": "f5e5c4ff"},
                "hover": {"backgroundColor": "a62230ff", "textColor": "ffffffff"}}},
            "secondaryButton": {"darkMode": {
                "defaults": {"backgroundColor": "181c29ff", "borderColor": "a58248ff",
                             "textColor": "f5e5c4ff"}}}
        },
        "componentClasses": {"buttons": {"borderRadius": 2.0},
                             "input": {"borderRadius": 2.0}}
    },
    "Assets": [{"Category": "PAGE_BACKGROUND", "ColorMode": "DARK", "Extension": "JPEG",
                "Bytes": base64.b64encode(arte).decode()}]
}
alvo = root / "web/branding.local.json"
alvo.write_text(json.dumps(pedido), encoding="utf-8")
print(f"Branding preparado: {len(arte)} bytes de arte, arquivo local ignorado pelo Git.")
