// Loja do Elias e manivela (Etapa 8.2; PRD §11.3, §14.3/F19): ofertas sorteadas sem repetir e
// com o preço da tabela, compra (moedas, espaço, vendido), Eco comprado, a manivela que cai com
// a Chave do Medo, a grade que o Elias abre, o Elias que não é "resgate", e o piloto comprando.
#include <filesystem>

#include "jogo/clube.h"
#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    carregarDialogos(std::filesystem::path(ECOS_PASTA_DADOS) / "data/dialogos.txt", c.dialogos, erro);
    return c;
}

int indicePorId(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

EstadoPartida naLoja(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, seed, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, "b1_loja"), nullptr);
    return p;
}

int& moedas(EstadoPartida& p) { return p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)]; }

void irAoElias(EstadoPartida& p) {
    for (const ObjetoSala& o : p.sala.objetos) {
        if (nomeDoNpc(p, o) == "elias") posicionarJogador(p.bill, centroCelula({o.celula.col, o.celula.lin + 1}));
    }
}
}  // namespace

TESTE(loja_ofertas_sorteadas_sem_repetir_e_com_preco_da_tabela) {
    const Conteudo c = conteudoReal();
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        EstadoPartida p = naLoja(c, seed);
        VERIFICAR(p.loja.size() >= 3 && p.loja.size() <= 5);
        for (std::size_t a = 0; a < p.loja.size(); ++a) {
            const OfertaLoja& o = p.loja[a];
            VERIFICAR(o.preco == (o.item == TipoItem::Total ? PRECO_ECO : definicaoDe(o.item).preco) && o.preco > 0);
            for (std::size_t b = a + 1; b < p.loja.size(); ++b) VERIFICAR(o.item != p.loja[b].item);
        }
    }
    // Nada que não caiba: com 3 melhorias e escudo, eles não aparecem.
    EstadoPartida cheio;
    std::string erro;
    iniciarTentativa(cheio, c, 5, Dificuldade::Normal, erro);
    coletarNoInventario(cheio.bill, cheio.inventario, TipoItem::MelhoriaArma, 3);
    coletarNoInventario(cheio.bill, cheio.inventario, TipoItem::Escudo, 1);
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        Rng rng;
        semear(rng, seed);
        for (const OfertaLoja& o : montarLoja(cheio, rng)) VERIFICAR(o.item != TipoItem::MelhoriaArma && o.item != TipoItem::Escudo);
    }
    // Fora da loja, nada à venda.
    entrarNaSala(cheio, c, indicePorId(c, "b1_refugio"), nullptr);
    VERIFICAR(cheio.loja.empty());
}

TESTE(loja_compra_com_moedas_espaco_e_vendido) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naLoja(c, 3);
    irAoElias(p);
    VERIFICAR(eliasProximo(p) != nullptr && amigoParaResgatar(p) == nullptr);   // o Elias não é resgate
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Loja);
    moedas(p) = 0;
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.avisoMenu == "Moedas de prata insuficientes" && !p.loja[0].vendida);
    moedas(p) = 500;
    const OfertaLoja primeira = p.loja[0];
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.loja[0].vendida && moedas(p) == 500 - primeira.preco && p.compras == 1);
    if (primeira.item == TipoItem::Total) VERIFICAR(p.ecos.size() == 1);
    else VERIFICAR(quantidadeDe(p.bill, p.inventario, primeira.item) >= 1);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.avisoMenu == "Já vendido" && moedas(p) == 500 - primeira.preco);
    apertar(p, c, Comando::Voltar);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
    // Sem espaço: kits no limite não se compram (e as moedas ficam).
    EstadoPartida q = naLoja(c, 3);
    q.loja = {{TipoItem::Kit, 30, false}};
    coletarNoInventario(q.bill, q.inventario, TipoItem::Kit, 10);
    moedas(q) = 100;
    VERIFICAR(comprar(q, 0) == ResultadoCompra::SemEspaco && moedas(q) == 100);
    // O Eco comprado é do amigo do bioma.
    q.loja = {{TipoItem::Total, PRECO_ECO, false}};
    VERIFICAR(comprar(q, 0) == ResultadoCompra::Comprou && q.ecos.size() == 1 && moedas(q) == 20);
    VERIFICAR(definicaoEco(q.ecos[0].definicao).amigo == Amigo::Ben);
}

TESTE(loja_manivela_vem_com_a_chave_e_o_elias_abre_a_grade) {
    const Conteudo c = conteudoReal();
    // A sala da Chave do Medo, concluída, solta também a manivela (bioma 1).
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 13, Dificuldade::Normal, erro);
    p.etapa = 3;
    p.salaPrometida = p.plano.etapas[3].opcoes[0];
    entrarNaSala(p, c, p.salaPrometida.sala, nullptr);
    p.inimigos.clear();
    p.ondaAtual = p.sala.modelo->ondas;   // última onda já vencida
    p.timerOnda = 0.0f;
    for (int q = 0; q < 240 && !p.salaConcluida; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.salaConcluida);
    bool chave = false, manivela = false;
    for (const ItemNoChao& it : p.itensChao) {
        chave |= it.tipo == TipoItem::ChaveDoMedo;
        manivela |= it.tipo == TipoItem::Manivela;
    }
    VERIFICAR(chave && manivela);
    // Na loja: o Elias recebe a manivela e a grade vira chão (passagem e visão).
    EstadoPartida l = naLoja(c, 13);
    int grades = 0;
    for (int i = 0; i < l.sala.cols * l.sala.lins; ++i) grades += l.sala.tiles[static_cast<std::size_t>(i)].tipo == TipoTile::Grade ? 1 : 0;
    VERIFICAR(grades > 0);
    coletarNoInventario(l.bill, l.inventario, TipoItem::Manivela, 1);
    irAoElias(l);
    const int versao = l.sala.versaoTerreno;
    apertar(l, c, Comando::Interagir);
    VERIFICAR(l.manivelaEntregue && l.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] == 0);
    VERIFICAR(l.sala.versaoTerreno > versao);
    for (int i = 0; i < l.sala.cols * l.sala.lins; ++i) VERIFICAR(l.sala.tiles[static_cast<std::size_t>(i)].tipo != TipoTile::Grade);
    VERIFICAR(l.pendencia == Pendencia::Dialogo && l.dialogo.atual->id == "elias_manivela");
    // Depois da conversa, o Elias volta a ser a loja.
    for (int k = 0; k < 40 && l.pendencia == Pendencia::Dialogo; ++k) apertar(l, c, Comando::Confirmar);
    apertar(l, c, Comando::Interagir);
    VERIFICAR(l.pendencia == Pendencia::Loja);
}

TESTE(loja_fala_do_elias_lembra_a_morte_anterior) {
    VERIFICAR(std::string(falaDoElias(static_cast<int>(TipoInimigo::Rato), 0)).find("Ratos") != std::string::npos);
    VERIFICAR(std::string(falaDoElias(-1, 0)) != falaDoElias(static_cast<int>(TipoInimigo::Sombra), 0));
    VERIFICAR(std::string(falaDoElias(-1, 0)) != falaDoElias(-1, 1));   // varia de uma conversa para outra
}

TESTE(loja_piloto_entrega_a_manivela_compra_e_abre_o_bau) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naLoja(c, 21);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Manivela, 1);
    moedas(p) = 200;
    p.loja = {{TipoItem::Bateria, 20, false}, {TipoItem::MelhoriaArma, 100, false}, {TipoItem::ChaveEnferrujada, 25, false}};
    MemoriaPiloto m;
    bool abriuBau = false;
    for (int q = 0; q < 60 * 60; ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
        for (const ObjetoSala& o : p.sala.objetos) abriuBau |= o.codigo == 'B' && o.usado;
        if (abriuBau && p.loja[1].vendida) break;
    }
    VERIFICAR(p.manivelaEntregue);
    VERIFICAR(p.loja[1].vendida);                              // a melhoria (a melhor oferta que dá para pagar)
    VERIFICAR(!p.loja[2].vendida);                             // a chave enferrujada não vale para ele
    VERIFICAR(abriuBau);                                       // e foi buscar o baú do depósito
    VERIFICAR(ofertaPreferidaDoPiloto(p) == -1 || moedas(p) < 20);
}
