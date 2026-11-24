#pragma once
// Joystick (SDD §6, PRD v2 §17). Ordem de tentativa, aprendida no Bomberman:
//   1. SDL2.dll oficial ao lado do exe (GameController: Xbox, PlayStation, Switch Pro, 8BitDo);
//   2. XInput (xinput1_4 → xinput1_3 → xinput9_1_0), se a SDL2 faltar;
//   3. nenhum: só teclado.
// Nunca DirectInput (lição nº 1: entradas fantasmas no 8BitDo).
#include <string>

#include "core/controle.h"

// Carrega a SDL2 (ou o XInput). Nunca falha: sem nada, o jogo segue só com o teclado.
void iniciarGamepad(const std::wstring& pastaExecutavel);
void encerrarGamepad();

// Chamada uma vez por quadro. Com controle: lê o estado e percebe a desconexão (barato).
// Sem controle: procura um novo no máximo a cada 2 s (a busca é cara — lição nº 2).
// `info.origem == Nenhuma` depois da chamada = sem controle; `info.leitura` fica zerada.
void atualizarGamepad(InfoControle& info, double agoraSegundos);

// Testes por captura sem controle físico: as leituras passam a vir de um roteiro fixo.
void simularGamepad(bool ativo);
