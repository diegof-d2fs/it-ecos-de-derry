#include "entidades/jogador.h"

#include <algorithm>
#include <cmath>

#include "mundo/colisao.h"

namespace {

// Golpes do taco (SDD §11.3): raio, abertura, multiplicador de dano, recuo, tempo ativo, tempo total.
struct GolpeTaco {
    float raio, angulo, multiplicador, recuo, ativo, total;
};
const GolpeTaco COMBO[3] = {
    {40.0f, 100.0f, 1.0f, 40.0f, 0.10f, 0.24f},
    {40.0f, 100.0f, 1.0f, 40.0f, 0.10f, 0.24f},
    {46.0f, 140.0f, 1.5f, 110.0f, 0.14f, 0.34f},   // o 3º empurra longe
};

void contarMovimento(Jogador& j) {
    const Celula agora = celulaDe(j.pes);
    if (agora.col != j.celula.col || agora.lin != j.celula.lin) {
        j.celula = agora;
        ++j.movimentos;
    }
}

}  // namespace

Golpe golpeDoBill(const Jogador& j, std::uint32_t& proximoGolpe, const Area& area, int dano) {
    Golpe g;
    g.id = proximoGolpe++;
    g.equipe = Equipe::Jogador;
    g.origem = ID_JOGADOR;
    g.area = area;
    g.dano = dano;
    g.focoAtacante = j.attr.foco;
    g.rolaAcerto = true;
    g.podeCritico = true;
    g.critico = j.stats.critico;
    return g;
}

namespace {

void iniciarGolpeCombo(Jogador& j, int indice, std::vector<Golpe>& golpes, std::uint32_t& proximoGolpe,
                       std::vector<IdSom>& sons) {
    const GolpeTaco& t = COMBO[indice - 1];
    j.combo = indice;
    j.acao = AcaoJogador::Atacando;
    j.timerAcao = t.total;
    j.comboPedido = false;
    const int dano = static_cast<int>(static_cast<float>(j.stats.danoBase) * t.multiplicador);
    Golpe g = golpeDoBill(j, proximoGolpe, areaArco(corpoJogador(j), j.mira, t.raio, t.angulo), dano);
    g.recuo = t.recuo;
    g.duracao = t.ativo;
    g.passoCombo = static_cast<std::uint8_t>(indice);
    golpes.push_back(g);
    sons.push_back(IdSom::Golpe);
}

}  // namespace

float velocidadeMovimento(int agilidade) { return 120.0f + 8.0f * static_cast<float>(agilidade); }

float recargaDoDash(int agilidade) { return 0.70f - 0.03f * static_cast<float>(agilidade); }

bool estaInvulneravel(const Jogador& j) { return j.dashRestante > 0.0f || j.invulneravel > 0.0f; }

bool estaVivo(const Jogador& j) { return j.hp > 0; }

void recalcularJogador(Jogador& j) {
    const int hpMaxAntes = j.stats.hpMax;
    j.equip.bonusDanoPct = j.coragem > 0.0f ? BONUS_DANO_CORAGEM : 0;
    j.equip.bonusVelocidadePct = j.tonico > 0.0f ? BONUS_VELOCIDADE_TONICO : 0;
    j.stats = calcularStats(j.attr, j.equip);
    // Coragem a mais aumenta o HP máximo E o atual na mesma quantidade (SDD §14).
    if (j.stats.hpMax > hpMaxAntes) j.hp += j.stats.hpMax - hpMaxAntes;
    if (j.hp > j.stats.hpMax) j.hp = j.stats.hpMax;
}

Direcao direcaoSuave(Vec2 v, Direcao atual) { return direcaoComHisterese(v, atual, HISTERESE_DIRECAO); }

Vec2 vetorDaDirecao(Direcao d) {
    switch (d) {
        case Direcao::Cima: return {0.0f, -1.0f};
        case Direcao::Esquerda: return {-1.0f, 0.0f};
        case Direcao::Direita: return {1.0f, 0.0f};
        default: return {0.0f, 1.0f};
    }
}

Retangulo hurtboxJogador(const Jogador& j) { return {j.pes.x - 8.0f, j.pes.y - 30.0f, 16.0f, 30.0f}; }

Vec2 corpoJogador(const Jogador& j) { return {j.pes.x, j.pes.y - 14.0f}; }

void posicionarJogador(Jogador& j, Vec2 pes) {
    j.pes = pes;
    j.celula = celulaDe(pes);
    j.dashRestante = 0.0f;
    j.andando = false;
    // A velocidade suavizada continua: a esteira reposiciona Bill a cada passo (zerar aqui o deixava a 30%).
    j.acao = AcaoJogador::Livre;
}

void empurrarJogador(Jogador& j, Vec2 pes) {
    j.pes = pes;
    j.celula = celulaDe(pes);
}

void acoesDoJogador(Jogador& j, const EstadoEntrada& entrada, float dt, std::vector<Golpe>& golpes,
                    std::uint32_t& proximoGolpe, std::vector<IdSom>& sons) {
    if (j.recargaEspecial > 0.0f) j.recargaEspecial -= dt;
    if (j.invulneravel > 0.0f) j.invulneravel -= dt;
    if (j.flash > 0.0f) j.flash -= dt;
    if (j.janelaCombo > 0.0f) {
        j.janelaCombo -= dt;
        if (j.janelaCombo <= 0.0f) j.combo = 0;
    }
    if (pressionou(entrada, Comando::Atacar)) j.bufferAtaque = BUFFER_ATAQUE;
    else if (j.bufferAtaque > 0.0f) j.bufferAtaque -= dt;
    if (pressionou(entrada, Comando::Especial)) j.bufferEspecial = BUFFER_ATAQUE;
    else if (j.bufferEspecial > 0.0f) j.bufferEspecial -= dt;
    if (pressionou(entrada, Comando::Clarao)) j.bufferClarao = BUFFER_ATAQUE;
    else if (j.bufferClarao > 0.0f) j.bufferClarao -= dt;
    // Mira: o eixo, se houver; senão, para onde Bill olha.
    if (comprimento(entrada.eixo) > 0.2f) j.ultimoRumo = normalizar(entrada.eixo);
    j.mira = j.ultimoRumo;

    // Fim da ação atual: encadeia o combo pedido ou abre a janela para o próximo golpe.
    if (j.acao != AcaoJogador::Livre) {
        j.timerAcao -= dt;
        if (j.acao == AcaoJogador::Atacando && j.bufferAtaque > 0.0f) {
            j.comboPedido = true;
            j.bufferAtaque = 0.0f;
        }
        if (j.timerAcao <= 0.0f) {
            const bool eraAtaque = j.acao == AcaoJogador::Atacando;
            j.acao = AcaoJogador::Livre;
            if (eraAtaque && j.comboPedido) {
                iniciarGolpeCombo(j, j.combo % 3 + 1, golpes, proximoGolpe, sons);
            } else if (eraAtaque) {
                j.janelaCombo = JANELA_COMBO;
            }
        }
        return;
    }
    if (j.dashRestante > 0.0f) return;   // durante o dash não ataca (golpe de dash é P1)

    if (j.bufferAtaque > 0.0f) {
        j.bufferAtaque = 0.0f;
        const int proximo = (j.janelaCombo > 0.0f) ? j.combo % 3 + 1 : 1;
        iniciarGolpeCombo(j, proximo, golpes, proximoGolpe, sons);
    } else if (j.bufferEspecial > 0.0f && j.recargaEspecial <= 0.0f) {
        j.bufferEspecial = 0.0f;
        // Giro 360° (taco): círculo de raio 52, dano ×2, recarga 3 s.
        Golpe g = golpeDoBill(j, proximoGolpe, areaCirculo(corpoJogador(j), 52.0f), j.stats.danoBase * 2);
        g.recuo = 90.0f;
        g.duracao = 0.25f;
        g.derrubaParede = true;
        g.especial = true;
        golpes.push_back(g);
        j.acao = AcaoJogador::Especial;
        j.timerAcao = 0.32f;
        j.recargaEspecial = RECARGA_ESPECIAL;
        j.combo = 0;
        sons.push_back(IdSom::Golpe);
    } else if (j.bufferClarao > 0.0f && j.cargasLanterna > 0) {
        j.bufferClarao = 0.0f;
        // Clarão: feixe de 4 células; não erra; atordoa 1,5 s; gasta 1 carga.
        Golpe g = golpeDoBill(j, proximoGolpe, areaLinha(corpoJogador(j), j.mira, 4.0f * TILE, 24.0f), j.stats.danoClarao);
        g.rolaAcerto = false;
        g.podeCritico = false;
        g.atordoa = 1.5f;
        g.duracao = 0.06f;
        g.derrubaParede = true;
        golpes.push_back(g);
        j.acao = AcaoJogador::Clarao;
        j.timerAcao = 0.25f;
        --j.cargasLanterna;
    }
    if (j.acao != AcaoJogador::Livre) j.direcao = direcaoSuave(j.mira, j.direcao);
}

void atualizarJogador(Jogador& j, const SalaViva& sala, const EstadoEntrada& entrada, float dt) {
    if (j.recargaDash > 0.0f) j.recargaDash -= dt;
    if (j.recargaDash <= 0.0f) j.dashExtrasDisponiveis = j.dashExtras;   // recarga cheia: cargas de volta
    if (j.preso > 0.0f) j.preso -= dt;
    if (j.lento > 0.0f) j.lento -= dt;

    // Começa um dash: na direção do eixo ou, parado, para onde Bill olha. Cancela o ataque.
    // Dash: com a recarga pronta ou, durante ela, gastando uma carga extra (Asas).
    const bool temCarga = j.recargaDash <= 0.0f || j.dashExtrasDisponiveis > 0;
    if (pressionou(entrada, Comando::Dash) && j.dashRestante <= 0.0f && temCarga && j.preso <= 0.0f) {
        if (j.recargaDash > 0.0f) --j.dashExtrasDisponiveis;
        const bool temEixo = comprimento(entrada.eixo) > 0.1f;
        j.dashDirecao = temEixo ? normalizar(entrada.eixo) : vetorDaDirecao(j.direcao);
        j.dashRestante = DASH_DURACAO;
        j.recargaDash = DASH_DURACAO + recargaDoDash(j.attr.agilidade);
        j.acao = AcaoJogador::Livre;
        j.comboPedido = false;
    }

    Vec2 delta;
    Vec2 alvo;   // velocidade pedida pelo eixo (fora do dash)
    if (j.dashRestante > 0.0f) {
        const float t = dt < j.dashRestante ? dt : j.dashRestante;
        delta = escalar(j.dashDirecao, DASH_DISTANCIA / DASH_DURACAO * t);
        j.dashRestante -= dt;
        j.direcao = direcaoSuave(j.dashDirecao, j.direcao);
        j.andando = true;
        // Saindo do dash, Bill continua na velocidade de andar (não para seco nem dispara).
        j.velocidade = escalar(j.dashDirecao, j.stats.velocidade);
    } else {
        const Celula c = celulaDe(j.pes);
        float fator = fatorVelocidade(tileEm(sala, c.col, c.lin).tipo);
        // Atacando, Bill quase para (o golpe tem peso); no clarão, fica parado.
        if (j.acao == AcaoJogador::Atacando || j.acao == AcaoJogador::Especial) fator *= 0.3f;
        if (j.acao == AcaoJogador::Clarao) fator = 0.0f;
        if (j.preso > 0.0f) fator = 0.0f;   // a faixa segura as pernas (golpes e clarão continuam)
        if (j.lento > 0.0f) fator *= FATOR_AGARRADO;   // o agarrão do Afogado
        if (j.naPoca) fator *= FATOR_POCA;             // poça do Afogado: como água rasa
        alvo = escalar(entrada.eixo, j.stats.velocidade * fator);   // inclui o Tônico
        if (fator <= 0.0f) {
            j.velocidade = Vec2{};   // preso ou no clarão: para na hora (não escorrega)
        } else {
            if (comprimento(alvo) > 0.01f) {
                j.velocidade = alvo;   // arranque e curva imediatos
            } else {                   // soltou: desliza um palmo e para
                j.velocidade = escalar(j.velocidade, std::exp(-dt / TEMPO_FREAR));
                if (comprimento(j.velocidade) < 6.0f) j.velocidade = Vec2{};
            }
        }
        delta = escalar(j.velocidade, dt);
        j.andando = comprimento(entrada.eixo) > 0.01f && fator > 0.0f;
        if (j.acao == AcaoJogador::Livre) j.direcao = direcaoSuave(entrada.eixo, j.direcao);
    }

    const Vec2 antes = j.pes;
    j.pes = moverComColisao(sala, j.pes, CAIXA_BILL, delta, false);
    // Quina: andando num eixo só e travado pela ponta de uma parede, escorrega de lado para contornar
    // (portas e corredores de 1 célula deixavam Bill "engatado" na borda).
    if (j.dashRestante <= 0.0f && comprimento(alvo) > 1.0f) {
        const bool horizontal = std::fabs(alvo.x) > 0.9f * comprimento(alvo);
        const bool vertical = std::fabs(alvo.y) > 0.9f * comprimento(alvo);
        const float queria = horizontal ? delta.x : delta.y;
        const float andou = horizontal ? j.pes.x - antes.x : j.pes.y - antes.y;
        if ((horizontal || vertical) && std::fabs(andou) < 0.25f * std::fabs(queria)) {
            const Vec2 frente = horizontal ? Vec2{queria > 0.0f ? 2.0f : -2.0f, 0.0f} : Vec2{0.0f, queria > 0.0f ? 2.0f : -2.0f};
            for (float folga = 1.0f; folga <= FOLGA_QUINA; folga += 1.0f) {
                bool achou = false;
                for (float lado : {-1.0f, 1.0f}) {
                    const Vec2 desvio = horizontal ? Vec2{0.0f, lado * folga} : Vec2{lado * folga, 0.0f};
                    if (colideComSala(sala, caixaNosPes(somar(somar(j.pes, desvio), frente), CAIXA_BILL), false)) continue;
                    const float passo = std::min(folga, j.stats.velocidade * 0.6f * dt);
                    j.pes = moverComColisao(sala, j.pes, CAIXA_BILL, escalar(normalizar(desvio), passo), false);
                    achou = true;
                    break;
                }
                if (achou) break;
            }
        }
    }
    // Parede: o componente travado da velocidade zera (não acumula empurrando a parede).
    if (dt > 0.0f && j.dashRestante <= 0.0f) {
        if (std::fabs(j.pes.x - antes.x) < 0.5f * std::fabs(delta.x)) j.velocidade.x = 0.0f;
        if (std::fabs(j.pes.y - antes.y) < 0.5f * std::fabs(delta.y)) j.velocidade.y = 0.0f;
    }
    if (j.andando && comprimento(subtrair(j.pes, antes)) < 0.01f) j.andando = false;
    // Os passos acompanham a velocidade real (na água e arrancando, as pernas não "patinam").
    const float ritmo = std::clamp(comprimento(subtrair(j.pes, antes)) / std::max(1e-4f, dt) / velocidadeMovimento(1), 0.45f, 1.7f);
    j.tempoAnimacao = j.andando ? j.tempoAnimacao + dt * ritmo : 0.0f;
    contarMovimento(j);
}
