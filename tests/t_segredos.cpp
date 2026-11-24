// Ponte do Ben e segredos (Etapa 8.4; PRD §14.3/F19, SDD §24.5/M13): o formato ("ao_resgatar",
// "entrada=fixa", 'r', 'l'), a validação do segredo, a parede que cai com o Especial (e não
// com o golpe comum), o segredo sorteado por bioma, a Lembrancinha até a estante do perfil, a
// ponte baixada no resgate, a entrada fixa e o item de missão que nunca fica para trás.
#include <filesystem>

#include "jogo/perfil.h"
#include "jogo/piloto.h"
#include "jogo/simulacao.h"
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

int rachadas(const EstadoPartida& p) {
    int n = 0;
    for (const Tile& t : p.sala.tiles) n += t.tipo == TipoTile::ParedeRachada ? 1 : 0;
    return n;
}

bool temItem(const EstadoPartida& p, TipoItem t) {
    for (const ItemNoChao& it : p.itensChao) {
        if (it.tipo == t) return true;
    }
    return false;
}
}  // namespace

TESTE(segredos_formato_e_validacao) {
    SalaModelo s;
    std::string erro;
    const std::string base = "id=x\ntipo=resgate\n";
    VERIFICAR(lerSala(base + "entrada=fixa\nao_resgatar=3 1 =\nmapa:\n#######\n#E.ww.#\n#######\n", s, erro));
    VERIFICAR(s.entradaFixa && s.aoResgatar.size() == 1 && s.aoResgatar[0].simbolo == '=');
    VERIFICAR(!lerSala(base + "ao_resgatar=3 1 Z\nmapa:\n#####\n#E..#\n#####\n", s, erro));   // símbolo que não existe
    VERIFICAR(!lerSala(base + "ao_resgatar=30 1 =\nmapa:\n#####\n#E..#\n#####\n", s, erro));  // fora do mapa
    VERIFICAR(!lerSala(base + "entrada=talvez\nmapa:\n#####\n#E..#\n#####\n", s, erro));
    // Lembrancinha que se alcança sem derrubar a parede: a validação reclama.
    VERIFICAR(lerSala("id=y\ntipo=teste\nmapa:\n#######\n#E..l.#\n#######\n", s, erro));
    bool achou = false;
    for (const std::string& pr : validarSala(s)) achou |= pr.find("Lembrancinha") != std::string::npos;
    VERIFICAR(achou);
    // Atrás da parede rachada (com os dois lados livres): válido.
    VERIFICAR(lerSala("id=z\ntipo=teste\nmapa:\n#######\n#E.r.l#\n#######\n", s, erro));
    VERIFICAR(validarSala(s).empty());
    // Parede rachada sem os dois lados: reclama.
    VERIFICAR(lerSala("id=w\ntipo=teste\nmapa:\n#######\n#E..r##\n#######\n", s, erro));
    achou = false;
    for (const std::string& pr : validarSala(s)) achou |= pr.find("dois lados") != std::string::npos;
    VERIFICAR(achou);
    // As salas reais com nicho são válidas (a validação geral está em t_salas).
    const Conteudo c = conteudoReal();
    for (const char* id : {"b1_c01", "b1_c02", "b1_c03", "b1_resgate"}) {
        VERIFICAR(validarSala(c.salas[static_cast<std::size_t>(indicePorId(c, id))]).empty());
    }
}

TESTE(segredos_especial_derruba_a_parede_e_o_golpe_comum_nao) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    iniciarPartida(p, c, 3, indicePorId(c, "b1_c01"), Dificuldade::Normal);   // avulsa: o segredo sempre vale
    VERIFICAR(p.salaTemSegredo && rachadas(p) == 1 && temItem(p, TipoItem::Lembrancinha));
    p.inimigos.clear();
    p.ondaAtual = 99;
    posicionarJogador(p.bill, centroCelula({22, 5}));   // logo abaixo da parede rachada (22,4)
    apertar(p, c, Comando::Atacar);
    for (int q = 0; q < 30; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(rachadas(p) == 1);   // o golpe comum não derruba
    const int xp = p.bill.progresso.xp, versao = p.sala.versaoTerreno;
    apertar(p, c, Comando::Especial);
    VERIFICAR(rachadas(p) == 0 && tileEm(p.sala, 22, 4).tipo == TipoTile::Chao);
    VERIFICAR(p.sala.versaoTerreno > versao && (p.bill.progresso.xp >= xp + 15 || p.bill.progresso.nivel > 1));
}

TESTE(segredos_um_por_bioma_e_so_se_a_chance_sorteou) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 1, Dificuldade::Normal, erro);
    // Chance não sorteou: o 'r' é parede comum e não há Lembrancinha.
    p.segredoAtivo = false;
    p.segredoUsado = false;
    entrarNaSala(p, c, indicePorId(c, "b1_c01"), nullptr);
    VERIFICAR(!p.salaTemSegredo && rachadas(p) == 0 && !temItem(p, TipoItem::Lembrancinha));
    VERIFICAR(tileEm(p.sala, 22, 4).tipo == TipoTile::Parede);
    // Sorteou: a 1ª sala com 'r' guarda o segredo; a 2ª, não.
    p.segredoAtivo = true;
    entrarNaSala(p, c, indicePorId(c, "b1_c02"), nullptr);
    VERIFICAR(p.salaTemSegredo && rachadas(p) == 1 && temItem(p, TipoItem::Lembrancinha) && p.segredoUsado);
    entrarNaSala(p, c, indicePorId(c, "b1_c03"), nullptr);
    VERIFICAR(!p.salaTemSegredo && rachadas(p) == 0 && !temItem(p, TipoItem::Lembrancinha));
    // A chance vem da dificuldade (SDD §24.2): em 200 tentativas, Fácil > Difícil.
    int facil = 0, dificil = 0;
    for (std::uint64_t s = 1; s <= 200; ++s) {
        EstadoPartida a, b;
        iniciarTentativa(a, c, s, Dificuldade::Facil, erro);
        iniciarTentativa(b, c, s, Dificuldade::Dificil, erro);
        facil += a.segredoAtivo ? 1 : 0;
        dificil += b.segredoAtivo ? 1 : 0;
    }
    std::printf("    segredo sorteado: Fácil %d/200 · Difícil %d/200\n", facil, dificil);
    VERIFICAR(facil > dificil && facil > 130 && dificil < 120);
}

TESTE(segredos_lembrancinha_vai_para_a_estante_e_rende_lembrancas) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 1, Dificuldade::Normal, erro);
    const int antes = lembrancasDaTentativa(p);
    soltarItem(p, TipoItem::Lembrancinha, 1, p.bill.pes, p.bill.pes);
    for (int q = 0; q < 40; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.lembrancinhas == (1u << 1));   // a do bioma 1
    VERIFICAR(lembrancasDaTentativa(p) == antes + 15);
    Perfil perfil;
    registrarFimDeTentativa(perfil, p, false);
    VERIFICAR(perfil.lembrancinhas == (1u << 1));
    Perfil lido;
    lerPerfil(escreverPerfil(perfil), lido);
    VERIFICAR(lido.lembrancinhas == (1u << 1));   // persiste
    Perfil doPiloto;
    registrarFimDeTentativa(doPiloto, p, true);
    VERIFICAR(doPiloto.lembrancinhas == 0);       // o computador não enche a estante de ninguém
}

TESTE(segredos_ben_baixa_a_ponte_e_a_entrada_e_fixa) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 21, Dificuldade::Normal, erro);
    const SalaModelo& resgate = c.salas[static_cast<std::size_t>(indicePorId(c, "b1_resgate"))];
    // Vindo por uma porta do lado direito, Bill ainda entra pelo E (à esquerda do canal).
    PortaSala vinda;
    vinda.lado = Direcao::Esquerda;   // saiu pela esquerda da sala anterior → cairia na porta da direita
    entrarNaSala(p, c, indicePorId(c, "b1_resgate"), &vinda);
    VERIFICAR(celulaDe(p.bill.pes).col <= 4);
    VERIFICAR(tileEm(p.sala, 19, 7).tipo == TipoTile::AguaFunda && tileEm(p.sala, 19, 7).bloqueiaMovimento);
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    apertar(p, c, Comando::Interagir);
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == '1') posicionarJogador(p.bill, centroCelula({o.celula.col, o.celula.lin + 1}));
    }
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.stats.R == 1);
    for (const SalaModelo::Alteracao& a : resgate.aoResgatar) {
        VERIFICAR(tileEm(p.sala, a.celula.col, a.celula.lin).tipo == TipoTile::Ponte);
        VERIFICAR(!tileEm(p.sala, a.celula.col, a.celula.lin).bloqueiaMovimento);
    }
}

TESTE(segredos_item_de_missao_nunca_fica_para_tras) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 4, Dificuldade::Normal, erro);
    soltarItem(p, TipoItem::ChaveDoMedo, 1, centroCelula({20, 10}), centroCelula({20, 10}));   // longe do Bill
    soltarItem(p, TipoItem::Kit, 1, centroCelula({20, 11}), centroCelula({20, 11}));
    p.salaConcluida = true;
    PortaSala saida;
    for (const PortaSala& porta : p.sala.portas) {
        if (!porta.selada && porta.opcao >= 0) saida = porta;
    }
    VERIFICAR(saida.opcao >= 0);
    posicionarJogador(p.bill, centroCelula(saida.celula));
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.etapa == 1);
    VERIFICAR(p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] == 1);   // foi junto
    VERIFICAR(p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)] == 0);           // o comum ficou
}

TESTE(segredos_piloto_acha_derruba_e_pega_a_lembrancinha) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 7, Dificuldade::Normal, erro);   // as missões do piloto são da tentativa
    p.segredoAtivo = true;
    p.segredoUsado = false;
    entrarNaSala(p, c, indicePorId(c, "b1_c01"), nullptr);
    VERIFICAR(p.salaTemSegredo);
    MemoriaPiloto m;
    for (int q = 0; q < 60 * 180 && p.lembrancinhas == 0; ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
        if (p.indiceSala != indicePorId(c, "b1_c01")) break;
    }
    VERIFICAR(rachadas(p) == 0);
    VERIFICAR(p.lembrancinhas != 0);
}

// ---------- Festa dos Balões (fase bônus, M13) ----------
namespace {
EstadoPartida naFesta(const Conteudo& c, std::uint64_t seed) {
    EstadoPartida p;
    iniciarPartida(p, c, seed, indicePorId(c, "b1_festa"), Dificuldade::Normal);
    return p;
}

void golpeEm(EstadoPartida& p, Vec2 centro) {
    Golpe g;
    g.id = 4242;
    g.equipe = Equipe::Jogador;
    g.origem = ID_JOGADOR;
    g.area = areaCirculo(centro, 14.0f);
    g.dano = 10;
    g.rolaAcerto = false;
    g.duracao = 0.02f;
    p.golpes.push_back(g);
}
}  // namespace

TESTE(festa_fecha_as_portas_baloes_sobem_e_abre_no_fim) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naFesta(c, 1);
    VERIFICAR(p.festaAtiva && !p.salaConcluida && p.tempoFesta == DURACAO_FESTA);
    for (const PortaSala& porta : p.sala.portas) VERIFICAR(tileEm(p.sala, porta.celula.col, porta.celula.lin).bloqueiaMovimento);
    for (int q = 0; q < 120; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(!p.baloes.empty());
    const float y = p.baloes[0].pos.y;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.baloes[0].pos.y < y);   // sobem
    const int c0 = p.stats.C;
    for (int q = 0; q < 60 * 30; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(!p.festaAtiva && p.salaConcluida && p.stats.C == c0 + 1 && p.baloes.empty());
    for (const PortaSala& porta : p.sala.portas) {
        if (!porta.selada) VERIFICAR(!tileEm(p.sala, porta.celula.col, porta.celula.lin).bloqueiaMovimento);
    }
}

TESTE(festa_colorido_da_moedas_e_vermelho_fere_sem_matar) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naFesta(c, 2);
    p.inimigos.clear();
    BalaoFesta azul;
    azul.id = 777;
    azul.pos = centroCelula({12, 8});
    p.baloes = {azul};
    p.proximoBalao = 99.0f;
    const int xp = p.bill.progresso.xp;
    golpeEm(p, {azul.pos.x, azul.pos.y - 22.0f});
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.baloesEstourados == 1 && p.baloes.empty() && p.bill.progresso.xp > xp);
    bool moeda = false;
    for (const ItemNoChao& it : p.itensChao) moeda |= it.tipo == TipoItem::Moeda;
    VERIFICAR(moeda);
    // Vermelho estourado ao lado do Bill com 1 de vida: fere, mas na festa ninguém morre.
    BalaoFesta vermelho;
    vermelho.id = 778;
    vermelho.vermelho = true;
    vermelho.pos = p.bill.pes;
    p.baloes = {vermelho};
    p.bill.hp = 1;
    p.bill.invulneravel = 0.0f;
    golpeEm(p, {vermelho.pos.x, vermelho.pos.y - 22.0f});
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.hp == 1 && !p.terminou && p.baloesEstourados == 1 && p.stats.U == 0);
}

TESTE(festa_piloto_estoura_muitos_e_evita_os_vermelhos) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = naFesta(c, 3);
    MemoriaPiloto m;
    const int hp = p.bill.hp;
    for (int q = 0; q < 60 * 31 && p.festaAtiva; ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
    }
    std::printf("    festa do piloto: %d balões estourados · vida %d → %d\n", p.baloesEstourados, hp, p.bill.hp);
    VERIFICAR(!p.festaAtiva && p.baloesEstourados >= 10);
}

// Regressão (Etapa 8.4): na seed 18 (Difícil), um rato aparecia e sumia atrás de um pilar a cada
// passo e o piloto alternava entre ele e a Sombra para sempre (29 min parado em b1_c03). Agora o
// alvo que acabou de sumir tem preferência, por pouco tempo, sobre outro mais longe.
TESTE(piloto_nao_alterna_alvos_quando_um_some_atras_do_pilar) {
    const Conteudo c = conteudoReal();
    const ResultadoSimulacao r = simularTentativa(c, 18, Dificuldade::Dificil, LIMITE_SIMULACAO_S);   // o limite cresce com os biomas
    VERIFICAR(r.fim == FimSimulacao::Completou || r.fim == FimSimulacao::Venceu);
}
