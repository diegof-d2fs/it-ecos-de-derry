// Piloto v2 (SDD §24.7, Etapa 6.5): kit no Q, power-up PELO INVENTÁRIO (mesmos comandos de uma
// pessoa), porta escolhida pela nota, baú aberto, e a simulação sem janela (--simular).
#include <algorithm>
#include <filesystem>

#include "jogo/piloto.h"
#include "jogo/simulacao.h"
#include "conteudo_teste.h"
#include "teste.h"

namespace {

Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

int indiceDaSala(const Conteudo& c, const char* id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return 0;
}

// Joga com o piloto até `parar` ou `segundos`; devolve se `parar` aconteceu.
template <typename Cond>
bool jogarAte(EstadoPartida& p, const Conteudo& c, MemoriaPiloto& m, float segundos, Cond parar, bool* abriuInventario = nullptr) {
    for (int q = 0; q < static_cast<int>(segundos * 60.0f); ++q) {
        const EstadoEntrada e = pilotar(p, m, DT);
        atualizarPartida(p, c, e, DT);
        p.sons.clear();
        if (abriuInventario && p.pendencia == Pendencia::Inventario) *abriuInventario = true;
        if (parar(p)) return true;
        if (p.terminou) return false;
    }
    return false;
}

}  // namespace

TESTE(piloto_v2_usa_o_kit_com_pouca_vida) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 4, Dificuldade::Normal, erro);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 2);
    p.bill.hp = p.bill.stats.hpMax / 5;
    MemoriaPiloto m;
    VERIFICAR(jogarAte(p, c, m, 1.0f, [](const EstadoPartida& x) { return x.kitsUsados >= 1; }));
    VERIFICAR(p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)] == 1);   // não gasta os dois
}

TESTE(piloto_v2_usa_power_up_pelo_inventario) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    iniciarPartida(p, c, 3, indiceDaSala(c, "b1_c01"), Dificuldade::Normal);
    coletarNoInventario(p.bill, p.inventario, TipoItem::FichaCoragem, 1);
    MemoriaPiloto m;
    bool abriu = false;
    VERIFICAR(jogarAte(p, c, m, 20.0f, [](const EstadoPartida& x) { return x.bill.coragem > 0.0f; }, &abriu));
    VERIFICAR(abriu);   // usou o MENU (não um atalho exclusivo do piloto)
    VERIFICAR(p.inventario.quantidades[static_cast<std::size_t>(TipoItem::FichaCoragem)] == 0);
    // E fecha o menu depois: o jogo volta a correr.
    jogarAte(p, c, m, 1.0f, [](const EstadoPartida& x) { return x.pendencia == Pendencia::Nenhuma; });
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
}

TESTE(piloto_v2_escolhe_a_porta_pela_nota) {
    const Conteudo c = salasReais();
    for (int caso = 0; caso < 2; ++caso) {
        EstadoPartida p;
        std::string erro;
        iniciarTentativa(p, c, 6, Dificuldade::Normal, erro);
        // Caso 0: vida baixa, [Moedas | Kit] → Kit. Caso 1: vida cheia, [Melhoria | Moedas] → Melhoria.
        p.plano.etapas[1].opcoes[0].recompensa = caso == 0 ? Recompensa::Moedas : Recompensa::Melhoria;
        p.plano.etapas[1].opcoes[1].recompensa = caso == 0 ? Recompensa::Kit : Recompensa::Moedas;
        if (caso == 0) p.bill.hp = p.bill.stats.hpMax * 40 / 100;
        MemoriaPiloto m;
        VERIFICAR(jogarAte(p, c, m, 30.0f, [](const EstadoPartida& x) { return x.etapa == 1; }));
        VERIFICAR(p.salaPrometida.recompensa == (caso == 0 ? Recompensa::Kit : Recompensa::Melhoria));
    }
}

TESTE(piloto_v2_abre_bau_antes_de_sair) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    iniciarPartida(p, c, 2, indiceDaSala(c, "b1_teste_deposito"), Dificuldade::Facil);
    MemoriaPiloto m;
    const int sala = p.indiceSala;
    bool abriu = false;
    for (int q = 0; q < 60 * 120 && p.indiceSala == sala && !p.terminou; ++q) {
        const EstadoEntrada e = pilotar(p, m, DT);
        atualizarPartida(p, c, e, DT);
        p.sons.clear();
        for (const ObjetoSala& o : p.sala.objetos) abriu = abriu || (o.codigo == 'B' && o.usado);
    }
    VERIFICAR(abriu && p.indiceSala != sala);
}

TESTE(simulacao_e_deterministica_e_gera_csv) {
    Conteudo c = salasReais();
    limitarAoBioma(c.salas, 1);   // só o bioma 1: a tentativa tem 8 salas e termina no chefe
    const ResultadoSimulacao a = simularTentativa(c, 42, Dificuldade::Normal, 900.0f);
    const ResultadoSimulacao b = simularTentativa(c, 42, Dificuldade::Normal, 900.0f);
    VERIFICAR(a.fim == FimSimulacao::Completou && a.fim == b.fim && a.pontos == b.pontos && a.salas.size() == b.salas.size());
    VERIFICAR(a.salas.size() == 8 && a.salas.front().sala == "b1_entrada" && a.salas.back().sala == "b1_chefe");
    float soma = 0.0f;
    for (const SalaSimulada& s : a.salas) soma += s.segundos;
    VERIFICAR_PERTO(soma, a.tempo, 0.05f);   // o tempo das salas fecha com o total
    const std::vector<ResultadoSimulacao> lista{a, b};
    const std::string csv = csvTentativas(lista);
    VERIFICAR(csv.rfind("seed;dificuldade;fim;", 0) == 0 && std::count(csv.begin(), csv.end(), '\n') == 3);
    const std::string salas = csvSalas(lista);   // uma string só: begin e end do MESMO objeto
    VERIFICAR(std::count(salas.begin(), salas.end(), '\n') == 1 + 16);   // 8 salas por tentativa
    VERIFICAR(resumoSimulacao(lista).find("100%") != std::string::npos);
}
