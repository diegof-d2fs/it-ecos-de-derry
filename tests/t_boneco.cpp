// Boneco palhaço (Etapa 11.3, PRD §9): mantém 4–6 células, arremessa um pino em arco (aviso no chão
// onde Bill estava), foge de perto, não arremessa através de parede.
#include <filesystem>
#include <set>

#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo salaUnica(const std::string& texto) {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala(texto, s, erro);
    c.salas.push_back(s);
    return c;
}

const char* const SALAO =
    "id=t\ntipo=teste\nmapa:\n"
    "##################\n"
    "#E...............#\n"
    "#................#\n"
    "#................#\n"
    "#................#\n"
    "##################\n";

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 3, 0, Dificuldade::Normal);
    p.bill.attr.agilidade = 0;   // sem esquiva: o teste não depende da sorte
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

IdEntidade colocar(EstadoPartida& p, Celula cel) {
    Inimigo i = criarInimigo(TipoInimigo::Boneco, p.proximoId++, centroCelula(cel));
    i.surgindo = 0.0f;
    p.inimigos.push_back(i);
    return i.id;
}
}  // namespace

TESTE(boneco_arremessa_o_pino_onde_bill_estava_e_acerta_quem_fica) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({3, 2}));
    const IdEntidade id = colocar(p, {9, 2});   // 6 células: distância de arremesso
    const int cheio = p.bill.hp;
    bool viuPino = false;
    Vec2 alvo;
    for (int q = 0; q < 90 && p.bill.hp == cheio; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (const Aviso& a : p.avisos) {
            if (a.projetil && a.golpe.origem == id && !viuPino) {
                viuPino = true;
                alvo = a.golpe.area.centro;
                VERIFICAR(a.tempoTotal >= PREPARO_MINIMO_AVISO);
            }
        }
    }
    VERIFICAR(viuPino && comprimento(subtrair(alvo, centroCelula({3, 2}))) < 1.0f);   // onde Bill ESTAVA
    VERIFICAR(p.bill.hp < cheio);                                                      // ficou: levou o pino
}

TESTE(boneco_quem_sai_do_circulo_nao_e_atingido) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({3, 2}));
    colocar(p, {9, 2});
    const int cheio = p.bill.hp;
    for (int q = 0; q < 90; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        if (!p.avisos.empty()) break;
    }
    VERIFICAR(!p.avisos.empty());
    posicionarJogador(p.bill, centroCelula({3, 4}));   // sai do círculo antes do pino cair
    for (int q = 0; q < 70; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp == cheio);
}

TESTE(boneco_foge_de_perto_e_nao_arremessa_atraves_da_parede) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({8, 2}));
    const IdEntidade id = colocar(p, {9, 2});    // 1 célula: perto demais (foge a menos de 2)
    float antes = 0.0f;
    for (const Inimigo& i : p.inimigos) antes = i.id == id ? comprimento(subtrair(i.pes, p.bill.pes)) : antes;
    for (int q = 0; q < 40; ++q) {
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    float depois = 0.0f;
    for (const Inimigo& i : p.inimigos) depois = i.id == id ? comprimento(subtrair(i.pes, p.bill.pes)) : depois;
    VERIFICAR(depois > antes + 30.0f);   // ganhou distância
    // Uma parede entre os dois: nada de pino.
    const Conteudo c2 = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "##################\n"
        "#E.......#.......#\n"
        "#........#.......#\n"
        "#........#.......#\n"
        "##################\n");
    EstadoPartida q = comecar(c2);
    posicionarJogador(q.bill, centroCelula({5, 2}));
    colocar(q, {12, 2});
    bool pino = false;
    for (int k = 0; k < 40; ++k) {
        atualizarPartida(q, c2, EstadoEntrada{}, DT);
        for (const Aviso& a : q.avisos) pino |= a.projetil;
    }
    VERIFICAR(!pino);
}

TESTE(boneco_aparece_na_neibolt) {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    int sala = -1;
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) sala = c.salas[static_cast<std::size_t>(i)].id == "b3_c01" ? i : sala;
    std::set<TipoInimigo> vistos;
    for (std::uint64_t seed = 1; seed <= 6; ++seed) {
        EstadoPartida p;
        iniciarPartida(p, c, seed, sala, Dificuldade::Normal);
        for (int q = 0; q < 60 * 6; ++q) {
            p.bill.invulneravel = 1.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            for (const Inimigo& i : p.inimigos) vistos.insert(i.tipo);
        }
    }
    VERIFICAR(vistos.count(TipoInimigo::Boneco) == 1);
    for (TipoInimigo t : vistos) VERIFICAR(t == TipoInimigo::Boneco || t == TipoInimigo::Rato || t == TipoInimigo::Sanguessuga);
}
