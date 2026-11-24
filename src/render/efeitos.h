#pragma once
// Desenho de áreas no chão usado pelos golpes.
// Usa a MESMA função das regras (pontoNaArea): o que se vê é exatamente o que acerta.
#include <cstdint>

#include "combate/area.h"
#include "render/canvas.h"

// `hachura`: listras diagonais (o aviso não depende só da cor: PRD §15.6).
void desenharArea(Canvas& canvas, const Area& area, float camX, float camY, std::uint32_t cor, std::uint8_t alfaPreenchimento,
                  std::uint8_t alfaContorno, bool hachura);
