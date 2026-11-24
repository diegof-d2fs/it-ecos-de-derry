// Chefe 4 (Etapa 12.3, PRD §10.1): o Ralo de Sangue → Pennywise II+. Fixo no ralo; cabelos em cruz e
// em xis; o ralo transborda em quadrantes OPOSTOS alternados (poças de sangue temporárias); golfada;
// chama Cabelos pelos ralos. Na fase 2 o Pennywise sai do ralo e persegue, e o ralo fica sangrando.
#include <filesystem>
#include <set>

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
    iniciarPartida(p, c, seed, indicePorId(c, "b4_chefe"), Dificuldade::Normal);
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
}  // namespace

TESTE(chefe4_arena_nome_e_regras_dos_padroes) {
    const Conteudo c = conteudoReal();
    VERIFICAR(chefeDoBioma(4) == TipoInimigo::RaloSangue && chefeDoBioma(5) == TipoInimigo::Passaro);
    EstadoPartida p = naArena(c, 1);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->tipo == TipoInimigo::RaloSangue && std::string(nomeDoChefe(*ch)) == "O Ralo de Sangue");
    if (ch) VERIFICAR(celulaDe(ch->pes).col == 13 && celulaDe(ch->pes).lin == 8);   // no ralo '@'
    int quantos = 0;
    const PadraoChefe* ps = padroesDoChefe(TipoInimigo::RaloSangue, quantos);
    for (int i = 0; i < quantos; ++i) {
        VERIFICAR(ps[i].preparo >= PREPARO_MINIMO_AVISO && ps[i].recuperacao >= 0.6f);
        if (!padraoSemAlcance(ps[i].forma)) VERIFICAR(ps[i].distanciaMax <= ps[i].comprimento + 4.0f);
    }
}

TESTE(chefe4_fixo_no_ralo_na_fase_1_mesmo_apanhando) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 2);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch != nullptr);
    if (!ch) return;
    const Vec2 ralo = ch->pes;
    posicionarJogador(p.bill, centroCelula({3, 13}));   // longe: um chefe que anda iria até ele
    for (int q = 0; q < 60 * 10; ++q) {
        if (q % 20 == 0) {   // golpe com recuo: não o tira do ralo
            Golpe g;
            g.id = 9100 + static_cast<std::uint32_t>(q);
            g.equipe = Equipe::Jogador;
            g.origem = ID_JOGADOR;
            g.area = areaCirculo({ralo.x - 14.0f, ralo.y - 20.0f}, 16.0f);
            g.dano = 1;
            g.rolaAcerto = false;
            g.recuo = 120.0f;
            p.golpes.push_back(g);
        }
        p.hitStop = 0.0f;
        passo(p, c);
        ch = chefe(p);
        VERIFICAR(ch && comprimento(subtrair(ch->pes, ralo)) < 0.01f);
        if (!ch) return;
    }
}

TESTE(chefe4_cruz_e_xis_alternam_o_lugar_seguro) {
    const Conteudo c = conteudoReal();
    bool cruz = false, xis = false;
    for (std::uint64_t seed = 1; seed <= 6 && !(cruz && xis); ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({16, 8}));
        for (int q = 0; q < 60 * 20; ++q) {
            passo(p, c);
            Inimigo* ch = chefe(p);
            // O aviso some um quadro antes de o chefe sair do preparo: confere no começo dele.
            if (!ch || ch->estado != EstadoIA::Preparando || ch->timer < 0.2f) continue;
            const FormaPadrao f = padraoAtual(*ch).forma;
            if (f != FormaPadrao::Cruz && f != FormaPadrao::Xis) continue;
            int linhas = 0;
            for (const Aviso& a : p.avisos) {
                if (a.golpe.origem != ch->id || a.golpe.area.forma != FormaArea::Linha) continue;
                ++linhas;
                const Vec2 d = a.golpe.area.direcao;
                const bool diagonal = std::fabs(std::fabs(d.x) - std::fabs(d.y)) < 0.01f;
                VERIFICAR(diagonal == (f == FormaPadrao::Xis));
                VERIFICAR(a.golpe.area.comprimento >= 2.0f * 7.0f * TILE - 1.0f);   // a arena quase inteira
            }
            VERIFICAR(linhas == 2);
            (f == FormaPadrao::Cruz ? cruz : xis) = true;
        }
    }
    VERIFICAR(cruz && xis);
}

TESTE(chefe4_inunda_quadrantes_opostos_alternados_e_seca) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 10 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({4, 2}));
        Inimigo* ch = chefe(p);
        if (!ch) continue;
        const Celula ralo = celulaDe(ch->origem);
        int inundacoes = 0;
        bool primeiroParNE = false;
        for (int q = 0; q < 60 * 60 && inundacoes < 2; ++q) {
            const int antes = ch ? ch->inundacoes : 0;
            passo(p, c);
            ch = chefe(p);
            if (!ch) break;
            if (ch->inundacoes == antes) continue;
            // Anunciou: espera cair (1,2 s) e confere o chão.
            for (int k = 0; k < 75; ++k) passo(p, c);
            ch = chefe(p);
            if (!ch) break;
            auto poca = [&](int dc, int dl) { return tileEm(p.sala, ralo.col + dc, ralo.lin + dl).tipo == TipoTile::PocaSangue; };
            const bool ne = poca(4, -4), sw = poca(-4, 4), nw = poca(-4, -4), se = poca(4, 4);
            VERIFICAR(ne == sw && nw == se && ne != nw);   // dois OPOSTOS, nunca os quatro
            VERIFICAR(!poca(4, 0) && !poca(0, 4) && !poca(-4, 0));   // os eixos ficam secos
            if (inundacoes == 0) primeiroParNE = ne;
            else VERIFICAR(ne != primeiroParNE);   // a seguinte é o OUTRO par
            ++inundacoes;
        }
        if (inundacoes < 2) continue;
        viu = true;
        p.inimigos.clear();   // só a água importa agora
        p.avisos.clear();
        for (int k = 0; k < static_cast<int>(DURACAO_INUNDACAO * 60.0f) + 5; ++k) passo(p, c);
        VERIFICAR(p.pocasDeSangue.empty() && tileEm(p.sala, ralo.col + 4, ralo.lin - 4).tipo == TipoTile::Chao &&
                  tileEm(p.sala, ralo.col - 4, ralo.lin - 4).tipo == TipoTile::Chao);
    }
    VERIFICAR(viu);
}

TESTE(chefe4_golfada_deixa_poca_que_fere_bill) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 10 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({7, 13}));
        for (int q = 0; q < 60 * 30 && !viu; ++q) {
            passo(p, c);
            Inimigo* ch = chefe(p);
            if (!ch || ch->estado != EstadoIA::Preparando || padraoAtual(*ch).forma != FormaPadrao::Golfada) continue;
            const Vec2 alvo = ch->alvoRegistrado;
            for (int k = 0; k < 70; ++k) passo(p, c);
            const Celula cel = celulaDe(alvo);
            if (tileEm(p.sala, cel.col, cel.lin).tipo != TipoTile::PocaSangue) continue;   // caiu na água rasa
            viu = true;
            p.inimigos.clear();
            p.avisos.clear();
            posicionarJogador(p.bill, centroCelula(cel));
            p.bill.invulneravel = 0.0f;
            p.bill.hp = p.bill.stats.hpMax;
            const int cheio = p.bill.hp;
            for (int k = 0; k < 90; ++k) {
                p.bill.invulneravel = 0.0f;
                atualizarPartida(p, c, EstadoEntrada{}, DT);
            }
            VERIFICAR(p.bill.hp < cheio);
        }
    }
    VERIFICAR(viu);
}

TESTE(chefe4_chama_cabelos_pelos_ralos_da_arena) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 12 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        posicionarJogador(p.bill, centroCelula({22, 13}));
        for (int q = 0; q < 60 * 40 && !viu; ++q) {
            passo(p, c);
            std::vector<Celula> cabelos;
            for (const Inimigo& i : p.inimigos)
                if (i.vivo && i.tipo == TipoInimigo::Cabelo) cabelos.push_back(celulaDe(i.pes));
            if (cabelos.empty()) continue;
            viu = true;
            std::set<std::pair<int, int>> lugares;
            for (const Celula& cel : cabelos) {
                bool emRalo = false;
                for (const PontoSpawn& sp : p.sala.modelo->spawns) emRalo |= sp.celula.col == cel.col && sp.celula.lin == cel.lin;
                VERIFICAR(emRalo);
                lugares.insert({cel.col, cel.lin});
            }
            VERIFICAR(lugares.size() == cabelos.size());   // um por ralo
            VERIFICAR(lugares.count({24, 12}) == 1);       // o ralo mais perto de Bill
        }
    }
    VERIFICAR(viu);
}

TESTE(chefe4_fase_2_sai_do_ralo_persegue_e_o_ralo_sangra) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 3);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch != nullptr);
    if (!ch) return;
    const Celula ralo = celulaDe(ch->origem);
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::RaloSangue);
    ch->hp = par.hp / 2 + 1;   // o golpe (6 − armadura 3) o leva abaixo da metade
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
    VERIFICAR(ch && ch->fase == 2 && std::string(nomeDoChefe(*ch)) == "Pennywise II+" && !inimigoFixo(*ch));
    VERIFICAR(tileEm(p.sala, ralo.col, ralo.lin).tipo == TipoTile::PocaSangue);
    posicionarJogador(p.bill, centroCelula({3, 13}));
    for (int q = 0; q < 60 * 6; ++q) passo(p, c);
    ch = chefe(p);
    VERIFICAR(ch && comprimento(subtrair(ch->pes, ch->origem)) > 3.0f * TILE);   // saiu atrás de Bill
    for (int q = 0; q < 60 * 10; ++q) passo(p, c);   // as inundações secam; o ralo, não
    VERIFICAR(tileEm(p.sala, ralo.col, ralo.lin).tipo == TipoTile::PocaSangue);
}
