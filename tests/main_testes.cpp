// Executa todos os testes das regras, sem abrir janela.
// Uso: testes_regras.exe            (todos)
//      testes_regras.exe trecho     (só os testes cujo nome contém o trecho)
#include <cstdio>
#include <cstring>

#include "teste.h"

int g_falhas = 0;
int g_verificacoes = 0;

std::vector<RegistroTeste>& registroTestes() {
    static std::vector<RegistroTeste> registro;
    return registro;
}

int main(int argc, char** argv) {
    const char* filtro = argc > 1 ? argv[1] : nullptr;
    int testesComFalha = 0, rodados = 0;
    for (const RegistroTeste& t : registroTestes()) {
        if (filtro && !std::strstr(t.nome, filtro)) continue;
        ++rodados;
        // O nome sai ANTES de rodar (e o buffer é descarregado): se o teste derrubar o processo,
        // a última linha impressa diz qual foi.
        std::printf("[ .. ] %s\r", t.nome);
        std::fflush(stdout);
        const int antes = g_falhas;
        t.funcao();
        const bool ok = g_falhas == antes;
        if (!ok) ++testesComFalha;
        std::printf("[%s] %s\n", ok ? " ok " : "FALHA", t.nome);
        std::fflush(stdout);
    }
    std::printf("\n%d testes, %d verificações, %d falhas\n", rodados, g_verificacoes, g_falhas);
    return testesComFalha == 0 ? 0 : 1;
}
