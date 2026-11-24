#pragma once
// Menus repaginados (Diego, 24/11/2026: "repaginar todo o menu de inventário, itens e árvore de habilidades,
// inspire-se em Resident Evil, Final Fantasy e Hades; algo profissional e bonito").
//
//   * Inventário: o Bill de corpo inteiro com vida, XP e atributos à esquerda (Hades); abas de caderno no alto;
//     a lista em placas no meio (Final Fantasy) e o item escolhido em destaque, grande, numa moldura de latão
//     com a descrição (Resident Evil).
//   * Subir de nível: os quatro atributos em medalhões, com a prévia do que muda.
//   * Mural dos Perdedores (talentos): o quadro de cortiça do Clube, os talentos como polaroides presas em três
//     colunas (Coragem, Preparo, Os amigos) ligadas por fios vermelhos, e o detalhe do escolhido ao lado.
// As regras (cursor, abas, compra) continuam em jogo/: aqui só se desenha.
#include "jogo/partida.h"
#include "jogo/perfil.h"
#include "render/canvas.h"
#include "render/recursos.h"

void desenharTelaInventario(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo);
void desenharTelaNivel(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo);
void desenharTelaMural(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const Perfil& pf, float tempo);

// Grade do Mural: o talento i fica na coluna i / 3 e na linha i % 3 (↑↓ andam na coluna, ← → trocam de coluna).
constexpr int COLUNAS_MURAL = 3;
