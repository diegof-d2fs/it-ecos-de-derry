#include "jogo/loja.h"

#include <algorithm>

#include "jogo/partida.h"

namespace {

int moedas(const EstadoPartida& p) { return p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)]; }

int amigoDaTentativa(const EstadoPartida& p) { return p.emTentativa ? p.plano.bioma : p.sala.modelo->bioma; }

}  // namespace

std::vector<OfertaLoja> montarLoja(const EstadoPartida& p, Rng& rng) {
    // Catálogo do PRD §11.3, na ordem da tela; o que não faz sentido agora fica de fora.
    const TipoItem catalogo[] = {TipoItem::Kit,    TipoItem::Bateria,          TipoItem::Total /* Eco */,
                                 TipoItem::MelhoriaArma, TipoItem::Escudo, TipoItem::ChaveEnferrujada,
                                 TipoItem::Barquinho};
    std::vector<int> possiveis;
    for (int i = 0; i < static_cast<int>(sizeof catalogo / sizeof catalogo[0]); ++i) {
        const TipoItem t = catalogo[i];
        if (t == TipoItem::Total) {
            if (sortearEcos(p.ecos, amigoDoBioma(amigoDaTentativa(p)), rng).empty()) continue;
        } else if (espacoPara(p.bill, p.inventario, t) <= 0) {
            continue;
        }
        possiveis.push_back(i);
    }
    embaralhar(possiveis, rng);
    const int quantas = std::min(static_cast<int>(possiveis.size()), intervalo(rng, 3, 5));
    possiveis.resize(static_cast<std::size_t>(quantas));
    std::sort(possiveis.begin(), possiveis.end());   // ordem estável do catálogo na tela
    std::vector<OfertaLoja> ofertas;
    for (int i : possiveis) {
        OfertaLoja o;
        o.item = catalogo[i];
        o.preco = o.item == TipoItem::Total ? PRECO_ECO : definicaoDe(o.item).preco;
        ofertas.push_back(o);
    }
    return ofertas;
}

ResultadoCompra comprar(EstadoPartida& p, int indice) {
    if (indice < 0 || indice >= static_cast<int>(p.loja.size())) return ResultadoCompra::Invalida;
    OfertaLoja& o = p.loja[static_cast<std::size_t>(indice)];
    if (o.vendida) return ResultadoCompra::Vendida;
    if (moedas(p) < o.preco) return ResultadoCompra::SemMoedas;
    if (o.item == TipoItem::Total) {
        const std::vector<OfertaEco> eco = sortearEcos(p.ecos, amigoDoBioma(amigoDaTentativa(p)), p.rng);
        if (eco.empty()) return ResultadoCompra::SemEspaco;
        aplicarEco(p, eco[0].definicao, eco[0].raridade);
    } else {
        if (espacoPara(p.bill, p.inventario, o.item) <= 0) return ResultadoCompra::SemEspaco;
        coletarNoInventario(p.bill, p.inventario, o.item, 1);
    }
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)] -= o.preco;
    o.vendida = true;
    ++p.compras;
    return ResultadoCompra::Comprou;
}

std::string nomeDaOferta(const EstadoPartida& p, const OfertaLoja& o) {
    if (o.item != TipoItem::Total) return definicaoDe(o.item).nome;
    return std::string("Eco do ") + nomeAmigo(amigoDoBioma(amigoDaTentativa(p))) + " (surpresa)";
}

const char* falaDoElias(int assassinoAnterior, int vez) {
    // Por TipoInimigo (Balao, Rato, Sombra, Espectro); a última linha é a de quem não morreu.
    static const char* const FALAS[5][2] = {
        {"Balão vermelho de novo? Não segue. Nunca segue.", "Ouvi um estouro lá embaixo. Foi com você, garoto?"},
        {"Ratos, é? Guarda as coisas no bolso, não no chão.", "Esses ratos levam tudo. Até coragem, se deixar."},
        {"A sombra avisa antes de bater. Olha o chão escurecer.", "Sombra não gosta de luz. Leva bateria."},
        {"Fuligem até nas orelhas. Não deixa nada no chão perto dele.", "O espectro queima o que você larga. Pega rápido."},
        {"Moeda de prata compra o que o medo não leva.", "Trinta anos limpando essas galerias. Escolhe com calma."},
    };
    const int linha = (assassinoAnterior >= 0 && assassinoAnterior < 4) ? assassinoAnterior : 4;
    return FALAS[linha][((vez % 2) + 2) % 2];
}
