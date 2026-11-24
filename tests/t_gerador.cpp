// Testes do gerador da tentativa e da estrutura dela (SDD §9, PRD §5): plano por bioma, portas
// com opção e seladas, recompensa ao concluir, pendência de nível, porta do Medo, refúgio e descida.
#include <filesystem>
#include <set>

#include "jogo/partida.h"
#include "conteudo_teste.h"
#include "teste.h"

namespace {

Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

PlanoBioma plano(const Conteudo& c, std::uint64_t seed, int pesoKit = 100) {
    Rng r;
    semear(r, seed);
    PlanoBioma pl;
    std::string erro;
    gerarBioma(c.salas, 1, r, pesoKit, pl, erro);
    return pl;
}

const SalaModelo& modelo(const Conteudo& c, const OpcaoPorta& o) { return c.salas[static_cast<std::size_t>(o.sala)]; }

// Conclui a sala atual na marra (como se a última onda tivesse acabado).
void concluirAgora(EstadoPartida& p, const Conteudo& c) {
    p.inimigos.clear();
    p.avisos.clear();
    p.ondaAtual = p.sala.modelo->ondas;
    p.timerOnda = 0.0f;
    atualizarPartida(p, c, EstadoEntrada{}, DT);
}

// Põe Bill em cima da porta com a opção `opcao` e dá um passo: atravessa.
void atravessar(EstadoPartida& p, const Conteudo& c, int opcao) {
    for (const PortaSala& porta : p.sala.portas) {
        if (porta.opcao != opcao || !porta.aberta) continue;
        p.bill.pes = centroCelula(porta.celula);
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        return;
    }
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

}  // namespace

TESTE(gerador_plano_do_bioma_1_segue_a_estrutura_do_prd) {
    const Conteudo c = salasReais();
    int tesouros = 0, altares = 0, festas = 0;
    for (std::uint64_t seed = 1; seed <= 30; ++seed) {
        const PlanoBioma pl = plano(c, seed);
        VERIFICAR(pl.etapas.size() == 8 && pl.avisos.empty());
        if (pl.etapas.size() != 8) return;
        auto tipo = [&](int e, int o) { return modelo(c, pl.etapas[static_cast<std::size_t>(e)].opcoes[static_cast<std::size_t>(o)]).tipo; };
        VERIFICAR(tipo(0, 0) == TipoSala::Entrada);
        VERIFICAR(pl.etapas[1].opcoes.size() == 2 && tipo(1, 0) == TipoSala::Combate && tipo(1, 1) == TipoSala::Combate);
        VERIFICAR(pl.etapas[1].opcoes[0].recompensa != pl.etapas[1].opcoes[1].recompensa);   // sem repetir
        // Especial: Tesouro ou o altar de Eco do Ben (Etapa 8.1); as duas aparecem nas 30 seeds.
        VERIFICAR(tipo(2, 0) == TipoSala::Combate && (tipo(2, 1) == TipoSala::Tesouro || tipo(2, 1) == TipoSala::Eco || tipo(2, 1) == TipoSala::Bonus));
        festas += tipo(2, 1) == TipoSala::Bonus ? 1 : 0;
        tesouros += tipo(2, 1) == TipoSala::Tesouro ? 1 : 0;
        altares += tipo(2, 1) == TipoSala::Eco ? 1 : 0;
        VERIFICAR(tipo(3, 0) == TipoSala::Combate && pl.etapas[3].opcoes[0].recompensa == Recompensa::ChaveDoMedo);
        // Etapa 8.5: mais duas portas de combate antes da área segura (PRD §18: 3 combates + especial).
        VERIFICAR(pl.etapas[4].opcoes.size() == 2 && tipo(4, 0) == TipoSala::Combate && tipo(4, 1) == TipoSala::Combate);
        VERIFICAR(pl.etapas[4].opcoes[0].recompensa != pl.etapas[4].opcoes[1].recompensa);
        VERIFICAR(tipo(5, 0) == TipoSala::Loja && tipo(5, 1) == TipoSala::Refugio);
        VERIFICAR(tipo(6, 0) == TipoSala::Resgate && tipo(7, 0) == TipoSala::Chefe);
        // A Chave do Medo só aparece na etapa 3, antes do resgate (PRD §5.3).
        for (int e = 0; e < 8; ++e) {
            for (const OpcaoPorta& o : pl.etapas[static_cast<std::size_t>(e)].opcoes) {
                VERIFICAR(e == 3 || o.recompensa != Recompensa::ChaveDoMedo);
            }
        }
        // 6 combates sorteados de um banco de 7: nenhum repete.
        std::set<int> combates{pl.etapas[1].opcoes[0].sala, pl.etapas[1].opcoes[1].sala, pl.etapas[2].opcoes[0].sala,
                               pl.etapas[3].opcoes[0].sala, pl.etapas[4].opcoes[0].sala, pl.etapas[4].opcoes[1].sala};
        VERIFICAR(combates.size() == 6);
    }
    VERIFICAR(tesouros > 0 && altares > 0 && festas > 0 && festas < 15);   // a festa é rara (20%)
}

TESTE(gerador_mesma_seed_mesmo_plano_e_seeds_variam) {
    const Conteudo c = salasReais();
    const PlanoBioma a = plano(c, 77), b = plano(c, 77);
    bool iguais = a.etapas.size() == b.etapas.size();
    for (std::size_t e = 0; iguais && e < a.etapas.size(); ++e) {
        for (std::size_t o = 0; o < a.etapas[e].opcoes.size(); ++o) {
            iguais = iguais && a.etapas[e].opcoes[o].sala == b.etapas[e].opcoes[o].sala &&
                     a.etapas[e].opcoes[o].recompensa == b.etapas[e].opcoes[o].recompensa;
        }
    }
    VERIFICAR(iguais);
    std::set<int> primeiras;
    for (std::uint64_t s = 1; s <= 20; ++s) primeiras.insert(plano(c, s).etapas[1].opcoes[0].sala);
    VERIFICAR(primeiras.size() >= 3);   // a ordem das salas muda de seed para seed
}

TESTE(gerador_bioma_sem_entrada_nao_e_jogavel) {
    const Conteudo c = salasReais();
    VERIFICAR(biomaJogavel(c.salas, 1) && biomaJogavel(c.salas, 2));   // Etapa 10.1: os Barrens
    VERIFICAR(biomaJogavel(c.salas, 6) && biomaJogavel(c.salas, 7) && !biomaJogavel(c.salas, 8));   // Etapa 15: até o Covil
    Rng r;
    PlanoBioma pl;
    std::string erro;
    VERIFICAR(!gerarBioma(c.salas, 8, r, 100, pl, erro) && !erro.empty());
}

TESTE(gerador_recompensas_seguem_o_peso_de_kit) {
    const Conteudo c = salasReais();
    int kitsFacil = 0, kitsDificil = 0;
    for (std::uint64_t s = 1; s <= 300; ++s) {
        for (const OpcaoPorta& o : plano(c, s, 150).etapas[1].opcoes) kitsFacil += o.recompensa == Recompensa::Kit ? 1 : 0;
        for (const OpcaoPorta& o : plano(c, s, 70).etapas[1].opcoes) kitsDificil += o.recompensa == Recompensa::Kit ? 1 : 0;
    }
    VERIFICAR(kitsFacil > kitsDificil);
}

TESTE(tentativa_portas_levam_as_opcoes_e_a_entrada_fica_selada) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    std::string erro;
    VERIFICAR(iniciarTentativa(p, c, 5, Dificuldade::Normal, erro));
    VERIFICAR(p.emTentativa && p.etapa == 0 && p.sala.modelo->tipo == TipoSala::Entrada && p.salaConcluida);
    int comOpcao = 0;
    for (const PortaSala& porta : p.sala.portas) comOpcao += porta.opcao >= 0 ? 1 : 0;
    VERIFICAR(comOpcao == 2);
    const OpcaoPorta esperada = p.plano.etapas[1].opcoes[1];
    atravessar(p, c, 1);
    VERIFICAR(p.etapa == 1 && p.sala.modelo == &c.salas[static_cast<std::size_t>(esperada.sala)]);
    VERIFICAR(p.salaPrometida.recompensa == esperada.recompensa);
    // Combate: todas as portas fechadas; quem entrou por uma porta a encontra selada.
    for (const PortaSala& porta : p.sala.portas) VERIFICAR(!porta.aberta);
    int seladas = 0, opcoes = 0;
    for (const PortaSala& porta : p.sala.portas) {
        seladas += porta.selada ? 1 : 0;
        opcoes += porta.opcao >= 0 ? 1 : 0;
    }
    VERIFICAR(opcoes == static_cast<int>(p.plano.etapas[2].opcoes.size()) && seladas >= 1);
}

TESTE(tentativa_concluir_entrega_a_recompensa_e_abre_a_ficha) {
    const Conteudo c = salasReais();
    for (std::uint64_t seed = 1; seed <= 40; ++seed) {
        EstadoPartida p;
        std::string erro;
        iniciarTentativa(p, c, seed, Dificuldade::Normal, erro);
        atravessar(p, c, 0);
        const Recompensa r = p.salaPrometida.recompensa;
        if (r == Recompensa::PontoAtributo) continue;
        p.bill.progresso.pontosPendentes = 1;
        concluirAgora(p, c);
        VERIFICAR(p.salaConcluida && p.stats.C == 1);
        bool achou = false;
        for (const ItemNoChao& it : p.itensChao) achou = achou || it.tipo == definicaoRecompensa(r).item;
        VERIFICAR(achou);
        VERIFICAR(p.pendencia == Pendencia::SubirNivel);
        // As portas abrem, menos as seladas.
        for (const PortaSala& porta : p.sala.portas) VERIFICAR(porta.aberta == !porta.selada);
        return;   // um caso basta
    }
}

TESTE(tentativa_pendencia_congela_o_mundo_e_resolve_por_comandos) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 3, Dificuldade::Normal, erro);
    p.bill.progresso.pontosPendentes = 2;
    apertar(p, c, Comando::Ficha);   // C abre a ficha a qualquer momento
    VERIFICAR(p.pendencia == Pendencia::SubirNivel);
    const float tempo = p.stats.tempo;
    const Vec2 pes = p.bill.pes;
    EstadoEntrada andar;
    andar.eixo = {1.0f, 0.0f};
    for (int q = 0; q < 30; ++q) atualizarPartida(p, c, andar, DT);
    VERIFICAR(p.stats.tempo == tempo && p.bill.pes.x == pes.x);   // nada anda, o relógio para
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Baixo);   // cursor em Agilidade
    VERIFICAR(p.cursor == 2);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.bill.attr.agilidade == 2 && p.bill.progresso.pontosPendentes == 1 && p.pendencia == Pendencia::SubirNivel);
    apertar(p, c, Comando::Cima);
    apertar(p, c, Comando::Cima);
    apertar(p, c, Comando::Cima);    // volta dando a volta: Coragem
    apertar(p, c, Comando::Atacar);  // J também confirma
    VERIFICAR(p.bill.attr.coragem == 2 && p.pendencia == Pendencia::Nenhuma);
    VERIFICAR(p.bill.stats.hpMax == 40 + 6 * 2);
    // Sem pontos, C não abre nada; com pontos, Voltar fecha e os pontos ficam guardados.
    apertar(p, c, Comando::Ficha);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
    p.bill.progresso.pontosPendentes = 1;
    apertar(p, c, Comando::Ficha);
    apertar(p, c, Comando::Voltar);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && p.bill.progresso.pontosPendentes == 1);
}

TESTE(tentativa_porta_do_medo_abre_so_com_a_chave) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 9, Dificuldade::Normal, erro);
    // Vai direto para a etapa do resgate (atalho de teste).
    p.etapa = 5;
    p.salaPrometida = p.plano.etapas[6].opcoes[0];
    entrarNaSala(p, c, p.salaPrometida.sala, nullptr);
    VERIFICAR(p.sala.modelo->tipo == TipoSala::Resgate);
    Celula t{-1, -1};
    for (int l = 0; l < p.sala.lins; ++l)
        for (int col = 0; col < p.sala.cols; ++col)
            if (tileEm(p.sala, col, l).tipo == TipoTile::PortaTrancada) t = {col, l};
    VERIFICAR(t.col >= 0);
    p.bill.pes = centroCelula({t.col - 1, t.lin});
    apertar(p, c, Comando::Interagir);
    VERIFICAR(tileEm(p.sala, t.col, t.lin).tipo == TipoTile::PortaTrancada);   // sem chave
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    apertar(p, c, Comando::Interagir);
    VERIFICAR(tileEm(p.sala, t.col, t.lin).tipo == TipoTile::Chao);
    VERIFICAR(!tileEm(p.sala, t.col, t.lin).bloqueiaMovimento && p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] == 0);
}

TESTE(tentativa_refugio_cura_e_chefe_desce_para_o_fim_do_conteudo) {
    Conteudo c = salasReais();
    limitarAoBioma(c.salas, 1);   // depois do chefe do bioma 1, não há bioma 2: fim do conteúdo
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 11, Dificuldade::Normal, erro);
    p.bill.hp = 10;
    p.etapa = 4;
    atravessar(p, c, 1);   // a etapa 5 tem [Loja | Refúgio]: a opção 1 é o Refúgio
    VERIFICAR(p.sala.modelo->tipo == TipoSala::Refugio && p.bill.hp == 10 + p.bill.stats.hpMax * 30 / 100);
    // Chefe: depois dele a porta é a descida. Sem o bioma 2 no conteúdo → fim do conteúdo.
    p.etapa = 6;
    p.salaPrometida = p.plano.etapas[7].opcoes[0];
    entrarNaSala(p, c, p.salaPrometida.sala, nullptr);
    p.etapa = 7;
    concluirAgora(p, c);
    p.pendencia = Pendencia::Nenhuma;
    const int vidas = p.stats.U;
    atravessar(p, c, 0);
    VERIFICAR(p.terminou && p.fimDoConteudo && !p.vitoria && p.stats.U == vidas);
}
