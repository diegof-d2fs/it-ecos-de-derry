// Cartão de memória (pedido do Diego em 23/11/2026): salvar o ponto de entrada da sala em 15 blocos, carregar e
// continuar exatamente igual (os mesmos sorteios), gravar/listar/apagar, e a opção "Salvar jogo" da pausa.
#include <filesystem>

#include "jogo/clube.h"
#include "jogo/partida.h"
#include "jogo/piloto.h"
#include "jogo/salvamento.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

// Uma tentativa jogada pelo piloto até a etapa `etapa` (algumas salas, itens, XP, talvez um Eco).
EstadoPartida avancar(const Conteudo& c, int etapa) {
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 4242, Dificuldade::Normal, erro);
    MemoriaPiloto m;
    for (int q = 0; q < 60 * 900 && p.etapa < etapa && !p.terminou; ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
    }
    return p;
}

void jogar(EstadoPartida& p, const Conteudo& c, int quadros) {
    MemoriaPiloto m;
    for (int q = 0; q < quadros && !p.terminou; ++q) {
        atualizarPartida(p, c, pilotar(p, m, DT), DT);
        p.sons.clear();
    }
}
}  // namespace

TESTE(salvamento_ponto_vai_e_volta_pelo_arquivo) {
    const Conteudo c = conteudoReal();
    const EstadoPartida p = avancar(c, 3);
    VERIFICAR(p.etapa >= 3 && p.pontoDaSala.valido);
    const PontoSalvamento& a = p.pontoDaSala;
    PontoSalvamento b;
    ResumoBloco r;
    std::string erro;
    VERIFICAR(lerPonto(escreverPonto(a, c, "24/11/2026 10:00"), c, b, r, erro));
    if (!erro.empty()) std::printf("    %s\n", erro.c_str());
    VERIFICAR(b.seed == a.seed && b.dificuldade == a.dificuldade && b.indiceSala == a.indiceSala && b.etapa == a.etapa);
    for (int i = 0; i < 4; ++i) VERIFICAR(b.rng.s[i] == a.rng.s[i] && b.rngGerador.s[i] == a.rngGerador.s[i]);
    VERIFICAR(b.plano.etapas.size() == a.plano.etapas.size() && b.plano.bioma == a.plano.bioma);
    VERIFICAR(b.hp == a.hp && b.progresso.nivel == a.progresso.nivel && b.progresso.xp == a.progresso.xp);
    VERIFICAR(b.inventario.quantidades == a.inventario.quantidades && b.ecos.size() == a.ecos.size());
    VERIFICAR(b.stats.K == a.stats.K && b.stats.C == a.stats.C && b.pactos == a.pactos);
    VERIFICAR(r.ocupado && r.bioma == a.plano.bioma && r.data == "24/11/2026 10:00" && !r.nomeSala.empty());
}

// Carregar o mesmo bloco duas vezes dá a MESMA continuação: o jogo salvo não é "parecido", é aquele.
TESTE(salvamento_carregar_continua_exatamente_igual) {
    const Conteudo c = conteudoReal();
    const EstadoPartida original = avancar(c, 3);
    const PontoSalvamento ponto = original.pontoDaSala;
    EstadoPartida x, y;
    std::string erro;
    VERIFICAR(restaurarPonto(x, c, ponto, erro) && restaurarPonto(y, c, ponto, erro));
    VERIFICAR(x.indiceSala == ponto.indiceSala && x.etapa == ponto.etapa && x.bill.hp == ponto.hp);
    VERIFICAR(x.inventario.quantidades == ponto.inventario.quantidades && x.bill.progresso.nivel == ponto.progresso.nivel);
    VERIFICAR(x.emTentativa && !x.terminou);
    jogar(x, c, 60 * 40);
    jogar(y, c, 60 * 40);
    VERIFICAR(x.indiceSala == y.indiceSala && x.bill.hp == y.bill.hp && x.stats.K == y.stats.K);
    VERIFICAR(std::fabs(x.bill.pes.x - y.bill.pes.x) < 1e-3f && std::fabs(x.bill.pes.y - y.bill.pes.y) < 1e-3f);
}

TESTE(salvamento_blocos_gravar_listar_apagar) {
    const Conteudo c = conteudoReal();
    const EstadoPartida p = avancar(c, 2);
    const std::filesystem::path pasta = std::filesystem::temp_directory_path() / "ecos_teste_cartao";
    std::error_code ec;
    std::filesystem::remove_all(pasta, ec);
    for (const ResumoBloco& b : listarBlocos(pasta, c)) VERIFICAR(!b.ocupado);   // cartão novo: 15 livres
    VERIFICAR(gravarBloco(pasta, 3, p.pontoDaSala, c, "24/11/2026 11:11"));
    VERIFICAR(gravarBloco(pasta, 15, p.pontoDaSala, c, "24/11/2026 11:12"));
    VERIFICAR(!gravarBloco(pasta, 16, p.pontoDaSala, c, "x") && !gravarBloco(pasta, 0, p.pontoDaSala, c, "x"));
    auto blocos = listarBlocos(pasta, c);
    VERIFICAR(blocos[2].ocupado && blocos[14].ocupado && !blocos[0].ocupado);
    VERIFICAR(blocos[14].data == "24/11/2026 11:12");
    PontoSalvamento lido;
    std::string erro;
    VERIFICAR(carregarBloco(pasta, 3, c, lido, erro) && lido.indiceSala == p.pontoDaSala.indiceSala);
    VERIFICAR(!carregarBloco(pasta, 4, c, lido, erro));   // livre
    VERIFICAR(apagarBloco(pasta, 3));
    blocos = listarBlocos(pasta, c);
    VERIFICAR(!blocos[2].ocupado && blocos[14].ocupado);
    std::filesystem::remove_all(pasta, ec);
}

TESTE(salvamento_pausa_pede_o_cartao_so_na_tentativa) {
    const Conteudo c = conteudoReal();
    EstadoPartida p = avancar(c, 1);
    p.pendencia = Pendencia::Pausa;
    p.cursor = static_cast<int>(OpcaoPausa::Salvar);
    EstadoEntrada e;
    e.pressionou[static_cast<int>(Comando::Confirmar)] = true;
    atualizarPartida(p, c, e, DT);
    VERIFICAR(p.pedidoSalvar);
    VERIFICAR(std::string(nomeOpcaoPausa(OpcaoPausa::Salvar)) == "Salvar jogo");
    // No Clube não há tentativa: um aviso, nada de cartão.
    EstadoPartida clube;
    if (!iniciarClube(clube, c, 7, Dificuldade::Normal)) return;
    clube.pendencia = Pendencia::Pausa;
    clube.cursor = static_cast<int>(OpcaoPausa::Salvar);
    atualizarPartida(clube, c, e, DT);
    VERIFICAR(!clube.pedidoSalvar && !clube.avisoMenu.empty());
}

// Regressão (24/11, captura): salvar na 1ª sala e carregar perdia o Mike companheiro (o ponto era tirado antes de
// ele entrar) e os talentos do Mural.
TESTE(salvamento_primeira_sala_guarda_o_companheiro) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    VERIFICAR(iniciarTentativa(p, c, 99, Dificuldade::Normal, erro));
    VERIFICAR(p.pontoDaSala.valido && p.pontoDaSala.companheiroAtivo && p.pontoDaSala.companheiro == Amigo::Mike);
    EstadoPartida q;
    VERIFICAR(restaurarPonto(q, c, p.pontoDaSala, erro));
    VERIFICAR(q.companheiro.ativo && q.companheiro.quem == Amigo::Mike);
}
