#include "jogo/tentativa.h"

#include <algorithm>

#include "jogo/ecos.h"

namespace {

// Mesma ordem de Recompensa.
const DefinicaoRecompensa RECOMPENSAS[static_cast<int>(Recompensa::Total)] = {
    // nome                 peso  item                         min max
    {"Nada",                 0, TipoItem::Total,             0, 0},
    {"Moedas de prata",     30, TipoItem::Moeda,            40, 60},   // ~meio item da loja (Etapa 8.5)
    {"Kit de cura",         20, TipoItem::Kit,               1, 1},
    {"Bateria",             15, TipoItem::Bateria,           2, 2},
    {"Melhoria de arma",    10, TipoItem::MelhoriaArma,      1, 1},
    {"Chave enferrujada",   12, TipoItem::ChaveEnferrujada,  1, 1},
    {"Ponto de atributo",    5, TipoItem::Total,             1, 1},   // raro: +1 ponto para distribuir
    {"Chave do Medo",        0, TipoItem::ChaveDoMedo,       1, 1},   // fixa na 3ª sala de combate
    {"Eco",                 14, TipoItem::Total,             1, 1},   // escolher 1 de 3 (só se o amigo tem Ecos)
};

constexpr int CHANCE_FESTA_PCT = 20;   // a porta da Festa dos Balões é rara

bool ehKitOuBateria(Recompensa r) { return r == Recompensa::Kit || r == Recompensa::Bateria; }

int pesoDe(Recompensa r, int pesoKitPct) {
    const int base = RECOMPENSAS[static_cast<int>(r)].peso;
    return ehKitOuBateria(r) ? base * pesoKitPct : base * 100;   // ×100: a % não se perde
}

// Sorteia uma recompensa de porta de combate, diferente de `evitar`. Eco só se o amigo do
// bioma já tem Ecos no jogo (nunca prometer o que não existe).
Recompensa sortearRecompensa(Rng& rng, int pesoKitPct, Recompensa evitar, bool ecoPermitido) {
    auto fora = [&](Recompensa r) { return r == evitar || (r == Recompensa::Eco && !ecoPermitido); };
    int total = 0;
    for (int i = 0; i < static_cast<int>(Recompensa::Total); ++i) {
        const Recompensa r = static_cast<Recompensa>(i);
        if (!fora(r)) total += pesoDe(r, pesoKitPct);
    }
    int sorteio = intervalo(rng, 1, total);
    for (int i = 0; i < static_cast<int>(Recompensa::Total); ++i) {
        const Recompensa r = static_cast<Recompensa>(i);
        if (fora(r)) continue;
        sorteio -= pesoDe(r, pesoKitPct);
        if (sorteio <= 0) return r;
    }
    return Recompensa::Moedas;
}

std::vector<int> salasDoTipo(const std::vector<SalaModelo>& salas, int bioma, TipoSala tipo) {
    std::vector<int> v;
    for (int i = 0; i < static_cast<int>(salas.size()); ++i) {
        const SalaModelo& s = salas[static_cast<std::size_t>(i)];
        if (s.bioma == bioma && s.tipo == tipo) v.push_back(i);
    }
    return v;
}

// Estado do sorteio de um bioma: salas de combate embaralhadas, consumidas em ordem (sem
// repetir enquanto houver), e o que fazer quando falta um tipo especial.
struct Sorteio {
    const std::vector<SalaModelo>& salas;
    int bioma;
    Rng& rng;
    std::vector<int> combates;
    std::size_t proximo = 0;
    PlanoBioma& plano;

    OpcaoPorta combate(Recompensa r) {
        const int sala = combates[proximo % combates.size()];   // banco pequeno: repete em ciclo
        ++proximo;
        return {sala, TipoSala::Combate, r};
    }

    OpcaoPorta especial(TipoSala tipo) {
        const std::vector<int> v = salasDoTipo(salas, bioma, tipo);
        if (v.empty()) {   // falta a sala desenhada: um combate no lugar (e o aviso nos testes)
            plano.avisos.push_back(std::string("bioma ") + std::to_string(bioma) + ": sem sala do tipo " +
                                   std::to_string(static_cast<int>(tipo)) + "; usei combate");
            return combate(Recompensa::Moedas);
        }
        return {v[static_cast<std::size_t>(intervalo(rng, 0, static_cast<int>(v.size()) - 1))], tipo, Recompensa::Nenhuma};
    }
};

}  // namespace

const DefinicaoRecompensa& definicaoRecompensa(Recompensa r) {
    const int i = static_cast<int>(r);
    return RECOMPENSAS[(i >= 0 && i < static_cast<int>(Recompensa::Total)) ? i : 0];
}

void embaralhar(std::vector<int>& v, Rng& rng) {
    for (int i = static_cast<int>(v.size()) - 1; i > 0; --i) {
        const int j = intervalo(rng, 0, i);
        std::swap(v[static_cast<std::size_t>(i)], v[static_cast<std::size_t>(j)]);
    }
}

const char* nomeBioma(int bioma) {
    static const char* const NOMES[] = {"Rua de Derry", "Galeria pluvial", "O Barrens", "Neibolt Street",
                                        "Porões da cidade", "Siderúrgica Kitchener", "Cisterna", "O Covil"};
    return (bioma >= 0 && bioma < 8) ? NOMES[bioma] : "?";
}

bool biomaJogavel(const std::vector<SalaModelo>& salas, int bioma) {
    if (bioma == BIOMA_COVIL)   // o Covil (Etapa 15): a antecâmara e a arena do Pennywise IV
        return !salasDoTipo(salas, bioma, TipoSala::Entrada).empty() && !salasDoTipo(salas, bioma, TipoSala::Chefe).empty();
    return !salasDoTipo(salas, bioma, TipoSala::Entrada).empty() && !salasDoTipo(salas, bioma, TipoSala::Combate).empty();
}

bool gerarBioma(const std::vector<SalaModelo>& salas, int bioma, Rng& rng, int pesoKitPct, PlanoBioma& plano,
                std::string& erro) {
    plano = PlanoBioma{};
    plano.bioma = bioma;
    if (!biomaJogavel(salas, bioma)) {
        erro = "bioma " + std::to_string(bioma) + " sem sala de entrada ou de combate";
        return false;
    }
    Sorteio s{salas, bioma, rng, salasDoTipo(salas, bioma, TipoSala::Combate), 0, plano};
    embaralhar(s.combates, rng);

    auto etapa = [&](std::vector<OpcaoPorta> opcoes) { plano.etapas.push_back({std::move(opcoes)}); };
    if (bioma == BIOMA_COVIL) {   // o Covil (PRD §4.5): a escada de brinquedos e o Pennywise IV. Sem portas a escolher.
        etapa({s.especial(TipoSala::Entrada)});
        etapa({s.especial(TipoSala::Chefe)});
        return true;
    }
    // 0. Entrada (segura, sem recompensa).
    etapa({s.especial(TipoSala::Entrada)});
    // 1. Duas portas de combate com recompensas diferentes.
    const bool eco = temEcos(amigoDoBioma(bioma));
    const Recompensa r1 = sortearRecompensa(rng, pesoKitPct, Recompensa::Nenhuma, eco);
    const Recompensa r2 = sortearRecompensa(rng, pesoKitPct, r1, eco);
    etapa({s.combate(r1), s.combate(r2)});
    // 2. Combate ou sala especial: Tesouro ou o altar de Eco do amigo do bioma (PRD §5.2).
    const bool temAltar = eco && !salasDoTipo(salas, bioma, TipoSala::Eco).empty();
    TipoSala especial = temAltar && intervalo(rng, 0, 1) == 1 ? TipoSala::Eco : TipoSala::Tesouro;
    // Porta rara (SDD §24.5): a Festa dos Balões, no máximo uma por bioma (só nesta etapa).
    if (!salasDoTipo(salas, bioma, TipoSala::Bonus).empty() && intervalo(rng, 1, 100) <= CHANCE_FESTA_PCT) especial = TipoSala::Bonus;
    etapa({s.combate(sortearRecompensa(rng, pesoKitPct, Recompensa::Nenhuma, eco)), s.especial(especial)});
    // 3. A sala da Chave do Medo: sempre ANTES do resgate (PRD §5.3).
    etapa({s.combate(Recompensa::ChaveDoMedo)});
    // 4. Mais duas portas de combate (Etapa 8.5: o PRD §18 pede 3 combates + especial por bioma).
    const Recompensa r4 = sortearRecompensa(rng, pesoKitPct, Recompensa::Nenhuma, eco);
    etapa({s.combate(r4), s.combate(sortearRecompensa(rng, pesoKitPct, r4, eco))});
    // 5. Loja do Elias ou Refúgio (área segura).
    etapa({s.especial(TipoSala::Loja), s.especial(TipoSala::Refugio)});
    // 6. Resgate (porta trancada pela Chave do Medo) e 7. Chefe-medo.
    etapa({s.especial(TipoSala::Resgate)});
    etapa({s.especial(TipoSala::Chefe)});
    return true;
}
