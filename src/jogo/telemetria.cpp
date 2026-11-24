#include "jogo/telemetria.h"

#include <cstdio>

namespace {

void abrirSala(Telemetria& t, const EstadoPartida& p) {
    t.atual = SalaTelemetria{};
    t.atual.bioma = p.plano.bioma;
    t.atual.etapa = p.etapa;
    t.atual.sala = p.sala.modelo ? p.sala.modelo->id : "?";
    t.inicioJogo = p.stats.tempo;
    t.hpAntes = p.bill.hp;
}

void fecharSala(Telemetria& t, const EstadoPartida& p) {
    t.atual.segundosJogo = p.stats.tempo - t.inicioJogo;
    t.salas.push_back(t.atual);
}

}  // namespace

void iniciarTelemetria(Telemetria& t, const EstadoPartida& p) {
    t = Telemetria{};
    if (!p.emTentativa) return;
    t.ativa = true;
    abrirSala(t, p);
}

void passoTelemetria(Telemetria& t, const EstadoPartida& p, float dtReal) {
    if (!t.ativa) return;
    if (p.trocouDeSala) {   // o passo que trocou de sala: o tempo dele já é da sala nova
        fecharSala(t, p);
        abrirSala(t, p);
    }
    t.atual.segundosReais += dtReal;
    if (p.bill.hp < t.hpAntes) t.atual.danoRecebido += t.hpAntes - p.bill.hp;
    t.hpAntes = p.bill.hp;
}

void encerrarTelemetria(Telemetria& t, const EstadoPartida& p) {
    if (!t.ativa) return;
    fecharSala(t, p);
    t.ativa = false;
}

std::string csvTelemetria(const Telemetria& t, const std::string& data, std::uint64_t seed, bool piloto, Dificuldade d,
                          const char* fim) {
    std::string s;
    char linha[256];
    for (const SalaTelemetria& sala : t.salas) {
        std::snprintf(linha, sizeof linha, "%s;%llu;%s;%s;%s;%d;%d;%s;%.1f;%.1f;%d\n", data.c_str(),
                      static_cast<unsigned long long>(seed), piloto ? "computador" : "jogador", codigoDificuldade(d), fim,
                      sala.bioma, sala.etapa, sala.sala.c_str(), static_cast<double>(sala.segundosJogo),
                      static_cast<double>(sala.segundosReais), sala.danoRecebido);
        s += linha;
    }
    return s;
}

std::string anexarTelemetria(const std::string& conteudoAtual, const std::string& linhas) {
    if (conteudoAtual.empty()) return std::string(CABECALHO_TELEMETRIA) + linhas;
    std::string s = conteudoAtual;
    if (s.back() != '\n') s += '\n';
    return s + linhas;
}
