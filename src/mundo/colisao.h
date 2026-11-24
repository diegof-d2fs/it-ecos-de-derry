#pragma once
// Colisão de corpos com os tiles da sala (SDD §10, critérios F05 e F06).
// Cada corpo é uma CAIXA nos pés (vista 3/4: só os pés colidem; a cabeça pode passar na
// frente de uma parede). O movimento é feito eixo X e depois eixo Y, em sub-passos curtos:
// isso dá o "deslizamento" natural nas paredes e impede atravessar parede fina em alta
// velocidade (dash).
#include "core/tipos.h"
#include "mundo/sala.h"

constexpr float SUBPASSO_MAXIMO = 4.0f;   // px por sub-passo (bem menor que um tile)

// Caixa de tamanho `tamanho` centrada no ponto dos pés.
Retangulo caixaNosPes(Vec2 pes, Vec2 tamanho);

// A caixa encosta em algum tile que bloqueia (movimento, ou voo se `voa`)?
bool colideComSala(const SalaViva& sala, const Retangulo& caixa, bool voa);

// Move os pés por `delta` respeitando as paredes; devolve a posição final.
// Em colisão, encosta exatamente na borda do tile e continua no outro eixo.
Vec2 moverComColisao(const SalaViva& sala, Vec2 pes, Vec2 tamanho, Vec2 delta, bool voa);

// Porta da sala tocada pela caixa (nullptr se nenhuma). Ponteiro: "não achou" é válido (T03).
const PortaSala* portaTocada(const SalaViva& sala, Vec2 pes, Vec2 tamanho);
