// Inimigos dos Barrens (Etapa 10.2, PRD §9): Sanguessuga alada (investida anunciada que drena) e
// Afogado (surge da água, agarrão que deixa Bill lento, poças pelo caminho).
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

// Sala sem inimigos (o altar dos Barrens), Bill sem esquiva para o teste não depender da sorte.
EstadoPartida salaVazia(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b2_eco"), Dificuldade::Normal);
    p.bill.attr.agilidade = 0;
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

Inimigo& colocar(EstadoPartida& p, TipoInimigo tipo, Vec2 pos) {
    Inimigo ini = criarInimigo(tipo, p.proximoId++, pos);
    ini.surgindo = 0.0f;   // nasce Vagando, como na partida: percebe Bill (ou caça, se for o último)
    p.inimigos.push_back(ini);
    return p.inimigos.back();
}

Inimigo* porId(EstadoPartida& p, IdEntidade id) {
    for (Inimigo& i : p.inimigos) {
        if (i.id == id) return &i;
    }
    return nullptr;
}
}  // namespace

TESTE(sanguessuga_anuncia_cruza_a_linha_e_se_cura_ao_acertar) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c, 1);
    const IdEntidade id = colocar(p, TipoInimigo::Sanguessuga, somar(p.bill.pes, Vec2{100.0f, 0.0f})).id;
    porId(p, id)->hp = 8;   // ferida: a drenagem tem de aparecer
    const Vec2 inicio = porId(p, id)->pes;
    const int hpBill = p.bill.hp;
    bool avisou = false;
    float maiorDeslocamento = 0.0f;
    for (int q = 0; q < 90; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (const Aviso& a : p.avisos) avisou |= a.golpe.origem == id && a.golpe.drena;
        if (const Inimigo* s = porId(p, id)) maiorDeslocamento = std::max(maiorDeslocamento, comprimento(subtrair(s->pes, inicio)));
    }
    VERIFICAR(avisou);                        // a investida é anunciada (PRD §8.3)
    VERIFICAR(maiorDeslocamento > 80.0f);     // e cruza a linha (passa por Bill)
    VERIFICAR(p.bill.hp < hpBill);
    const Inimigo* s = porId(p, id);
    VERIFICAR(s && s->hp > 8 && s->hp <= parametrosDe(TipoInimigo::Sanguessuga).hp);   // curou-se com o dano
    VERIFICAR(parametrosDe(TipoInimigo::Sanguessuga).voa);
}

TESTE(afogado_agarrao_anunciado_deixa_bill_lento) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c, 2);
    colocar(p, TipoInimigo::Afogado, somar(p.bill.pes, Vec2{30.0f, 0.0f}));
    bool avisou = false;
    for (int q = 0; q < 75 && p.bill.lento <= 0.0f; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (const Aviso& a : p.avisos) avisou |= a.golpe.lentidao > 0.0f;
    }
    VERIFICAR(avisou && p.bill.lento > 1.0f);
}

TESTE(jogador_lento_anda_a_metade_e_o_dash_continua) {
    const Conteudo c = conteudoReal();
    EstadoEntrada direita;
    direita.eixo = {1.0f, 0.0f};
    auto andar = [&](float lento) {
        EstadoPartida p = salaVazia(c, 3);
        const Vec2 antes = p.bill.pes;
        for (int q = 0; q < 20; ++q) {
            p.bill.lento = lento;
            atualizarJogador(p.bill, p.sala, direita, DT);
        }
        return p.bill.pes.x - antes.x;
    };
    const float normal = andar(0.0f), agarrado = andar(1.0f);
    VERIFICAR(normal > 10.0f);
    VERIFICAR_PERTO(agarrado / normal, FATOR_AGARRADO, 0.02f);
    EstadoPartida p = salaVazia(c, 3);
    p.bill.lento = 1.0f;
    EstadoEntrada dash = direita;
    dash.pressionou[static_cast<int>(Comando::Dash)] = true;
    atualizarJogador(p.bill, p.sala, dash, DT);
    VERIFICAR(p.bill.dashRestante > 0.0f);   // agarrado ainda foge (o dash é a resposta)
}

TESTE(afogado_deixa_pocas_que_deixam_lento_e_somem) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c, 4);
    colocar(p, TipoInimigo::Afogado, somar(p.bill.pes, Vec2{200.0f, 0.0f}));
    for (int q = 0; q < 60 * 3; ++q) {   // anda até Bill por 3 s
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    VERIFICAR(p.pocas.size() >= 3);
    if (p.pocas.empty()) {
        std::printf("      afogado: %zu inimigos, andando=%d estado=%d\n", p.inimigos.size(), p.inimigos.empty() ? -1 : (int)p.inimigos[0].andando,
                    p.inimigos.empty() ? -1 : (int)p.inimigos[0].estado);
        return;
    }
    for (std::size_t i = 0; i < p.pocas.size(); ++i) {   // sem empilhar
        for (std::size_t j = i + 1; j < p.pocas.size(); ++j) VERIFICAR(comprimento(subtrair(p.pocas[i].pos, p.pocas[j].pos)) >= RAIO_POCA - 0.01f);
    }
    p.inimigos.clear();
    p.avisos.clear();
    posicionarJogador(p.bill, p.pocas.front().pos);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.naPoca);
    for (int q = 0; q < static_cast<int>(VIDA_POCA * 60.0f) + 10; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.pocas.empty() && !p.bill.naPoca);
}

TESTE(barrens_sorteiam_rato_sanguessuga_e_afogado_e_o_afogado_surge_da_agua) {
    const Conteudo c = conteudoReal();
    std::set<TipoInimigo> vistos;
    int afogados = 0, naAgua = 0;
    for (std::uint64_t seed = 1; seed <= 12; ++seed) {
        EstadoPartida p;
        iniciarPartida(p, c, seed, indicePorId(c, "b2_c01"), Dificuldade::Normal);
        std::set<IdEntidade> conferidos;
        for (int q = 0; q < 60 * 40; ++q) {
            p.bill.hp = p.bill.stats.hpMax;
            p.bill.invulneravel = 1.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            for (Inimigo& i : p.inimigos) {
                if (!conferidos.insert(i.id).second) continue;
                vistos.insert(i.tipo);
                if (i.tipo != TipoInimigo::Afogado) continue;
                ++afogados;
                const Celula cel = celulaDe(i.pes);
                const auto agua = [&](int col, int lin) {
                    const TipoTile t = tileEm(p.sala, col, lin).tipo;
                    return t == TipoTile::AguaRasa || t == TipoTile::Lama;
                };
                if (agua(cel.col, cel.lin)) {
                    ++naAgua;
                } else {   // fora da água só se não havia água a até 4 células do ponto (regra exata)
                    bool haviaAgua = false;
                    for (int dl = -4; dl <= 4; ++dl)
                        for (int dc = -4; dc <= 4; ++dc) haviaAgua |= agua(cel.col + dc, cel.lin + dl);
                    VERIFICAR(!haviaAgua);
                }
                VERIFICAR(!tileEm(p.sala, celulaDe(i.pes).col, celulaDe(i.pes).lin).bloqueiaMovimento);
            }
            if (p.ondaAtual >= 1 && hostisVivos(p) > 0) {   // a 1ª onda basta: derruba e segue
                for (Inimigo& i : p.inimigos) i.hp = std::min(i.hp, 1);
            }
        }
    }
    for (TipoInimigo t : vistos) {
        VERIFICAR(t == TipoInimigo::Rato || t == TipoInimigo::Sanguessuga || t == TipoInimigo::Afogado);
    }
    VERIFICAR(vistos.count(TipoInimigo::Sanguessuga) == 1 && vistos.count(TipoInimigo::Afogado) == 1);
    VERIFICAR(afogados > 0 && naAgua > 0);
}
