// Porta fechada (pedido do Diego, 23/11/2026): quem tenta passar 2 vezes seguidas por uma porta
// cinza recebe um aviso dizendo por quê (derrotar os inimigos da sala; a moldura fica dourada e abre).
// Sala de combate real (b1_c01, 3 ondas), com as ondas adiadas para nada atrapalhar o empurrão.
#include <filesystem>

#include "entidades/jogador.h"
#include "jogo/partida.h"
#include "teste.h"

namespace {
constexpr float PASSO = 1.0f / 60.0f;

Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

int indiceDe(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

// Bill parado na célula de dentro, colado na porta 0 (fechada pelo combate).
EstadoPartida colarNaPorta(const Conteudo& c, Vec2& paraPorta) {
    EstadoPartida p;
    iniciarPartida(p, c, 7, indiceDe(c, "b1_c01"), Dificuldade::Normal);
    p.timerOnda = 999.0f;   // ninguém aparece durante o teste
    const PortaSala& porta = p.sala.portas.at(0);
    paraPorta = vetorDaDirecao(porta.lado);
    const Vec2 dentro = subtrair(centroCelula(porta.celula), escalar(paraPorta, static_cast<float>(TILE)));
    p.bill.pes = dentro;
    return p;
}

void andar(EstadoPartida& p, const Conteudo& c, Vec2 eixo, float segundos) {
    EstadoEntrada e;
    e.eixo = eixo;
    for (float t = 0.0f; t < segundos; t += PASSO) atualizarPartida(p, c, e, PASSO);
}
}  // namespace

TESTE(porta_fechada_avisa_na_segunda_tentativa_seguida) {
    const Conteudo c = salasReais();
    Vec2 paraPorta;
    EstadoPartida p = colarNaPorta(c, paraPorta);
    VERIFICAR(!p.sala.portas[0].aberta);
    andar(p, c, paraPorta, 0.3f);   // 1ª tentativa: ainda sem aviso (pode ter sido sem querer)
    VERIFICAR(p.tempoAvisoPorta <= 0.0f);
    andar(p, c, {0, 0}, 0.3f);
    andar(p, c, paraPorta, 0.3f);   // 2ª tentativa seguida: o aviso
    VERIFICAR(p.tempoAvisoPorta > 0.0f);
    VERIFICAR(p.avisoPorta.find("inimigos") != std::string::npos);
    VERIFICAR(p.avisoPorta.find("dourada") != std::string::npos);
    VERIFICAR(p.avisoPorta.find("onda") != std::string::npos);   // b1_c01 tem 3 ondas: diz em qual está
}

TESTE(porta_fechada_segurar_contra_ela_tambem_conta) {
    const Conteudo c = salasReais();
    Vec2 paraPorta;
    EstadoPartida p = colarNaPorta(c, paraPorta);
    andar(p, c, paraPorta, 1.0f);
    VERIFICAR(p.tempoAvisoPorta <= 0.0f);
    andar(p, c, paraPorta, 0.4f);   // passou de 1,2 s segurando: é a 2ª tentativa
    VERIFICAR(p.tempoAvisoPorta > 0.0f);
}

TESTE(porta_fechada_tentativas_espacadas_nao_avisam) {
    const Conteudo c = salasReais();
    Vec2 paraPorta;
    EstadoPartida p = colarNaPorta(c, paraPorta);
    andar(p, c, paraPorta, 0.3f);
    andar(p, c, {0, 0}, 6.0f);   // muito tempo depois: a contagem zerou
    andar(p, c, paraPorta, 0.3f);
    VERIFICAR(p.tempoAvisoPorta <= 0.0f);
}

TESTE(porta_fechada_andar_para_longe_nao_conta) {
    const Conteudo c = salasReais();
    Vec2 paraPorta;
    EstadoPartida p = colarNaPorta(c, paraPorta);
    for (int i = 0; i < 4; ++i) {
        andar(p, c, escalar(paraPorta, -1.0f), 0.2f);
        andar(p, c, {0, 0}, 0.2f);
    }
    VERIFICAR(p.tempoAvisoPorta <= 0.0f);
}

TESTE(porta_selada_explica_que_e_para_usar_outra) {
    const Conteudo c = salasReais();
    Vec2 paraPorta;
    EstadoPartida p = colarNaPorta(c, paraPorta);
    p.salaConcluida = true;   // combate acabou; esta porta é a selada (a de entrada, como na tentativa)
    p.sala.portas[0].selada = true;
    andar(p, c, paraPorta, 0.3f);
    andar(p, c, {0, 0}, 0.2f);
    andar(p, c, paraPorta, 0.3f);
    VERIFICAR(p.avisoPorta.find("selada") != std::string::npos);
}
