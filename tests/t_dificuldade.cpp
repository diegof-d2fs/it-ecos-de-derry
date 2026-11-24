// Testes da dificuldade (SDD §24.2; critérios M01 e M06): a dificuldade muda probabilidades e
// orçamento, os valores vêm do arquivo de dados e seed + dificuldade é determinístico.
#include <filesystem>

#include "core/arquivo.h"
#include "jogo/partida.h"
#include "teste.h"

namespace {

const char* const ARENA_GRANDE =
    "id=arena_grande\nbioma=1\ntipo=combate\nondas=1\norcamento=12\nmapa:\n"
    "################\n"
    "#s............s#\n"
    "#..............#\n"
    "#..............#\n"
    "#......E.......#\n"
    "#..............#\n"
    "#s............s#\n"
    "################\n";

Conteudo conteudoArena() {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    lerSala(ARENA_GRANDE, m, erro);
    c.salas.push_back(m);
    return c;
}

// Roda parado até a primeira onda nascer.
void esperarPrimeiraOnda(EstadoPartida& p, const Conteudo& c) {
    const EstadoEntrada nada;
    for (int q = 0; q < 200 && p.ondaAtual == 0; ++q) atualizarPartida(p, c, nada, DT);
}

int custoDaOnda(const EstadoPartida& p) {
    int total = 0;
    for (const Inimigo& i : p.inimigos) total += parametrosDe(i.tipo).custoOnda;
    return total;
}

}  // namespace

TESTE(dificuldade_arquivo_de_dados_igual_a_tabela_do_sdd) {
    std::string texto, erro;
    VERIFICAR(lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / "data/balanceamento.txt", texto));
    // Começa com −1 em tudo: se o arquivo esquecer um parâmetro, o −1 aparece na comparação.
    TabelaDificuldade lida;
    for (auto& n : lida.niveis) {
        n.orcamentoOnda = n.pesoFortes = n.chanceElite = n.chanceDrop = n.pesoKit = n.chanceRoubo = -1;
        n.armadilhasOpcionais = n.criticoInimigo = n.danoRecebido = n.chanceSecreta = n.pactosExtras = -1;
        n.multiplicadorPontos = -1;
    }
    VERIFICAR(lerBalanceamento(texto, lida, erro));
    const TabelaDificuldade padrao = tabelaDificuldadePadrao();
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        const ParametrosDificuldade& a = lida.niveis[static_cast<std::size_t>(d)];
        const ParametrosDificuldade& b = padrao.niveis[static_cast<std::size_t>(d)];
        VERIFICAR(a.orcamentoOnda == b.orcamentoOnda && a.pesoFortes == b.pesoFortes && a.chanceElite == b.chanceElite);
        VERIFICAR(a.chanceDrop == b.chanceDrop && a.pesoKit == b.pesoKit && a.chanceRoubo == b.chanceRoubo);
        VERIFICAR(a.armadilhasOpcionais == b.armadilhasOpcionais && a.criticoInimigo == b.criticoInimigo);
        VERIFICAR(a.danoRecebido == b.danoRecebido && a.chanceSecreta == b.chanceSecreta);
        VERIFICAR(a.pactosExtras == b.pactosExtras && a.multiplicadorPontos == b.multiplicadorPontos);
    }
    // A tabela do SDD §24.2, conferida número a número nos extremos.
    VERIFICAR(padrao.niveis[0].chanceDrop == 22 && padrao.niveis[2].chanceDrop == 10);
    VERIFICAR(padrao.niveis[0].multiplicadorPontos == 75 && padrao.niveis[2].multiplicadorPontos == 150);
    VERIFICAR(padrao.niveis[2].danoRecebido == 125 && padrao.niveis[0].pactosExtras == 1);
}

TESTE(dificuldade_erros_do_arquivo_apontam_a_linha) {
    TabelaDificuldade t = tabelaDificuldadePadrao();
    std::string erro;
    VERIFICAR(!lerBalanceamento("# ok\nchance_drop_pct = 1 2 3\nnao_existe = 1 2 3\n", t, erro));
    VERIFICAR(erro.find("linha 3") != std::string::npos && erro.find("nao_existe") != std::string::npos);
    VERIFICAR(!lerBalanceamento("chance_drop_pct = 1 2\n", t, erro));
    VERIFICAR(erro.find("3 números") != std::string::npos);
    VERIFICAR(!lerBalanceamento("chance_drop_pct = 1 2 3 4\n", t, erro));
    VERIFICAR(!lerBalanceamento("chance_drop_pct 1 2 3\n", t, erro));
    // Uma chave lida muda só ela; comentário no fim da linha é aceito.
    TabelaDificuldade u = tabelaDificuldadePadrao();
    VERIFICAR(lerBalanceamento("chance_drop_pct = 50 40 30  # teste\n", u, erro));
    VERIFICAR(u.niveis[1].chanceDrop == 40 && u.niveis[1].danoRecebido == 100);
}

TESTE(dificuldade_drop_dificil_rende_menos_que_facil) {
    const TabelaDificuldade t = tabelaDificuldadePadrao();
    Rng a, b;
    semear(a, 99);
    semear(b, 99);
    int facil = 0, dificil = 0;
    for (int i = 0; i < 1000; ++i) {
        facil += sortearPorcentagem(a, parametros(t, Dificuldade::Facil).chanceDrop) ? 1 : 0;
        dificil += sortearPorcentagem(b, parametros(t, Dificuldade::Dificil).chanceDrop) ? 1 : 0;
    }
    VERIFICAR(dificil < facil);
    VERIFICAR(facil > 180 && facil < 260);     // ≈ 22%
    VERIFICAR(dificil > 70 && dificil < 130);  // ≈ 10%
}

TESTE(dificuldade_porcentagens_exatas_nos_extremos) {
    Rng r, copia;
    semear(r, 5);
    copia = r;
    for (int i = 0; i < 100; ++i) VERIFICAR(!sortearPorcentagem(r, 0) && sortearPorcentagem(r, 100));
    // 0% e 100% não gastam número do Rng (o Fácil sem crítico não desloca a partida).
    VERIFICAR(proximo(r) == proximo(copia));
    VERIFICAR(aplicarPorcentagem(8, 75) == 6 && aplicarPorcentagem(8, 130) == 10 && aplicarPorcentagem(6, 75) == 5);
    VERIFICAR(aplicarPorcentagem(10, 125) == 13 && aplicarPorcentagem(10, 80) == 8);
}

TESTE(dificuldade_orcamento_da_onda_escala) {
    const TabelaDificuldade t = tabelaDificuldadePadrao();
    VERIFICAR(orcamentoDaOnda(parametros(t, Dificuldade::Facil), 8) == 6);
    VERIFICAR(orcamentoDaOnda(parametros(t, Dificuldade::Normal), 8) == 8);
    VERIFICAR(orcamentoDaOnda(parametros(t, Dificuldade::Dificil), 8) == 10);
    VERIFICAR(orcamentoDaOnda(parametros(t, Dificuldade::Facil), 1) == 1);   // nunca zera
    // Na partida: a onda gasta no máximo o orçamento escalado, e o Difícil põe mais inimigos.
    const Conteudo c = conteudoArena();
    int totalFacil = 0, totalDificil = 0;
    for (std::uint64_t seed = 1; seed <= 20; ++seed) {
        EstadoPartida f, d;
        iniciarPartida(f, c, seed, 0, Dificuldade::Facil);
        iniciarPartida(d, c, seed, 0, Dificuldade::Dificil);
        esperarPrimeiraOnda(f, c);
        esperarPrimeiraOnda(d, c);
        VERIFICAR(custoDaOnda(f) <= 9 && custoDaOnda(d) <= 16);   // 12 × 0,75 e 12 × 1,3
        totalFacil += custoDaOnda(f);
        totalDificil += custoDaOnda(d);
    }
    VERIFICAR(totalDificil > totalFacil);
}

TESTE(dificuldade_peso_dos_fortes_muda_o_sorteio) {
    const Conteudo c = conteudoArena();
    int sombrasFacil = 0, sombrasDificil = 0, totalFacil = 0, totalDificil = 0;
    for (std::uint64_t seed = 1; seed <= 60; ++seed) {
        EstadoPartida f, d;
        iniciarPartida(f, c, seed, 0, Dificuldade::Facil);
        iniciarPartida(d, c, seed, 0, Dificuldade::Dificil);
        esperarPrimeiraOnda(f, c);
        esperarPrimeiraOnda(d, c);
        for (const Inimigo& i : f.inimigos) { ++totalFacil; sombrasFacil += i.tipo == TipoInimigo::Sombra ? 1 : 0; }
        for (const Inimigo& i : d.inimigos) { ++totalDificil; sombrasDificil += i.tipo == TipoInimigo::Sombra ? 1 : 0; }
    }
    // Proporção de Sombras (o inimigo forte do bioma 1) maior no Difícil.
    VERIFICAR(sombrasDificil * totalFacil > sombrasFacil * totalDificil);
}

TESTE(dificuldade_dano_recebido_e_pactos) {
    const Conteudo c = conteudoArena();
    int perdido[TOTAL_DIFICULDADES] = {};
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        EstadoPartida p;
        iniciarPartida(p, c, 3, 0, static_cast<Dificuldade>(d));
        if (d == 0) VERIFICAR(p.bill.pactos == 2);   // Fácil: +1 Pacto de Sangue
        else VERIFICAR(p.bill.pactos == 1);
        p.ajustes.criticoInimigo = 0;   // isola o multiplicador (o crítico é sorteado)
        p.bill.stats.esquiva = 0;
        p.bill.stats.armadura = 0;
        Golpe g;
        g.equipe = Equipe::Hostil;
        g.origem = 999;
        g.area = areaCirculo(p.bill.pes, 40.0f);
        g.dano = 20;
        g.rolaAcerto = false;
        p.golpes.push_back(g);
        const int antes = p.bill.hp;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        perdido[d] = antes - p.bill.hp;
    }
    VERIFICAR(perdido[0] == 16 && perdido[1] == 20 && perdido[2] == 25);   // ×0,8 · ×1 · ×1,25
}

TESTE(dificuldade_seed_e_dificuldade_reproduzem_a_partida) {
    const Conteudo c = conteudoArena();
    EstadoPartida a, b;
    iniciarPartida(a, c, 2026, 0, Dificuldade::Dificil);
    iniciarPartida(b, c, 2026, 0, Dificuldade::Dificil);
    EstadoEntrada e;
    for (int q = 0; q < 600; ++q) {
        e.eixo = (q / 60) % 2 ? Vec2{1.0f, 0.0f} : Vec2{-1.0f, 0.0f};
        atualizarPartida(a, c, e, DT);
        atualizarPartida(b, c, e, DT);
    }
    VERIFICAR(a.bill.hp == b.bill.hp && a.inimigos.size() == b.inimigos.size() && a.stats.M == b.stats.M);
    VERIFICAR(pontuacaoAtual(a) == pontuacaoAtual(b));
}

// Pedido do Diego (23/11): no Fácil, os inimigos comuns 10% mais fracos (vida e dano) e 5% mais lentos, os
// chefes 12% mais fracos. As viradas de fase usam a vida cheia DO chefe (hpMax), não a da tabela.
TESTE(dificuldade_facil_enfraquece_comuns_e_chefes) {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    lerBalanceamento([] { std::string t; lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / "data/balanceamento.txt", t); return t; }(),
                     c.dificuldades, erro);
    auto indice = [&](const char* id) {
        for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
        return -1;
    };
    auto primeiro = [&](const char* sala, Dificuldade d) {
        EstadoPartida p;
        iniciarPartida(p, c, 3, indice(sala), d);
        for (int q = 0; q < 600 && p.inimigos.empty(); ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
        return p.inimigos.empty() ? Inimigo{} : p.inimigos.front();
    };
    const Inimigo chefeFacil = primeiro("b1_chefe", Dificuldade::Facil), chefeNormal = primeiro("b1_chefe", Dificuldade::Normal);
    VERIFICAR(chefeNormal.hpMax == parametrosDe(TipoInimigo::Mumia).hp);
    VERIFICAR(chefeFacil.hpMax == parametrosDe(TipoInimigo::Mumia).hp * 88 / 100 && chefeFacil.forcaPct == 88);
    const Inimigo comumFacil = primeiro("b1_c01", Dificuldade::Facil), comumNormal = primeiro("b1_c01", Dificuldade::Normal);
    VERIFICAR(comumFacil.velocidadePct == 95 && comumNormal.velocidadePct == 100);
    VERIFICAR(comumFacil.forcaPct == 90 && comumNormal.forcaPct == 100);   // bioma 1: escala 100%
}
