#pragma once
// Escuridão (SDD §7.5, passo 7 do §7.4). Lê o MapaVisao (regras) e escurece o canvas:
//   iluminada agora  → cor plena, com a borda da lanterna suave;
//   explorada        → 35% (o jogador lembra do terreno);
//   desconhecida     → preto.
// A luz de cada célula é interpolada pixel a pixel (bilinear), sem degraus de 32 px.
#include "mundo/visao.h"
#include "render/canvas.h"

constexpr float LUZ_MEMORIA = 0.35f;

void aplicarEscuridao(Canvas& canvas, const MapaVisao& visao, float camX, float camY);
