// Stan e os Barrens (Etapa 10.4, PRD §12, §4.4, §9): os 4 Ecos + o lendário Asas, o resgate que fecha
// a comporta (a água vira lama e o enfeite troca de quadro) e as elites "Assombrados".
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

int ecoPorNome(const std::string& nome) {
    for (int i = 0; i < totalEcos(); ++i) {
        if (nome == definicaoEco(i).nome) return i;
    }
    return -1;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

EstadoPartida salaVazia(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 5, indicePorId(c, "b2_eco"), Dificuldade::Normal);
    return p;
}

// Golpe de Bill no alvo, repetido até ACERTAR (a esquiva é regra do jogo). Devolve o dano tirado.
int golpear(EstadoPartida& p, const Conteudo& c, IdEntidade id, int dano, bool podeCritico = false) {
    for (int t = 0; t < 20; ++t) {
        Inimigo* alvo = nullptr;
        for (Inimigo& i : p.inimigos) alvo = i.id == id ? &i : alvo;
        if (!alvo) return 0;
        alvo->hp = 999;
        p.hitStop = 0.0f;    // o hitstop de um acerto congelaria o próximo golpe, e eles cairiam juntos
        p.golpes.clear();
        Golpe g;
        g.id = 7000 + static_cast<std::uint32_t>(t) + static_cast<std::uint32_t>(p.proximoGolpe++);
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({alvo->pes.x, alvo->pes.y - 14.0f}, 16.0f);
        g.dano = dano;
        g.rolaAcerto = false;
        g.podeCritico = podeCritico;
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (Inimigo& i : p.inimigos) {
            if (i.id == id && i.hp != 999) return 999 - i.hp;
        }
    }
    return 0;
}
}  // namespace

TESTE(stan_ecos_na_tabela_com_textos) {
    for (const char* nome : {"Livro de pássaros", "Escoteiro", "Calma", "Ordem", "Asas"}) {
        const int i = ecoPorNome(nome);
        VERIFICAR(i >= 0);
        if (i >= 0) VERIFICAR(definicaoEco(i).amigo == Amigo::Stan);
    }
    VERIFICAR(ecoLendarioDe(Amigo::Stan) == ecoPorNome("Asas") && temEcos(Amigo::Stan));
    const int calma = ecoPorNome("Calma");
    VERIFICAR(descricaoEco(calma, Raridade::Comum) == "+0,05 s de invulnerabilidade no fim do dash");
    VERIFICAR(descricaoEco(calma, Raridade::Raro) == "+0,1 s de invulnerabilidade no fim do dash");
    VERIFICAR(descricaoEco(calma, Raridade::Epico) == "+0,15 s de invulnerabilidade no fim do dash");
    VERIFICAR(descricaoEco(ecoPorNome("Ponte firme"), Raridade::Raro) == "O dash deixa uma barreira de tijolos por 1,5 s");
}

TESTE(stan_livro_de_passaros_o_terceiro_golpe_da_mais) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    aplicarEco(p, ecoPorNome("Livro de pássaros"), Raridade::Comum);
    Inimigo ini = criarInimigo(TipoInimigo::Afogado, p.proximoId++, somar(p.bill.pes, Vec2{120.0f, 0.0f}));
    ini.surgindo = 0.0f;
    p.inimigos.push_back(ini);
    const int d1 = golpear(p, c, ini.id, 10), d2 = golpear(p, c, ini.id, 10), d3 = golpear(p, c, ini.id, 10);
    const int d4 = golpear(p, c, ini.id, 10);
    VERIFICAR(d1 > 0 && d1 == d2 && d4 == d1);
    VERIFICAR(d3 == d1 + 10);   // Comum: +10 no 3º golpe
}

TESTE(stan_ordem_da_critico_em_alvo_atordoado) {
    const Conteudo c = conteudoReal();
    auto taxa = [&](bool comEco) {
        EstadoPartida p = salaVazia(c);
        if (comEco) aplicarEco(p, ecoPorNome("Ordem"), Raridade::Epico);
        Inimigo ini = criarInimigo(TipoInimigo::Afogado, p.proximoId++, somar(p.bill.pes, Vec2{120.0f, 0.0f}));
        ini.surgindo = 0.0f;
        p.inimigos.push_back(ini);
        const int normal = danoEfetivo(20, parametrosDe(TipoInimigo::Afogado).armadura);
        int criticos = 0, n = 0;
        for (int k = 0; k < 400; ++k) {
            for (Inimigo& i : p.inimigos) i.atordoado = 5.0f;
            const int d = golpear(p, c, ini.id, 20, true);
            if (d <= 0) continue;
            ++n;
            criticos += d > normal ? 1 : 0;
        }
        return n ? static_cast<float>(criticos) / static_cast<float>(n) : -1.0f;
    };
    VERIFICAR(taxa(false) == 0.0f);
    const float com = taxa(true);
    VERIFICAR(com > 0.10f && com < 0.23f);   // Épico: 16%
}

TESTE(stan_escoteiro_paga_a_sala_sem_dano) {
    const Conteudo c = conteudoReal();
    auto moedasNoChao = [&](bool comEco, bool levaDano) {
        EstadoPartida p;
        iniciarPartida(p, c, 9, indicePorId(c, "b2_c01"), Dificuldade::Normal);
        if (comEco) aplicarEco(p, ecoPorNome("Escoteiro"), Raridade::Comum);
        for (int q = 0; q < 60 * 60 && !p.salaConcluida; ++q) {
            p.bill.invulneravel = 1.0f;
            for (Inimigo& i : p.inimigos) i.vivo = false;   // as ondas "caem" sem drop
            if (levaDano) p.danoNaSala = true;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
        }
        int moedas = 0;
        for (const ItemNoChao& it : p.itensChao) moedas += it.tipo == TipoItem::Moeda ? it.quantidade : 0;
        return moedas;
    };
    VERIFICAR(moedasNoChao(true, false) - moedasNoChao(false, false) == 10);
    VERIFICAR(moedasNoChao(true, true) == moedasNoChao(false, true));
}

TESTE(stan_calma_estende_a_invulnerabilidade_do_dash) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    aplicarEco(p, ecoPorNome("Calma"), Raridade::Epico);
    apertar(p, c, Comando::Dash);
    VERIFICAR(p.bill.dashRestante > 0.0f);
    for (int q = 0; q < 30 && p.bill.dashRestante > 0.0f; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.invulneravel > 0.11f && estaInvulneravel(p.bill));
}

TESTE(stan_asas_o_dash_tem_duas_cargas) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    EstadoEntrada dash;
    dash.pressionou[static_cast<int>(Comando::Dash)] = true;
    auto terminarDash = [&]() {
        for (int q = 0; q < 30 && p.bill.dashRestante > 0.0f; ++q) atualizarJogador(p.bill, p.sala, EstadoEntrada{}, DT);
    };
    // Sem Asas: o 2º dash espera a recarga.
    atualizarJogador(p.bill, p.sala, dash, DT);
    terminarDash();
    atualizarJogador(p.bill, p.sala, dash, DT);
    VERIFICAR(p.bill.dashRestante <= 0.0f && p.bill.recargaDash > 0.0f);
    // Com Asas (o lendário do resgate): dois seguidos; o 3º espera; a recarga devolve a carga.
    EstadoPartida q = salaVazia(c);
    aplicarEco(q, ecoLendarioDe(Amigo::Stan), Raridade::Lendario);
    VERIFICAR(q.bill.dashExtras == 1);
    p = q;
    atualizarJogador(p.bill, p.sala, dash, DT);
    terminarDash();
    atualizarJogador(p.bill, p.sala, dash, DT);
    VERIFICAR(p.bill.dashRestante > 0.0f);   // o 2º saiu
    terminarDash();
    atualizarJogador(p.bill, p.sala, dash, DT);
    VERIFICAR(p.bill.dashRestante <= 0.0f);   // o 3º não
    for (int k = 0; k < 120; ++k) atualizarJogador(p.bill, p.sala, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.recargaDash <= 0.0f && p.bill.dashExtrasDisponiveis == 1);
}

TESTE(stan_resgate_fecha_a_comporta_e_da_asas) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 21, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, "b2_resgate"), nullptr);
    auto comporta = [&]() {
        for (const ObjetoSala& o : p.sala.objetos) {
            if (o.codigo == 'e' && o.celula.col == 19 && o.celula.lin == 2) return o.quadro;
        }
        return -1;
    };
    VERIFICAR(comporta() == 0 && tileEm(p.sala, 19, 7).tipo == TipoTile::AguaFunda);
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    apertar(p, c, Comando::Interagir);
    const ObjetoSala* stan = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) stan = o.codigo == '1' ? &o : stan;
    VERIFICAR(stan != nullptr);
    if (!stan) return;
    posicionarJogador(p.bill, centroCelula({stan->celula.col, stan->celula.lin + 1}));
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Stan);
    VERIFICAR(comporta() == 1);                                            // a comporta fechou
    VERIFICAR(tileEm(p.sala, 19, 7).tipo == TipoTile::Lama && !tileEm(p.sala, 20, 12).bloqueiaMovimento);   // água → lama
    VERIFICAR(p.bill.dashExtras == 1);                                     // o lendário Asas
}

TESTE(elite_assombrado_um_por_sala_so_do_bioma_2_em_diante) {
    const Conteudo c = conteudoReal();
    auto elites = [&](const char* sala, std::set<int>& vidas) {
        EstadoPartida p;
        iniciarPartida(p, c, 13, indicePorId(c, sala), Dificuldade::Dificil);
        p.ajustes.chanceElite = 100;
        entrarNaSala(p, c, indicePorId(c, sala), nullptr);   // sorteia de novo com 100%
        std::set<IdEntidade> vistos;
        int n = 0;
        for (int q = 0; q < 60 * 60 && !p.salaConcluida; ++q) {
            p.bill.invulneravel = 1.0f;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            for (Inimigo& i : p.inimigos) {
                if (!vistos.insert(i.id).second) continue;
                if (i.elite) {
                    ++n;
                    // descontada a escala do bioma (forcaPct), sobra o +60% da elite
                    vidas.insert(i.hp * 100 / aplicarPorcentagem(parametrosDe(i.tipo).hp, i.forcaPct));
                }
            }
            for (Inimigo& i : p.inimigos) i.vivo = false;
        }
        return n;
    };
    std::set<int> vidas;
    VERIFICAR(elites("b2_c01", vidas) == 1);   // no máximo (e aqui exatamente) 1 por sala
    VERIFICAR(vidas.size() == 1 && *vidas.begin() >= 158 && *vidas.begin() <= 160);   // +60% de vida
    std::set<int> nenhuma;
    VERIFICAR(elites("b1_c01", nenhuma) == 0);   // o bioma 1 não tem elites
}

TESTE(sala_ao_resgatar_enfeite_precisa_de_enfeite) {
    SalaModelo s;
    std::string erro;
    VERIFICAR(lerSala("id=x\ntipo=teste\nenfeite=0 2 1\nao_resgatar_enfeite=2 1 1\nmapa:\n#####\n#E..#\n#####\n", s, erro));
    VERIFICAR(validarSala(s).empty() && s.enfeitesAoResgatar.size() == 1);
    VERIFICAR(lerSala("id=x\ntipo=teste\nao_resgatar_enfeite=2 1 1\nmapa:\n#####\n#E..#\n#####\n", s, erro));
    VERIFICAR(!validarSala(s).empty());
    VERIFICAR(!lerSala("id=x\ntipo=teste\nao_resgatar_enfeite=2 1 99\nmapa:\n#####\n#E..#\n#####\n", s, erro));
}
