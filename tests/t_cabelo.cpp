// Cabelo do ralo (Etapa 12.2, PRD §9): torreta fixa; chicote em cruz anunciado; recolhe-se depois
// de atacar (não é atingido); a diagonal é segura.
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
    "##############\n"
    "#E...........#\n"
    "#............#\n"
    "#............#\n"
    "#............#\n"
    "#............#\n"
    "#............#\n"
    "##############\n";

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 3, 0, Dificuldade::Normal);
    p.bill.attr.agilidade = 0;
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

IdEntidade colocar(EstadoPartida& p, Celula cel) {
    Inimigo i = criarInimigo(TipoInimigo::Cabelo, p.proximoId++, centroCelula(cel));
    i.surgindo = 0.0f;
    p.inimigos.push_back(i);
    return i.id;
}

const Inimigo* achar(const EstadoPartida& p, IdEntidade id) { return buscarInimigo(p, id); }
}  // namespace

TESTE(cabelo_chicote_em_cruz_acerta_na_linha_e_nao_na_diagonal) {
    const Conteudo c = salaUnica(SALAO);
    for (int caso = 0; caso < 2; ++caso) {
        EstadoPartida p = comecar(c);
        colocar(p, {7, 4});
        posicionarJogador(p.bill, centroCelula(caso == 0 ? Celula{9, 4} : Celula{9, 6}));   // na linha × na diagonal
        const int cheio = p.bill.hp;
        int cruzes = 0;
        for (int q = 0; q < 70; ++q) {
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            int linhas = 0;
            for (const Aviso& a : p.avisos) linhas += a.golpe.area.forma == FormaArea::Linha ? 1 : 0;
            cruzes = std::max(cruzes, linhas);
        }
        VERIFICAR(cruzes == 2);   // horizontal + vertical
        if (caso == 0) VERIFICAR(p.bill.hp < cheio);
        else VERIFICAR(p.bill.hp == cheio);
    }
}

TESTE(cabelo_se_recolhe_depois_de_atacar_e_nao_sai_do_lugar) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    const IdEntidade id = colocar(p, {7, 4});
    const Vec2 onde = achar(p, id)->pes;
    posicionarJogador(p.bill, centroCelula({9, 6}));
    bool recolheu = false, voltou = false;
    for (int q = 0; q < 60 * 4; ++q) {
        p.bill.invulneravel = 1.0f;
        // Golpes de Bill o tempo todo: o recuo NÃO o tira do ralo.
        Golpe g;
        g.id = 7000 + static_cast<std::uint32_t>(q);
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({onde.x, onde.y - 10.0f}, 14.0f);
        g.dano = 1;
        g.rolaAcerto = false;
        g.recuo = 80.0f;
        if (q % 10 == 0) p.golpes.push_back(g);
        p.hitStop = 0.0f;   // golpe a cada quadro manteria a IA congelada no hitstop
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        const Inimigo* i = achar(p, id);
        if (!i) break;
        if (i->submerso > 0.0f) {
            recolheu = true;
            VERIFICAR(!podeSerAtingido(*i));
        } else if (recolheu) {
            voltou = true;
        }
        VERIFICAR(comprimento(subtrair(i->pes, onde)) < 0.01f);
        for (Inimigo& x : p.inimigos) x.hp = std::max(x.hp, 30);   // não morre: o teste é do movimento
    }
    VERIFICAR(recolheu && voltou);
}

TESTE(cabelo_aparece_nos_poroes) {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    int sala = -1;
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) sala = c.salas[static_cast<std::size_t>(i)].id == "b4_c04" ? i : sala;
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
    VERIFICAR(vistos.count(TipoInimigo::Cabelo) == 1);
    for (TipoInimigo t : vistos)
        VERIFICAR(t == TipoInimigo::Cabelo || t == TipoInimigo::Sombra || t == TipoInimigo::Afogado || t == TipoInimigo::Rato);
}

TESTE(cabelo_ultimo_da_sala_sobe_pelo_ralo_mais_perto_de_bill) {
    // Sozinho e com Bill longe, ele se recolhe e sobe no spawn mais perto de Bill — exatamente num
    // ponto de spawn (regressão: um campo compartilhado com o "vagar" o levava a uma célula qualquer).
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "######################\n"
        "#E...................#\n"
        "#..s...........s.....#\n"
        "#....................#\n"
        "#..s...........s.....#\n"
        "######################\n");
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({19, 3}));
    const IdEntidade id = colocar(p, {3, 2});
    bool subiu = false;
    for (int q = 0; q < 60 * 6 && !subiu; ++q) {
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        const Inimigo* i = achar(p, id);
        if (i && i->submerso <= 0.0f && celulaDe(i->pes).col != 3) subiu = true;
    }
    VERIFICAR(subiu);
    const Inimigo* i = achar(p, id);
    VERIFICAR(i && celulaDe(i->pes).col == 15 && (celulaDe(i->pes).lin == 2 || celulaDe(i->pes).lin == 4));
}
