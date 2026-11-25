#!/usr/bin/env bash
# Cria a API, Cognito e bucket privado depois que a distribuição estática existe.
# Requer autorização explícita para os recursos de uso variável.
set -euo pipefail
cd "$(dirname "$0")/.."
# .env: AWS_ACCESS_KEY_ID=... ou aws_access_key_id=... (como no "credentials" da AWS); o nome vira maiúsculo.
if [ -f .env ]; then set -a; source <(grep -iE '^(AWS|ECOS)_[A-Z0-9_]+=' .env | tr -d '\r' | sed -E 's/^([^=]+)=/\U\1=/'); set +a; fi
if [ ! -f web/deploy.local.json ]; then
    echo "Primeiro execute bash tools/publicar_web.sh preparar" >&2
    exit 1
fi
REGIAO=us-east-1
# O e-mail do administrador fica só no .env local (ignorado pelo Git): ECOS_ADMIN_EMAIL=...
ADMIN_EMAIL="${ECOS_ADMIN_EMAIL:?Defina ECOS_ADMIN_EMAIL no .env}"
CONTA=$(aws sts get-caller-identity --query Account --output text)
BUCKET=$(python -c 'import json; print(json.load(open("web/deploy.local.json"))["bucket"])')
DOMINIO=$(python -c 'import json; print(json.load(open("web/deploy.local.json"))["dominio"])')
# Domínio próprio (opcional, "dominioProprio" no deploy.local.json): o jogo aceita os dois endereços.
PROPRIO=$(python -c 'import json; print(json.load(open("web/deploy.local.json")).get("dominioProprio", ""))')
SITE="https://$DOMINIO"
RETORNO="https://$DOMINIO/"
if [ -n "$PROPRIO" ]; then SITE="https://$PROPRIO,$SITE"; RETORNO="https://$PROPRIO/,$RETORNO"; fi
SAVE_BUCKET="it-ecos-de-derry-saves-${CONTA: -6}"
PREFIXO="ecos-de-derry-${CONTA: -6}"
CHAVE="interno/save-api-$(sha256sum web/save_api.py | cut -c1-16).zip"
python -c 'import zipfile; z=zipfile.ZipFile("web/lambda.local.zip","w",zipfile.ZIP_DEFLATED); z.write("web/save_api.py","save_api.py"); z.close()'
aws s3 cp web/lambda.local.zip "s3://$BUCKET/$CHAVE" --content-type application/zip --region "$REGIAO" > /dev/null
aws cloudformation deploy --region "$REGIAO" --stack-name ecos-de-derry-backend \
    --template-file web/infra.yaml --capabilities CAPABILITY_IAM --tags projeto=ecos-de-derry \
    --no-fail-on-empty-changeset \
    --parameter-overrides "SiteOrigin=$SITE" "CallbackUrl=$RETORNO" "CodeBucket=$BUCKET" \
    "CodeKey=$CHAVE" "SaveBucketName=$SAVE_BUCKET" "CognitoPrefix=$PREFIXO" "AdminEmail=$ADMIN_EMAIL"
aws cloudformation describe-stacks --region "$REGIAO" --stack-name ecos-de-derry-backend \
    --query 'Stacks[0].Outputs' --output json > web/outputs.local.json
python -c 'import json; o={x["OutputKey"]:x["OutputValue"] for x in json.load(open("web/outputs.local.json"))}; s=json.load(open("web/deploy.local.json")); c={"apiUrl":o["ApiUrl"],"cognitoDomain":o["CognitoDomain"],"clientId":o["ClientId"],"wikiUrl":"https://diegof-d2fs.github.io/it-ecos-de-derry/"}; json.dump(c,open("web/config.local.json","w"),indent=2)'
python tools/preparar_branding.py
aws cognito-idp update-managed-login-branding --region "$REGIAO" \
    --cli-input-json file://web/branding.local.json --query ManagedLoginBranding.ManagedLoginBrandingId --output text
if ! aws sesv2 get-email-identity --region "$REGIAO" --email-identity "$ADMIN_EMAIL" > /dev/null 2>&1; then
    aws sesv2 create-email-identity --region "$REGIAO" --email-identity "$ADMIN_EMAIL" > /dev/null
    echo "A AWS enviou a verificação SES para $ADMIN_EMAIL. Clique no link antes de testar feedback."
fi
echo "Backend configurado. Teste login, saves, ranking e feedback antes de publicar os arquivos web."
