// Inimigos × itens (SDD §24.5; critério M10): o Rato rouba e foge, solta ao morrer, some no
// escuro; o Espectro queima itens com as brasas; a Sombra esmaga caixas; missão é imune.
#include "jogo/partida.h"
#include "teste.h"

namespace {

// Sala larga e iluminável; sem ondas (os inimigos são postos à mão).
const char* const SALA =
    "id=sala_ladrao\nbioma=1\ntipo=combate\nondas=0\nmapa:\n"
    "##################\n"
    "#s...............#\n"
    "#................#\n"
    "#.......E........#\n"
    "#..c.............#\n"
    "#...............s#\n"
    "##################\n";

Conteudo conteudo() {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    lerSala(SALA, m, erro);
    c.salas.push_back(m);
    return c;
}

EstadoPartida partida(const Conteudo& c, Dificuldade d = Dificuldade::Dificil) {
    EstadoPartida p;
    iniciarPartida(p, c, 5, 0, d);
    p.bill.stats.hpMax = p.bill.hp = 100000;   // ninguém morre por acaso neste teste
    return p;
}

Inimigo& por(EstadoPartida& p, TipoInimigo t, Celula c) {
    Inimigo ini = criarInimigo(t, p.proximoId++, centroCelula(c));
    ini.surgindo = 0.0f;
    p.inimigos.push_back(ini);
    return p.inimigos.back();
}

void item(EstadoPartida& p, TipoItem t, Celula c) {
    soltarItem(p, t, 1, centroCelula(c), centroCelula(c));
    p.itensChao.back().tempo = DURACAO_PULO_ITEM;
}

void passos(EstadoPartida& p, const Conteudo& c, int n) {
    for (int i = 0; i < n; ++i) atualizarPartida(p, c, EstadoEntrada{}, DT);
}

}  // namespace

TESTE(ladrao_rato_pega_item_comum_e_foge) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    p.ajustes.chanceRoubo = 100;   // isola o sorteio (a chance da dificuldade tem teste próprio)
    item(p, TipoItem::Bateria, {14, 2});
    const IdEntidade id = por(p, TipoInimigo::Rato, {14, 2}).id;
    passos(p, c, 2);
    const Inimigo* r = buscarInimigo(p, id);
    VERIFICAR(r && r->itemCarregado == static_cast<int>(TipoItem::Bateria) && r->estado == EstadoIA::Fugindo);
    VERIFICAR(p.itensChao.empty());
}

TESTE(ladrao_ignora_item_de_missao) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    p.ajustes.chanceRoubo = 100;
    item(p, TipoItem::ChaveDoMedo, {14, 2});
    const IdEntidade id = por(p, TipoInimigo::Rato, {14, 2}).id;
    passos(p, c, 30);
    VERIFICAR(buscarInimigo(p, id)->itemCarregado == -1 && p.itensChao.size() == 1);
}

TESTE(ladrao_morto_solta_o_item_onde_caiu) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    p.ajustes.chanceRoubo = 100;
    item(p, TipoItem::Kit, {14, 2});
    Inimigo& r = por(p, TipoInimigo::Rato, {14, 2});
    const IdEntidade id = r.id;
    passos(p, c, 2);
    Inimigo* rato = nullptr;
    for (Inimigo& i : p.inimigos) rato = i.id == id ? &i : rato;
    VERIFICAR(rato && rato->itemCarregado >= 0);
    if (!rato) return;
    // Um golpe do Bill (sem erro, sem esquiva possível) bem em cima do rato.
    rato->hp = 1;
    Golpe g;
    g.equipe = Equipe::Jogador;
    g.origem = ID_JOGADOR;
    g.area = areaCirculo(rato->pes, 40.0f);
    g.dano = 50;
    g.rolaAcerto = false;
    const Vec2 onde = rato->pes;
    p.golpes.push_back(g);
    for (int i = 0; i < 10 && buscarInimigo(p, id); ++i) passos(p, c, 1);
    VERIFICAR(!buscarInimigo(p, id));
    VERIFICAR(p.itensChao.size() == 1 && p.itensChao[0].tipo == TipoItem::Kit);
    if (!p.itensChao.empty()) VERIFICAR(comprimento(subtrair(p.itensChao[0].pos, onde)) < 2.0f * TILE);
}

TESTE(ladrao_some_no_escuro_depois_de_8_s) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    p.ajustes.chanceRoubo = 100;
    item(p, TipoItem::Moeda, {16, 5});
    const IdEntidade id = por(p, TipoInimigo::Rato, {16, 5}).id;
    // Bill bem longe e a sala escura onde o rato está: ninguém vê o ladrão.
    p.bill.pes = centroCelula({1, 1});
    passos(p, c, 2);
    VERIFICAR(buscarInimigo(p, id)->itemCarregado >= 0);
    // Sem luz no rato (a lanterna tem raio 6): ele some depois de 8 s sem ser visto.
    int quadros = 0;
    while (buscarInimigo(p, id) && quadros < 60 * 12) {
        p.bill.pes = centroCelula({1, 1});   // Bill parado no canto (o rato não o persegue: está fugindo)
        passos(p, c, 1);
        ++quadros;
    }
    VERIFICAR(!buscarInimigo(p, id) && p.stats.K == 0);   // sumiu sem dar abate
    VERIFICAR(quadros >= static_cast<int>(TEMPO_LADRAO_SUMIR * 60.0f) - 2);
}

TESTE(ladrao_chance_vem_da_dificuldade) {
    const Conteudo c = conteudo();
    int roubos[TOTAL_DIFICULDADES] = {};
    for (int d = 0; d < TOTAL_DIFICULDADES; d += 2) {
        for (std::uint64_t s = 1; s <= 200; ++s) {
            EstadoPartida p;
            iniciarPartida(p, c, s, 0, static_cast<Dificuldade>(d));
            item(p, TipoItem::Bateria, {14, 2});
            const IdEntidade id = por(p, TipoInimigo::Rato, {14, 2}).id;
            passos(p, c, 2);   // UMA tentativa (depois, 2 s de recarga)
            roubos[d] += buscarInimigo(p, id)->itemCarregado >= 0 ? 1 : 0;
        }
    }
    VERIFICAR(roubos[0] > 25 && roubos[0] < 75);     // Fácil ≈ 25%
    VERIFICAR(roubos[2] > 140 && roubos[2] < 185);   // Difícil ≈ 80%
}

TESTE(espectro_queima_item_comum_e_poupa_a_chave) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    item(p, TipoItem::Kit, {14, 2});
    item(p, TipoItem::ChaveDoMedo, {14, 2});
    Inimigo& e = por(p, TipoInimigo::Espectro, {14, 2});
    e.estado = EstadoIA::Recuperando;   // parado em cima dos itens
    e.timer = 5.0f;
    passos(p, c, 3);
    VERIFICAR(!p.brasas.empty());
    VERIFICAR(p.itensChao.size() == 1 && p.itensChao[0].tipo == TipoItem::ChaveDoMedo);
}

TESTE(sombra_esmaga_caixa_e_o_conteudo_se_perde) {
    const Conteudo c = conteudo();
    EstadoPartida p = partida(c);
    VERIFICAR(tileEm(p.sala, 3, 4).bloqueiaMovimento);
    Inimigo& s = por(p, TipoInimigo::Sombra, {4, 4});
    s.estado = EstadoIA::Recuperando;
    s.timer = 5.0f;
    passos(p, c, 2);
    VERIFICAR(!tileEm(p.sala, 3, 4).bloqueiaMovimento && p.itensChao.empty());
}
