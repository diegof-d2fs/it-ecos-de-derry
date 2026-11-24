#include "core/controle.h"

namespace {

struct MapeamentoBotao {
    BotaoControle botao;
    Comando comando;
};

// Tabela única do joystick (PRD v2 §6.2): alterar um controle = alterar uma linha aqui.
// Um botão pode gerar dois comandos: um de jogo e um de menu (cada cena lê o que lhe serve).
const MapeamentoBotao TABELA_BOTOES[] = {
    {BotaoControle::Sul, Comando::Dash},           {BotaoControle::Sul, Comando::Confirmar},
    {BotaoControle::Leste, Comando::Clarao},       {BotaoControle::Leste, Comando::Voltar},
    {BotaoControle::Oeste, Comando::Atacar},
    {BotaoControle::Norte, Comando::Especial},
    {BotaoControle::OmbroDir, Comando::Interagir}, {BotaoControle::OmbroDir, Comando::AbaDireita},
    {BotaoControle::OmbroEsq, Comando::UsarKit},   {BotaoControle::OmbroEsq, Comando::AbaEsquerda},
    {BotaoControle::Selecionar, Comando::Inventario},
    {BotaoControle::Iniciar, Comando::Pausa},
    {BotaoControle::Cima, Comando::Cima},          {BotaoControle::Baixo, Comando::Baixo},
    {BotaoControle::Esquerda, Comando::Esquerda},  {BotaoControle::Direita, Comando::Direita},
};

void marcar(EstadoEntrada& e, Comando c) { e.segurando[static_cast<int>(c)] = true; }

bool confirmarNintendo = false;

// Confirmar e Voltar trocam de posição no controle Nintendo; o resto da tabela fica igual.
BotaoControle botaoDoComando(const MapeamentoBotao& m) {
    if (!confirmarNintendo) return m.botao;
    if (m.comando == Comando::Confirmar) return BotaoControle::Leste;
    if (m.comando == Comando::Voltar) return BotaoControle::Sul;
    return m.botao;
}

}  // namespace

float normalizarEixo(int bruto) {
    return limitar(static_cast<float>(bruto) / 32767.0f, -1.0f, 1.0f);
}

float normalizarGatilho(int bruto, int maximo) {
    if (maximo <= 0) return 0.0f;
    return limitar(static_cast<float>(bruto) / static_cast<float>(maximo), 0.0f, 1.0f);
}

Vec2 aplicarZonaMorta(Vec2 v, float zona) {
    const float tamanho = comprimento(v);
    if (tamanho <= zona || zona >= 1.0f) return {0.0f, 0.0f};
    // Um analógico real passa um pouco de 1 nas diagonais: limitar mantém a velocidade máxima.
    const float reescalado = limitar((tamanho - zona) / (1.0f - zona), 0.0f, 1.0f);
    return escalar(v, reescalado / tamanho);
}

void definirConfirmarNintendo(bool nintendo) { confirmarNintendo = nintendo; }

Vec2 aplicarControle(const LeituraControle& leitura, EstadoEntrada& entrada) {
    bool atividade = false;
    // LT segurado = Diário/minimapa; LT + cima = Ficha (o "cima" não move o Bill nesse caso).
    const bool ltSegurado = leitura.gatilhoEsq >= LIMIAR_GATILHO;
    for (const MapeamentoBotao& m : TABELA_BOTOES) {
        const BotaoControle b = botaoDoComando(m);
        if (!botao(leitura, b)) continue;
        atividade = true;
        if (ltSegurado && b == BotaoControle::Cima) continue;
        marcar(entrada, m.comando);
    }
    if (ltSegurado) {
        atividade = true;
        marcar(entrada, botao(leitura, BotaoControle::Cima) ? Comando::Ficha : Comando::Mapa);
    }
    if (leitura.gatilhoDir >= LIMIAR_GATILHO) atividade = true;

    const Vec2 analogico = aplicarZonaMorta(leitura.analogicoEsq, ZONA_MORTA_ANALOGICO);
    if (analogico.x != 0.0f || analogico.y != 0.0f) atividade = true;
    if (atividade) entrada.ultimoUsado = Dispositivo::Gamepad;
    return analogico;
}

void combinarEixo(EstadoEntrada& entrada, Vec2 analogico) {
    const Vec2 digital = eixoDosDirecionais(entrada);
    entrada.eixo = (digital.x != 0.0f || digital.y != 0.0f) ? digital : analogico;
    if (analogico.x <= -LIMIAR_NAVEGACAO) marcar(entrada, Comando::Esquerda);
    if (analogico.x >= LIMIAR_NAVEGACAO) marcar(entrada, Comando::Direita);
    if (analogico.y <= -LIMIAR_NAVEGACAO) marcar(entrada, Comando::Cima);
    if (analogico.y >= LIMIAR_NAVEGACAO) marcar(entrada, Comando::Baixo);
}
