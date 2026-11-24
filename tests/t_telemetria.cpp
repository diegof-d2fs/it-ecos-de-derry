// Telemetria local (PRD §18, Etapa 8.5): uma linha por sala com o tempo de jogo e o tempo REAL
// (menus e diálogos congelam o cronômetro, mas contam para quem joga).
#include <algorithm>
#include <filesystem>

#include "jogo/piloto.h"
#include "jogo/telemetria.h"
#include "conteudo_teste.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}
}  // namespace

TESTE(telemetria_uma_linha_por_sala_e_o_tempo_fecha_com_o_cronometro) {
    Conteudo c = conteudoReal();
    limitarAoBioma(c.salas, 1);   // a tentativa termina no chefe do bioma 1 (fim do conteúdo)
    EstadoPartida p;
    std::string erro;
    VERIFICAR(iniciarTentativa(p, c, 42, Dificuldade::Normal, erro));
    Telemetria t;
    iniciarTelemetria(t, p);
    VERIFICAR(t.ativa && t.atual.sala == "b1_entrada");
    MemoriaPiloto m;
    int trocas = 0;
    for (int q = 0; q < 60 * 60 * 15 && !p.terminou; ++q) {
        const EstadoEntrada e = pilotar(p, m, DT);
        atualizarPartida(p, c, e, DT);
        p.sons.clear();
        trocas += p.trocouDeSala ? 1 : 0;
        passoTelemetria(t, p, DT);
    }
    VERIFICAR(p.fimDoConteudo);
    encerrarTelemetria(t, p);
    VERIFICAR(!t.ativa && static_cast<int>(t.salas.size()) == trocas + 1 && t.salas.size() == 8);
    float jogo = 0.0f, real = 0.0f;
    for (const SalaTelemetria& s : t.salas) {
        jogo += s.segundosJogo;
        real += s.segundosReais;
        VERIFICAR(s.segundosReais + 0.05f >= s.segundosJogo);   // o real nunca é menor que o de jogo
    }
    VERIFICAR_PERTO(jogo, p.stats.tempo, 0.05f);
    VERIFICAR(real >= jogo);   // o piloto abre menus (nível, loja, Eco): o real passa do de jogo
    VERIFICAR(t.salas.front().sala == "b1_entrada" && t.salas.back().sala == "b1_chefe");
    const std::string csv = csvTelemetria(t, "2026-11-17", 42, true, Dificuldade::Normal, "completou");
    VERIFICAR(std::count(csv.begin(), csv.end(), '\n') == 8);
    VERIFICAR(csv.rfind("2026-11-17;42;computador;normal;completou;1;0;b1_entrada;", 0) == 0);
}

TESTE(telemetria_menu_aberto_conta_so_no_tempo_real) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 7, Dificuldade::Normal, erro);
    Telemetria t;
    iniciarTelemetria(t, p);
    abrirPausa(p);
    for (int q = 0; q < 120; ++q) {   // 2 s com a pausa aberta
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        passoTelemetria(t, p, DT);
    }
    encerrarTelemetria(t, p);
    VERIFICAR(t.salas.size() == 1);
    VERIFICAR_PERTO(t.salas[0].segundosJogo, 0.0f, 0.01f);
    VERIFICAR_PERTO(t.salas[0].segundosReais, 2.0f, 0.02f);
}

TESTE(telemetria_arquivo_acumula_com_um_cabecalho_so) {
    const std::string a = anexarTelemetria("", "x;1\n");
    VERIFICAR(a == std::string(CABECALHO_TELEMETRIA) + "x;1\n");
    const std::string b = anexarTelemetria(a, "y;2\n");
    VERIFICAR(b == a + "y;2\n");
    VERIFICAR(anexarTelemetria("cab", "z\n") == "cab\nz\n");   // arquivo sem quebra no fim
    Telemetria fora;
    EstadoPartida avulsa;   // sala avulsa (não é tentativa): não mede
    iniciarTelemetria(fora, avulsa);
    VERIFICAR(!fora.ativa);
}
