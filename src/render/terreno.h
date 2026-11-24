#pragma once
// Cache do terreno de uma sala (SDD §7.3.1 e §7.4, passo 1).
//
// O chão, a água e as paredes de uma sala não mudam a cada quadro: são desenhados UMA vez
// num canvas próprio do tamanho da sala (na escala da tela) e, a cada quadro, só o pedaço
// visível é copiado. Isso deixa barato aplicar, na criação do cache, as BORDAS ORGÂNICAS
// entre materiais (piso ↔ variante ↔ água), que pixel a pixel seriam caras por quadro.
// O cache é refeito quando muda a sala, a escala ou a versão do terreno (porta, ponte…).
#include "mundo/sala.h"
#include "render/canvas.h"
#include "render/recursos.h"

struct CacheTerreno {
    Canvas canvas;                        // cols×32 por lins×32 lógicos, na escala da tela
    const SalaModelo* modelo = nullptr;   // de qual sala é este cache
    int versao = -1;
    int escala = 0;
    bool valido = false;                  // false = sala grande demais para cache: desenha direto
};

// Garante o cache da sala atual (refaz só se algo mudou).
void prepararCacheTerreno(CacheTerreno& cache, const SalaViva& sala, const Recursos& recursos, int escala);
void liberarCacheTerreno(CacheTerreno& cache);

// Desenha o terreno com a câmera em (camX, camY) lógico. Sala menor que a tela: a câmera
// pode ser negativa (sala centralizada) e o resto fica preto.
void desenharTerreno(Canvas& canvas, const CacheTerreno& cache, const SalaViva& sala, const Recursos& recursos,
                     float camX, float camY);
