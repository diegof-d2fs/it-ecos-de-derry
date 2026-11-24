// Ecos (Etapa 8.1, PRD §12): a tabela, o sorteio de 3 sem repetir, os efeitos do Ben
// (armadura, vida, golpe contra a parede, barreira do dash, Poema), o altar, o lendário no
// resgate e a escolha do piloto. Sem janela.
#include <filesystem>

#include "jogo/clube.h"
#include "jogo/piloto.h"
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

int ecoChamado(const char* nome) {
    for (int i = 0; i < totalEcos(); ++i) {
        if (std::string(definicaoEco(i).nome) == nome) return i;
    }
    return -1;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

// Tentativa parada numa sala (para testar efeitos sem as ondas atrapalharem).
EstadoPartida naSala(const Conteudo& c, const std::string& id) {
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 3, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, id), nullptr);
    p.salaConcluida = true;
    return p;
}

// Dano que um golpe do Bill (origem ID_JOGADOR, sem rolar acerto) causa num rato em `pos`.
// Repete com seeds diferentes até não haver esquiva (a esquiva do rato é sorteada).
int danoNoRato(const Conteudo& c, Vec2 pos, bool comPlanta) {
    for (std::uint64_t seed = 1; seed < 50; ++seed) {
        EstadoPartida p = naSala(c, "b1_refugio");
        semear(p.rng, seed);
        if (comPlanta) aplicarEco(p, ecoChamado("Planta baixa"), Raridade::Comum);
        Inimigo rato = criarInimigo(TipoInimigo::Rato, p.proximoId++, pos);
        rato.surgindo = 0.0f;
        rato.hp = 500;
        p.inimigos.push_back(rato);
        Golpe g;
        g.id = 1;
        g.equipe = Equipe::Jogador;
        g.origem = ID_JOGADOR;
        g.area = areaCirculo({pos.x, pos.y - 8.0f}, 10.0f);
        g.dano = 20;
        g.rolaAcerto = false;
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        if (!p.inimigos.empty() && p.inimigos[0].hp < 500) return 500 - p.inimigos[0].hp;
    }
    return -1;
}
}  // namespace

TESTE(ecos_tabela_do_ben_e_sorteio_sem_repetir) {
    VERIFICAR(temEcos(Amigo::Ben) && temEcos(Amigo::Stan) && temEcos(Amigo::Eddie) && temEcos(Amigo::Beverly) && temEcos(Amigo::Mike) && temEcos(Amigo::Richie));   // Etapa 14.3: os seis
    VERIFICAR(amigoDoBioma(1) == Amigo::Ben && amigoDoBioma(6) == Amigo::Richie && amigoDoBioma(0) == Amigo::Total);
    int doBen = 0;
    for (int i = 0; i < totalEcos(); ++i) {
        const DefinicaoEco& d = definicaoEco(i);
        if (d.amigo == Amigo::Ben && !d.lendario) {
            ++doBen;
            VERIFICAR(d.valor[0] < d.valor[1] && d.valor[1] < d.valor[2]);   // raridade maior = número maior
        }
    }
    VERIFICAR(doBen == 4);
    VERIFICAR(ecoLendarioDe(Amigo::Ben) == ecoChamado("Poema") && definicaoEco(ecoChamado("Poema")).lendario);
    VERIFICAR(descricaoEco(ecoChamado("Tijolo a tijolo"), Raridade::Epico) == "+3 de armadura");
    VERIFICAR(descricaoEco(ecoChamado("Ponte firme"), Raridade::Raro) == "O dash deixa uma barreira de tijolos por 1,5 s");
    VERIFICAR(descricaoEco(ecoChamado("Ponte firme"), Raridade::Comum) == "O dash deixa uma barreira de tijolos por 1 s");
    Rng rng;
    semear(rng, 9);
    std::vector<EcoAtivo> ativos;
    for (int rodada = 0; rodada < 4; ++rodada) {
        const std::vector<OfertaEco> of = sortearEcos(ativos, Amigo::Ben, rng);
        VERIFICAR(static_cast<int>(of.size()) == std::min(3, 4 - rodada));
        for (std::size_t a = 0; a < of.size(); ++a) {
            VERIFICAR(!definicaoEco(of[a].definicao).lendario && of[a].raridade != Raridade::Lendario);
            for (const EcoAtivo& at : ativos) VERIFICAR(at.definicao != of[a].definicao);   // não repete o que tem
            for (std::size_t b = a + 1; b < of.size(); ++b) VERIFICAR(of[a].definicao != of[b].definicao);
        }
        ativos.push_back({of[0].definicao, of[0].raridade});
    }
    VERIFICAR(sortearEcos(ativos, Amigo::Ben, rng).empty());
    // Raridades: 70/25/5 em 3000 sorteios (com folga).
    int contagem[3] = {};
    for (int k = 0; k < 3000; ++k) ++contagem[static_cast<int>(sortearRaridade(rng))];
    VERIFICAR(contagem[0] > 1950 && contagem[0] < 2250 && contagem[2] > 80 && contagem[2] < 230);
}

TESTE(ecos_passivos_entram_nos_atributos) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naSala(c, "b1_refugio");
    const int hpMax = p.bill.stats.hpMax, hp = p.bill.hp, arm = p.bill.stats.armadura;
    aplicarEco(p, ecoChamado("Coração pesado"), Raridade::Raro);
    VERIFICAR(p.bill.stats.hpMax == hpMax + 15 && p.bill.hp == hp + 15);
    aplicarEco(p, ecoChamado("Tijolo a tijolo"), Raridade::Epico);
    VERIFICAR(p.bill.stats.armadura == arm + 3);
    recalcularJogador(p.bill);   // subir de nível ou pegar item recalcula: o Eco continua valendo
    VERIFICAR(p.bill.stats.hpMax == hpMax + 15 && p.bill.stats.armadura == arm + 3);
}

TESTE(ecos_planta_baixa_so_vale_contra_a_parede) {
    const Conteudo c = conteudoReal();
    const Vec2 parede = centroCelula({1, 6});    // encostado na parede esquerda do refúgio
    const Vec2 meio = centroCelula({12, 8});     // no meio da sala
    const int semEco = danoNoRato(c, parede, false), comEcoParede = danoNoRato(c, parede, true),
              comEcoMeio = danoNoRato(c, meio, true);
    std::printf("    rato na parede: %d sem Eco, %d com Planta baixa; no meio com o Eco: %d\n", semEco, comEcoParede, comEcoMeio);
    VERIFICAR(semEco > 0 && comEcoParede > semEco && comEcoMeio == semEco);
}

TESTE(ecos_ponte_firme_deixa_barreira_que_segura_golpes) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naSala(c, "b1_refugio");
    aplicarEco(p, ecoChamado("Ponte firme"), Raridade::Comum);   // 1000 ms
    EstadoEntrada dash;
    dash.pressionou[static_cast<int>(Comando::Dash)] = true;
    dash.eixo = {1.0f, 0.0f};
    atualizarPartida(p, c, dash, DT);
    for (int q = 0; q < 40 && p.barreiras.empty(); ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.barreiras.size() == 1);
    Golpe g;
    g.id = 77;
    g.equipe = Equipe::Hostil;
    g.area = areaCirculo(p.bill.pes, 20.0f);
    g.dano = 12;
    g.rolaAcerto = false;
    p.golpes.push_back(g);
    p.bill.invulneravel = 0.0f;
    const int hp = p.bill.hp;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp == hp);
    for (int q = 0; q < 70; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.barreiras.empty());   // durou o tempo do Eco e sumiu
}

TESTE(ecos_poema_salva_uma_vez_por_bioma) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naSala(c, "b1_refugio");
    aplicarEco(p, ecoChamado("Poema"), Raridade::Lendario);
    const int pactos = p.bill.pactos;
    p.bill.hp = 0;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp == 1 && p.poemaUsado && p.bill.pactos == pactos && p.stats.U == 0);
    p.bill.hp = 0;
    p.bill.invulneravel = 0.0f;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.pactos == pactos - 1 && p.stats.U == 1);   // a 2ª queda gasta o Pacto
}

TESTE(ecos_altar_oferece_tres_e_acende_uma_vez) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naSala(c, "b1_eco");
    const ObjetoSala* altar = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) altar = o.codigo == '*' ? &o : altar;
    VERIFICAR(altar != nullptr);
    if (!altar) return;
    posicionarJogador(p.bill, centroCelula({altar->celula.col, altar->celula.lin + 1}));
    VERIFICAR(altarProximo(p) == altar);
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::EscolherEco && p.ofertasEco.size() == 3);
    const OfertaEco segunda = p.ofertasEco[1];
    apertar(p, c, Comando::Direita);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && p.ecos.size() == 1 && p.ecos[0].definicao == segunda.definicao);
    VERIFICAR(altarProximo(p) == nullptr);   // apagado
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && p.ecos.size() == 1);
}

TESTE(ecos_resgate_do_ben_da_o_poema) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naSala(c, "b1_resgate");
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    apertar(p, c, Comando::Interagir);   // abre a porta do Medo
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == '1') posicionarJogador(p.bill, centroCelula({o.celula.col, o.celula.lin + 1}));
    }
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.stats.R == 1 && p.ecos.size() == 1);
    VERIFICAR(p.ecos[0].definicao == ecoChamado("Poema") && p.ecos[0].raridade == Raridade::Lendario);
}

TESTE(ecos_piloto_prefere_raridade_e_depois_vida) {
    std::vector<OfertaEco> of = {{ecoChamado("Planta baixa"), Raridade::Comum},
                                 {ecoChamado("Coração pesado"), Raridade::Comum},
                                 {ecoChamado("Tijolo a tijolo"), Raridade::Comum}};
    VERIFICAR(ecoPreferidoDoPiloto(of) == 1);
    of[0].raridade = Raridade::Epico;
    VERIFICAR(ecoPreferidoDoPiloto(of) == 0);
}
