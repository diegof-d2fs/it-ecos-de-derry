// Testes do mapeamento do joystick (core/controle): eixos, zona morta e comandos.
#include "core/controle.h"
#include "teste.h"

namespace {

LeituraControle comBotao(BotaoControle b) {
    LeituraControle l;
    l.botoes[static_cast<int>(b)] = true;
    return l;
}

}  // namespace

TESTE(controle_normaliza_eixos_e_gatilhos) {
    VERIFICAR_PERTO(normalizarEixo(0), 0.0f, 1e-6f);
    VERIFICAR_PERTO(normalizarEixo(32767), 1.0f, 1e-6f);
    VERIFICAR_PERTO(normalizarEixo(-32768), -1.0f, 1e-6f);   // o extremo negativo não passa de -1
    VERIFICAR_PERTO(normalizarGatilho(255, 255), 1.0f, 1e-6f);
    VERIFICAR_PERTO(normalizarGatilho(0, 32767), 0.0f, 1e-6f);
    VERIFICAR_PERTO(normalizarGatilho(10, 0), 0.0f, 1e-6f);
}

TESTE(controle_zona_morta_radial_com_reescala) {
    // Dentro da zona: zero (o "drift" de um analógico gasto não move o Bill).
    Vec2 v = aplicarZonaMorta({0.2f, 0.1f}, 0.25f);
    VERIFICAR(v.x == 0.0f && v.y == 0.0f);
    // Na borda da zona começa do zero; no fim do curso chega a 1, mantendo a direção.
    v = aplicarZonaMorta({0.25f + 0.375f, 0.0f}, 0.25f);
    VERIFICAR_PERTO(v.x, 0.5f, 1e-5f);
    v = aplicarZonaMorta({0.0f, -1.0f}, 0.25f);
    VERIFICAR_PERTO(v.y, -1.0f, 1e-5f);
    // Diagonal real passa de 1: limita a 1 sem mudar o ângulo.
    v = aplicarZonaMorta({1.0f, 1.0f}, 0.25f);
    VERIFICAR_PERTO(comprimento(v), 1.0f, 1e-5f);
    VERIFICAR_PERTO(v.x, v.y, 1e-6f);
    // Radial, não quadrada: um pequeno desvio no outro eixo NÃO é cortado (sem travar no eixo).
    v = aplicarZonaMorta({0.9f, 0.2f}, 0.25f);
    VERIFICAR(v.y > 0.0f);
}

TESTE(controle_botoes_por_posicao_viram_comandos) {
    EstadoEntrada e;
    aplicarControle(comBotao(BotaoControle::Sul), e);
    VERIFICAR(segurando(e, Comando::Dash) && segurando(e, Comando::Confirmar));
    VERIFICAR(e.ultimoUsado == Dispositivo::Gamepad);

    EstadoEntrada e2;
    aplicarControle(comBotao(BotaoControle::Leste), e2);
    VERIFICAR(segurando(e2, Comando::Clarao) && segurando(e2, Comando::Voltar));
    VERIFICAR(!segurando(e2, Comando::Dash));

    EstadoEntrada e3;
    aplicarControle(comBotao(BotaoControle::Oeste), e3);
    VERIFICAR(segurando(e3, Comando::Atacar));
    EstadoEntrada e4;
    aplicarControle(comBotao(BotaoControle::Iniciar), e4);
    VERIFICAR(segurando(e4, Comando::Pausa));
    EstadoEntrada e5;
    aplicarControle(comBotao(BotaoControle::OmbroDir), e5);
    VERIFICAR(segurando(e5, Comando::Interagir) && segurando(e5, Comando::AbaDireita));
}

TESTE(controle_gatilho_esquerdo_mapa_e_ficha) {
    LeituraControle l;
    l.gatilhoEsq = 0.8f;
    EstadoEntrada e;
    aplicarControle(l, e);
    VERIFICAR(segurando(e, Comando::Mapa) && !segurando(e, Comando::Ficha));

    l.botoes[static_cast<int>(BotaoControle::Cima)] = true;
    EstadoEntrada e2;
    aplicarControle(l, e2);
    VERIFICAR(segurando(e2, Comando::Ficha));
    VERIFICAR(!segurando(e2, Comando::Cima));   // LT + cima abre a ficha e não move o Bill

    LeituraControle leve;
    leve.gatilhoEsq = 0.3f;   // abaixo do limiar
    EstadoEntrada e3;
    aplicarControle(leve, e3);
    VERIFICAR(!segurando(e3, Comando::Mapa));
}

TESTE(controle_sem_atividade_nao_troca_dispositivo) {
    LeituraControle l;
    l.analogicoEsq = {0.1f, -0.1f};   // ruído dentro da zona morta
    EstadoEntrada e;
    e.ultimoUsado = Dispositivo::Teclado;
    const Vec2 a = aplicarControle(l, e);
    VERIFICAR(a.x == 0.0f && a.y == 0.0f);
    VERIFICAR(e.ultimoUsado == Dispositivo::Teclado);
}

TESTE(controle_direcional_digital_tem_prioridade) {
    // Teclado/cruz para a direita + analógico para a esquerda: vale o digital.
    EstadoEntrada e;
    e.segurando[static_cast<int>(Comando::Direita)] = true;
    combinarEixo(e, {-1.0f, 0.0f});
    VERIFICAR_PERTO(e.eixo.x, 1.0f, 1e-6f);

    // Só o analógico: o eixo é analógico (velocidade parcial preservada).
    EstadoEntrada e2;
    combinarEixo(e2, {0.3f, 0.0f});
    VERIFICAR_PERTO(e2.eixo.x, 0.3f, 1e-6f);
    VERIFICAR(!segurando(e2, Comando::Direita));   // abaixo do limiar de navegação

    // Analógico forte também navega nos menus.
    EstadoEntrada e3;
    combinarEixo(e3, {0.0f, 0.9f});
    VERIFICAR(segurando(e3, Comando::Baixo) && !segurando(e3, Comando::Cima));
    VERIFICAR_PERTO(e3.eixo.y, 0.9f, 1e-6f);
}

// Diego, 24/11: no controle Nintendo o A fica à direita e o B embaixo (o contrário do Xbox). Nos menus, A confirma
// e B volta, como no Switch; as ações da partida continuam por posição (baixo = dash, direita = clarão).
TESTE(controle_nintendo_confirma_com_a_e_volta_com_b) {
    definirConfirmarNintendo(true);
    EstadoEntrada direita;
    aplicarControle(comBotao(BotaoControle::Leste), direita);   // o "A" do Nintendo
    VERIFICAR(segurando(direita, Comando::Confirmar) && segurando(direita, Comando::Clarao));
    VERIFICAR(!segurando(direita, Comando::Voltar));
    EstadoEntrada baixo;
    aplicarControle(comBotao(BotaoControle::Sul), baixo);       // o "B" do Nintendo
    VERIFICAR(segurando(baixo, Comando::Voltar) && segurando(baixo, Comando::Dash));
    VERIFICAR(!segurando(baixo, Comando::Confirmar));
    definirConfirmarNintendo(false);   // Xbox e PlayStation: baixo confirma, direita volta
    EstadoEntrada xbox;
    aplicarControle(comBotao(BotaoControle::Sul), xbox);
    VERIFICAR(segurando(xbox, Comando::Confirmar) && !segurando(xbox, Comando::Voltar));
}
