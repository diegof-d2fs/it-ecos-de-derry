#include "plataforma/tempo.h"

#include <windows.h>
#include <dwmapi.h>
#include <mmsystem.h>

namespace {
double g_frequencia = 0.0;
}

void iniciarRelogio() {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    g_frequencia = static_cast<double>(f.QuadPart);
    timeBeginPeriod(1);   // Sleep com resolução de 1 ms
}

void encerrarRelogio() { timeEndPeriod(1); }

double agoraSegundos() {
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) / g_frequencia;
}

void esperarProximoQuadro(double inicioQuadro, double alvoSegundos) {
    // DwmFlush bloqueia até o compositor apresentar o próximo quadro: ritmo suave, sem
    // gastar CPU. Em monitores de 144 Hz desenhamos mais vezes; a lógica continua a 60 Hz.
    if (SUCCEEDED(DwmFlush())) {
        return;
    }
    for (;;) {
        const double restante = alvoSegundos - (agoraSegundos() - inicioQuadro);
        if (restante <= 0.0) return;
        if (restante > 0.002) {
            Sleep(static_cast<DWORD>((restante - 0.001) * 1000.0));
        }
    }
}
