#pragma once
// Mini framework de testes (sem dependências). Cada TESTE se registra sozinho;
// main_testes.cpp executa todos e retorna 1 se algum VERIFICAR falhar.
#include <cmath>
#include <cstdio>
#include <vector>

struct RegistroTeste {
    const char* nome;
    void (*funcao)();
};

std::vector<RegistroTeste>& registroTestes();
extern int g_falhas;
extern int g_verificacoes;

struct AutoRegistro {
    AutoRegistro(const char* nome, void (*funcao)()) { registroTestes().push_back({nome, funcao}); }
};

#define TESTE(nome)                                         \
    static void nome();                                     \
    static AutoRegistro registro_##nome(#nome, nome);       \
    static void nome()

#define VERIFICAR(condicao)                                                          \
    do {                                                                             \
        ++g_verificacoes;                                                            \
        if (!(condicao)) {                                                           \
            ++g_falhas;                                                              \
            std::printf("    FALHOU %s:%d  %s\n", __FILE__, __LINE__, #condicao);    \
        }                                                                            \
    } while (0)

#define VERIFICAR_PERTO(a, b, tolerancia) VERIFICAR(std::fabs((a) - (b)) <= (tolerancia))
