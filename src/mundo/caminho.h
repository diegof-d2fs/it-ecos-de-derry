#pragma once
// Caminhos na grade (SDD §12.2): BFS de 4 vizinhos, no máximo 40×24 = 960 nós. Ninguém
// atravessa parede: o caminho só passa por células que não bloqueiam (ou não bloqueiam voo).
#include "core/tipos.h"
#include "mundo/sala.h"

// Próxima célula a partir de `origem` rumo a `destino`. Devolve a própria origem se já
// chegou e {-1, -1} se não há caminho.
Celula proximoPasso(const SalaViva& sala, Celula origem, Celula destino, bool voa);

// Distância em passos (4 vizinhos); -1 se inalcançável.
int distanciaCaminho(const SalaViva& sala, Celula origem, Celula destino, bool voa);
