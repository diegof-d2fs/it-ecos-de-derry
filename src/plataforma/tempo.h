#pragma once
// Relógio de alta resolução e espera até o próximo quadro.

void iniciarRelogio();
void encerrarRelogio();
double agoraSegundos();
// Espera a próxima atualização do compositor (vsync do DWM). Se não houver compositor,
// dorme até completar `alvoSegundos` desde `inicioQuadro`.
void esperarProximoQuadro(double inicioQuadro, double alvoSegundos);
