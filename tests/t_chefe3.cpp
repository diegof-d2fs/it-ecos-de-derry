// Chefe 3 (Etapa 11.4, PRD §10.1): o Leproso → Pennywise II. Nuvem pútrida (lento por 5 s),
// investida longa, e na fase 2 ele some e volta pela PORTA com um bote anunciado.
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

EstadoPartida naArena(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b3_chefe"), Dificuldade::Normal);
    for (int q = 0; q < 60 * 4; ++q) {
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    return p;
}

// Golpe de Bill no chefe, repetido até acertar (esquiva é regra do jogo).
void golpear(EstadoPartida& p, const Conteudo& c, int dano) {
    for (int t = 0; t < 8; ++t) {
        Inimigo* ch = chefe(p);
        if (!ch) return;
        ch->submerso = 0.0f;
        const int antes = ch->hp;
        Golpe g;
        g.id = 9500 + static_cast<std::uint32_t>(t);
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({ch->pes.x, ch->pes.y - 20.0f}, 16.0f);
        g.dano = dano;
        g.rolaAcerto = false;
        p.hitStop = 0.0f;
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        if (!chefe(p) || chefe(p)->hp != antes) return;
    }
}
}  // namespace

TESTE(chefe3_arena_e_nome) {
    const Conteudo c = conteudoReal();
    VERIFICAR(chefeDoBioma(3) == TipoInimigo::Leproso && chefeDoBioma(7) == TipoInimigo::PennywiseIV && chefeDoBioma(8) == TipoInimigo::Total);   // Etapa 15: o Covil
    EstadoPartida p = naArena(c, 1);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->tipo == TipoInimigo::Leproso && std::string(nomeDoChefe(*ch)) == "O Leproso");
    int quantos = 0;
    const PadraoChefe* ps = padroesDoChefe(TipoInimigo::Leproso, quantos);
    for (int i = 0; i < quantos; ++i) {   // a mesma regra de dados de todos os chefes
        VERIFICAR(ps[i].preparo >= PREPARO_MINIMO_AVISO && ps[i].recuperacao >= 0.6f);
        if (!padraoSemAlcance(ps[i].forma)) VERIFICAR(ps[i].distanciaMax <= ps[i].comprimento + 4.0f);
    }
}

TESTE(chefe3_nuvem_putrida_deixa_lento_e_some_em_5_s) {
    const Conteudo c = conteudoReal();
    bool viu = false;
    for (std::uint64_t seed = 1; seed <= 10 && !viu; ++seed) {
        EstadoPartida p = naArena(c, seed);
        p.nuvens.clear();   // uma nuvem da preparação da arena não vale: só a que nascer agora
        for (int q = 0; q < 60 * 25 && !viu; ++q) {
            p.bill.invulneravel = 1.0f;
            p.bill.hp = p.bill.stats.hpMax;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            if (p.nuvens.empty()) continue;
            viu = true;
            const NuvemPutrida n = p.nuvens.front();
            VERIFICAR(n.vida > DURACAO_NUVEM - 0.1f);
            posicionarJogador(p.bill, n.centro);
            p.inimigos.clear();   // só a nuvem importa agora
            p.avisos.clear();
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            VERIFICAR(p.bill.lento > 0.0f);   // dentro: anda a 50%
            for (int k = 0; k < static_cast<int>(DURACAO_NUVEM * 60.0f) + 5; ++k) atualizarPartida(p, c, EstadoEntrada{}, DT);
            VERIFICAR(p.nuvens.empty());
        }
    }
    VERIFICAR(viu);
}

TESTE(chefe3_fase_2_some_e_volta_pela_porta) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naArena(c, 2);
    const ParametrosInimigo& par = parametrosDe(TipoInimigo::Leproso);
    golpear(p, c, par.hp / 2 + par.armadura);
    Inimigo* ch = chefe(p);
    VERIFICAR(ch && ch->fase == 2 && std::string(nomeDoChefe(*ch)) == "Pennywise II");
    bool viu = false;
    for (int q = 0; q < 60 * 30 && !viu; ++q) {
        p.bill.invulneravel = 1.0f;
        p.bill.hp = p.bill.stats.hpMax;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        ch = chefe(p);
        if (!ch || ch->submerso <= 0.0f) continue;
        int quantos = 0;
        if (padroesDoChefe(ch->tipo, quantos)[ch->padrao].forma != FormaPadrao::Porta) continue;
        viu = true;
        VERIFICAR(!podeSerAtingido(*ch));
        // A saída é a célula de dentro de uma porta da sala, e a linha anunciada parte dela.
        const Vec2 saida = ch->alvoRegistrado;
        bool juntoDeUmaPorta = false;
        for (const PortaSala& porta : p.sala.portas) juntoDeUmaPorta |= comprimento(subtrair(centroCelula(porta.celula), saida)) <= TILE + 0.5f;
        VERIFICAR(juntoDeUmaPorta);
        bool linha = false;
        for (const Aviso& a : p.avisos) linha |= a.golpe.origem == ch->id && a.golpe.area.forma == FormaArea::Linha &&
                                               comprimento(subtrair(a.golpe.area.centro, saida)) < 0.5f;
        VERIFICAR(linha);
        const IdEntidade id = ch->id;
        for (int k = 0; k < 75; ++k) {
            p.bill.invulneravel = 1.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
        }
        const Inimigo* depois = buscarInimigo(p, id);
        VERIFICAR(depois && depois->submerso <= 0.0f && comprimento(subtrair(depois->pes, saida)) > TILE);   // saiu e deu o bote
    }
    VERIFICAR(viu);
}
