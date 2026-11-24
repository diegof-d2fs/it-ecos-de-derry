#pragma once
// Visão e escuridão (SDD §7.5, critério F11). Regras puras: decide o que Bill ENXERGA; o render
// só pinta. A sala começa toda desconhecida; a lanterna ilumina um raio com linha de visão
// (paredes e portas fechadas bloqueiam); o que já foi visto fica "explorado" (penumbra).
//
// Algoritmo: shadowcasting SIMÉTRICO (Albert Ford): se A vê B, então B vê A. Isso evita
// injustiças (um inimigo que atinge Bill de um lugar que Bill não enxerga) e é testado.
#include <array>
#include <cstdint>

#include "core/tipos.h"
#include "mundo/sala.h"

constexpr float RAIO_LANTERNA = 6.0f;   // células (PRD §14.1); Ecos/companheiro podem somar

struct CelulaVisao {
    bool explorada = false;       // já foi vista alguma vez nesta sala (persiste)
    std::uint8_t luz = 0;         // 0..255 neste quadro (0 = fora da visão agora)
};

struct MapaVisao {
    int cols = 0, lins = 0;
    std::array<CelulaVisao, SALA_MAX_CELULAS> celulas{};
};

// Sala nova: tudo desconhecido.
void iniciarVisao(MapaVisao& visao, const SalaViva& sala);

// Recalcula a luz a partir da posição (pixels do mundo) com o raio dado, e marca explorado.
void atualizarVisao(MapaVisao& visao, const SalaViva& sala, Vec2 origem, float raioCelulas);

// Revela a sala inteira (gerador do Mike no bioma 5; Modo Defesa).
void revelarTudo(MapaVisao& visao);

const CelulaVisao& visaoEm(const MapaVisao& visao, int col, int lin);   // fora = desconhecida
// O ponto (pixels do mundo) está iluminado AGORA? Quem decide se um ator aparece é esta regra.
bool estaVisivel(const MapaVisao& visao, Vec2 posicao);

// A célula `b` é visível a partir de `a` (independe do raio)? Usada pelos testes de simetria.
// `alcanceCelulas` limita a varredura (inimigos só olham até a distância de detecção).
bool linhaDeVisao(const SalaViva& sala, Celula a, Celula b, int alcanceCelulas = SALA_MAX_COLS + SALA_MAX_LINS);

// O SEGMENTO a→b (pixels) não cruza nenhuma célula que bloqueia: com `paraAndar`, o que bloqueia
// o movimento (parede, água funda…); sem, o que bloqueia a visão (parede, pilar). Amostra a cada
// 4 px. Diferente de linhaDeVisao (célula a célula, que aceita quinas): é o caminho reto de VERDADE.
bool segmentoLivre(const SalaViva& sala, Vec2 a, Vec2 b, bool paraAndar);
