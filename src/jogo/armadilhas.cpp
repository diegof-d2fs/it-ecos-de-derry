#include "jogo/armadilhas.h"

#include <algorithm>
#include <cstdlib>

#include "mundo/colisao.h"

namespace {

Golpe golpeDeTerreno(EstadoPartida& p, Celula c, int dano) {
    Golpe g;
    g.id = p.proximoGolpe++;
    g.equipe = Equipe::Todos;
    g.origem = ID_NULO;
    g.area = areaCirculo(centroCelula(c), TILE * 0.5f);   // a célula (julgada pelos pés)
    g.dano = dano;
    g.rolaAcerto = false;
    g.terreno = true;
    g.recuo = 20.0f;
    return g;
}

std::size_t indice(const EstadoPartida& p, Celula c) { return static_cast<std::size_t>(c.lin * p.sala.cols + c.col); }

bool rachando(const EstadoPartida& p, Celula c) {
    return std::any_of(p.pisosCedendo.begin(), p.pisosCedendo.end(),
                       [&](const PisoCedendo& x) { return x.celula.col == c.col && x.celula.lin == c.lin; });
}

// Um ator pisou na célula `c`: dispara o que houver ali.
void pisar(EstadoPartida& p, Celula c) {
    if (!dentroDaSala(p.sala, c.col, c.lin)) return;
    switch (tileEm(p.sala, c.col, c.lin).tipo) {
        case TipoTile::Espinhos: {
            float& recarga = p.recargaArmadilha[indice(p, c)];
            if (recarga > 0.0f) return;
            recarga = RECARGA_ESPINHOS;
            p.golpes.push_back(golpeDeTerreno(p, c, DANO_ESPINHOS));
            p.sons.push_back(IdSom::Armadilha);
            break;
        }
        case TipoTile::PocaSangue: {   // 4 de dano por segundo a quem está nela
            float& recarga = p.recargaArmadilha[indice(p, c)];
            if (recarga > 0.0f) return;
            recarga = RECARGA_POCA_SANGUE;
            p.golpes.push_back(golpeDeTerreno(p, c, DANO_POCA_SANGUE));
            break;
        }
        case TipoTile::Labareda: {   // o fogo da chuva de brasas: 6 por segundo
            float& recarga = p.recargaArmadilha[indice(p, c)];
            if (recarga > 0.0f) return;
            recarga = RECARGA_LABAREDA;
            p.golpes.push_back(golpeDeTerreno(p, c, DANO_LABAREDA));
            p.golpes.back().chama = true;
            break;
        }
        case TipoTile::PisoCedendo: {
            if (rachando(p, c)) return;
            p.pisosCedendo.push_back({c, AVISO_PISO_CEDENDO + QUEDA_DEPOIS_DO_AVISO});
            Aviso a;
            a.tempoTotal = a.tempoRestante = AVISO_PISO_CEDENDO;
            a.golpe = golpeDeTerreno(p, c, DANO_PISO_CEDENDO);
            p.avisos.push_back(a);
            p.sons.push_back(IdSom::Armadilha);
            break;
        }
        default: break;
    }
}

// Célula livre mais perto de `c` (anéis crescentes, ordem fixa): para onde vai quem ficou no buraco.
Celula livrePerto(const EstadoPartida& p, Celula c, bool voa) {
    for (int r = 1; r <= 3; ++r) {
        for (int dl = -r; dl <= r; ++dl) {
            for (int dc = -r; dc <= r; ++dc) {
                if (std::max(std::abs(dc), std::abs(dl)) != r) continue;
                const Tile& t = tileEm(p.sala, c.col + dc, c.lin + dl);
                const Celula n{c.col + dc, c.lin + dl};
                const Vec2 centro = centroCelula(n);
                const bool cabe = !colideComSala(p.sala, caixaNosPes(centro, voa ? CAIXA_INIMIGO : CAIXA_BILL), voa);
                if ((voa ? !t.bloqueiaVoo : !t.bloqueiaMovimento) && cabe) return n;
            }
        }
    }
    return c;
}

}  // namespace

bool ehArmadilha(TipoTile t) {
    return t == TipoTile::Espinhos || t == TipoTile::PisoCedendo || t == TipoTile::Jato || t == TipoTile::Fogo ||
           t == TipoTile::PocaSangue || t == TipoTile::Labareda;
}

EstadoGrelha estadoDaGrelha(const EstadoPartida& p, Celula c) {
    const std::size_t i = static_cast<std::size_t>(c.lin * p.sala.cols + c.col);
    if (i >= p.timerJato.size()) return EstadoGrelha::Apagada;
    // O timer conta até a próxima acesa; ao zerar volta a PERIODO: brasas logo depois, então chamas.
    const float desde = PERIODO_GRELHA - p.timerJato[i];
    if (desde < AVISO_GRELHA) return EstadoGrelha::Brasas;
    if (desde < AVISO_GRELHA + CHAMAS_GRELHA) return EstadoGrelha::Chamas;
    return EstadoGrelha::Apagada;
}

Vec2 direcaoDoJato(const SalaViva& sala, Celula c) {
    const int dc[4] = {0, 0, -1, 1}, dl[4] = {-1, 1, 0, 0};   // N, S, O, L
    for (int k = 0; k < 4; ++k) {
        if (tileEm(sala, c.col + dc[k], c.lin + dl[k]).tipo == TipoTile::Parede)
            return {-static_cast<float>(dc[k]), -static_cast<float>(dl[k])};
    }
    return {0.0f, 1.0f};
}

void prepararArmadilhas(EstadoPartida& p) {
    p.recargaArmadilha.assign(static_cast<std::size_t>(p.sala.cols * p.sala.lins), 0.0f);
    // Jatos: fase própria por cano (determinística pela posição), para não dispararem todos juntos.
    p.timerJato.assign(static_cast<std::size_t>(p.sala.cols * p.sala.lins), 0.0f);
    for (int l = 0; l < p.sala.lins; ++l)
        for (int c = 0; c < p.sala.cols; ++c)
            p.timerJato[static_cast<std::size_t>(l * p.sala.cols + c)] =
                tileEm(p.sala, c, l).tipo == TipoTile::Fogo
                    // Grelha: a fase inicial cai na parte APAGADA do ciclo (o estado é deduzido do
                    // timer; acima de PERIODO − brasas − chamas ela "pareceria" acesa sem golpe).
                    ? 0.3f + static_cast<float>((c * 7 + l * 13) % 10) * (PERIODO_GRELHA - AVISO_GRELHA - CHAMAS_GRELHA - 0.3f) / 10.0f
                    : 1.0f + static_cast<float>((c * 7 + l * 13) % 10) * PERIODO_JATO / 10.0f;
    p.pisosCedendo.clear();
    bool mudou = false;
    for (const Celula& c : p.sala.modelo->armadilhasOpcionais) {
        if (sortearPorcentagem(p.rng, p.ajustes.armadilhasOpcionais)) continue;   // esta fica
        p.sala.tiles[indice(p, c)] = criarTile(TipoTile::Chao);
        mudou = true;
    }
    if (mudou) ++p.sala.versaoTerreno;
}

void atualizarArmadilhas(EstadoPartida& p, float dt) {
    if (p.recargaArmadilha.size() != static_cast<std::size_t>(p.sala.cols * p.sala.lins)) prepararArmadilhas(p);
    for (float& r : p.recargaArmadilha) r = std::max(0.0f, r - dt);
    if (p.timerJato.size() != p.recargaArmadilha.size()) prepararArmadilhas(p);
    // Jatos e grelhas: disparam sozinhos, em ciclo (não dependem de alguém pisar).
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            const TipoTile tipo = tileEm(p.sala, c, l).tipo;
            if (tipo == TipoTile::Fogo) {
                float& t = p.timerJato[static_cast<std::size_t>(l * p.sala.cols + c)];
                t -= dt;
                if (t > 0.0f) continue;
                t += PERIODO_GRELHA;
                Aviso a;   // as brasas: o aviso; ao zerar, as chamas ficam 1 s ferindo quem entrar
                a.tempoTotal = a.tempoRestante = AVISO_GRELHA;
                a.golpe = golpeDeTerreno(p, {c, l}, DANO_GRELHA);
                a.golpe.duracao = CHAMAS_GRELHA;
                a.golpe.chama = true;
                a.golpe.recuo = 0.0f;
                p.avisos.push_back(a);
                continue;
            }
            if (tipo != TipoTile::Jato) continue;
            float& t = p.timerJato[static_cast<std::size_t>(l * p.sala.cols + c)];
            t -= dt;
            if (t > 0.0f) continue;
            t += PERIODO_JATO;
            const Vec2 dir = direcaoDoJato(p.sala, {c, l});
            Aviso a;
            a.tempoTotal = a.tempoRestante = AVISO_JATO;
            a.golpe = golpeDeTerreno(p, {c, l}, DANO_JATO);
            a.golpe.area = areaLinha(centroCelula({c, l}), dir, ALCANCE_JATO, 22.0f);
            p.avisos.push_back(a);
        }
    }
    // Esteiras: levam quem está em cima (o dash passa por cima; voadores, chefes e fixos não).
    if (estaVivo(p.bill) && p.bill.dashRestante <= 0.0f) {
        const Tile& t = tileEm(p.sala, celulaDe(p.bill.pes).col, celulaDe(p.bill.pes).lin);
        if (t.tipo == TipoTile::Esteira)
            empurrarJogador(p.bill, moverComColisao(p.sala, p.bill.pes, CAIXA_BILL, escalar(direcaoDaEsteira(t), VELOCIDADE_ESTEIRA * dt), false));
    }
    for (Inimigo& i : p.inimigos) {
        const ParametrosInimigo& par = parametrosDe(i.tipo);
        if (!i.vivo || par.voa || par.chefe || inimigoFixo(i) || i.surgindo > 0.0f || i.submerso > 0.0f) continue;
        const Tile& t = tileEm(p.sala, celulaDe(i.pes).col, celulaDe(i.pes).lin);
        if (t.tipo == TipoTile::Esteira)
            i.pes = moverComColisao(p.sala, i.pes, CAIXA_INIMIGO, escalar(direcaoDaEsteira(t), VELOCIDADE_ESTEIRA * dt), false);
    }
    // Quem pisa: Bill e os inimigos terrestres (voadores, chefes e NPCs não acionam: PRD §14.2).
    if (estaVivo(p.bill) && p.bill.dashRestante <= 0.0f) pisar(p, celulaDe(p.bill.pes));
    for (const Inimigo& i : p.inimigos) {
        const ParametrosInimigo& par = parametrosDe(i.tipo);
        if (!i.vivo || par.voa || par.chefe || i.surgindo > 0.0f || i.submerso > 0.0f) continue;
        pisar(p, celulaDe(i.pes));
    }
    // Pisos rachando: depois do aviso (o golpe já saiu), viram buraco.
    bool caiu = false;
    for (PisoCedendo& x : p.pisosCedendo) {
        x.timer -= dt;
        if (x.timer > 0.0f) continue;
        Tile buraco = criarTile(TipoTile::Buraco);
        buraco.estado = 1;   // veio do assoalho que cedeu; o terreno desenha as tábuas partidas
        p.sala.tiles[indice(p, x.celula)] = buraco;
        caiu = true;
    }
    if (!caiu) return;
    p.pisosCedendo.erase(std::remove_if(p.pisosCedendo.begin(), p.pisosCedendo.end(), [](const PisoCedendo& x) { return x.timer <= 0.0f; }),
                         p.pisosCedendo.end());
    ++p.sala.versaoTerreno;
    // Ninguém fica dentro de um buraco: vale a CAIXA de colisão inteira, não só a célula dos pés
    // (com os pés ao lado, a caixa ainda invadia o buraco).
    if (colideComSala(p.sala, caixaNosPes(p.bill.pes, CAIXA_BILL), false))
        posicionarJogador(p.bill, centroCelula(livrePerto(p, celulaDe(p.bill.pes), false)));
    for (Inimigo& i : p.inimigos) {
        const bool voa = parametrosDe(i.tipo).voa;
        if (i.vivo && colideComSala(p.sala, caixaNosPes(i.pes, CAIXA_INIMIGO), voa)) i.pes = centroCelula(livrePerto(p, celulaDe(i.pes), voa));
    }
}
