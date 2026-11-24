// Armadilhas (Etapa 11.1, PRD §14.2, critério F14: "armadilha atinge Bill e inimigo"). Salas mínimas
// montadas aqui: cada regra isolada (pregos, piso cedendo, quem aciona, opcionais, validação, piloto).
#include <cstdio>
#include "jogo/armadilhas.h"
#include "jogo/piloto.h"
#include "teste.h"

namespace {
// Uma sala só, sem ondas: Bill parado e nada mais acontece além das armadilhas.
Conteudo salaUnica(const std::string& texto) {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala(texto, s, erro);
    c.salas.push_back(s);
    return c;
}

const char* const CORREDOR =
    "id=t\ntipo=teste\nmapa:\n"
    "##########\n"
    "#E..^..X.#\n"
    "#........#\n"
    "##########\n";

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0, Dificuldade::Normal);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

void passar(EstadoPartida& p, const Conteudo& c, int quadros) {
    for (int q = 0; q < quadros; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
}

Inimigo& colocar(EstadoPartida& p, TipoInimigo t, Celula cel) {
    Inimigo i = criarInimigo(t, p.proximoId++, centroCelula(cel));
    i.surgindo = 0.0f;
    i.hp = 100;
    p.inimigos.push_back(i);
    return p.inimigos.back();
}
}  // namespace

TESTE(armadilha_pregos_ferem_bill_ao_pisar_uma_vez_por_segundo) {
    const Conteudo c = salaUnica(CORREDOR);
    EstadoPartida p = comecar(c);
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({4, 1}));
    passar(p, c, 3);
    const int esperado = std::max(1, danoEfetivo(DANO_ESPINHOS, p.bill.stats.armadura));
    VERIFICAR(p.bill.hp == cheio - esperado);   // sem esquiva (terreno) e sem crítico
    passar(p, c, 30);
    VERIFICAR(p.bill.hp == cheio - esperado);   // dentro do mesmo segundo: não de novo
    passar(p, c, 50);
    VERIFICAR(p.bill.hp == cheio - 2 * esperado);
}

TESTE(armadilha_pregos_julgam_pelos_pes_e_poupam_o_dash) {
    const Conteudo c = salaUnica(CORREDOR);
    EstadoPartida p = comecar(c);
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({4, 2}));   // logo abaixo: o corpo "sobe" sobre os pregos
    passar(p, c, 10);
    VERIFICAR(p.bill.hp == cheio);
    posicionarJogador(p.bill, centroCelula({4, 1}));
    p.bill.dashRestante = 0.15f;   // atravessando em dash: não aciona
    passar(p, c, 1);
    VERIFICAR(p.bill.hp == cheio);
}

TESTE(armadilha_atinge_inimigo_terrestre_e_nao_voador_nem_chefe) {
    const Conteudo c = salaUnica(CORREDOR);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({1, 2}));
    const IdEntidade rato = colocar(p, TipoInimigo::Rato, {4, 1}).id;
    const IdEntidade sang = colocar(p, TipoInimigo::Sanguessuga, {4, 1}).id;
    const IdEntidade mumia = colocar(p, TipoInimigo::Mumia, {4, 1}).id;
    for (Inimigo& i : p.inimigos) {   // atordoados: ficam na célula (a IA não os tira de lá)
        i.pes = centroCelula({4, 1});
        i.atordoado = 5.0f;
    }
    passar(p, c, 2);
    const Inimigo* r = buscarInimigo(p, rato);
    const Inimigo* s = buscarInimigo(p, sang);
    const Inimigo* m = buscarInimigo(p, mumia);
    VERIFICAR(r && r->hp == 100 - danoEfetivo(DANO_ESPINHOS, parametrosDe(TipoInimigo::Rato).armadura));
    VERIFICAR(s && s->hp == 100);   // voa
    VERIFICAR(m && m->hp == 100);   // chefe não aciona
    // Morte por armadilha dá K e XP (PRD §9).
    for (Inimigo& i : p.inimigos) if (i.id == rato) i.hp = 1;
    const int k = p.stats.K;
    p.recargaArmadilha.assign(p.recargaArmadilha.size(), 0.0f);
    p.golpes.clear();
    for (Inimigo& i : p.inimigos) if (i.id == rato) i.pes = centroCelula({4, 1});
    passar(p, c, 2);
    VERIFICAR(p.stats.K == k + 1);
}

TESTE(armadilha_piso_cedendo_avisa_fere_e_vira_buraco_sem_prender_ninguem) {
    const Conteudo c = salaUnica(CORREDOR);
    EstadoPartida p = comecar(c);
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({7, 1}));
    passar(p, c, 1);
    bool aviso = false;
    for (const Aviso& a : p.avisos) aviso |= a.golpe.terreno && a.tempoTotal >= AVISO_PISO_CEDENDO - 0.01f;
    VERIFICAR(aviso && p.bill.hp == cheio);   // primeiro o aviso (rachaduras), nada de dano ainda
    passar(p, c, 45);
    VERIFICAR(p.bill.hp == cheio - std::max(1, danoEfetivo(DANO_PISO_CEDENDO, p.bill.stats.armadura)));
    VERIFICAR(tileEm(p.sala, 7, 1).tipo == TipoTile::Buraco && tileEm(p.sala, 7, 1).bloqueiaMovimento);
    VERIFICAR(tileEm(p.sala, 7, 1).estado == 1);   // preserva a origem para o desenho do assoalho quebrado
    const Celula agora = celulaDe(p.bill.pes);
    VERIFICAR(!tileEm(p.sala, agora.col, agora.lin).bloqueiaMovimento);   // posto fora do buraco
    passar(p, c, 120);
    VERIFICAR(p.pisosCedendo.empty() && tileEm(p.sala, 7, 1).tipo == TipoTile::Buraco);
}

TESTE(armadilha_opcional_segue_a_dificuldade) {
    const std::string texto = "id=t\ntipo=teste\nopcional=4 1\nmapa:\n##########\n#E..^.^..#\n#........#\n##########\n";
    const Conteudo c = salaUnica(texto);
    for (int pct : {0, 100}) {
        EstadoPartida p;
        iniciarPartida(p, c, 1, 0, Dificuldade::Normal);
        p.ajustes.armadilhasOpcionais = pct;
        entrarNaSala(p, c, 0, nullptr);
        VERIFICAR(tileEm(p.sala, 4, 1).tipo == (pct == 0 ? TipoTile::Chao : TipoTile::Espinhos));
        VERIFICAR(tileEm(p.sala, 6, 1).tipo == TipoTile::Espinhos);   // a não opcional fica sempre
    }
    SalaModelo s;
    std::string erro;
    VERIFICAR(lerSala("id=t\ntipo=teste\nopcional=2 1\nmapa:\n#####\n#E..#\n#####\n", s, erro));
    VERIFICAR(!validarSala(s).empty());   // opcional sem armadilha
}

TESTE(armadilha_validador_piso_cedendo_nao_pode_isolar_a_porta) {
    SalaModelo s;
    std::string erro;
    VERIFICAR(lerSala("id=t\ntipo=teste\nmapa:\n#######\n#E.X..L\n#######\n", s, erro));
    VERIFICAR(!validarSala(s).empty());   // caído, o piso isola a porta
    VERIFICAR(lerSala("id=t\ntipo=teste\nmapa:\n#######\n#E.X..L\n#.....#\n#######\n", s, erro));
    VERIFICAR(validarSala(s).empty());    // há caminho por baixo
}

TESTE(armadilha_piloto_contorna_os_pregos_que_ve) {
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "############\n"
        "#E..^^^^...L\n"
        "#..........#\n"
        "############\n");
    EstadoPartida p = comecar(c);
    const int cheio = p.bill.hp;
    MemoriaPiloto m;
    float maisLonge = p.bill.pes.x;
    for (int q = 0; q < 60 * 25; ++q) {
        const EstadoEntrada e = pilotar(p, m, DT);
        const int antes = p.bill.hp;
        atualizarPartida(p, c, e, DT);
        if (p.bill.hp < antes) std::printf("      pisou em (%d,%d) px(%.0f,%.0f) objetivo '%s' caminho %zu\n", celulaDe(p.bill.pes).col,
                                           celulaDe(p.bill.pes).lin, p.bill.pes.x, p.bill.pes.y, m.descricao.c_str(), m.caminho.size());
        maisLonge = std::max(maisLonge, p.bill.pes.x);
        if (p.indiceSala != 0) break;
    }
    VERIFICAR(p.bill.hp == cheio);                    // nunca pisou
    VERIFICAR(maisLonge > centroCelula({8, 1}).x);    // e passou por eles (por baixo)
}

TESTE(armadilha_jato_avisa_dispara_longe_da_parede_e_fere_so_na_linha) {
    // O cano 'J' está colado na parede de cima: o jato desce 3 células.
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "##########\n"
        "#E...J...#\n"
        "#........#\n"
        "#........#\n"
        "#........#\n"
        "#........#\n"
        "##########\n");
    EstadoPartida p = comecar(c);
    const Vec2 d = direcaoDoJato(p.sala, {5, 1});
    VERIFICAR(d.x == 0.0f && d.y == 1.0f);
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({5, 3}));   // na linha do jato
    bool aviso = false;
    for (int q = 0; q < 60 * 5 && p.bill.hp == cheio; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (const Aviso& a : p.avisos) aviso |= a.golpe.terreno && a.golpe.area.forma == FormaArea::Linha && a.tempoTotal >= AVISO_JATO - 0.01f;
    }
    VERIFICAR(aviso && p.bill.hp == cheio - std::max(1, danoEfetivo(DANO_JATO, p.bill.stats.armadura)));
    EstadoPartida q = comecar(c);
    posicionarJogador(q.bill, centroCelula({7, 3}));   // ao lado da linha: nada
    for (int k = 0; k < 60 * 7; ++k) atualizarPartida(q, c, EstadoEntrada{}, DT);
    VERIFICAR(q.bill.hp == q.bill.stats.hpMax);
}

TESTE(armadilha_poca_de_sangue_fere_por_segundo) {
    const Conteudo c = salaUnica("id=t\ntipo=teste\nmapa:\n##########\n#E..b....#\n#........#\n##########\n");
    EstadoPartida p = comecar(c);
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({4, 1}));
    for (int q = 0; q < 60 * 3 - 10; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);   // ~3 s
    const int porGolpe = std::max(1, danoEfetivo(DANO_POCA_SANGUE, p.bill.stats.armadura));
    VERIFICAR(p.bill.hp == cheio - 3 * porGolpe);
}

// Etapa 13.1 (bioma 5): a grelha de fogo e a esteira.
TESTE(armadilha_grelha_brasas_avisam_e_as_chamas_ferem_quem_entra) {
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "##########\n"
        "#E...f...#\n"
        "#........#\n"
        "##########\n");
    EstadoPartida p = comecar(c);
    p.bill.attr.agilidade = 0;
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    const int cheio = p.bill.hp;
    posicionarJogador(p.bill, centroCelula({5, 2}));   // ao lado: espera a grelha acender
    bool brasas = false;
    int q = 0;
    for (; q < 60 * 6 && estadoDaGrelha(p, {5, 1}) != EstadoGrelha::Chamas; ++q) {
        brasas |= estadoDaGrelha(p, {5, 1}) == EstadoGrelha::Brasas;
        passar(p, c, 1);
    }
    VERIFICAR(brasas);   // sempre há o aviso antes
    VERIFICAR(estadoDaGrelha(p, {5, 1}) == EstadoGrelha::Chamas);
    VERIFICAR(p.bill.hp == cheio);   // ao lado, nada
    posicionarJogador(p.bill, centroCelula({5, 1}));   // entra nas chamas acesas
    passar(p, c, 3);
    const int esperado = std::max(1, danoEfetivo(DANO_GRELHA, p.bill.stats.armadura));
    VERIFICAR(p.bill.hp == cheio - esperado);
    passar(p, c, 20);
    VERIFICAR(p.bill.hp == cheio - esperado);   // uma vez por acesa
    // Um rato que fica em cima também queima (F14: atinge Bill e inimigo).
    EstadoPartida r = comecar(c);
    posicionarJogador(r.bill, centroCelula({1, 2}));
    r.bill.invulneravel = 99.0f;
    Inimigo& rato = colocar(r, TipoInimigo::Rato, {5, 1});
    rato.atordoado = 99.0f;
    const IdEntidade id = rato.id;
    passar(r, c, 60 * 5);
    VERIFICAR(buscarInimigo(r, id) && buscarInimigo(r, id)->hp < 100);
}

TESTE(armadilha_esteira_leva_bill_e_rato_e_nao_o_voador) {
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "############\n"
        "#E.........#\n"
        "#.>>>>>>>..#\n"
        "#.vvvvvvv..#\n"
        "############\n");
    VERIFICAR(tileEm(c.salas[0], 3, 2).tipo == TipoTile::Esteira && tileEm(c.salas[0], 3, 2).estado == 0);
    VERIFICAR(tileEm(c.salas[0], 3, 3).estado == 2 && tileDoSimbolo('A').estado == 3 && tileDoSimbolo('<').estado == 1);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({2, 2}));
    p.bill.invulneravel = 99.0f;
    // Ids e posições ANTES do 2º colocar: o push_back realoca o vetor (referência inválida).
    const IdEntidade idRato = colocar(p, TipoInimigo::Rato, {5, 2}).id;
    const IdEntidade idBalao = colocar(p, TipoInimigo::Balao, {8, 3}).id;   // na esteira de baixo, longe do rato
    for (Inimigo& i : p.inimigos) i.atordoado = 99.0f;
    const Vec2 bill0 = p.bill.pes, rato0 = buscarInimigo(p, idRato)->pes, balao0 = buscarInimigo(p, idBalao)->pes;
    passar(p, c, 60);   // 1 s: 60 px para a direita
    VERIFICAR_PERTO(p.bill.pes.x - bill0.x, VELOCIDADE_ESTEIRA, 3.0f);
    VERIFICAR_PERTO(p.bill.pes.y, bill0.y, 0.5f);
    VERIFICAR(buscarInimigo(p, idRato)->pes.x - rato0.x > 40.0f);
    VERIFICAR_PERTO(buscarInimigo(p, idBalao)->pes.y, balao0.y, 0.5f);   // voa por cima
    // O dash passa por cima sem ser levado (quem está no ar não está na esteira).
    EstadoPartida d = comecar(c);
    posicionarJogador(d.bill, centroCelula({3, 3}));
    const float y0 = d.bill.pes.y;
    d.bill.dashRestante = 0.15f;
    d.bill.dashDirecao = {0.0f, 0.0f};
    atualizarPartida(d, c, EstadoEntrada{}, DT);
    VERIFICAR_PERTO(d.bill.pes.y, y0, 0.5f);
}

// Regressão (13.1): em cima da esteira, andando na diagonal, a componente contra ela perdia para os
// 60 px/s e Bill era levado contra a parede para sempre (seeds 3 e 42 na simulação). O piloto faz
// força contra a esteira, como uma pessoa.
TESTE(armadilha_piloto_vence_a_esteira_para_pegar_o_que_esta_contra_ela) {
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "##############\n"
        "#E...........#\n"
        "#.....#......#\n"
        "#..>>>>>>>>..#\n"
        "#............#\n"
        "##############\n");
    EstadoPartida p = comecar(c);
    p.salaConcluida = true;
    posicionarJogador(p.bill, centroCelula({7, 3}));   // na esteira, logo à direita da parede
    soltarItem(p, TipoItem::Moeda, 5, centroCelula({5, 2}), centroCelula({5, 2}));   // em cima e CONTRA a esteira
    const int antes = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)];
    MemoriaPiloto m;
    for (int q = 0; q < 60 * 10 && !p.itensChao.empty(); ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
    }
    VERIFICAR(p.itensChao.empty() && p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)] > antes);
}

// Morte por armadilha não é culpa do último inimigo que bateu (Etapa 14.4: uma morte num jato da Cisterna saía
// como "morreu para o Pássaro", o chefe da sala anterior; isso também escolhia a fala errada no Clube).
TESTE(armadilha_limpa_o_ultimo_agressor) {
    const Conteudo c = salaUnica(CORREDOR);
    EstadoPartida p = comecar(c);
    p.ultimoAgressor = static_cast<int>(TipoInimigo::Passaro);
    Golpe g;
    g.id = p.proximoGolpe++;
    g.equipe = Equipe::Hostil;
    g.terreno = true;
    g.rolaAcerto = false;
    g.dano = 5;
    g.area = areaCirculo(p.bill.pes, 10.0f);
    g.duracao = 0.05f;
    p.golpes.push_back(g);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp < p.bill.stats.hpMax);
    VERIFICAR(p.ultimoAgressor == -1);
}
