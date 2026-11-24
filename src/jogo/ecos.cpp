#include "jogo/ecos.h"

#include <algorithm>
#include <cstdio>

#include "combate/atributos.h"
#include "jogo/partida.h"
#include "jogo/tentativa.h"

namespace {

// Tabela dos Ecos (PRD §12). Um Eco novo = uma linha aqui + o ponto de evento que o consulta.
const DefinicaoEco ECOS[] = {
    // amigo        nome                 efeito                        Comum Raro Épico Lend.   texto
    {Amigo::Ben, "Tijolo a tijolo", EfeitoEco::ArmaduraFixa, {1, 2, 3, 0}, "+%d de armadura", false},
    {Amigo::Ben, "Coração pesado", EfeitoEco::HpMax, {10, 15, 20, 0}, "+%d de vida máxima", false},
    {Amigo::Ben, "Planta baixa", EfeitoEco::DanoContraParede, {150, 175, 200, 0},
     "Golpe em inimigo encostado na parede: %d%% do dano", false},
    {Amigo::Ben, "Ponte firme", EfeitoEco::BarreiraDash, {1000, 1500, 2000, 0},
     "O dash deixa uma barreira de tijolos por %s s", false},   // valor em ms; o texto mostra "1,5 s"
    {Amigo::Ben, "Poema", EfeitoEco::Poema, {0, 0, 0, 1},
     "Ao chegar a 0 de vida pela 1ª vez no bioma, fica com 1", true},
    // Stan (PRD §12): ordem e precisão.
    {Amigo::Stan, "Livro de pássaros", EfeitoEco::MarcaTerceiroGolpe, {10, 15, 20, 0},
     "Cada 3º golpe no mesmo alvo: +%d de dano", false},
    {Amigo::Stan, "Escoteiro", EfeitoEco::MoedasSalaSemDano, {10, 15, 25, 0},
     "Sala de combate sem levar dano: +%d moedas", false},
    {Amigo::Stan, "Calma", EfeitoEco::InvulnerabilidadeDash, {50, 100, 150, 0},
     "+%s s de invulnerabilidade no fim do dash", false},   // valor em ms
    {Amigo::Stan, "Ordem", EfeitoEco::CriticoAtordoado, {8, 12, 16, 0},
     "+%d%% de crítico em alvo atordoado", false},
    {Amigo::Stan, "Asas", EfeitoEco::DashDuplo, {0, 0, 0, 1},
     "O dash ganha 2 cargas", true},
    // Eddie (PRD §12): cura e rapidez.
    {Amigo::Eddie, "Inalador", EfeitoEco::NevoaDash, {1000, 1500, 2000, 0},
     "O dash solta uma névoa: inimigos perto ficam lentos por %s s", false},   // valor em ms
    {Amigo::Eddie, "Remédio", EfeitoEco::KitMaisForte, {20, 35, 50, 0},
     "O kit de cura cura %d%% a mais", false},
    {Amigo::Eddie, "Gesso", EfeitoEco::GessoUmGolpe, {30, 45, 60, 0},
     "1× por sala, um golpe em Bill perde %d%% do dano", false},
    {Amigo::Eddie, "Pressa", EfeitoEco::VelocidadePct, {6, 9, 12, 0},
     "+%d%% de velocidade", false},
    {Amigo::Eddie, "Cuidado", EfeitoEco::Cuidado, {0, 0, 0, 1},
     "Ao ser resgatado, o Eddie cura Bill por completo e dá 1 kit", true},
    // Beverly (PRD §12): mira e prata.
    {Amigo::Beverly, "Mira firme", EfeitoEco::MiraFirme, {5, 8, 12, 0},
     "+%d%% de chance de acerto", false},
    {Amigo::Beverly, "Prata", EfeitoEco::Prata, {20, 30, 40, 0},
     "+%d%% de dano contra chefes", false},
    {Amigo::Beverly, "Ricochete", EfeitoEco::RicocheteClarao, {1, 2, 3, 0},
     "O clarão salta para +%d alvo(s) perto de quem acertou", false},
    {Amigo::Beverly, "Estilingue", EfeitoEco::EspecialEstilingue, {10, 15, 20, 0},
     "O especial também dispara um tiro de prata de %d de dano", false},
    {Amigo::Beverly, "Tiro certeiro", EfeitoEco::TiroCerteiro, {0, 0, 0, 800},
     "Golpes críticos atordoam por %s s", true},   // valor em ms
    // Mike (PRD §12): força e fogo.
    {Amigo::Mike, "Marreta", EfeitoEco::Marreta, {30, 45, 60, 0},
     "O 3º golpe do combo: +%d%% de dano e mais recuo", false},
    {Amigo::Mike, "Cronista", EfeitoEco::Cronista, {10, 15, 20, 0},
     "+%d%% de XP", false},
    {Amigo::Mike, "Fogo controlado", EfeitoEco::FogoControlado, {3, 4, 5, 0},
     "O clarão queima: %d de dano por segundo por 3 s", false},
    {Amigo::Mike, "Resistência", EfeitoEco::Resistencia, {1, 1, 2, 0},
     "+%d de armadura e imune ao fogo do chão", false},
    {Amigo::Mike, "Luz do gerador", EfeitoEco::LuzGerador, {0, 0, 0, 2},
     "+%d de raio de visão", true},
    // Richie (PRD §12, Etapa 14.3): voz e confusão.
    {Amigo::Richie, "Voz do locutor", EfeitoEco::VozLocutor, {600, 900, 1200, 0},
     "O especial atordoa por %s s (chefes, metade)", false},   // valor em ms
    {Amigo::Richie, "Óculos", EfeitoEco::OculosRichie, {3, 4, 5, 0},
     "Inimigos até %d células além da luz aparecem em contorno", false},
    {Amigo::Richie, "Piada ruim", EfeitoEco::PiadaRuim, {5, 8, 12, 0},
     "Inimigos: %d pontos a menos de acerto em Bill", false},
    {Amigo::Richie, "Bip-bip", EfeitoEco::BipBip, {8, 12, 16, 0},
     "O dash fere em %d quem Bill atravessa", false},
    {Amigo::Richie, "Imitação", EfeitoEco::Imitacao, {0, 0, 0, 6},
     "1× por sala, o 1º inimigo que o especial acerta luta do seu lado por %d s", true},
};
constexpr int TOTAL = static_cast<int>(sizeof ECOS / sizeof ECOS[0]);

}  // namespace

int totalEcos() { return TOTAL; }

const DefinicaoEco& definicaoEco(int indice) { return ECOS[(indice >= 0 && indice < TOTAL) ? indice : 0]; }

const char* nomeRaridade(Raridade r) {
    static const char* const NOMES[] = {"Comum", "Raro", "Épico", "Lendário"};
    const int i = static_cast<int>(r);
    return (i >= 0 && i < 4) ? NOMES[i] : "?";
}

int valorDaRaridade(const DefinicaoEco& d, Raridade r) {
    const int i = static_cast<int>(r);
    return (i >= 0 && i < 4) ? d.valor[i] : 0;
}

std::string descricaoEco(int definicao, Raridade r) {
    const DefinicaoEco& d = definicaoEco(definicao);
    const int v = valorDaRaridade(d, r);
    char t[160];
    if (d.efeito == EfeitoEco::BarreiraDash || d.efeito == EfeitoEco::InvulnerabilidadeDash || d.efeito == EfeitoEco::NevoaDash ||
        d.efeito == EfeitoEco::TiroCerteiro) {
        // milissegundos → "1", "1,5", "0,05" segundos (sem zeros à direita)
        char segundos[16];
        const int resto = v % 1000;
        if (resto == 0) std::snprintf(segundos, sizeof segundos, "%d", v / 1000);
        else if (resto % 100 == 0) std::snprintf(segundos, sizeof segundos, "%d,%d", v / 1000, resto / 100);
        else std::snprintf(segundos, sizeof segundos, "%d,%02d", v / 1000, resto / 10);
        std::snprintf(t, sizeof t, d.texto, segundos);
    } else {
        std::snprintf(t, sizeof t, d.texto, v);
    }
    return t;
}

Amigo amigoDoBioma(int bioma) {
    static const Amigo AMIGOS[] = {Amigo::Total, Amigo::Ben,  Amigo::Stan,   Amigo::Eddie,
                                   Amigo::Beverly, Amigo::Mike, Amigo::Richie, Amigo::Total};
    return (bioma >= 0 && bioma < 8) ? AMIGOS[bioma] : Amigo::Total;
}

bool temEcos(Amigo a) {
    for (const DefinicaoEco& d : ECOS) {
        if (d.amigo == a && !d.lendario) return true;
    }
    return false;
}

Raridade sortearRaridade(Rng& rng) {
    const int n = intervalo(rng, 1, 100);
    return n <= 70 ? Raridade::Comum : (n <= 95 ? Raridade::Raro : Raridade::Epico);
}

std::vector<OfertaEco> sortearEcos(const std::vector<EcoAtivo>& ativos, Amigo amigo, Rng& rng) {
    std::vector<int> candidatos;
    for (int i = 0; i < TOTAL; ++i) {
        if (ECOS[i].amigo != amigo || ECOS[i].lendario) continue;
        const bool jaTem = std::any_of(ativos.begin(), ativos.end(), [i](const EcoAtivo& e) { return e.definicao == i; });
        if (!jaTem) candidatos.push_back(i);
    }
    embaralhar(candidatos, rng);
    std::vector<OfertaEco> ofertas;
    for (std::size_t k = 0; k < candidatos.size() && k < 3; ++k) ofertas.push_back({candidatos[k], sortearRaridade(rng)});
    return ofertas;
}

int ecoLendarioDe(Amigo a) {
    for (int i = 0; i < TOTAL; ++i) {
        if (ECOS[i].amigo == a && ECOS[i].lendario) return i;
    }
    return -1;
}

int valorEco(const EstadoPartida& p, EfeitoEco efeito) {
    int soma = 0;
    for (const EcoAtivo& e : p.ecos) {
        const DefinicaoEco& d = definicaoEco(e.definicao);
        if (d.efeito == efeito) soma += valorDaRaridade(d, e.raridade);
    }
    return soma;
}

void aplicarEco(EstadoPartida& p, int definicao, Raridade r) {
    if (definicao < 0 || definicao >= TOTAL) return;
    p.ecos.push_back({definicao, r});
    // Passivos: entram nos atributos pelo Equipamento (recalcularJogador soma tudo).
    const int hpAntes = p.bill.stats.hpMax;
    p.bill.equip.ecoArmadura = valorEco(p, EfeitoEco::ArmaduraFixa) + valorEco(p, EfeitoEco::Resistencia);
    p.bill.equip.ecoHp = valorEco(p, EfeitoEco::HpMax);
    p.bill.dashExtras = valorEco(p, EfeitoEco::DashDuplo);   // Asas: 1 carga extra = 2 cargas
    p.bill.dashExtrasDisponiveis = p.bill.dashExtras;
    p.bill.equip.ecoVelocidadePct = valorEco(p, EfeitoEco::VelocidadePct);   // Pressa
    p.bill.equip.ecoKitPct = valorEco(p, EfeitoEco::KitMaisForte);           // Remédio
    recalcularJogador(p.bill);
    p.bill.hp = std::min(p.bill.stats.hpMax, p.bill.hp + std::max(0, p.bill.stats.hpMax - hpAntes));
    if (definicaoEco(definicao).efeito == EfeitoEco::Cuidado) {   // lendário do Eddie: efeito na hora do resgate
        p.bill.hp = p.bill.stats.hpMax;
        coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 1);
    }
}
