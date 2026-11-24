// Beverly (Etapa 12.4, PRD §12 e §4.4): os 4 Ecos (Mira firme, Prata, Ricochete, Estilingue), o
// lendário Tiro certeiro, e o resgate em que ela derruba a passarela (vira ponte).
#include <filesystem>

#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

int indicePorId(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

int ecoPorNome(const std::string& nome) {
    for (int i = 0; i < totalEcos(); ++i) {
        if (nome == definicaoEco(i).nome) return i;
    }
    return -1;
}

// Salão com uma parede solta no meio (o tiro não atravessa parede).
Conteudo salao() {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n"
            "####################\n"
            "#E.................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..........#.......#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "####################\n",
            s, erro);
    c.salas.push_back(s);
    return c;
}

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 7, 0, Dificuldade::Normal);
    p.bill.attr.foco = 10;   // acerto garantido: os testes medem os Ecos, não a sorte
    recalcularJogador(p.bill);
    return p;
}

// Alvo parado (atordoado) e sem esquiva: o Cabelo do ralo (agilidade 0, fixo).
IdEntidade alvo(EstadoPartida& p, Celula cel, TipoInimigo t = TipoInimigo::Cabelo) {
    Inimigo i = criarInimigo(t, p.proximoId++, centroCelula(cel));
    i.surgindo = 0.0f;
    i.atordoado = 30.0f;
    i.hp = 5000;
    p.inimigos.push_back(i);
    return i.id;
}

Golpe golpeDeBill(Vec2 onde, int dano, std::uint32_t id) {
    Golpe g;
    g.id = id;
    g.equipe = Equipe::Jogador;
    g.origem = ID_JOGADOR;
    g.area = areaCirculo(onde, 14.0f);
    g.dano = dano;
    g.rolaAcerto = false;
    return g;
}

Vec2 meioDoCorpo(const EstadoPartida& p, IdEntidade id) {
    const Retangulo h = hurtboxInimigo(*buscarInimigo(p, id));
    return {h.x + h.w / 2.0f, h.y + h.h / 2.0f};
}
}  // namespace

TESTE(beverly_ecos_na_tabela) {
    for (const char* nome : {"Mira firme", "Prata", "Ricochete", "Estilingue", "Tiro certeiro"}) {
        const int i = ecoPorNome(nome);
        VERIFICAR(i >= 0);
        if (i >= 0) VERIFICAR(definicaoEco(i).amigo == Amigo::Beverly);
    }
    VERIFICAR(ecoLendarioDe(Amigo::Beverly) == ecoPorNome("Tiro certeiro") && temEcos(Amigo::Beverly));
    VERIFICAR(descricaoEco(ecoPorNome("Tiro certeiro"), Raridade::Lendario) == "Golpes críticos atordoam por 0,8 s");
    VERIFICAR(descricaoEco(ecoPorNome("Prata"), Raridade::Epico) == "+40% de dano contra chefes");
}

TESTE(beverly_prata_fere_mais_os_chefes_e_so_eles) {
    const Conteudo c = salao();
    auto dano = [&](bool comPrata, TipoInimigo t) {
        EstadoPartida p = comecar(c);
        if (comPrata) aplicarEco(p, ecoPorNome("Prata"), Raridade::Comum);
        const IdEntidade id = alvo(p, {10, 6}, t);
        const int antes = buscarInimigo(p, id)->hp;
        p.golpes.push_back(golpeDeBill(meioDoCorpo(p, id), 100, 8001));
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        return antes - buscarInimigo(p, id)->hp;
    };
    const int arm = parametrosDe(TipoInimigo::Mumia).armadura;
    VERIFICAR(dano(false, TipoInimigo::Mumia) == 100 - arm);
    VERIFICAR(dano(true, TipoInimigo::Mumia) == 120 - arm);   // +20%
    VERIFICAR(dano(true, TipoInimigo::Cabelo) == dano(false, TipoInimigo::Cabelo));   // comum: nada muda
}

TESTE(beverly_mira_firme_acerta_mais) {
    const Conteudo c = salao();
    auto acertos = [&](bool comMira) {
        EstadoPartida p = comecar(c);
        p.bill.attr.foco = 0;   // 80% de acerto contra agilidade 0
        recalcularJogador(p.bill);
        if (comMira) aplicarEco(p, ecoPorNome("Mira firme"), Raridade::Epico);   // +12
        const IdEntidade id = alvo(p, {10, 6});
        int n = 0;
        for (int k = 0; k < 300; ++k) {
            const int antes = buscarInimigo(p, id)->hp;
            Golpe g = golpeDeBill(meioDoCorpo(p, id), 5, 9000 + static_cast<std::uint32_t>(k));
            g.rolaAcerto = true;
            g.focoAtacante = 0;
            p.golpes.push_back(g);
            p.hitStop = 0.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            n += buscarInimigo(p, id)->hp < antes ? 1 : 0;
            p.golpes.clear();
        }
        return n;
    };
    const int sem = acertos(false), com = acertos(true);
    VERIFICAR(sem > 200 && sem < 270);   // ~80%
    VERIFICAR(com > sem + 15);           // ~92%
}

TESTE(beverly_ricochete_o_clarao_salta_para_os_mais_perto) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoPorNome("Ricochete"), Raridade::Raro);   // +2 alvos
    posicionarJogador(p.bill, centroCelula({3, 4}));
    p.bill.cargasLanterna = 3;
    p.bill.invulneravel = 5.0f;
    const IdEntidade a = alvo(p, {6, 4}), b = alvo(p, {6, 6}), cc = alvo(p, {6, 2}), d = alvo(p, {7, 7}), longe = alvo(p, {6, 8});
    for (Inimigo& i : p.inimigos) i.atordoado = 0.0f;   // ninguém começa atordoado; o clarão é que atordoa
    for (Inimigo& i : p.inimigos) i.recarga = 30.0f;    // e ninguém ataca durante o teste
    EstadoEntrada e;
    e.eixo = {1.0f, 0.0f};
    e.pressionou[static_cast<int>(Comando::Clarao)] = true;
    atualizarPartida(p, c, e, DT);
    for (int q = 0; q < 3; ++q) {
        p.hitStop = 0.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    auto atordoado = [&](IdEntidade id) { return buscarInimigo(p, id)->atordoado > 1.0f; };
    VERIFICAR(atordoado(a));                     // o clarão
    VERIFICAR(atordoado(b) && atordoado(cc));    // os 2 saltos: os mais perto de A (2 células)
    VERIFICAR(!atordoado(d) && !atordoado(longe));
    VERIFICAR(p.tracos.size() == 2);             // um traço de luz por salto
}

TESTE(beverly_estilingue_especial_dispara_prata_que_para_na_parede) {
    const Conteudo c = salao();
    auto tiro = [&](bool comEco, Celula onde, Celula bill) {
        EstadoPartida p = comecar(c);
        if (comEco) aplicarEco(p, ecoPorNome("Estilingue"), Raridade::Epico);   // 20
        posicionarJogador(p.bill, centroCelula(bill));
        p.bill.invulneravel = 5.0f;
        const IdEntidade id = alvo(p, onde);
        const int antes = buscarInimigo(p, id)->hp;
        EstadoEntrada e;
        e.eixo = {1.0f, 0.0f};
        e.pressionou[static_cast<int>(Comando::Especial)] = true;
        atualizarPartida(p, c, e, DT);
        for (int q = 0; q < 3; ++q) {
            p.hitStop = 0.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
        }
        return antes - buscarInimigo(p, id)->hp;
    };
    const int arm = parametrosDe(TipoInimigo::Cabelo).armadura;
    VERIFICAR(tiro(true, {9, 3}, {3, 3}) == 20 - arm);   // 6 células: longe do giro, perto do tiro
    VERIFICAR(tiro(false, {9, 3}, {3, 3}) == 0);         // sem o Eco, o especial só gira
    VERIFICAR(tiro(true, {14, 4}, {3, 4}) == 0);         // a parede (11,4) para a pedra
    VERIFICAR(tiro(true, {14, 3}, {3, 3}) == 0);         // 11 células: além do alcance (7)
}

TESTE(beverly_tiro_certeiro_critico_atordoa_o_chefe_pela_metade) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoLendarioDe(Amigo::Beverly), Raridade::Lendario);
    const IdEntidade comum = alvo(p, {6, 6}), chefe = alvo(p, {14, 6}, TipoInimigo::Mumia);
    for (Inimigo& i : p.inimigos) i.atordoado = 0.0f;
    for (IdEntidade id : {comum, chefe}) {
        Golpe g = golpeDeBill(meioDoCorpo(p, id), 5, 7000 + id);
        g.podeCritico = true;
        g.critico = 100.0f;
        p.golpes.push_back(g);
    }
    for (Inimigo& i : p.inimigos) i.recarga = 30.0f;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR_PERTO(buscarInimigo(p, comum)->atordoado, 0.8f, 0.05f);
    VERIFICAR_PERTO(buscarInimigo(p, chefe)->atordoado, 0.4f, 0.05f);
}

TESTE(beverly_resgate_derruba_a_passarela_e_da_o_tiro_certeiro) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 41, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, "b4_resgate"), nullptr);
    VERIFICAR(tileEm(p.sala, 19, 7).bloqueiaMovimento && tileEm(p.sala, 20, 7).bloqueiaMovimento);   // o canal fundo
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    EstadoEntrada e;
    e.pressionou[static_cast<int>(Comando::Interagir)] = true;
    atualizarPartida(p, c, e, DT);
    const ObjetoSala* bev = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) bev = o.codigo == '1' ? &o : bev;
    VERIFICAR(bev != nullptr);
    if (!bev) return;
    posicionarJogador(p.bill, centroCelula({bev->celula.col, bev->celula.lin + 1}));
    atualizarPartida(p, c, e, DT);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Beverly);
    VERIFICAR(tileEm(p.sala, 19, 7).tipo == TipoTile::Ponte && tileEm(p.sala, 20, 7).tipo == TipoTile::Ponte);
    VERIFICAR(!tileEm(p.sala, 19, 7).bloqueiaMovimento);
    VERIFICAR(valorEco(p, EfeitoEco::TiroCerteiro) == 800);
}
