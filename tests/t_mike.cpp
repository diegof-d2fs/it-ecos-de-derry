// Mike (Etapa 13.4, PRD §12 e §14.3): os 4 Ecos (Marreta, Cronista, Fogo controlado, Resistência), o
// lendário Luz do gerador e o resgate em que ele LIGA O GERADOR (a sala inteira à vista).
#include <filesystem>

#include "jogo/armadilhas.h"
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

Conteudo salao() {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n"
            "####################\n"
            "#E.................#\n"
            "#..................#\n"
            "#..................#\n"
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
    return p;
}

// Alvo parado e sem esquiva (o Cabelo do ralo: agilidade 0, fixo), com vida de sobra.
IdEntidade alvo(EstadoPartida& p, Celula cel) {
    Inimigo i = criarInimigo(TipoInimigo::Cabelo, p.proximoId++, centroCelula(cel));
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

TESTE(mike_ecos_na_tabela) {
    for (const char* nome : {"Marreta", "Cronista", "Fogo controlado", "Resistência", "Luz do gerador"}) {
        const int i = ecoPorNome(nome);
        VERIFICAR(i >= 0);
        if (i >= 0) VERIFICAR(definicaoEco(i).amigo == Amigo::Mike);
    }
    VERIFICAR(ecoLendarioDe(Amigo::Mike) == ecoPorNome("Luz do gerador") && temEcos(Amigo::Mike));
    VERIFICAR(descricaoEco(ecoPorNome("Marreta"), Raridade::Epico) == "O 3º golpe do combo: +60% de dano e mais recuo");
}

TESTE(mike_marreta_so_o_terceiro_golpe_do_combo) {
    const Conteudo c = salao();
    auto dano = [&](bool comMarreta, int passo) {
        EstadoPartida p = comecar(c);
        if (comMarreta) aplicarEco(p, ecoPorNome("Marreta"), Raridade::Comum);   // +30%
        const IdEntidade id = alvo(p, {10, 4});
        const int antes = buscarInimigo(p, id)->hp;
        Golpe g = golpeDeBill(meioDoCorpo(p, id), 100, 8101);
        g.passoCombo = static_cast<std::uint8_t>(passo);
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        return antes - buscarInimigo(p, id)->hp;
    };
    const int arm = parametrosDe(TipoInimigo::Cabelo).armadura;
    VERIFICAR(dano(true, 3) == 130 - arm);
    VERIFICAR(dano(true, 1) == 100 - arm && dano(true, 2) == 100 - arm);   // só o 3º
    VERIFICAR(dano(false, 3) == 100 - arm);
}

TESTE(mike_cronista_da_mais_xp) {
    const Conteudo c = salao();
    auto xp = [&](bool comCronista) {
        EstadoPartida p = comecar(c);
        if (comCronista) aplicarEco(p, ecoPorNome("Cronista"), Raridade::Epico);   // +20%
        Inimigo s = criarInimigo(TipoInimigo::Sombra, p.proximoId++, centroCelula({10, 4}));   // XP 15
        s.surgindo = 0.0f;
        s.atordoado = 5.0f;
        s.hp = 1;
        p.inimigos.push_back(s);
        const IdEntidade id = s.id;
        const int antes = p.bill.progresso.xp;
        p.golpes.push_back(golpeDeBill(meioDoCorpo(p, id), 50, 8301));
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        return p.bill.progresso.xp - antes;
    };
    VERIFICAR(xp(false) == 15 && xp(true) == 18);   // +20%
}

TESTE(mike_fogo_controlado_o_clarao_queima_por_3_s_atravessando_armadura) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoPorNome("Fogo controlado"), Raridade::Epico);   // 5 por segundo
    const IdEntidade id = alvo(p, {10, 4});
    Golpe clarao = golpeDeBill(meioDoCorpo(p, id), 1, 8201);
    clarao.atordoa = 0.5f;
    p.golpes.push_back(clarao);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    const int depoisDoClarao = buscarInimigo(p, id)->hp;
    VERIFICAR(buscarInimigo(p, id)->queimando > 2.9f);
    for (int q = 0; q < 60 * 4; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(depoisDoClarao - buscarInimigo(p, id)->hp == 15);   // 3 queimaduras de 5, sem armadura
    VERIFICAR(buscarInimigo(p, id)->queimando <= 0.0f);
    // Sem o Eco, o clarão não queima.
    EstadoPartida q = comecar(c);
    const IdEntidade id2 = alvo(q, {10, 4});
    Golpe c2 = golpeDeBill(meioDoCorpo(q, id2), 1, 8202);
    c2.atordoa = 0.5f;
    q.golpes.push_back(c2);
    atualizarPartida(q, c, EstadoEntrada{}, DT);
    VERIFICAR(buscarInimigo(q, id2)->queimando <= 0.0f);
}

TESTE(mike_resistencia_armadura_e_imune_ao_fogo_do_chao) {
    const Conteudo c = salao();
    auto dano = [&](bool comResistencia) {
        EstadoPartida p = comecar(c);
        const int arm = p.bill.stats.armadura;
        if (comResistencia) {
            aplicarEco(p, ecoPorNome("Resistência"), Raridade::Comum);
            VERIFICAR(p.bill.stats.armadura == arm + 1);
        }
        // Uma labareda debaixo de Bill (o fogo da chuva de brasas do Pássaro).
        const Celula cel = celulaDe(p.bill.pes);
        p.sala.tiles[static_cast<std::size_t>(cel.lin * p.sala.cols + cel.col)] = criarTile(TipoTile::Labareda);
        const int antes = p.bill.hp;
        for (int q = 0; q < 130; ++q) {
            p.bill.invulneravel = 0.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
        }
        return antes - p.bill.hp;
    };
    VERIFICAR(dano(false) > 0);
    VERIFICAR(dano(true) == 0);
}

TESTE(mike_resgate_liga_o_gerador_e_da_a_luz_do_gerador) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 41, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, "b5_resgate"), nullptr);
    VERIFICAR(!visaoEm(p.visao, 23, 11).explorada);   // o canto escuro da sala do gerador
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    EstadoEntrada e;
    e.pressionou[static_cast<int>(Comando::Interagir)] = true;
    atualizarPartida(p, c, e, DT);   // abre a porta do Medo
    const ObjetoSala* mike = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) mike = o.codigo == '1' ? &o : mike;
    VERIFICAR(mike != nullptr);
    if (!mike) return;
    posicionarJogador(p.bill, centroCelula({mike->celula.col, mike->celula.lin + 1}));
    atualizarPartida(p, c, e, DT);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Mike);
    VERIFICAR(p.salaIluminada && visaoEm(p.visao, 23, 11).explorada && visaoEm(p.visao, 1, 2).explorada);
    VERIFICAR(valorEco(p, EfeitoEco::LuzGerador) == 2);
    // O gerador e a lâmpada trocam de quadro (enfeites).
    bool gerador = false, lampada = false;
    for (const ObjetoSala& o : p.sala.objetos) {
        gerador |= o.codigo == 'e' && o.celula.col == 18 && o.celula.lin == 2 && o.quadro == 3;
        lampada |= o.codigo == 'e' && o.celula.col == 21 && o.celula.lin == 2 && o.quadro == 10;
    }
    VERIFICAR(gerador && lampada);
    // Na sala seguinte, o raio volta ao normal, com os +2 do lendário.
    entrarNaSala(p, c, indicePorId(c, "b5_c01"), nullptr);
    VERIFICAR(!p.salaIluminada);
    VERIFICAR_PERTO(raioDaLuz(p), RAIO_LANTERNA + 2.0f + static_cast<float>(bonusDeLuz(p)), 0.01f);
}
