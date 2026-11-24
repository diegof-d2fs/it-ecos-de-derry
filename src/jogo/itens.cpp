#include "jogo/itens.h"

#include <algorithm>
#include <climits>

namespace {

// PRD §11.1. Mesma ordem de TipoItem. Ícones provisórios: Tônico (bota) e Óculos (olho) usam a
// folha `icones` até a arte própria (item_tonico/item_oculos) ficar pronta.
const DefinicaoItem TABELA[TOTAL_ITENS] = {
    // nome                 descrição                                              categoria                    lim atlas               q   preço
    {"Kit de cura",         "Recupera 35% da vida máxima.",                        CategoriaItem::Consumivel,   3, "item_kit",          0,  30},
    {"Bateria",             "+1 carga da lanterna (clarão).",                      CategoriaItem::Consumivel,   5, "item_bateria",      0,  20},
    {"Ficha de coragem",    "+25% de dano por 30 s.",                              CategoriaItem::PowerUp,      2, "item_coragem",      0,  40},
    {"Tônico do Eddie",     "+30% de velocidade por 20 s.",                        CategoriaItem::PowerUp,      2, "icones",           15,  40},
    {"Óculos do Richie",    "Revela inimigos e paredes rachadas por 30 s.",        CategoriaItem::PowerUp,      1, "icones",           11,  50},
    {"Barquinho de papel",  "Recupera 10 de vida.",                                CategoriaItem::Consumivel,   3, "item_lembranca",    0,  25},
    {"Escudo",              "Tampa de lixeira: +2 de armadura.",                   CategoriaItem::Equipamento,  1, "item_escudo",       0,  60},
    {"Melhoria de arma",    "Pregos no taco: +3 de dano (até 3).",                 CategoriaItem::Equipamento,  3, "item_melhoria",     0, 100},
    {"Chave enferrujada",   "Abre baús trancados.",                                CategoriaItem::Chave,        3, "item_chave_ferro",  0,  25},
    {"Chave do Medo",       "Abre a sala de resgate do bioma.",                    CategoriaItem::Missao,       1, "item_chave_latao",  0,   0},
    {"Manivela",            "Entregue ao Elias para abrir a grade.",               CategoriaItem::Missao,       1, "item_manivela",     0,   0},
    {"Moeda de prata",      "Gaste na Loja do Elias. Perdida ao morrer.",          CategoriaItem::Moeda,        0, "item_moeda",        0,   0},
    {"Pacto de Sangue",     "Vida extra: ao cair, levanta com 50% da vida.",       CategoriaItem::Consumivel,   3, "item_pacto",        0,   0},
    {"Lembrancinha",        "Um objeto dos Perdedores. Vai para a estante do Clube.", CategoriaItem::Segredo,   1, "icones",           10,   0},
};

// Tabela de sorteio: item, peso, quantidade mínima e máxima.
struct EntradaSorteio {
    TipoItem tipo;
    int peso;
    int qMin, qMax;
};

const EntradaSorteio DROPS[] = {
    {TipoItem::Moeda, 50, 2, 5},
    {TipoItem::Kit, 14, 1, 1},
    {TipoItem::Bateria, 14, 1, 1},
    {TipoItem::Barquinho, 10, 1, 1},
    {TipoItem::FichaCoragem, 6, 1, 1},
    {TipoItem::Tonico, 6, 1, 1},
};

const EntradaSorteio BAU[] = {
    {TipoItem::Moeda, 30, 6, 12},
    {TipoItem::Kit, 18, 1, 1},
    {TipoItem::Bateria, 14, 1, 2},
    {TipoItem::Barquinho, 8, 1, 2},
    {TipoItem::FichaCoragem, 8, 1, 1},
    {TipoItem::Tonico, 8, 1, 1},
    {TipoItem::Oculos, 5, 1, 1},
    {TipoItem::Escudo, 4, 1, 1},
    {TipoItem::MelhoriaArma, 4, 1, 1},
    {TipoItem::ChaveEnferrujada, 6, 1, 1},
};

bool ehKitOuBateria(TipoItem t) { return t == TipoItem::Kit || t == TipoItem::Bateria; }

// Peso × 100 para a porcentagem da dificuldade não se perder no arredondamento.
template <int N>
ItemSorteado sortear(Rng& rng, const EntradaSorteio (&tabela)[N], int pesoKitPct) {
    int total = 0;
    for (const EntradaSorteio& e : tabela) total += ehKitOuBateria(e.tipo) ? e.peso * pesoKitPct : e.peso * 100;
    int r = intervalo(rng, 1, total);
    for (const EntradaSorteio& e : tabela) {
        r -= ehKitOuBateria(e.tipo) ? e.peso * pesoKitPct : e.peso * 100;
        if (r <= 0) return {e.tipo, intervalo(rng, e.qMin, e.qMax)};
    }
    return {tabela[0].tipo, tabela[0].qMin};
}

float* timerDoPowerUp(Jogador& j, TipoItem tipo) {
    switch (tipo) {
        case TipoItem::FichaCoragem: return &j.coragem;
        case TipoItem::Tonico: return &j.tonico;
        case TipoItem::Oculos: return &j.oculos;
        default: return nullptr;
    }
}

float duracaoDe(TipoItem tipo) {
    switch (tipo) {
        case TipoItem::FichaCoragem: return DURACAO_CORAGEM;
        case TipoItem::Tonico: return DURACAO_TONICO;
        default: return DURACAO_OCULOS;
    }
}

}  // namespace

const DefinicaoItem& definicaoDe(TipoItem tipo) {
    const int i = static_cast<int>(tipo);
    return TABELA[(i >= 0 && i < TOTAL_ITENS) ? i : 0];
}

bool ehItemDeMissao(TipoItem tipo) { return definicaoDe(tipo).categoria == CategoriaItem::Missao; }

int quantidadeDe(const Jogador& j, const Inventario& inv, TipoItem tipo) {
    switch (tipo) {
        case TipoItem::Bateria: return j.cargasLanterna;
        case TipoItem::Pacto: return j.pactos;
        default: return inv.quantidades[static_cast<std::size_t>(tipo)];
    }
}

int espacoPara(const Jogador& j, const Inventario& inv, TipoItem tipo) {
    const int limite = tipo == TipoItem::Bateria ? j.cargasMaximo : definicaoDe(tipo).limite;
    if (limite <= 0 || ehItemDeMissao(tipo)) return INT_MAX / 2;   // missão nunca fica no chão
    return std::max(0, limite - quantidadeDe(j, inv, tipo));
}

int coletarNoInventario(Jogador& j, Inventario& inv, TipoItem tipo, int quantidade) {
    const int entra = std::min(quantidade, espacoPara(j, inv, tipo));
    if (entra <= 0) return 0;
    switch (tipo) {
        case TipoItem::Bateria: j.cargasLanterna += entra; break;
        case TipoItem::Pacto: j.pactos += entra; break;
        default: inv.quantidades[static_cast<std::size_t>(tipo)] += entra; break;
    }
    // Equipamento vale na hora: os derivados (armadura, dano) se recalculam.
    if (tipo == TipoItem::Escudo || tipo == TipoItem::MelhoriaArma) {
        j.equip.armaduraEscudo = inv.quantidades[static_cast<std::size_t>(TipoItem::Escudo)] > 0 ? 2 : 0;
        j.equip.melhorias = inv.quantidades[static_cast<std::size_t>(TipoItem::MelhoriaArma)];
        recalcularJogador(j);
    }
    return entra;
}

ResultadoUso usarItem(Jogador& j, Inventario& inv, TipoItem tipo) {
    int& q = inv.quantidades[static_cast<std::size_t>(tipo)];
    switch (tipo) {
        case TipoItem::Kit:
        case TipoItem::Barquinho: {
            if (q <= 0) return ResultadoUso::NaoTem;
            if (j.hp >= j.stats.hpMax) return ResultadoUso::NaoPrecisa;   // não gasta
            // Eco "Remédio" (Eddie): o kit cura um tanto a mais (Equipamento::ecoKitPct).
            const int cura = tipo == TipoItem::Kit ? std::max(1, j.stats.hpMax * CURA_KIT_PCT * (100 + j.equip.ecoKitPct) / 10000)
                                                   : CURA_BARQUINHO;
            j.hp = std::min(j.stats.hpMax, j.hp + cura);
            j.cura = 0.4f;   // clarão verde no sprite (SDD §24.4)
            --q;
            return ResultadoUso::Usou;
        }
        case TipoItem::FichaCoragem:
        case TipoItem::Tonico:
        case TipoItem::Oculos: {
            if (q <= 0) return ResultadoUso::NaoTem;
            *timerDoPowerUp(j, tipo) = duracaoDe(tipo);
            --q;
            recalcularJogador(j);
            return ResultadoUso::Usou;
        }
        default: return ResultadoUso::NaoUsavel;   // equipamento, chaves e moedas não se "usam"
    }
}

void atualizarPowerUps(Jogador& j, float dt) {
    bool acabou = false;
    for (float* t : {&j.coragem, &j.tonico, &j.oculos}) {
        if (*t <= 0.0f) continue;
        *t -= dt;
        if (*t <= 0.0f) {
            *t = 0.0f;
            acabou = true;
        }
    }
    if (j.cura > 0.0f) j.cura = std::max(0.0f, j.cura - dt);
    if (acabou) recalcularJogador(j);
}

ItemSorteado sortearDrop(Rng& rng, int pesoKitPct) { return sortear(rng, DROPS, pesoKitPct); }

ItemSorteado sortearItemDeBau(Rng& rng, int pesoKitPct) { return sortear(rng, BAU, pesoKitPct); }
