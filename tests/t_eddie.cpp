// Eddie (Etapa 11.5, PRD §12 e §4.4): os 4 Ecos + o lendário Cuidado, e o resgate em que ele
// destranca a porta acorrentada por dentro.
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

int ecoPorNome(const std::string& nome) {
    for (int i = 0; i < totalEcos(); ++i) {
        if (nome == definicaoEco(i).nome) return i;
    }
    return -1;
}

EstadoPartida salaVazia(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 5, indicePorId(c, "b3_eco"), Dificuldade::Normal);
    return p;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}
}  // namespace

TESTE(eddie_ecos_na_tabela) {
    for (const char* nome : {"Inalador", "Remédio", "Gesso", "Pressa", "Cuidado"}) {
        const int i = ecoPorNome(nome);
        VERIFICAR(i >= 0);
        if (i >= 0) VERIFICAR(definicaoEco(i).amigo == Amigo::Eddie);
    }
    VERIFICAR(ecoLendarioDe(Amigo::Eddie) == ecoPorNome("Cuidado") && temEcos(Amigo::Eddie));
    VERIFICAR(descricaoEco(ecoPorNome("Inalador"), Raridade::Raro) == "O dash solta uma névoa: inimigos perto ficam lentos por 1,5 s");
}

TESTE(eddie_inalador_deixa_lentos_os_inimigos_perto_do_fim_do_dash) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    aplicarEco(p, ecoPorNome("Inalador"), Raridade::Epico);
    Inimigo perto = criarInimigo(TipoInimigo::Rato, p.proximoId++, somar(p.bill.pes, Vec2{120.0f, 0.0f}));
    Inimigo longe = criarInimigo(TipoInimigo::Rato, p.proximoId++, somar(p.bill.pes, Vec2{120.0f, 160.0f}));
    perto.surgindo = longe.surgindo = 0.0f;
    perto.atordoado = longe.atordoado = 5.0f;   // parados: o teste mede só a névoa
    p.inimigos.push_back(perto);
    p.inimigos.push_back(longe);
    EstadoEntrada dash;
    dash.eixo = {1.0f, 0.0f};
    dash.pressionou[static_cast<int>(Comando::Dash)] = true;
    atualizarPartida(p, c, dash, DT);
    for (int q = 0; q < 20; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    const Inimigo* a = buscarInimigo(p, perto.id);
    const Inimigo* b = buscarInimigo(p, longe.id);
    VERIFICAR(a && a->lento > 1.5f);    // Épico: 2 s
    VERIFICAR(b && b->lento <= 0.0f);   // longe da névoa
}

TESTE(eddie_remedio_kit_cura_mais_e_pressa_acelera) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    const float antes = p.bill.stats.velocidade;
    aplicarEco(p, ecoPorNome("Pressa"), Raridade::Epico);
    VERIFICAR_PERTO(p.bill.stats.velocidade, antes * 1.12f, 0.5f);
    auto curaDoKit = [&](EstadoPartida& q) {
        q.bill.hp = 1;
        q.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)] = 1;
        usarItem(q.bill, q.inventario, TipoItem::Kit);
        return q.bill.hp - 1;
    };
    EstadoPartida sem = salaVazia(c);
    EstadoPartida com = salaVazia(c);
    aplicarEco(com, ecoPorNome("Remédio"), Raridade::Epico);
    const int base = curaDoKit(sem), mais = curaDoKit(com);
    VERIFICAR(mais == base * 150 / 100 || mais == base * 150 / 100 + 1);   // +50%
}

TESTE(eddie_gesso_um_golpe_por_sala) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = salaVazia(c);
    aplicarEco(p, ecoPorNome("Gesso"), Raridade::Epico);
    p.bill.attr.agilidade = 0;
    recalcularJogador(p.bill);
    auto golpe = [&](std::uint32_t id) {
        p.bill.invulneravel = 0.0f;
        p.hitStop = 0.0f;   // o congelamento do acerto anterior seguraria este golpe
        const int antes = p.bill.hp;
        Golpe g;
        g.id = id;
        g.equipe = Equipe::Hostil;
        g.area = areaCirculo(p.bill.pes, 24.0f);
        g.dano = 20;
        g.rolaAcerto = false;
        g.terreno = true;   // sem esquiva nem crítico: o teste mede só o Gesso
        p.golpes.push_back(g);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        return antes - p.bill.hp;
    };
    const int primeiro = golpe(1), segundo = golpe(2);
    VERIFICAR(primeiro > 0 && segundo > primeiro);   // o 1º perdeu 60%; o 2º não
    VERIFICAR(p.gessoUsado);
}

TESTE(eddie_resgate_destranca_a_porta_e_cuidado_cura_e_da_kit) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 31, Dificuldade::Normal, erro);
    entrarNaSala(p, c, indicePorId(c, "b3_resgate"), nullptr);
    VERIFICAR(tileEm(p.sala, 19, 7).bloqueiaMovimento);   // a porta acorrentada fecha a saída
    p.bill.hp = 5;
    const int kits = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)];
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    apertar(p, c, Comando::Interagir);
    const ObjetoSala* eddie = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) eddie = o.codigo == '1' ? &o : eddie;
    VERIFICAR(eddie != nullptr);
    if (!eddie) return;
    posicionarJogador(p.bill, centroCelula({eddie->celula.col, eddie->celula.lin + 1}));
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Eddie);
    VERIFICAR(!tileEm(p.sala, 19, 7).bloqueiaMovimento);   // destrancou por dentro
    int quadro = -1;
    for (const ObjetoSala& o : p.sala.objetos) quadro = (o.codigo == 'e' && o.celula.col == 19 && o.celula.lin == 7) ? o.quadro : quadro;
    VERIFICAR(quadro == 1);                                // porta aberta
    VERIFICAR(p.bill.hp == p.bill.stats.hpMax);            // Cuidado: cura total
    VERIFICAR(p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Kit)] == kits + 1);
}
