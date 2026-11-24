// Companheiro (M11, SDD §24.6) e prólogo (PRD §4.3). O SDD pede: segue sem atravessar paredes
// (10 mil passos), teleporta ao ficar longe, troca para o último resgatado e o Eddie cura só
// 1× por sala. Aqui também cada habilidade da tabela e o prólogo completo (pelo piloto).
#include <algorithm>
#include <filesystem>

#include "jogo/clube.h"
#include "jogo/piloto.h"
#include "mundo/colisao.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    carregarDialogos(std::filesystem::path(ECOS_PASTA_DADOS) / "data/dialogos.txt", c.dialogos, erro);
    return c;
}

int indicePorId(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

void lerAteOFim(EstadoPartida& p, const Conteudo& c) {
    for (int k = 0; k < 200 && p.pendencia == Pendencia::Dialogo; ++k) apertar(p, c, Comando::Confirmar);
}

// Tentativa parada numa sala sem combate (o Bill e o companheiro só andam).
EstadoPartida salaCalma(const Conteudo& c, const std::string& id, Amigo quem) {
    EstadoPartida p;
    iniciarPartida(p, c, 11, indicePorId(c, id), Dificuldade::Normal);
    entrarCompanheiro(p, quem);
    return p;
}
}  // namespace

TESTE(companheiro_tentativa_comeca_com_o_mike) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    VERIFICAR(iniciarTentativa(p, c, 5, Dificuldade::Normal, erro));
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Mike);
    VERIFICAR(std::string(habilidadeDe(Amigo::Mike).nome) == "Lanterna extra");
    VERIFICAR(raioDaLuz(p) == RAIO_LANTERNA + 1.0f);   // Mike: +1 célula de luz
    VERIFICAR(amigoDoCodigo("beverly") == Amigo::Beverly && amigoDoCodigo("georgie") == Amigo::Total);
}

TESTE(companheiro_segue_sem_atravessar_paredes_10_mil_passos) {
    const Conteudo c = conteudoReal();
    for (const char* id : {"b1_c01", "b1_c02", "b1_resgate", "b1_refugio"}) {
        EstadoPartida p = salaCalma(c, id, Amigo::Stan);
        p.inimigos.clear();
        p.ondaAtual = 99;   // sem ondas: só andar
        int dentroDaParede = 0, longe = 0;
        const Vec2 eixos[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}, {0.7f, 0.7f}, {-0.7f, 0.7f}, {0, 0}};
        for (int q = 0; q < 10000; ++q) {
            EstadoEntrada e;
            e.eixo = eixos[(q / 70 + q / 333) % 7];   // muda de direção de tempos em tempos
            atualizarPartida(p, c, e, DT);
            p.sons.clear();
            if (p.indiceSala != indicePorId(c, id)) {   // saiu por uma porta: volta e segue o teste
                entrarNaSala(p, c, indicePorId(c, id), nullptr);
                p.inimigos.clear();
            }
            if (colideComSala(p.sala, caixaNosPes(p.companheiro.pes, CAIXA_COMPANHEIRO), false)) ++dentroDaParede;
            if (comprimento(subtrair(p.companheiro.pes, p.bill.pes)) > (DISTANCIA_TELEPORTE + 1.0f) * TILE) ++longe;
        }
        std::printf("    %-10s parede %d · longe %d · teleportes %d\n", id, dentroDaParede, longe, p.companheiro.teleportes);
        VERIFICAR(dentroDaParede == 0);
        VERIFICAR(longe == 0);
    }
}

TESTE(companheiro_teleporta_quando_fica_longe) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Mike);
    posicionarJogador(p.bill, centroCelula({22, 10}));
    p.companheiro.pes = centroCelula({2, 3});
    atualizarCompanheiro(p, DT);
    VERIFICAR(p.companheiro.teleportes == 1);
    VERIFICAR(comprimento(subtrair(p.companheiro.pes, p.bill.pes)) <= 3.5f * TILE);
    VERIFICAR(!colideComSala(p.sala, caixaNosPes(p.companheiro.pes, CAIXA_COMPANHEIRO), false));
}

TESTE(companheiro_troca_para_o_ultimo_resgatado) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 21, Dificuldade::Normal, erro);
    p.emTentativa = true;
    entrarNaSala(p, c, indicePorId(c, "b1_resgate"), nullptr);
    VERIFICAR(p.companheiro.quem == Amigo::Mike);
    // Abre a porta do Medo e fala com o Ben.
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    apertar(p, c, Comando::Interagir);
    VERIFICAR(tileEm(p.sala, 13, 7).tipo != TipoTile::PortaTrancada);
    const ObjetoSala* ben = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) ben = o.codigo == '1' ? &o : ben;
    VERIFICAR(ben != nullptr);
    if (!ben) return;
    posicionarJogador(p.bill, centroCelula({ben->celula.col, ben->celula.lin + 1}));
    VERIFICAR(amigoParaResgatar(p) == ben);
    const int xp = p.bill.progresso.xp;
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.stats.R == 1 && p.bill.progresso.xp == xp + 25);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Ben);
    VERIFICAR(p.resgatados.size() == 1 && p.resgatados[0] == Amigo::Ben);
    VERIFICAR(p.pendencia == Pendencia::Dialogo && p.dialogo.atual->id == "resgate_ben");
    VERIFICAR(raioDaLuz(p) == RAIO_LANTERNA);   // o Mike voltou ao Clube: a luz extra foi junto
    lerAteOFim(p, c);
    VERIFICAR(amigoParaResgatar(p) == nullptr);   // resgatado uma vez só
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.stats.R == 1);
}

TESTE(companheiro_eddie_cura_so_uma_vez_por_sala) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Eddie);
    p.bill.hp = p.bill.stats.hpMax / 5;   // 20%
    const int antes = p.bill.hp;
    atualizarCompanheiro(p, DT);
    VERIFICAR(p.bill.hp == antes + std::max(1, p.bill.stats.hpMax / 10));
    p.bill.hp = p.bill.stats.hpMax / 5;
    for (int q = 0; q < 120; ++q) atualizarCompanheiro(p, DT);
    VERIFICAR(p.bill.hp == p.bill.stats.hpMax / 5);   // não cura de novo nesta sala
    companheiroEntrouNaSala(p);                       // sala nova: pode de novo
    atualizarCompanheiro(p, DT);
    VERIFICAR(p.bill.hp > p.bill.stats.hpMax / 5);
    VERIFICAR(p.companheiro.usos == 2);
}

namespace {
// Inimigo parado (sem IA rodando: só o companheiro age) a `dx` células à direita de Bill.
void porInimigo(EstadoPartida& p, TipoInimigo t, float dx, float dy = 0.0f) {
    Inimigo i = criarInimigo(t, p.proximoId++, somar(p.bill.pes, Vec2{dx * TILE, dy * TILE}));
    i.surgindo = 0.0f;
    p.inimigos.push_back(i);
}

// `segundos` só do companheiro (com a luz em dia), registrando os golpes que ele soltar.
std::vector<Golpe> soCompanheiro(EstadoPartida& p, float segundos) {
    std::vector<Golpe> soltos;
    for (float t = 0.0f; t < segundos; t += DT) {
        atualizarVisao(p.visao, p.sala, p.bill.pes, raioDaLuz(p));
        const std::size_t antes = p.golpes.size();
        atualizarCompanheiro(p, DT);
        for (std::size_t k = antes; k < p.golpes.size(); ++k) soltos.push_back(p.golpes[k]);
        p.golpes.clear();
    }
    return soltos;
}
}  // namespace

// Diego, 24/11: "esse personagem só pode ajudar o Bill quando tem 2 ou mais inimigos ao mesmo tempo em tela".
TESTE(amigo_nao_ataca_com_um_inimigo_so) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Beverly);
    porInimigo(p, TipoInimigo::Rato, 3.0f);
    VERIFICAR(soCompanheiro(p, 8.0f).empty());
    VERIFICAR(p.companheiro.ataquesArma + p.companheiro.ataquesPoder == 0);
}

// "Ele só pode atacar um inimigo de 3 em 3 s, e a cada vez uma é com a arma, outra com o poder; ele tem a mesma
// força que o Bill no início do jogo."
TESTE(amigo_ataca_de_3_em_3_s_alternando_arma_e_poder) {
    const Conteudo c = conteudoReal();
    for (int a = 0; a < TOTAL_AMIGOS; ++a) {
        EstadoPartida p = salaCalma(c, "b1_refugio", static_cast<Amigo>(a));
        porInimigo(p, TipoInimigo::Rato, 3.0f);
        porInimigo(p, TipoInimigo::Sombra, 3.0f, 1.0f);
        const std::vector<Golpe> soltos = soCompanheiro(p, 7.6f);   // ataques em 1 s, 4 s e 7 s
        VERIFICAR(soltos.size() == 3);
        VERIFICAR(p.companheiro.ataquesArma == 2 && p.companheiro.ataquesPoder == 1);
        for (const Golpe& g : soltos) {
            VERIFICAR(g.doAmigo && g.equipe == Equipe::Jogador && g.origem != ID_JOGADOR);
            // A Beverly (Pontaria) soma a armadura do alvo, que a regra do dano tira de volta.
            VERIFICAR(g.dano >= danoDoAmigo() && g.dano <= danoDoAmigo() + 3);
        }
        const CombateAmigo& def = combateDe(static_cast<Amigo>(a));
        VERIFICAR(soltos[1].atordoa == def.atordoa && soltos[1].lentidao == def.lento);   // o 2º foi o poder
        VERIFICAR(soltos[0].atordoa == 0.0f && soltos[0].lentidao == 0.0f);               // o 1º, a arma
    }
    VERIFICAR(danoDoAmigo() == calcularStats(Atributos{}, Equipamento{}).danoBase);
}

// "Ele não vence os inimigos sozinho, senão a pessoa só ia ficar fugindo": o golpe do amigo nunca derruba o
// último inimigo da sala nem um chefe.
TESTE(amigo_nao_derruba_o_ultimo_inimigo_nem_o_chefe) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Mike);
    // Golpes do amigo no 1º inimigo até um pegar (o alvo pode se esquivar), sem a IA andar com ele.
    auto golpear = [&](EstadoPartida& e) {
        const int antes = e.inimigos[0].hp;
        for (std::uint32_t k = 0; k < 20 && e.inimigos[0].vivo && e.inimigos[0].hp == antes; ++k) {
            Golpe g;
            g.id = 4242 + k;
            g.equipe = Equipe::Jogador;
            g.area = areaCirculo({e.inimigos[0].pes.x, e.inimigos[0].pes.y - 12.0f}, 14.0f);
            g.dano = 50;
            g.rolaAcerto = false;
            g.doAmigo = true;
            g.duracao = 0.05f;
            e.golpes.push_back(g);
            atualizarPartida(e, c, EstadoEntrada{}, DT);
        }
    };
    porInimigo(p, TipoInimigo::Rato, 3.0f);
    p.inimigos[0].hp = 5;
    golpear(p);
    VERIFICAR(p.inimigos[0].vivo && p.inimigos[0].hp == 1);   // apanhou, mas ficou para o Bill
    // Com outro inimigo vivo, o mesmo golpe derruba (ele ajuda de verdade).
    EstadoPartida q = salaCalma(c, "b1_refugio", Amigo::Mike);
    porInimigo(q, TipoInimigo::Rato, 3.0f);
    porInimigo(q, TipoInimigo::Rato, -3.0f);
    q.inimigos[0].hp = 5;
    const IdEntidade primeiro = q.inimigos[0].id;
    golpear(q);
    VERIFICAR(std::none_of(q.inimigos.begin(), q.inimigos.end(), [&](const Inimigo& i) { return i.id == primeiro && i.vivo; }));
    VERIFICAR(!amigoPodeDerrubar(q, criarInimigo(TipoInimigo::Mumia, 999, q.bill.pes)));
}

// "No chefão ele ajuda só até o chefão virar Pennywise; aí fala que está com medo (exceto no último chefão)."
TESTE(amigo_luta_com_o_medo_e_trava_quando_vira_pennywise) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Ben);
    porInimigo(p, TipoInimigo::Mumia, 3.0f);
    VERIFICAR(soCompanheiro(p, 1.5f).size() == 1);   // o chefe na forma do medo conta sozinho
    p.inimigos[0].fase = 2;
    VERIFICAR(soCompanheiro(p, 8.0f).empty());
    VERIFICAR(p.companheiro.comMedo);
    bool falou = false;
    for (const TextoFlutuante& t : p.textos) falou = falou || t.texto == combateDe(Amigo::Ben).falaMedo;
    VERIFICAR(falou);
    // O medo é da sala: na sala seguinte ele volta a ajudar.
    companheiroEntrouNaSala(p);
    VERIFICAR(!p.companheiro.comMedo);
    // No Covil, o Pennywise IV não assusta: o amigo luta em todas as fases.
    EstadoPartida q = salaCalma(c, "b1_refugio", Amigo::Richie);
    porInimigo(q, TipoInimigo::PennywiseIV, 3.0f);
    q.inimigos[0].fase = 3;
    VERIFICAR(soCompanheiro(q, 4.5f).size() == 2);   // lançados em 1 s e 4 s (o poder sai 0,22 s depois)
    VERIFICAR(!q.companheiro.comMedo && q.companheiro.falouNoCovil);
}

TESTE(companheiro_richie_provoca) {
    const Conteudo c = conteudoReal();
    // Richie: o inimigo mais perto de Bill é provocado por 2 s (recarga 10 s).
    EstadoPartida q = salaCalma(c, "b1_refugio", Amigo::Richie);
    Inimigo sombra = criarInimigo(TipoInimigo::Sombra, q.proximoId++, somar(q.bill.pes, Vec2{2.0f * TILE, 0.0f}));
    sombra.surgindo = 0.0f;
    q.inimigos.push_back(sombra);
    atualizarVisao(q.visao, q.sala, q.bill.pes, raioDaLuz(q));
    atualizarCompanheiro(q, DT);
    VERIFICAR(provocado(q, q.inimigos[0].id) && q.companheiro.recarga > 9.9f);
    for (int k = 0; k < 130; ++k) atualizarCompanheiro(q, DT);
    VERIFICAR(!provocado(q, q.inimigos[0].id));   // passou o efeito
}

TESTE(companheiro_ben_segura_o_golpe_anunciado) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Ben);
    Aviso a;
    a.tempoTotal = 0.5f;
    a.tempoRestante = 0.2f;
    a.golpe.equipe = Equipe::Hostil;
    a.golpe.area = areaCirculo(p.bill.pes, 30.0f);
    a.golpe.dano = 15;
    a.golpe.rolaAcerto = false;
    p.avisos.push_back(a);
    atualizarCompanheiro(p, DT);
    VERIFICAR(barreiraAtiva(p) && p.companheiro.recarga > 11.9f);
    // O golpe (o que o aviso vira ao disparar) em cima de Bill: a barreira segura.
    Golpe g = a.golpe;
    g.id = 999;
    g.duracao = 0.1f;
    p.avisos.clear();
    p.golpes.push_back(g);
    const int hp = p.bill.hp;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp == hp);
    // Sem o Ben, o mesmo golpe fere.
    EstadoPartida s = salaCalma(c, "b1_refugio", Amigo::Stan);
    g.area = areaCirculo(s.bill.pes, 30.0f);
    s.golpes.push_back(g);
    const int hp2 = s.bill.hp;
    atualizarPartida(s, c, EstadoEntrada{}, DT);
    VERIFICAR(s.bill.hp < hp2);
    VERIFICAR(revelaArmadilhas(s) && !revelaArmadilhas(p));
}

TESTE(prologo_mike_abre_a_rua_e_a_saida_comeca_a_tentativa) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    VERIFICAR(iniciarPrologo(p, c, 8, Dificuldade::Normal));
    VERIFICAR(p.noPrologo && !p.salaConcluida && !p.companheiro.ativo);
    for (const PortaSala& porta : p.sala.portas) VERIFICAR(tileEm(p.sala, porta.celula.col, porta.celula.lin).bloqueiaMovimento);
    // Pelo piloto (só comandos): fala com o Mike, lê, segue até a saída.
    MemoriaPiloto m;
    Perfil perfil;
    int quadros = 0;
    bool falou = false;
    for (; quadros < 60 * 60 && !p.emTentativa; ++quadros) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        falou |= p.pendencia == Pendencia::Dialogo && p.dialogo.atual && p.dialogo.atual->id == "prologo_mike";
        atenderClube(perfil, p, c, false);
    }
    VERIFICAR(falou);
    VERIFICAR(p.emTentativa && !p.noPrologo && quadros < 40 * 60);
    VERIFICAR(perfil.prologoVisto && perfil.falasVistas.count("prologo_mike"));
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Mike);
}

// 24/11: o "Olho atento" do Stan: o passarinho pousa nas armadilhas na luz e, na primeira de cada sala, o Stan avisa.
TESTE(stan_avisa_a_primeira_armadilha_de_cada_sala) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaCalma(c, "b1_refugio", Amigo::Stan);
    p.inimigos.clear();
    const Celula b = celulaDe(p.bill.pes);
    const int col = b.col + 1, lin = b.lin;
    p.sala.tiles[static_cast<std::size_t>(lin * p.sala.cols + col)].tipo = TipoTile::Espinhos;
    p.visao.celulas[static_cast<std::size_t>(lin * p.sala.cols + col)].luz = 200;
    VERIFICAR(stanApontaArmadilha(p, col, lin));
    VERIFICAR(!stanApontaArmadilha(p, b.col, b.lin - 50));   // fora da sala não é armadilha
    const auto falasDoStan = [](const EstadoPartida& e) {
        int n = 0;
        for (const TextoFlutuante& t : e.textos) n += t.texto == "Stan: Bill, cuidado ali!";
        return n;
    };
    atualizarCompanheiro(p, DT);
    VERIFICAR(p.companheiro.avisouArmadilha && falasDoStan(p) == 1);
    atualizarCompanheiro(p, DT);   // não repete na mesma sala
    VERIFICAR(falasDoStan(p) == 1);
    companheiroEntrouNaSala(p);    // sala nova: avisa de novo
    VERIFICAR(!p.companheiro.avisouArmadilha);
    // Com outro amigo, ninguém aponta.
    p.companheiro.quem = Amigo::Mike;
    VERIFICAR(!stanApontaArmadilha(p, col, lin));
}
