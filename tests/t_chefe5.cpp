// Chefe 5 (Etapa 13.3, PRD §10.1): o Pássaro das Cinzas → Pennywise III. Voa (imune a terreno);
// mergulho em linha longa anunciada; chuva de brasas em círculos, um depois do outro, que acendem
// FOGO temporário no piso (o mecanismo das poças do Ralo, com o tile Labareda); bater de asas contra
// quem cola. Fase 2: linha de 3, rasante → cruz de fogo, gargalhada.
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

Inimigo* chefe(EstadoPartida& p) {
    for (Inimigo& i : p.inimigos) {
        if (i.vivo && parametrosDe(i.tipo).chefe) return &i;
    }
    return nullptr;
}

EstadoPartida naArena(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b5_chefe"), Dificuldade::Normal);
    for (int q = 0; q < 60 * 4; ++q) {   // a onda do chefe começa depois e ele leva 1,6 s surgindo
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    return p;
}

void passo(EstadoPartida& p, const Conteudo& c) {
    p.bill.invulneravel = 1.0f;
    p.bill.hp = p.bill.stats.hpMax;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
}

const PadraoChefe& padraoAtual(const Inimigo& ch) {
    int quantos = 0;
    return padroesDoChefe(ch.tipo, quantos)[ch.padrao];
}

int labaredas(const EstadoPartida& p) {
    int n = 0;
    for (const Tile& t : p.sala.tiles) n += t.tipo == TipoTile::Labareda ? 1 : 0;
    return n;
}

// Espera o chefe começar o padrão de forma `f` (no 1º quadro do preparo); false se não vier.
bool esperarPadrao(EstadoPartida& p, const Conteudo& c, FormaPadrao f, int fase, int quadros) {
    for (int q = 0; q < quadros; ++q) {
        passo(p, c);
        Inimigo* ch = chefe(p);
        if (ch && ch->fase == fase && ch->estado == EstadoIA::Preparando && ch->padrao >= 0 && padraoAtual(*ch).forma == f &&
            ch->timer > padraoAtual(*ch).preparo - 0.02f)
            return true;
    }
    return false;
}
}  // namespace

TESTE(chefe5_arena_nome_voo_e_regras_dos_padroes) {
    const Conteudo c = conteudoReal();
    VERIFICAR(chefeDoBioma(5) == TipoInimigo::Passaro && chefeDoBioma(6) == TipoInimigo::Lobisomem && chefeDoBioma(7) == TipoInimigo::PennywiseIV);
    EstadoPartida p = naArena(c, 1);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->tipo == TipoInimigo::Passaro && std::string(nomeDoChefe(*ch)) == "O Pássaro das Cinzas");
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::Passaro);
    VERIFICAR(par.chefe && par.voa && par.dano == 14 && par.armadura == 3 && par.xp == 140);   // PRD §10.1
    int quantos = 0;
    const PadraoChefe* ps = padroesDoChefe(TipoInimigo::Passaro, quantos);
    bool mergulho = false, chuva = false, cruz = false;
    for (int i = 0; i < quantos; ++i) {
        VERIFICAR(ps[i].preparo >= PREPARO_MINIMO_AVISO && ps[i].recuperacao >= 0.6f);
        if (!padraoSemAlcance(ps[i].forma)) VERIFICAR(ps[i].distanciaMax <= ps[i].comprimento + 4.0f);
        mergulho |= ps[i].fase == 1 && ps[i].forma == FormaPadrao::Investida && ps[i].comprimento >= 6.0f * TILE;
        chuva |= ps[i].fase == 1 && ps[i].forma == FormaPadrao::Chuva;
        cruz |= ps[i].fase == 2 && ps[i].forma == FormaPadrao::Cruz;
    }
    VERIFICAR(mergulho && chuva && cruz);
}

TESTE(chefe5_chuva_cai_em_sequencia_e_acende_fogo_que_apaga) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 10 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({7, 12}));
        if (!esperarPadrao(p, c, FormaPadrao::Chuva, 1, 60 * 30)) continue;
        Inimigo* ch = chefe(p);
        if (!ch) continue;
        // Os círculos: um ONDE BILL ESTÁ, os outros em volta, sem se sobrepor, e caindo um depois do outro.
        std::vector<const Aviso*> brasas;
        for (const Aviso& a : p.avisos)
            if (a.golpe.origem == ch->id && a.golpe.fogo) brasas.push_back(&a);
        VERIFICAR(brasas.size() >= 3 && brasas.size() <= static_cast<std::size_t>(padraoAtual(*ch).invocar));
        if (brasas.size() < 3) continue;
        VERIFICAR(comprimento(subtrair(brasas[0]->golpe.area.centro, p.bill.pes)) < 1.0f);
        for (std::size_t k = 1; k < brasas.size(); ++k) {
            VERIFICAR(brasas[k]->tempoRestante > brasas[k - 1]->tempoRestante);
            for (std::size_t j = 0; j < k; ++j)
                VERIFICAR(comprimento(subtrair(brasas[k]->golpe.area.centro, brasas[j]->golpe.area.centro)) >= 1.6f * brasas[k]->golpe.area.raio - 0.1f);
        }
        const Celula alvo = celulaDe(brasas[0]->golpe.area.centro);
        for (int k = 0; k < 90; ++k) passo(p, c);   // caem todas
        VERIFICAR(labaredas(p) >= static_cast<int>(brasas.size()));
        VERIFICAR(tileEm(p.sala, alvo.col, alvo.lin).tipo == TipoTile::Labareda);
        viu = true;
        p.inimigos.clear();   // só o fogo importa agora
        p.avisos.clear();
        for (int k = 0; k < static_cast<int>(DURACAO_FOGO * 60.0f) + 5; ++k) passo(p, c);
        VERIFICAR(labaredas(p) == 0 && p.pocasDeSangue.empty());   // apagou: o piso voltou
        VERIFICAR(tileEm(p.sala, alvo.col, alvo.lin).tipo == TipoTile::Chao);
    }
    VERIFICAR(viu);
}

TESTE(chefe5_fogo_fere_bill_e_terrestres_mas_nao_o_passaro) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 2);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch != nullptr);
    if (!ch) return;
    // Acende o fogo como a chuva acende: um golpe com `fogo` sobre a célula de Bill e a do chefe.
    const Celula cb{8, 12};
    posicionarJogador(p.bill, centroCelula(cb));
    const Celula cc = celulaDe(ch->pes);
    for (const Celula& cel : {cb, cc}) {
        Aviso a;
        a.tempoTotal = a.tempoRestante = 0.01f;
        a.golpe.id = 9500u + static_cast<std::uint32_t>(cel.col);
        a.golpe.origem = ch->id;
        a.golpe.area = areaCirculo(centroCelula(cel), 10.0f);
        a.golpe.poca = DURACAO_FOGO;
        a.golpe.fogo = true;
        p.avisos.push_back(a);
    }
    p.inimigos.push_back(criarInimigo(TipoInimigo::Sombra, 777, centroCelula({cb.col + 1, cb.lin})));
    p.inimigos.back().surgindo = 0.0f;
    const int hpChefe = ch->hp;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(tileEm(p.sala, cb.col, cb.lin).tipo == TipoTile::Labareda && tileEm(p.sala, cc.col, cc.lin).tipo == TipoTile::Labareda);
    p.bill.hp = p.bill.stats.hpMax;
    const int cheio = p.bill.hp;
    for (int k = 0; k < 130; ++k) {
        p.bill.invulneravel = 0.0f;
        posicionarJogador(p.bill, centroCelula(cb));
        for (Inimigo& i : p.inimigos) {   // ninguém sai do lugar: só o fogo age
            if (i.tipo == TipoInimigo::Sombra) i.pes = centroCelula(cb);
            i.estado = EstadoIA::Recuperando;
            i.timer = 1.0f;
            i.recarga = 1.0f;
        }
        p.avisos.clear();
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    VERIFICAR(p.bill.hp < cheio);   // queimou (6 por segundo)
    ch = chefe(p);
    VERIFICAR(ch && ch->hp == hpChefe);   // voa: o fogo do chão não o fere
    bool sombraQueimou = false;
    for (const Inimigo& i : p.inimigos) sombraQueimou |= i.tipo == TipoInimigo::Sombra && i.hp < parametrosDe(TipoInimigo::Sombra).hp;
    VERIFICAR(sombraQueimou);
}

TESTE(chefe5_mergulho_cruza_a_arena_por_cima_da_grelha) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 12 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({22, 12}));
        if (!esperarPadrao(p, c, FormaPadrao::Investida, 1, 60 * 30)) continue;
        Inimigo* ch = chefe(p);
        if (!ch) continue;
        const Vec2 antes = ch->pes;
        const PadraoChefe pc = padraoAtual(*ch);
        bool temLinha = false;
        for (const Aviso& a : p.avisos)
            temLinha |= a.golpe.origem == ch->id && a.golpe.area.forma == FormaArea::Linha && a.golpe.area.comprimento >= 6.0f * TILE;
        VERIFICAR(temLinha);   // a linha inteira anunciada no chão
        for (int k = 0; k < static_cast<int>(pc.preparo * 60.0f) + 3; ++k) passo(p, c);
        ch = chefe(p);
        VERIFICAR(ch && comprimento(subtrair(ch->pes, antes)) > 4.0f * TILE);   // cruzou boa parte da arena
        viu = true;
    }
    VERIFICAR(viu);
}

TESTE(chefe5_fase_2_pennywise_III_rasante_seguido_da_cruz) {
    const Conteudo c = conteudoReal();
    bool combo = false;
    for (std::uint64_t seed = 1; seed <= 12 && !combo; ++seed) {
        EstadoPartida p = naArena(c, seed);
        Inimigo* ch = chefe(p);
        if (!ch) continue;
        ch->hp = parametrosDe(TipoInimigo::Passaro).hp / 2 + 1;
        Golpe g;
        g.id = 9900;
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({ch->pes.x, ch->pes.y - 20.0f}, 16.0f);
        g.dano = 6;
        g.rolaAcerto = false;
        p.golpes.push_back(g);
        p.hitStop = 0.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        ch = chefe(p);
        if (!ch || ch->fase != 2) continue;   // a esquiva (5%) pode ter evitado o golpe
        VERIFICAR(std::string(nomeDoChefe(*ch)) == "Pennywise III");
        posicionarJogador(p.bill, centroCelula({22, 12}));
        if (!esperarPadrao(p, c, FormaPadrao::Investida, 2, 60 * 40)) continue;
        // Depois do rasante, a cruz já vem anunciada (sem respiro): duas linhas ortogonais por ele.
        bool viuCruz = false;
        for (int k = 0; k < 90 && !viuCruz; ++k) {
            passo(p, c);
            ch = chefe(p);
            if (!ch || ch->estado != EstadoIA::Preparando || padraoAtual(*ch).forma != FormaPadrao::Cruz) continue;
            int linhas = 0;
            for (const Aviso& a : p.avisos) linhas += a.golpe.origem == ch->id && a.golpe.area.forma == FormaArea::Linha ? 1 : 0;
            VERIFICAR(linhas == 2);
            viuCruz = true;
        }
        VERIFICAR(viuCruz);
        combo = viuCruz;
    }
    VERIFICAR(combo);
}

// Modo Defesa (--pennywise): na arena, o chefe já surge se rasgando no Pennywise do estágio.
TESTE(chefes_modo_defesa_comeca_no_pennywise) {
    const Conteudo c = conteudoReal();
    const char* arenas[] = {"b1_chefe", "b3_chefe", "b4_chefe", "b5_chefe"};
    const char* nomes[] = {"Pennywise I", "Pennywise II", "Pennywise II+", "Pennywise III"};
    for (int k = 0; k < 4; ++k) {
        EstadoPartida p;
        iniciarPartida(p, c, 7, indicePorId(c, arenas[k]), Dificuldade::Normal);
        p.chefeNaFase2 = true;
        for (int q = 0; q < 60 * 5; ++q) passo(p, c);
        Inimigo* ch = chefe(p);
        VERIFICAR(ch && ch->fase == 2 && std::string(nomeDoChefe(*ch)) == nomes[k]);
        if (ch) VERIFICAR(ch->hp <= parametrosDe(ch->tipo).hp / 2);
    }
}
