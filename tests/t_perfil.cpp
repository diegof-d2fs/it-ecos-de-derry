// Perfil persistente e Mural (PRD §13, SDD §3.6 e §16): ida e volta pelo arquivo, leitura
// tolerante, compra de talentos, talentos aplicados na tentativa e Lembranças ganhas.
#include <filesystem>

#include "jogo/partida.h"
#include "jogo/perfil.h"
#include "teste.h"

namespace {
Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}
}  // namespace

TESTE(perfil_ida_e_volta_pelo_texto) {
    Perfil a;
    a.lembrancas = 137;
    a.talentos[static_cast<std::size_t>(Talento::CoragemHerdada)] = 3;
    a.tentativas = 9;
    a.mortes = 7;
    a.vitorias = 1;
    a.melhorPontuacao = 56100;
    a.ultimaDificuldade = Dificuldade::Dificil;
    a.prologoVisto = true;
    a.dicasVistas = 0x5;
    a.ultimoAssassino = 2;
    a.falasVistas = {"ben_hub_1", "mike_prologo_1"};
    a.opcoes.dicas = false;
    a.opcoes.volumeMusica = 30;
    Perfil b;
    VERIFICAR(lerPerfil(escreverPerfil(a), b) == 0);
    VERIFICAR(b.lembrancas == 137 && b.talentos[static_cast<std::size_t>(Talento::CoragemHerdada)] == 3);
    VERIFICAR(b.tentativas == 9 && b.mortes == 7 && b.vitorias == 1 && b.melhorPontuacao == 56100);
    VERIFICAR(b.ultimaDificuldade == Dificuldade::Dificil && b.prologoVisto && b.dicasVistas == 0x5 && b.ultimoAssassino == 2);
    VERIFICAR(b.falasVistas == a.falasVistas && !b.opcoes.dicas && b.opcoes.volumeMusica == 30);
}

TESTE(perfil_leitura_tolerante) {
    Perfil p;
    const int ruins = lerPerfil(
        "versao=1\nlembrancas=abc\nchave_do_futuro=7\ntalento.coragem=99\ntalento.inexistente=1\n"
        "opcoes.volume_musica=150\nsem_igual\n# comentário\n\nmortes=4\n",
        p);
    VERIFICAR(ruins == 6);                       // cada valor ruim é contado, nenhum derruba
    VERIFICAR(p.lembrancas == 0 && p.mortes == 4);
    VERIFICAR(p.talentos[static_cast<std::size_t>(Talento::CoragemHerdada)] == 0);   // 99 > máximo: ignorado
    VERIFICAR(p.opcoes.volumeMusica == 80);
}

TESTE(perfil_arquivo_ausente_e_gravacao_atomica) {
    const std::filesystem::path pasta = std::filesystem::temp_directory_path() / "ecos_teste_perfil";
    std::filesystem::remove_all(pasta);
    VERIFICAR(!perfilExiste(pasta / "perfil.txt"));
    Perfil p = carregarPerfil(pasta / "perfil.txt");
    VERIFICAR(p.lembrancas == 0 && p.tentativas == 0);
    p.lembrancas = 42;
    VERIFICAR(gravarPerfil(pasta / "perfil.txt", p));
    VERIFICAR(perfilExiste(pasta / "perfil.txt") && carregarPerfil(pasta / "perfil.txt").lembrancas == 42);
    VERIFICAR(!std::filesystem::exists(pasta / "perfil.txt.tmp"));
    std::filesystem::remove_all(pasta);
}

TESTE(mural_compra_respeita_custo_niveis_e_em_breve) {
    Perfil p;
    p.lembrancas = 35;
    VERIFICAR(custoProximoNivel(p, Talento::CoragemHerdada) == 10);
    VERIFICAR(comprarTalento(p, Talento::CoragemHerdada) && p.lembrancas == 25);
    VERIFICAR(custoProximoNivel(p, Talento::CoragemHerdada) == 20);
    VERIFICAR(comprarTalento(p, Talento::CoragemHerdada) && p.lembrancas == 5);
    VERIFICAR(!comprarTalento(p, Talento::CoragemHerdada) && p.lembrancas == 5);   // sem saldo
    p.lembrancas = 1000;
    VERIFICAR(!comprarTalento(p, Talento::FolegoExtra) && custoProximoNivel(p, Talento::FolegoExtra) == -1);   // em breve
    VERIFICAR(comprarTalento(p, Talento::MapaBen) == false);
    VERIFICAR(comprarTalento(p, Talento::PactoDeSangue) && comprarTalento(p, Talento::PactoDeSangue));
    VERIFICAR(!comprarTalento(p, Talento::PactoDeSangue) && custoProximoNivel(p, Talento::PactoDeSangue) == -1);   // máximo
    // O Mural inteiro (só o que está à venda) cabe em ~8–10 tentativas (PRD §13.1).
    int total = 0;
    for (int t = 0; t < TOTAL_TALENTOS; ++t) {
        const DefinicaoTalento& d = definicaoTalento(static_cast<Talento>(t));
        for (int n = 0; d.disponivel && n < d.niveis; ++n) total += d.custos[n];
    }
    VERIFICAR(total > 400 && total < 1200);
}

TESTE(talentos_valem_na_tentativa) {
    const Conteudo c = salasReais();
    Perfil perfil;
    perfil.talentos[static_cast<std::size_t>(Talento::PactoDeSangue)] = 2;
    perfil.talentos[static_cast<std::size_t>(Talento::CoragemHerdada)] = 3;
    perfil.talentos[static_cast<std::size_t>(Talento::LanternaReforcada)] = 1;
    perfil.talentos[static_cast<std::size_t>(Talento::BolsoFundo)] = 2;
    perfil.talentos[static_cast<std::size_t>(Talento::TreinoBarrens)] = 1;
    perfil.talentos[static_cast<std::size_t>(Talento::MaoLeve)] = 1;
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 3, Dificuldade::Normal, erro);
    const int hpBase = p.bill.stats.hpMax;
    aplicarTalentos(perfil, p);
    VERIFICAR(p.bill.pactos == 3 && p.bill.stats.hpMax == hpBase + 15 && p.bill.hp == p.bill.stats.hpMax);
    VERIFICAR(p.bill.cargasLanterna == CARGAS_LANTERNA_INICIAIS + 1 && espacoPara(p.bill, p.inventario, TipoItem::Bateria) == 2);
    VERIFICAR(quantidadeDe(p.bill, p.inventario, TipoItem::Kit) == 2 && p.bill.progresso.pontosPendentes == 1);
    soltarItem(p, TipoItem::Moeda, 10, p.bill.pes, p.bill.pes);
    VERIFICAR(p.itensChao.back().quantidade == 12);   // Mão leve: +20%
}

TESTE(lembrancas_e_fim_de_tentativa_no_perfil) {
    EstadoPartida p;
    p.stats.C = 5;
    p.stats.B = 1;
    p.stats.R = 1;
    VERIFICAR(lembrancasDaTentativa(p) == 15 + 10 + 5);
    p.terminou = true;   // morreu
    p.ultimoAgressor = 1;
    Perfil perfil;
    VERIFICAR(registrarFimDeTentativa(perfil, p, false) == 30);
    VERIFICAR(perfil.lembrancas == 30 && perfil.tentativas == 1 && perfil.mortes == 1 && perfil.ultimoAssassino == 1);
    // O computador jogando não mexe no perfil de quem joga.
    VERIFICAR(registrarFimDeTentativa(perfil, p, true) == 0 && perfil.lembrancas == 30 && perfil.tentativas == 1);
    // Fim do conteúdo não conta como morte; abandono também não.
    EstadoPartida fim;
    fim.terminou = fim.fimDoConteudo = true;
    registrarFimDeTentativa(perfil, fim, false);
    EstadoPartida desistiu;
    desistiu.terminou = desistiu.abandonou = true;
    registrarFimDeTentativa(perfil, desistiu, false);
    VERIFICAR(perfil.mortes == 1 && perfil.tentativas == 3);
}
