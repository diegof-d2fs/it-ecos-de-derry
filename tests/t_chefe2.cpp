// Chefe 2 (Etapa 10.3, PRD §10.1): o Afogado da Caixa d'Água → Pennywise I+. Mergulha e emerge onde
// Bill estava (círculo de 1,0 s), ondas em anel (o centro é seguro), fase 2 a ≤ 50%.
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

Inimigo* chefe(EstadoPartida& p) {
    for (Inimigo& i : p.inimigos) {
        if (i.vivo && parametrosDe(i.tipo).chefe) return &i;
    }
    return nullptr;
}

// Arena com o Afogado de pé; Bill intocável (aqui se testa o chefe).
EstadoPartida naArena(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b2_chefe"), Dificuldade::Normal);
    for (int q = 0; q < 60 * 4; ++q) {
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    return p;
}
}  // namespace

TESTE(area_anel_o_centro_e_seguro) {
    const Area a = areaAnel({100.0f, 100.0f}, 30.0f, 80.0f);
    VERIFICAR(!pontoNaArea(a, {100.0f, 100.0f}));    // no meio: seguro
    VERIFICAR(pontoNaArea(a, {150.0f, 100.0f}));     // na coroa
    VERIFICAR(!pontoNaArea(a, {190.0f, 100.0f}));    // fora
    VERIFICAR(!areaToca(a, {95.0f, 95.0f, 10.0f, 10.0f}));    // caixa inteira no buraco
    VERIFICAR(areaToca(a, {120.0f, 95.0f, 20.0f, 10.0f}));    // atravessa a borda de dentro
    VERIFICAR(areaToca(a, {175.0f, 95.0f, 20.0f, 10.0f}));    // atravessa a borda de fora
    VERIFICAR(!areaToca(a, {185.0f, 95.0f, 10.0f, 10.0f}));
}

TESTE(chefe2_arena_faz_o_afogado_surgir) {
    const Conteudo c = conteudoReal();
    VERIFICAR(chefeDoBioma(2) == TipoInimigo::MedoAfogado);
    EstadoPartida p = naArena(c, 1);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->tipo == TipoInimigo::MedoAfogado && ch->fase == 1);
    if (ch) VERIFICAR(std::string(nomeDoChefe(*ch)) == "O Afogado");
}

TESTE(chefes_padroes_so_atacam_de_onde_alcancam_todos) {
    const TipoInimigo chefes[] = {TipoInimigo::Mumia, TipoInimigo::MedoAfogado, TipoInimigo::Leproso, TipoInimigo::RaloSangue,
                                  TipoInimigo::Passaro};
    for (TipoInimigo t : chefes) {
        int quantos = 0;
        const PadraoChefe* ps = padroesDoChefe(t, quantos);
        VERIFICAR(ps != nullptr && quantos >= 6);
        int fases[3] = {0, 0, 0};
        for (int i = 0; i < quantos; ++i) {
            const PadraoChefe& pc = ps[i];
            ++fases[pc.fase];
            VERIFICAR(pc.preparo >= PREPARO_MINIMO_AVISO && pc.recuperacao >= 0.6f);
            if (!padraoSemAlcance(pc.forma)) VERIFICAR(pc.distanciaMax <= pc.comprimento + 4.0f);
            if (pc.seguinte >= 0) VERIFICAR(pc.seguinte < quantos && ps[pc.seguinte].fase == pc.fase);
        }
        VERIFICAR(fases[1] >= 3 && fases[2] >= 3);
    }
}

TESTE(chefe2_mergulha_some_marca_bill_e_emerge_onde_ele_estava) {
    const Conteudo c = conteudoReal();
    bool viuMergulho = false;
    for (std::uint64_t seed = 1; seed <= 8 && !viuMergulho; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({4, 12}));   // longe: Mergulho ou Sanguessugas
        bool estavaSubmerso = true;   // um mergulho que já tinha começado antes de Bill mudar de lugar não vale
        for (int q = 0; q < 60 * 20 && !viuMergulho; ++q) {
            p.bill.invulneravel = 1.0f;
            p.bill.hp = p.bill.stats.hpMax;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            Inimigo* ch = chefe(p);
            const bool agora = ch && ch->submerso > 0.0f;
            const bool comecou = agora && !estavaSubmerso;
            estavaSubmerso = agora;
            if (!comecou) continue;
            viuMergulho = true;
            VERIFICAR(!podeSerAtingido(*ch));   // no fundo não é atingido
            const Vec2 marcado = ch->alvoRegistrado;
            bool circulo = false;
            for (const Aviso& a : p.avisos) {
                circulo |= a.golpe.origem == ch->id && a.golpe.area.forma == FormaArea::Circulo &&
                           comprimento(subtrair(a.golpe.area.centro, marcado)) < 0.5f &&
                           a.tempoTotal >= AVISO_EMERGIR - 0.01f;
            }
            VERIFICAR(circulo);   // o círculo de 1,0 s onde ele vai sair
            VERIFICAR(comprimento(subtrair(marcado, p.bill.pes)) < 24.0f);   // é onde Bill estava
            if (comprimento(subtrair(marcado, p.bill.pes)) >= 24.0f)
                std::printf("      marcado (%.0f,%.0f) bill (%.0f,%.0f) chefe (%.0f,%.0f) q=%d\n", marcado.x, marcado.y, p.bill.pes.x,
                            p.bill.pes.y, ch->pes.x, ch->pes.y, q);
            posicionarJogador(p.bill, centroCelula({24, 12}));   // Bill foge: ele sai no lugar marcado assim mesmo
            const IdEntidade id = ch->id;
            for (int k = 0; k < 70; ++k) {
                p.bill.invulneravel = 1.0f;
                atualizarPartida(p, c, EstadoEntrada{}, DT);
            }
            const Inimigo* depois = buscarInimigo(p, id);
            VERIFICAR(depois && depois->submerso <= 0.0f && comprimento(subtrair(depois->pes, marcado)) < 8.0f);
        }
    }
    VERIFICAR(viuMergulho);
}

TESTE(chefe2_ondas_sao_dois_aneis_em_tempos_diferentes) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 10 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        for (int q = 0; q < 60 * 20 && !viu; ++q) {
            Inimigo* ch = chefe(p);
            if (ch) posicionarJogador(p.bill, somar(ch->pes, Vec2{0.0f, 70.0f}));   // ao alcance das ondas
            p.bill.invulneravel = 1.0f;
            p.bill.hp = p.bill.stats.hpMax;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            float tempos[2] = {0.0f, 0.0f};
            int aneis = 0;
            for (const Aviso& a : p.avisos) {
                if (a.golpe.area.forma == FormaArea::Anel && aneis < 2) tempos[aneis++] = a.tempoTotal;
            }
            if (aneis == 2) {
                viu = true;
                VERIFICAR(std::fabs(tempos[0] - tempos[1]) >= 0.45f);   // o de fora vem depois
            }
        }
    }
    VERIFICAR(viu);
}

TESTE(chefe2_fase_2_e_o_pennywise_i_mais_e_a_dispersao_da_80_xp) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 3);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch != nullptr);
    if (!ch) return;
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::MedoAfogado);
    ch->submerso = 0.0f;
    Golpe g;
    g.id = 9001;
    g.equipe = Equipe::Jogador;
    g.origem = ID_JOGADOR;
    g.area = areaCirculo({ch->pes.x, ch->pes.y - 20.0f}, 16.0f);
    g.dano = par.hp / 2 + par.armadura;
    g.rolaAcerto = false;
    for (int t = 0; t < 5 && chefe(p) && chefe(p)->fase == 1; ++t) {   // a esquiva é regra do jogo: repete
        g.id = 9001 + static_cast<std::uint32_t>(t);
        g.area.centro = {chefe(p)->pes.x, chefe(p)->pes.y - 20.0f};
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    ch = chefe(p);
    VERIFICAR(ch && ch->fase == 2 && std::string(nomeDoChefe(*ch)) == "Pennywise I+");
    for (int q = 0; q < 100; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);   // termina a transição
    const int xp = p.bill.progresso.xp;
    for (int t = 0; t < 5 && chefe(p); ++t) {
        g.id = 9100 + static_cast<std::uint32_t>(t);
        g.dano = par.hp;
        g.area.centro = {chefe(p)->pes.x, chefe(p)->pes.y - 20.0f};
        chefe(p)->submerso = 0.0f;
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    VERIFICAR(chefe(p) == nullptr && p.stats.B == 1);
    VERIFICAR(p.bill.progresso.xp >= xp + par.xp || p.bill.progresso.nivel > 1);
    VERIFICAR(par.xp == 80);
}
