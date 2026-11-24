// Chefe-medo do bioma 1 (Etapa 8.3; PRD §10.1, SDD §3.4/§12.3): a arena faz a Múmia surgir no
// '@', os padrões são dados coerentes (só escolhe de onde alcança), a faixa prende, a fase 2 a
// ≤ 50% (o medo se rasga: invulnerável, avisos cancelados), a invocação respeita o limite e a
// "Manifestação dispersa" conta B (não K), dá 60 de XP, desfaz os ratos e abre as portas.
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

// A arena com o chefe já de pé (as ondas começam depois de um atraso; aqui, sem esperar).
EstadoPartida naArena(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b1_chefe"), Dificuldade::Normal);
    for (int q = 0; q < 60 * 4 && !chefeNaSala(p); ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    return p;
}

Inimigo* chefe(EstadoPartida& p) {
    for (Inimigo& i : p.inimigos) {
        if (i.vivo && parametrosDe(i.tipo).chefe) return &i;
    }
    return nullptr;
}

// Passa o tempo com Bill intocável (aqui se testa o chefe, não a sobrevivência do Bill).
void esperar(EstadoPartida& p, const Conteudo& c, int quadros) {
    for (int q = 0; q < quadros; ++q) {
        p.bill.hp = p.bill.stats.hpMax;
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
}

// Um golpe do Bill no corpo do alvo. A esquiva do alvo (regra do jogo) é sorteada; repete até
// 5 vezes para o teste não depender da sorte. Devolve quantas tentativas usou (6 = não acertou).
int golpear(EstadoPartida& p, const Conteudo& c, Inimigo& alvo, int dano) {
    const IdEntidade id = alvo.id;
    for (int tentativa = 1; tentativa <= 5; ++tentativa) {
        const Inimigo* atual = buscarInimigo(p, id);
        if (!atual) return tentativa;
        const int antes = atual->hp;
        Golpe g;
        g.id = 9000 + static_cast<std::uint32_t>(tentativa);
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({atual->pes.x, atual->pes.y - 20.0f}, 16.0f);
        g.dano = dano;
        g.rolaAcerto = false;
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        const Inimigo* depois = buscarInimigo(p, id);
        if (!depois || !depois->vivo || depois->hp != antes) return tentativa;
    }
    return 6;
}
}  // namespace

TESTE(chefe_arena_faz_a_mumia_surgir_e_fecha_as_portas) {
    const Conteudo c = conteudoReal();
    const SalaModelo& arena = c.salas[static_cast<std::size_t>(indicePorId(c, "b1_chefe"))];
    VERIFICAR(arena.posChefe.col >= 0 && validarSala(arena).empty());
    EstadoPartida p = naArena(c, 1);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->tipo == TipoInimigo::Mumia && ch->hp == parametrosDe(TipoInimigo::Mumia).hp && ch->fase == 1);
    if (!ch) return;
    const Celula onde = celulaDe(ch->pes);
    VERIFICAR(onde.col == arena.posChefe.col && onde.lin == arena.posChefe.lin);
    VERIFICAR(!p.salaConcluida && std::string(nomeDoChefe(*ch)) == "A Múmia");
    for (const PortaSala& porta : p.sala.portas) VERIFICAR(tileEm(p.sala, porta.celula.col, porta.celula.lin).bloqueiaMovimento);
    VERIFICAR(chefeDoBioma(1) == TipoInimigo::Mumia && chefeDoBioma(7) == TipoInimigo::PennywiseIV && chefeDoBioma(8) == TipoInimigo::Total);   // Etapa 15: o Covil
}

TESTE(chefe_padroes_so_atacam_de_onde_alcancam) {
    int quantos = 0;
    const PadraoChefe* ps = padroesDoChefe(TipoInimigo::Mumia, quantos);
    VERIFICAR(quantos == 8);
    int fase1 = 0, fase2 = 0;
    for (int i = 0; i < quantos; ++i) {
        const PadraoChefe& pc = ps[i];
        (pc.fase == 1 ? fase1 : fase2) += 1;
        VERIFICAR(pc.preparo >= PREPARO_MINIMO_AVISO);        // todo ataque é anunciado (PRD §8.3)
        VERIFICAR(pc.recuperacao >= 0.6f);                    // há janela de punição
        // Só escolhe de onde ALCANÇA: com +20 px a "Mão estendida" (64) saía a 72 px, não chegava,
        // e o chefe a repetia sem andar (impasse da 8.5). A margem é só a da caixa de Bill.
        if (pc.forma != FormaPadrao::Invocar) VERIFICAR(pc.distanciaMax <= pc.comprimento + 4.0f);
    }
    VERIFICAR(fase1 == 4 && fase2 == 4);
    VERIFICAR(padroesDoChefe(TipoInimigo::Rato, quantos) == nullptr && quantos == 0);
}

TESTE(chefe_faixa_prende_bill_sem_impedir_o_golpe) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    iniciarPartida(p, c, 4, indicePorId(c, "b1_refugio"), Dificuldade::Normal);
    Golpe g;
    g.id = 1;
    g.equipe = Equipe::Hostil;
    g.area = areaCirculo(p.bill.pes, 24.0f);
    g.dano = 9;
    g.rolaAcerto = false;
    g.prende = 1.0f;
    p.golpes.push_back(g);
    p.bill.attr.agilidade = 0;   // sem esquiva para o teste ser determinístico
    recalcularJogador(p.bill);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.preso > 0.9f);
    const Vec2 antes = p.bill.pes;
    EstadoEntrada andar;
    andar.eixo = {1.0f, 0.0f};
    andar.pressionou[static_cast<int>(Comando::Dash)] = true;
    for (int q = 0; q < 30; ++q) atualizarPartida(p, c, andar, DT);
    VERIFICAR(p.bill.pes.x == antes.x && p.bill.dashRestante <= 0.0f);   // nem anda nem dá dash
    EstadoEntrada atacar;
    atacar.pressionou[static_cast<int>(Comando::Atacar)] = true;
    atualizarPartida(p, c, atacar, DT);
    VERIFICAR(p.bill.acao != AcaoJogador::Livre);                          // mas golpeia
    for (int q = 0; q < 60; ++q) atualizarPartida(p, c, andar, DT);
    VERIFICAR(p.bill.preso <= 0.0f && p.bill.pes.x > antes.x);             // solto, volta a andar
}

TESTE(chefe_fase_2_a_metade_da_vida) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 2);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch != nullptr);
    if (!ch) return;
    for (int q = 0; q < 110; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);   // termina de surgir (1,6 s)
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::Mumia);
    golpear(p, c, *chefe(p), par.hp / 2 + par.armadura);   // exatamente a metade (a armadura desconta)
    ch = chefe(p);
    VERIFICAR(ch && ch->fase == 2 && ch->transicao > 0.0f && std::string(nomeDoChefe(*ch)) == "Pennywise I");
    for (const Aviso& a : p.avisos) VERIFICAR(a.golpe.origem != ch->id);   // ataques em curso cancelados
    const int hp = ch->hp;
    golpear(p, c, *ch, 40);
    VERIFICAR(chefe(p)->hp == hp);   // rasgando-se: invulnerável
    for (int q = 0; q < 100; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(chefe(p)->transicao <= 0.0f);
    golpear(p, c, *chefe(p), 40);
    VERIFICAR(chefe(p)->hp < hp);
}

TESTE(chefe_invoca_ratos_sem_passar_do_limite) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 3);
    int maximo = 0;
    for (int q = 0; q < 60 * 30; ++q) {
        p.bill.hp = p.bill.stats.hpMax;   // Bill parado longe, imortal: só o chefe age
        posicionarJogador(p.bill, centroCelula({2, 12}));
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        maximo = std::max(maximo, hostisVivos(p));
    }
    VERIFICAR(maximo >= 3);   // invocou (chefe + 2 ratos)
    VERIFICAR(maximo <= 6);   // e não enche a arena (limite de 4 ao decidir + 2 de uma vez)
}

TESTE(chefe_dispersa_conta_b_da_xp_e_abre_as_portas) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 5);
    esperar(p, c, 110);   // termina de surgir
    Inimigo rato = criarInimigo(TipoInimigo::Rato, p.proximoId++, centroCelula({20, 8}));
    p.inimigos.push_back(rato);
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::Mumia);
    golpear(p, c, *chefe(p), par.hp / 2 + par.armadura);
    esperar(p, c, 100);
    const int xp = p.bill.progresso.xp, k = p.stats.K;
    golpear(p, c, *chefe(p), par.hp);
    VERIFICAR(chefe(p) == nullptr && p.stats.B == 1 && p.stats.K == k);
    VERIFICAR(p.bill.progresso.xp >= xp + 60 || p.bill.progresso.nivel > 1);
    VERIFICAR(hostisVivos(p) == 0 && !p.dispersoes.empty());   // os ratos se desfazem junto
    esperar(p, c, 60 * 3);
    VERIFICAR(p.salaConcluida);
    for (const PortaSala& porta : p.sala.portas) {
        if (!porta.selada) VERIFICAR(!tileEm(p.sala, porta.celula.col, porta.celula.lin).bloqueiaMovimento);
    }
}
