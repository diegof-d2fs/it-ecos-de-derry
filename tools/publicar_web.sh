#!/usr/bin/env bash
# Publica a versão web na AWS (Etapa 18): S3 PRIVADO + CloudFront (HTTPS) com Origin Access Control.
#
#   * o bucket não é público (Block Public Access ligado): só a distribuição do CloudFront lê os arquivos;
#   * HTTPS no domínio padrão *.cloudfront.net (domínio próprio fica para depois, com o Route 53 e o ACM);
#   * compressão (gzip/brotli) e cache pela política gerenciada CachingOptimized; cabeçalhos de segurança pela
#     política gerenciada SecurityHeadersPolicy;
#   * idempotente: rodar de novo só reenvia os arquivos e invalida o cache.
#
# Uso (Git Bash), com as credenciais da AWS no ambiente (ou no .env):
#   bash tools/publicar_web.sh
# Antes: tools/export_web.py e o build web (emcmake cmake -S . -B build-web && cmake --build build-web).
set -euo pipefail
cd "$(dirname "$0")/.."

if [ -f .env ]; then set -a; eval "$(grep -E '^AWS_' .env | tr -d '\r')"; set +a; fi
REGIAO="${AWS_REGION:-us-east-1}"
ESTADO="web/deploy.local.json"   # ids do bucket e da distribuição (fora do git)
mkdir -p web
CONTA=$(aws sts get-caller-identity --query Account --output text)
BUCKET="it-ecos-de-derry-${CONTA: -6}"

# ---------- 1. pacote ----------
DIST="web/dist"
rm -rf "$DIST" && mkdir -p "$DIST/assets/musics"
cp build-web/index.html build-web/index.js build-web/index.wasm build-web/index.data "$DIST/"
cp assets/musics/*.mp3 "$DIST/assets/musics/"
echo "pacote: $(du -sh "$DIST" | cut -f1)"

# ---------- 2. bucket privado ----------
if ! aws s3api head-bucket --bucket "$BUCKET" 2>/dev/null; then
    if [ "$REGIAO" = "us-east-1" ]; then aws s3api create-bucket --bucket "$BUCKET" --region "$REGIAO" > /dev/null
    else aws s3api create-bucket --bucket "$BUCKET" --region "$REGIAO" --create-bucket-configuration LocationConstraint="$REGIAO" > /dev/null; fi
    echo "bucket criado: $BUCKET"
fi
aws s3api put-public-access-block --bucket "$BUCKET" --public-access-block-configuration \
    BlockPublicAcls=true,IgnorePublicAcls=true,BlockPublicPolicy=true,RestrictPublicBuckets=true
aws s3api put-bucket-encryption --bucket "$BUCKET" --server-side-encryption-configuration \
    '{"Rules":[{"ApplyServerSideEncryptionByDefault":{"SSEAlgorithm":"AES256"}}]}'

# ---------- 3. CloudFront (OAC + distribuição) ----------
DIST_ID=""
if [ -f "$ESTADO" ]; then DIST_ID=$(python -c "import json;print(json.load(open('$ESTADO')).get('distribuicao',''))" 2>/dev/null || true); fi
if [ -z "$DIST_ID" ]; then
    OAC_ID=$(aws cloudfront create-origin-access-control --origin-access-control-config \
        "{\"Name\":\"$BUCKET-oac\",\"SigningProtocol\":\"sigv4\",\"SigningBehavior\":\"always\",\"OriginAccessControlOriginType\":\"s3\"}" \
        --query OriginAccessControl.Id --output text)
    CONFIG=$(cat <<JSON
{
  "CallerReference": "it-ecos-de-derry-$(date +%s)",
  "Comment": "IT: Ecos de Derry (web)",
  "Enabled": true,
  "DefaultRootObject": "index.html",
  "HttpVersion": "http2and3",
  "PriceClass": "PriceClass_All",
  "Origins": {"Quantity": 1, "Items": [{
      "Id": "s3-jogo", "DomainName": "$BUCKET.s3.$REGIAO.amazonaws.com",
      "OriginAccessControlId": "$OAC_ID",
      "S3OriginConfig": {"OriginAccessIdentity": ""}}]},
  "DefaultCacheBehavior": {
      "TargetOriginId": "s3-jogo",
      "ViewerProtocolPolicy": "redirect-to-https",
      "Compress": true,
      "CachePolicyId": "658327ea-f89d-4fab-a63d-7e88639e58f6",
      "ResponseHeadersPolicyId": "67f7725c-6f97-4210-82d7-5512b31e9d03",
      "AllowedMethods": {"Quantity": 2, "Items": ["GET", "HEAD"], "CachedMethods": {"Quantity": 2, "Items": ["GET", "HEAD"]}}}
}
JSON
)
    DIST_ID=$(aws cloudfront create-distribution --distribution-config "$CONFIG" --query Distribution.Id --output text)
    ARN=$(aws cloudfront get-distribution --id "$DIST_ID" --query Distribution.ARN --output text)
    DOMINIO=$(aws cloudfront get-distribution --id "$DIST_ID" --query Distribution.DomainName --output text)
    # Só esta distribuição lê o bucket.
    aws s3api put-bucket-policy --bucket "$BUCKET" --policy "{
      \"Version\": \"2012-10-17\",
      \"Statement\": [{\"Sid\": \"SoCloudFront\", \"Effect\": \"Allow\",
        \"Principal\": {\"Service\": \"cloudfront.amazonaws.com\"},
        \"Action\": \"s3:GetObject\", \"Resource\": \"arn:aws:s3:::$BUCKET/*\",
        \"Condition\": {\"StringEquals\": {\"AWS:SourceArn\": \"$ARN\"}}}]}"
    printf '{"bucket": "%s", "regiao": "%s", "distribuicao": "%s", "dominio": "%s"}\n' "$BUCKET" "$REGIAO" "$DIST_ID" "$DOMINIO" > "$ESTADO"
    echo "distribuição criada: $DIST_ID ($DOMINIO)"
fi

# ---------- 4. arquivos ----------
aws s3 cp "$DIST/index.html" "s3://$BUCKET/index.html" --content-type "text/html; charset=utf-8" --cache-control "no-cache"
aws s3 cp "$DIST/index.js" "s3://$BUCKET/index.js" --content-type "text/javascript" --cache-control "max-age=300"
aws s3 cp "$DIST/index.wasm" "s3://$BUCKET/index.wasm" --content-type "application/wasm" --cache-control "max-age=300"
aws s3 cp "$DIST/index.data" "s3://$BUCKET/index.data" --content-type "application/octet-stream" --cache-control "max-age=300"
aws s3 sync "$DIST/assets/musics" "s3://$BUCKET/assets/musics" --content-type "audio/mpeg" --cache-control "max-age=86400" --delete
aws cloudfront create-invalidation --distribution-id "$DIST_ID" --paths "/*" --query Invalidation.Id --output text > /dev/null
DOMINIO=$(aws cloudfront get-distribution --id "$DIST_ID" --query Distribution.DomainName --output text)
echo "publicado: https://$DOMINIO/"
