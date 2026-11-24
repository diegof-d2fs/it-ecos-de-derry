// Cena de sala: DESENHA a partida (jogo/partida, nas regras) e repassa os comandos. Nenhuma
// regra de jogo mora aqui; por isso a mesma partida roda sem janela no piloto e nos testes.
// < > (ou PageUp/PageDown, LB/RB) troca de sala (atalho de teste).
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#include "cenas/cena.h"
#include "cenas/tela_inventario.h"
#include "cenas/ui.h"
#include "jogo/aparencia.h"
#include "jogo/clube.h"
#include "jogo/telemetria.h"
#include "jogo/armadilhas.h"
#include "core/arquivo.h"
#include "core/medidor.h"
#include "mundo/colisao.h"
#include "plataforma/audio.h"   // a cena traduz eventos de som em áudio (as regras não tocam som)
#include "render/efeitos.h"
#include "render/luz.h"
#include "render/sprite.h"
#include "render/texto.h"

namespace {

constexpr float TEMPO_NOME_SALA = 2.5f;
constexpr float DURACAO_TELA_CICLO = 7.0f;   // "Bill acorda no Clube" (1ª morte)
constexpr float SUAVIZACAO_CAMERA = 0.15f;   // fração do caminho até o alvo a cada passo (SDD §7.4)

const std::uint32_t COR_TEXTO = rgb(240, 232, 214);
const std::uint32_t COR_PERIGO = rgb(220, 40, 46);

// Câmera desejada: Bill no centro, presa às bordas; sala menor que a tela fica centralizada.
Vec2 alvoCamera(const EstadoPartida& p) {
    const float larguraSala = static_cast<float>(p.sala.cols * TILE), alturaSala = static_cast<float>(p.sala.lins * TILE);
    Vec2 alvo{p.bill.pes.x - LARGURA_LOGICA / 2.0f, p.bill.pes.y - 24.0f - ALTURA_LOGICA / 2.0f};
    alvo.x = larguraSala <= LARGURA_LOGICA ? (larguraSala - LARGURA_LOGICA) / 2.0f
                                           : limitar(alvo.x, 0.0f, larguraSala - LARGURA_LOGICA);
    alvo.y = alturaSala <= ALTURA_LOGICA ? (alturaSala - ALTURA_LOGICA) / 2.0f
                                         : limitar(alvo.y, 0.0f, alturaSala - ALTURA_LOGICA);
    return alvo;
}

constexpr float DURACAO_CARTAO_BIOMA = 2.8f;

// Cartão do bioma: a ilustração (cartao_<ambiente>, 640×360) com o nome e "bioma N de 6" numa faixa
// embaixo; entra e sai em fade de 0,5 s.
void desenharCartaoBioma(Canvas& canvas, const Recursos& r, int bioma, float t) {
    static const char* const CARTOES[] = {nullptr, "cartao_galeria", "cartao_barrens", "cartao_neibolt", "cartao_poroes",
                                          "cartao_siderurgica", "cartao_cisterna", "cartao_covil"};
    const float fade = std::min(1.0f, std::min((DURACAO_CARTAO_BIOMA - t) / 0.5f, t / 0.5f));
    retanguloCheio(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0));
    if (bioma >= 1 && bioma <= 7) {
        const Sprite cartao = spriteDe(r, CARTOES[bioma]);
        if (spriteValido(cartao)) desenharFrame(canvas, cartao, 0, 0, 0, 0);
    }
    retanguloAlpha(canvas, 0, ALTURA_LOGICA - 64, LARGURA_LOGICA, 64, rgb(0, 0, 0), 170);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 58, paraWide(nomeBioma(bioma)), COR_TEXTO);
    if (bioma >= 1 && bioma <= 6) {
        wchar_t sub[48];
        std::swprintf(sub, 48, L"Bioma %d de 6", bioma);
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 22, sub, rgb(255, 200, 190));
    }
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), static_cast<std::uint8_t>(255.0f * (1.0f - fade)));
}

void aoEntrarNaSala(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    s.camera = alvoCamera(s.partida);   // sem deslizar a câmera de uma sala para outra
    s.tempoNome = TEMPO_NOME_SALA;
    retomarSomDaSala(jogo);
}

// Chegada a um bioma novo na tentativa (depois do chefe): a cena da história daquele capítulo, para
// quem joga, na 1ª vez (ou sempre, nas Opções). Devolve true se abriu (a sala fica congelada por baixo).
bool abrirCenaDaChegada(Jogo& jogo, int bioma) {
    if (jogo.pilotoAtivo) return false;
    const std::string id = cenaDaChegadaAoBioma(bioma);
    if (id.empty() || (!jogo.perfil.opcoes.cenasSempre && jogo.perfil.cenasVistas.count(id) > 0)) return false;
    return abrirCenaHistoria(jogo, id);
}

// ---------- desenho dos atores (ordenados pelo y dos pés) ----------
enum class TipoDesenho : std::uint8_t { Bill, Inimigo, Objeto, Item, Companheiro };
struct Desenhavel {
    float y;
    TipoDesenho tipo;
    int indice;   // posição na lista do tipo
};

// Objeto ou amigo deitado no chão (alçapão, entulho): fica atrás de quem passa por cima.
bool objetoNoPiso(const ObjetoSala& o) { return (o.codigo == 'c' && o.usado) || o.codigo == 'H'; }

// Baú (fechado/aberto; dourado = trancado) e caixa (inteira/quebrada), da folha objetos_comuns;
// no Clube: Mural, quadro de recordes, alçapão e enfeites (objetos_clube) e os amigos (<nome>_idle).
void desenharObjeto(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const ObjetoSala& o, float tempo, float camX,
                    float camY) {
    const Vec2 base{static_cast<float>(o.celula.col * TILE + TILE / 2) - camX, static_cast<float>((o.celula.lin + 1) * TILE) - camY};
    if (o.codigo >= '1' && o.codigo <= '9') {   // amigo: 2 quadros de respiração, desencontrados
        if (o.usado) return;                     // já acompanha Bill (prólogo) ou foi resgatado
        const Sprite s = spriteDe(r, p.sala.modelo->npcs[static_cast<std::size_t>(o.codigo - '0')] + "_idle");
        if (!spriteValido(s)) return;
        elipseAlpha(canvas, base.x, base.y - 4, 9, 4, rgb(0, 0, 0), 120);
        const int quadro = static_cast<int>(tempo * 1.6f + 0.37f * static_cast<float>(o.celula.col)) % std::max(1, s.colunas);
        desenharFramePes(canvas, s, quadro, 0, base.x, base.y - 3);
        if (p.falaNova[static_cast<std::size_t>(o.codigo - '0')]) {   // "!": tem fala nova (PRD §4.6)
            const float y = base.y - 62.0f + 2.5f * std::sin(tempo * 4.0f);
            elipseAlpha(canvas, base.x, y, 7, 8, rgb(255, 214, 90), 240);
            contornoRetangulo(canvas, base.x - 1, y + 9, 2, 3, rgb(255, 214, 90));
            desenharTextoCentralizado(canvas, r.fonteMedia, base.x, y - 9, L"!", rgb(60, 30, 10));
        }
        return;
    }
    int quadro = -1;
    std::string atlas = "objetos_comuns";
    switch (o.codigo) {
        case 'B': quadro = (bauTrancado(p) ? 2 : 0) + (o.usado ? 1 : 0); break;
        case 'c': quadro = o.usado ? 5 : 4; break;
        case '*': quadro = o.usado ? 7 : 6; break;   // altar de Eco: aceso (luz azul) ou apagado
        case 'M': atlas = "objetos_clube"; quadro = 0; break;
        case 'Q': atlas = "objetos_clube"; quadro = 6; break;
        case 'H': atlas = "objetos_clube"; quadro = p.pendencia == Pendencia::Alcapao ? 5 : 4; break;
        case 'e': atlas = "objetos_" + p.sala.modelo->ambiente; quadro = o.quadro; break;
        default: break;
    }
    if (quadro < 0) return;
    Sprite s = spriteDe(r, atlas);
    if (!spriteValido(s)) s = spriteDe(r, "objetos_comuns");
    if (!spriteValido(s)) return;
    if (!objetoNoPiso(o)) elipseAlpha(canvas, base.x, base.y - 3, 13, 4, rgb(0, 0, 0), 110);
    if (o.codigo == 'H') {   // brilho fraco no alçapão: é por ali que a noite começa
        const float pulso = 0.5f + 0.5f * std::sin(tempo * 2.5f);
        elipseAlpha(canvas, base.x, base.y - 14, 20, 12, rgb(255, 200, 110), static_cast<std::uint8_t>(25 + 35 * pulso));
    }
    if (o.codigo == 'c' && !o.usado) {   // caixa (17/11): treme a cada golpe e racha conforme a vida cai
        const float dx = o.tremor > 0.0f ? 2.5f * std::sin(o.tremor * 90.0f) * (o.tremor / TREMOR_CAIXA) : 0.0f;
        desenharFramePes(canvas, s, quadro % s.colunas, quadro / s.colunas, base.x + dx, base.y);
        const std::uint32_t risco = rgb(30, 18, 10);
        const float x = base.x + dx, y = base.y;
        if (o.vida <= VIDA_CAIXA - 1) {   // 1ª rachadura: diagonal na tampa
            retanguloAlpha(canvas, x - 7, y - 22, 6, 1, risco, 230);
            retanguloAlpha(canvas, x - 2, y - 21, 1, 6, risco, 230);
            retanguloAlpha(canvas, x - 1, y - 16, 5, 1, risco, 230);
        }
        if (o.vida <= VIDA_CAIXA - 2) {   // 2ª: a frente partida, uma tábua solta
            retanguloAlpha(canvas, x + 3, y - 12, 1, 8, risco, 230);
            retanguloAlpha(canvas, x - 8, y - 9, 7, 1, risco, 230);
            retanguloAlpha(canvas, x - 9, y - 9, 1, 4, rgb(150, 110, 60), 200);
        }
        return;
    }
    desenharFramePes(canvas, s, quadro % s.colunas, quadro / s.colunas, base.x, base.y);
}

// Lascas das caixas: retângulos de madeira com sombra no chão (a altura vem das regras).
void desenharEstilhacos(Canvas& canvas, const EstadoPartida& p, float camX, float camY) {
    static const std::uint32_t TONS[3] = {rgb(150, 104, 56), rgb(112, 74, 38), rgb(190, 146, 90)};
    for (const Estilhaco& e : p.estilhacos) {
        const float x = e.pos.x - camX, y = e.pos.y - camY;
        const std::uint8_t a = static_cast<std::uint8_t>(255.0f * std::min(1.0f, e.vida / 0.25f));
        elipseAlpha(canvas, x, y, 2.0f, 1.0f, rgb(0, 0, 0), static_cast<std::uint8_t>(a / 3));
        retanguloAlpha(canvas, x - 1.5f, y - e.altura - 1.0f, 3.0f, 2.0f, TONS[e.tom % 3], a);
    }
}

// Item no chão: sombra, flutuação leve e, ao surgir, o arco do "pulo". Missão ganha brilho.
void desenharItemChao(Canvas& canvas, const Recursos& r, const ItemNoChao& it, float tempoTotal, float camX, float camY) {
    const float t = std::min(1.0f, it.tempo / DURACAO_PULO_ITEM);
    const Vec2 pos = somar(it.origem, escalar(subtrair(it.pos, it.origem), t));
    const float arco = 18.0f * 4.0f * t * (1.0f - t);   // parábola: sobe e desce
    const float flutua = t >= 1.0f ? 1.5f * std::sin(tempoTotal * 3.0f + static_cast<float>(it.id)) : 0.0f;
    const float x = pos.x - camX, y = pos.y - camY;
    elipseAlpha(canvas, x, y + 2, 6, 2.5f, rgb(0, 0, 0), 120);
    if (ehItemDeMissao(it.tipo)) {
        const float pulso = 0.5f + 0.5f * std::sin(tempoTotal * 4.0f);
        elipseAlpha(canvas, x, y - 8, 12, 12, rgb(255, 220, 120), static_cast<std::uint8_t>(40 + 50 * pulso));
    }
    desenharIcone(canvas, r, it.tipo, x - 8, y - 16 - arco + flutua);
}

// Escudo (tampa de lixeira) no braço esquerdo do Bill. De frente/de lado esquerdo fica na
// frente do corpo; de costas/de lado direito, atrás (desenhado ANTES do sprite).
// (Provisório com o ícone do item até a sobreposição própria por direção: lote de arte.)
bool escudoNaFrente(Direcao d) { return d == Direcao::Baixo || d == Direcao::Esquerda; }

void desenharEscudo(Canvas& canvas, const Recursos& r, Direcao d, float bx, float by) {
    const float dx = d == Direcao::Baixo ? 7.0f : d == Direcao::Cima ? -7.0f : d == Direcao::Esquerda ? -5.0f : 3.0f;
    desenharIcone(canvas, r, TipoItem::Escudo, bx + dx - 8.0f, by - 30.0f);
}

// Corte do taco (24/11): o rastro desenhado para a direita, GIRADO para o rumo exato do golpe, animado pelo tempo
// do golpe inteiro (0,24 s; o 3º do combo, 0,34 s e 30% maior). A diagonal fica igual ao eixo (Diego: "melhorar
// os golpes diagonais"). Desenhado depois do Bill.
void desenharCorteDoTaco(Canvas& canvas, const Recursos& r, const Jogador& b, float camX, float camY) {
    if (b.acao != AcaoJogador::Atacando) return;
    const Sprite corte = spriteDe(r, "fx_corte_taco");
    if (!spriteValido(corte)) return;
    const float total = b.combo == 3 ? 0.34f : 0.24f;
    const float k = std::clamp(1.0f - b.timerAcao / total, 0.0f, 0.999f);
    const int quadro = std::min(corte.colunas - 1, static_cast<int>(k * static_cast<float>(corte.colunas)));
    const Vec2 corpo = corpoJogador(b);
    const float alcance = b.combo == 3 ? 26.0f : 22.0f;
    desenharFrameGirado(canvas, corte, quadro, 0, corpo.x + b.mira.x * alcance - camX, corpo.y + b.mira.y * alcance - camY,
                        std::atan2(b.mira.y, b.mira.x), b.combo == 3 ? 1.3f : 1.0f);
}

void desenharBill(Canvas& canvas, const Recursos& r, const Jogador& b, const Inventario& inv, float tempo, float camX,
                  float camY, bool naTeia = false) {
    // Golpe com peso (24/11): no instante do golpe o corpo avança 3 px na direção da mira e volta.
    float avanco = 0.0f;
    if (b.acao == AcaoJogador::Atacando) avanco = 3.0f * std::clamp((b.timerAcao - 0.06f) / 0.14f, 0.0f, 1.0f);
    const float bx = b.pes.x - camX + b.mira.x * avanco, by = b.pes.y - camY + b.mira.y * avanco * 0.6f;
    const int linha = static_cast<int>(b.direcao);
    // A aparência vem de uma função pura das regras (jogo/aparencia): aqui só se obedece.
    const AparenciaBill ap = aparenciaDe(b, inv, tempo);
    // Logo após um dano: translúcido (sem piscar contínuo: PRD §8.2; o branco é de 1 quadro).
    const std::uint8_t opacidade = (b.invulneravel > 0.0f && b.dashRestante <= 0.0f) ? 140 : 255;
    elipseAlpha(canvas, bx, by - 2, 9, 4, rgb(0, 0, 0), 120);
    if (ap.aura > 0.0f) {   // Ficha de coragem: aura laranja pulsante
        elipseAlpha(canvas, bx, by - 20, 16 + 2 * ap.aura, 26 + 2 * ap.aura, rgb(255, 140, 40),
                    static_cast<std::uint8_t>(40 + 50 * ap.aura));
    }
    if (ap.rastro && b.andando) {   // Tônico: rastro azul-claro nos passos
        for (int i = 1; i <= 3; ++i) {
            const Vec2 atras = escalar(vetorDaDirecao(b.direcao), -7.0f * static_cast<float>(i));
            elipseAlpha(canvas, bx + atras.x, by - 2 + atras.y, 5, 2, rgb(140, 220, 255), static_cast<std::uint8_t>(150 / i));
        }
    }
    if (!estaVivo(b)) {
        desenharFramePes(canvas, spriteDe(r, "bill_defeat"), 3, 0, bx, by);
        return;
    }
    Sprite sprite = spriteDe(r, "bill_walk");
    int coluna = 0;
    if (b.acao != AcaoJogador::Livre) {
        // Coluna 0 = preparo, 1 = golpe estendido (o golpe já saiu no 1º quadro da ação).
        sprite = spriteDe(r, "bill_attack");
        coluna = b.timerAcao > 0.12f ? 1 : 0;
    } else {
        const Animacao anim{linha, 0, 4, 8.0f, true};
        coluna = b.andando ? colunaAnimacao(anim, b.tempoAnimacao) : 0;
        if (b.dashRestante > 0.0f) {   // rastro translúcido do dash
            for (int i = 1; i <= 2; ++i) {
                const Vec2 atras = escalar(b.dashDirecao, -12.0f * static_cast<float>(i));
                desenharFramePes(canvas, sprite, coluna, linha, bx + atras.x, by + atras.y, false,
                                 static_cast<std::uint8_t>(110 / i));
            }
        }
    }
    if (ap.escudo && !escudoNaFrente(b.direcao)) desenharEscudo(canvas, r, b.direcao, bx, by);
    if (ap.contorno) contornoFramePes(canvas, sprite, coluna, linha, bx, by, rgb(0x7A, 0x0F, 0x14));
    if (b.dashRestante > 0.0f && !ap.contorno) {   // o dash estica o corpo no rumo (sem a moldura do pacto)
        const bool deLado = b.direcao == Direcao::Esquerda || b.direcao == Direcao::Direita;
        desenharFramePesEstica(canvas, sprite, coluna, linha, bx, by, deLado ? 1.1f : 0.93f, deLado ? 0.93f : 1.08f, false, opacidade,
                               ap.tinta, ap.forcaTinta);
    } else {
        desenharFramePesTinta(canvas, sprite, coluna, linha, bx, by, false, opacidade, ap.tinta, ap.forcaTinta);
    }
    if (ap.escudo && escudoNaFrente(b.direcao)) desenharEscudo(canvas, r, b.direcao, bx, by);
    if (b.preso > 0.0f && naTeia) {   // a teia do palhaço-aranha grudada nas pernas (24/11)
        const Sprite teia = spriteDe(r, "fx_teia");
        if (spriteValido(teia)) {
            const float meio = static_cast<float>(teia.frameW) / (2.0f * static_cast<float>(canvas.escala));
            desenharFrame(canvas, teia, 2, 0, bx - meio, by - 10.0f - meio);
        }
    } else if (b.preso > 0.0f) {   // a faixa da Múmia enrolada nas pernas
        for (int k = 0; k < 3; ++k) {
            retanguloAlpha(canvas, bx - 8, by - 6 - 5.0f * static_cast<float>(k), 16, 3, rgb(210, 190, 120), 230);
        }
    }
    if (ap.melhorias > 0 && b.acao != AcaoJogador::Livre) {   // pregos no taco: faíscas no golpe
        const Vec2 ponta = somar(Vec2{bx, by - 22}, escalar(b.mira, 20.0f));
        for (int k = 0; k < ap.melhorias; ++k) {
            elipseAlpha(canvas, ponta.x + 3.0f * static_cast<float>(k - 1), ponta.y - 2.0f * static_cast<float>(k), 1.6f, 1.6f,
                        rgb(220, 230, 255), 230);
        }
    }
}

// Névoa do Inalador (Eco do Eddie): um puff azulado onde o dash terminou.
void desenharNevoasDash(Canvas& canvas, const EstadoPartida& p, float camX, float camY) {
    for (std::size_t k = 0; k < p.nevoasDash.size(); ++k) {
        const float t = 1.0f - p.vidaNevoas[k] / 0.6f;
        elipseAlpha(canvas, p.nevoasDash[k].x - camX, p.nevoasDash[k].y - 4.0f - camY, 20.0f + 36.0f * t, 8.0f + 14.0f * t,
                    rgb(190, 215, 230), static_cast<std::uint8_t>(110 * (1.0f - t)));
    }
}

// Fase 2 e dispersão de cada chefe-medo: o Pennywise do estágio (I e I+ dividem a folha).
// II e II+ dividem a folha, como I e I+.
// III (o Pássaro, 13.3) tem a folha própria; III+ (o Lobisomem, Etapa 14) vai dividi-la.
bool estagioII(TipoInimigo t) { return t == TipoInimigo::Leproso || t == TipoInimigo::RaloSangue; }
bool estagioIII(TipoInimigo t) { return t == TipoInimigo::Passaro || t == TipoInimigo::Lobisomem; }
// Os estágios "+" (I+, II+, III+) têm folhas PRÓPRIAS desde 24/11 (Diego: "I e I+ parecem o mesmo"); se a folha
// nova faltar, o desenho cai na do estágio base (spriteDoEstagio).
const char* prefixoDoEstagio(TipoInimigo t) {
    switch (t) {
        case TipoInimigo::Mumia: return "pennywise_1";
        case TipoInimigo::MedoAfogado: return "pennywise_1p";
        case TipoInimigo::Leproso: return "pennywise_2";
        case TipoInimigo::RaloSangue: return "pennywise_2p";   // encharcado de sangue dos pés até a metade do rosto
        case TipoInimigo::Passaro: return "pennywise_3";
        case TipoInimigo::Lobisomem: return "pennywise_3p";
        default: return "pennywise_4";
    }
}
const char* prefixoBase(TipoInimigo t) {
    return t == TipoInimigo::PennywiseIV ? "pennywise_4" : estagioIII(t) ? "pennywise_3" : estagioII(t) ? "pennywise_2" : "pennywise_1";
}
// 8 direções (24/11, Diego: "todos os chefes têm que ter movimentação 360, não só ficar olhando para a tela").
// Folha <medo>_8dir (fase 1) ou <pennywise do estágio>_8dir (fase 2): 5 linhas (baixo, baixo-direita, direita,
// cima-direita, cima) × 5 colunas (parado, passo A, passo B, preparo, ataque). A esquerda é o espelho da direita.
constexpr int LINHA_OCTANTE[8] = {2, 1, 0, 1, 2, 3, 4, 3};
constexpr bool ESPELHO_OCTANTE[8] = {false, false, false, true, true, true, false, false};
int octanteDoRumo(float rumo) { return ((static_cast<int>(std::lround(rumo / 0.7853982f)) % 8) + 8) % 8; }


Sprite spriteDoEstagio(const Recursos& r, TipoInimigo t, const char* sufixo) {
    const Sprite proprio = spriteDe(r, std::string(prefixoDoEstagio(t)) + sufixo);
    return spriteValido(proprio) ? proprio : spriteDe(r, std::string(prefixoBase(t)) + sufixo);
}
const char* spriteFase2(TipoInimigo t) {
    if (t == TipoInimigo::PennywiseIV) return "pennywise_4_actions";   // o IV é sempre o mesmo corpo (Etapa 15)
    return estagioIII(t) ? "pennywise_3_actions" : estagioII(t) ? "pennywise_2_actions" : "pennywise_1_actions";
}
// A aura de cada estágio (forma + pulso, não só cor): cresce e esquenta a cada encontro.
struct AuraEstagio { std::uint32_t cor; float raio; std::uint8_t alfa; };
AuraEstagio auraDoEstagio(TipoInimigo t) {
    switch (t) {
        case TipoInimigo::Mumia: return {rgb(150, 150, 160), 12.0f, 50};       // I: quase nada
        case TipoInimigo::MedoAfogado: return {rgb(150, 30, 45), 16.0f, 90};   // I+: vinho
        case TipoInimigo::Leproso: return {rgb(120, 160, 60), 17.0f, 90};      // II: verde doentio
        case TipoInimigo::RaloSangue: return {rgb(200, 15, 20), 21.0f, 130};   // II+: sangue
        case TipoInimigo::Passaro: return {rgb(255, 120, 40), 21.0f, 110};     // III: brasa
        case TipoInimigo::Lobisomem: return {rgb(120, 40, 180), 24.0f, 130};   // III+: violeta escuro
        default: return {rgb(230, 110, 40), 52.0f, 120};                       // IV: as Luzes Mortas sob a aranha
    }
}
// Folha de ANDAR em 4 direções (Etapa 9b, P159–P166): o medo (<atlas>_walk) na fase 1 e o Pennywise
// do estágio na fase 2. "" apenas quando o chefe é fixo (o Ralo).
std::string folhaAndarChefe(const Inimigo& i) {
    if (i.fase == 1) return i.tipo == TipoInimigo::RaloSangue ? std::string() : std::string(parametrosDe(i.tipo).atlas) + "_walk";
    return estagioIII(i.tipo) ? "pennywise_3_walk" : estagioII(i.tipo) ? "pennywise_2_walk" : "pennywise_1_walk";
}
const char* spriteDispersao(TipoInimigo t) {
    if (t == TipoInimigo::PennywiseIV) return "pennywise_4_defeat";
    return estagioIII(t) ? "pennywise_3_disperse" : estagioII(t) ? "pennywise_2_disperse" : "pennywise_1_disperse";
}

// Chefe-medo (Etapas 8.3 e 10.3). Fase 1: a folha do medo (64×64; linha 0 andar, 1 preparar — no Afogado, afundar —, 2 golpear,
// 3 = o medo se rasgando e o Pennywise saindo). Fase 2: pennywise_1_actions (32×48; linhas
// espera, preparo, ataque). O estado da IA escolhe a linha: o que ele vai fazer se LÊ no corpo.
// O PALHAÇO-ARANHA (Pennywise IV, 24/11; Diego: "algo grande que se move 360 graus"). Duas folhas:
//   * pennywise_4_aranha: abdome e 8 patas vistos de cima, apontando para a DIREITA; desenhado GIRADO por `i.rumo`
//     (que a regra gira suave para onde ele anda ou ataca). Linha 0: caminhada; linha 1: agachar, saltar, cravar, apanhar.
//   * o tronco do palhaço, em pé, encaixado no babado da frente do corpo e VIRADO PARA ONDE A ARANHA APONTA (Diego,
//     24/11: "todos têm que ter movimentação 360"): 8 direções pelo rumo. Para baixo, a folha de ações (12 quadros);
//     nas outras, pennywise_4_tronco (linhas: baixo, baixo-direita, direita, cima-direita, cima; colunas: respira,
//     respira, prepara, ataca), e as 3 da esquerda são o espelho das da direita.
constexpr float ESCALA_ARANHA = 0.95f;
constexpr float FRENTE_ARANHA = 15.0f;   // px do centro do corpo até o babado onde o tronco encaixa

void desenharPalhacoAranha(Canvas& canvas, const Recursos& r, const Inimigo& i, float x, float y, int linha, int coluna,
                           std::uint8_t opacidade, std::uint32_t branco, float ex, float ey, float altura) {
    const Sprite corpo = spriteDe(r, "pennywise_4_aranha");
    const Sprite tronco = spriteDe(r, "pennywise_4_actions");
    const Vec2 frente{std::cos(i.rumo), std::sin(i.rumo)};
    // Patas: andando, a caminhada no ritmo dos passos; parado, respira (as patas quase não mexem).
    int lc = 0, cc = 0;
    if (i.flash > 0.0f) { lc = 1; cc = 3; }
    else if (i.transicao > 0.0f) { lc = 1; cc = 0; }
    else if (i.estado == EstadoIA::Preparando) { lc = 1; cc = 0; }
    else if (i.estado == EstadoIA::Atacando) { lc = 1; cc = 2; }
    else if (i.estado == EstadoIA::Recuperando) { lc = i.timer > 0.4f ? 1 : 0; cc = i.timer > 0.4f ? 2 : 0; }
    else if (i.andando) { cc = static_cast<int>(i.animTempo * 9.0f) % 4; }
    else { cc = static_cast<int>(i.animTempo * 1.5f) % 2 == 0 ? 0 : 2; }
    const float passo = i.andando ? std::fabs(std::sin(i.animTempo * 9.0f * 1.5708f)) : 0.0f;
    const float cx = x, cy = y - 16.0f - altura - 1.5f * passo;   // o centro do corpo (os pés ficam no chão)
    if (spriteValido(corpo)) {
        desenharFrameGirado(canvas, corpo, cc, lc, cx, cy, i.rumo, ESCALA_ARANHA * 0.5f * (ex + ey), opacidade);
    }
    // O tronco no babado da frente, virado para o mesmo lado do corpo (octante do rumo: 0 = direita, 2 = baixo…).
    const float tx = cx + frente.x * FRENTE_ARANHA * ESCALA_ARANHA;
    const float ty = cy + frente.y * FRENTE_ARANHA * ESCALA_ARANHA * 0.6f + 10.0f;
    const int octante = ((static_cast<int>(std::lround(i.rumo / 0.7853982f)) % 8) + 8) % 8;
    const Sprite girado = spriteDe(r, "pennywise_4_tronco");
    if (octante != 2 && spriteValido(girado)) {
        // linha da folha e espelho de cada octante (a esquerda é o espelho da direita)
        int col = static_cast<int>(i.animTempo * 2.0f) % 2;   // respira
        if (i.transicao > 0.0f || i.estado == EstadoIA::Preparando) col = 2;
        else if (i.estado == EstadoIA::Atacando || (i.estado == EstadoIA::Recuperando && i.timer > 0.4f) || linha == 2) col = 3;
        desenharFramePesEstica(canvas, girado, col, LINHA_OCTANTE[octante], tx, ty, ex, ey, ESPELHO_OCTANTE[octante], opacidade, 0xFFFFFF, branco);
        return;
    }
    if (spriteValido(tronco)) desenharFramePesEstica(canvas, tronco, coluna, linha, tx, ty, ex, ey, false, opacidade, 0xFFFFFF, branco);
}

void desenharChefe(Canvas& canvas, const Recursos& r, const Inimigo& i, float camX, float camY) {
    // Apanhou: o corpo treme por um instante (além do branco), para o acerto se ler sem parar o jogo.
    const float tremor = i.flash > 0.0f ? 3.0f * std::sin(i.animTempo * 90.0f) : 0.0f;
    const float x = i.pes.x - camX + tremor, y = i.pes.y - camY;
    if (i.submerso > 0.0f && i.tipo == TipoInimigo::PennywiseIV) {   // o salto de aranha: cai onde Bill estava
        const float t = 1.0f - std::clamp(i.submerso / AVISO_SALTO, 0.0f, 1.0f);
        const float sx = i.alvoRegistrado.x - camX, sy = i.alvoRegistrado.y - camY;
        elipseAlpha(canvas, sx, sy - 2, 14.0f + 26.0f * t, 5.0f + 8.0f * t, rgb(0, 0, 0), static_cast<std::uint8_t>(60 + 120 * t));
        if (t > 0.55f) {
            const float k = (1.0f - t) / 0.45f;   // 1 → 0: descendo
            desenharPalhacoAranha(canvas, r, i, sx, sy, 2, 1, 255, 0, 0.92f, 1.08f, 150.0f * k);
        }
        return;
    }
    if (i.submerso > 0.0f && i.tipo == TipoInimigo::Lobisomem) {
        // No ar (Etapa 14.2): a sombra cresce onde ele vai cair, e no fim ele aparece descendo sobre ela.
        const float t = 1.0f - std::clamp(i.submerso / AVISO_SALTO, 0.0f, 1.0f);
        const float sx = i.alvoRegistrado.x - camX, sy = i.alvoRegistrado.y - camY;
        elipseAlpha(canvas, sx, sy - 2, 8.0f + 16.0f * t, 3.0f + 5.0f * t, rgb(0, 0, 0), static_cast<std::uint8_t>(60 + 110 * t));
        if (t > 0.6f) {
            const Sprite s = spriteDe(r, i.fase == 1 ? parametrosDe(i.tipo).atlas : spriteFase2(i.tipo));
            if (spriteValido(s)) desenharFramePes(canvas, s, 3, 1, sx, sy - 140.0f * (1.0f - t) / 0.4f);
        }
        return;
    }
    if (i.submerso > 0.0f) {   // no fundo: só as bolhas de onde afundou (o círculo do aviso marca onde volta)
        const float fase = std::fmod(i.animTempo * 1.5f, 1.0f);
        elipseAlpha(canvas, x, y - 2, 10.0f + 14.0f * fase, 4.0f + 5.0f * fase, rgb(140, 190, 205), static_cast<std::uint8_t>(150 * (1.0f - fase)));
        return;
    }
    const bool medo = (i.fase == 1 || i.transicao > 0.0f) && i.tipo != TipoInimigo::PennywiseIV;
    // Fase 1: a folha do medo (da tabela); fase 2: o Pennywise do estágio, com folha, tamanho e aura próprios (24/11).
    const Sprite s = medo ? spriteDe(r, parametrosDe(i.tipo).atlas) : spriteDoEstagio(r, i.tipo, "_actions");
    const float escala = medo ? 1.0f : escalaDoPennywise(i.tipo);
    if (!medo) {
        const AuraEstagio a = auraDoEstagio(i.tipo);
        const float pulso = 0.5f + 0.5f * std::sin(i.animTempo * 3.5f);
        elipseAlpha(canvas, x, y - 2, a.raio + 4.0f * pulso, a.raio * 0.42f + 1.5f * pulso, a.cor, static_cast<std::uint8_t>(a.alfa * (0.7f + 0.3f * pulso)));
    }
    if (!spriteValido(s)) return;
    int quantos = 0;
    const PadraoChefe* padroes = padroesDoChefe(i.tipo, quantos);
    const bool mergulhando = medo && i.padrao >= 0 && i.padrao < quantos && padroes[i.padrao].forma == FormaPadrao::Mergulho;
    int linha = 0, coluna = static_cast<int>(i.animTempo * 6.0f) % 4;
    if (i.transicao > 0.0f && i.tipo == TipoInimigo::PennywiseIV) {   // a folha do IV não tem a linha de "rasgar"
        linha = 1;
    } else if (i.transicao > 0.0f) {
        linha = 3;
        coluna = 1 + std::min(2, static_cast<int>((1.0f - i.transicao / DURACAO_TRANSICAO_CHEFE) * 3.0f));
    } else if (i.estado == EstadoIA::Preparando && mergulhando) {   // afundando (medo_afogado, linha 1)
        linha = 1;
        coluna = std::min(3, static_cast<int>((1.0f - i.timer / padroes[i.padrao].preparo) * 4.0f));
    } else if (i.estado == EstadoIA::Preparando) {
        // Linha 1 é a pose "própria" do medo: preparar (Múmia), afundar (Afogado), soltar a nuvem
        // (Leproso). Os outros preparos do Afogado e do Leproso usam a linha 2 (o golpe).
        const bool nuvem = i.padrao >= 0 && i.padrao < quantos && padroes[i.padrao].forma == FormaPadrao::Nuvem;
        // O Ralo usa a linha 1 (os cabelos subindo) em todos os preparos: a leitura é "vai chicotear".
        // O Pássaro também (13.3): as brasas acendendo no corpo são o aviso de qualquer golpe dele.
        // O Lobisomem (14.2) agacha na linha 1 para o salto e o uivo; as garras armam na linha 2.
        const bool agacha = i.tipo == TipoInimigo::Lobisomem && i.padrao >= 0 && i.padrao < quantos &&
                            (padroes[i.padrao].forma == FormaPadrao::Salto || padroes[i.padrao].forma == FormaPadrao::Invocar);
        linha = (medo && i.tipo != TipoInimigo::Mumia && i.tipo != TipoInimigo::RaloSangue && i.tipo != TipoInimigo::Passaro && !nuvem &&
                 !agacha && i.tipo != TipoInimigo::PennywiseIV) ? 2 : 1;
    } else if (i.estado == EstadoIA::Atacando || i.estado == EstadoIA::Recuperando) {
        linha = 2;
        if (i.estado == EstadoIA::Recuperando) coluna = 3;   // parado, de guarda baixa: a hora de bater
    } else if (!i.andando) {
        coluna = static_cast<int>(i.animTempo * 2.0f) % 4;  // respira devagar
    }
    std::uint8_t opacidade = i.surgindo > 0.0f ? static_cast<std::uint8_t>(255.0f * (1.0f - i.surgindo / 1.6f)) : 255;
    // Luzes Mortas: fora da janela de punição ele é sombra (não pode ser atingido); na recuperação, sólido.
    if (i.tipo == TipoInimigo::PennywiseIV && i.fase == 3 && i.estado != EstadoIA::Recuperando) opacidade = 120;
    if (medo && i.tipo == TipoInimigo::RaloSangue) {
        // O ralo transbordando: um halo de sangue que pulsa sob a bacia. Sem ele, o cabelo escuro sumia
        // no piso escuro dos Porões (1ª captura da 12.3).
        const float pulso = 0.75f + 0.25f * std::sin(i.animTempo * 2.5f);
        elipseAlpha(canvas, x, y - 4, 34.0f, 13.0f, rgb(110, 10, 16), static_cast<std::uint8_t>(opacidade * 0.8f));
        elipseAlpha(canvas, x, y - 4, 26.0f * pulso, 9.0f * pulso, rgb(190, 30, 36), static_cast<std::uint8_t>(opacidade * 0.45f));
    }
    const bool grandao = medo || i.tipo == TipoInimigo::PennywiseIV;   // 64×64: sombra larga
    const bool aranha = i.tipo == TipoInimigo::PennywiseIV;   // o palhaço-aranha cobre ~4 células
    elipseAlpha(canvas, x, y - (aranha ? 14.0f : 2.0f), aranha ? 46.0f : (grandao ? 22.0f : 11.0f * escala),
                aranha ? 26.0f : (grandao ? 7.0f : 4.0f * escala), rgb(0, 0, 0), static_cast<std::uint8_t>(opacidade / 2));
    // O Pássaro VOA (13.3): a sombra fica no chão (os pés: onde ele é atingido) e o corpo flutua acima,
    // subindo e descendo com o bater das asas. Sem a altura, ele parecia andar como os outros medos.
    float altura = 0.0f;
    if (medo && i.tipo == TipoInimigo::Passaro && i.transicao <= 0.0f) {
        altura = 10.0f + 3.0f * std::sin(i.animTempo * 6.0f);
        elipseAlpha(canvas, x, y - 2, 26.0f, 8.0f, rgb(0, 0, 0), static_cast<std::uint8_t>(opacidade / 3));
    }
    const std::uint32_t branco = i.flash > 0.0f ? static_cast<std::uint32_t>(230.0f * std::min(1.0f, i.flash / 0.1f)) : 0;
    // Corpo vivo (Diego, 24/11: "os chefões estão muito duros em tela"): respira parado, agacha no preparo (quanto
    // mais perto do golpe, mais baixo), estica no bote, balança na passada e encolhe ao apanhar.
    float ex = 1.0f, ey = 1.0f, bote = 0.0f;
    if (i.transicao <= 0.0f) {
        if (i.estado == EstadoIA::Preparando) {
            float preparo = 0.6f;
            if (padroes && i.padrao >= 0 && i.padrao < quantos) preparo = std::max(0.1f, padroes[i.padrao].preparo);
            const float k = std::clamp(1.0f - i.timer / preparo, 0.0f, 1.0f);
            ex = 1.0f + 0.07f * k;
            ey = 1.0f - 0.09f * k;
        } else if (i.estado == EstadoIA::Atacando) {
            ex = 0.94f;
            ey = 1.07f;
            bote = 4.0f;
        } else if (i.estado == EstadoIA::Recuperando) {
            const float onda = std::sin(i.animTempo * 9.0f) * std::exp(-std::max(0.0f, 0.6f - i.timer) * 3.0f);
            ex = 1.0f + 0.03f * onda;
            ey = 1.0f - 0.03f * onda;
        } else if (!i.andando) {
            const float resp = std::sin(i.animTempo * 2.4f);
            ex = 1.0f - 0.012f * resp;
            ey = 1.0f + 0.022f * resp;
        }
        if (i.flash > 0.0f) { ex *= 1.05f; ey *= 0.95f; }
    }
    const Vec2 rumo = normalizar(subtrair(i.alvoRegistrado, i.pes));
    const float xb = x + rumo.x * bote, yb = y + rumo.y * bote * 0.5f;
    if (i.tipo == TipoInimigo::PennywiseIV) {
        desenharPalhacoAranha(canvas, r, i, xb, yb, linha, coluna, opacidade, branco, ex, ey, 0.0f);
        return;
    }
    // Andando (perseguir, buscar, vagar): a folha de andar na direção do movimento — o chefe vira
    // para onde vai e as pernas (ou as asas) se movem. Os golpes continuam na folha de ações, de frente.
    // Em 8 direções: vira para onde anda e, preparando ou golpeando, para onde vai o golpe (i.rumo, girado pela regra).
    // As poses próprias (o medo se rasgando, o Afogado afundando) continuam nas folhas antigas.
    if (i.transicao <= 0.0f && !mergulhando) {
        Sprite oito = medo ? spriteDe(r, std::string(parametrosDe(i.tipo).atlas) + "_8dir")
                           : spriteDe(r, std::string(prefixoDoEstagio(i.tipo)) + "_8dir");
        if (!medo && !spriteValido(oito)) oito = spriteDe(r, std::string(prefixoBase(i.tipo)) + "_8dir");
        if (spriteValido(oito)) {
            const int oct = octanteDoRumo(i.rumo);
            const bool voa = medo && i.tipo == TipoInimigo::Passaro;
            const bool anda = (i.estado == EstadoIA::Perseguindo || i.estado == EstadoIA::Buscando || i.estado == EstadoIA::Vagando) &&
                              (i.andando || voa);
            int col = 0;
            float sobe = 0.0f;
            if (i.estado == EstadoIA::Preparando) col = 3;
            else if (i.estado == EstadoIA::Atacando) col = 4;
            else if (i.estado == EstadoIA::Recuperando) col = i.timer > 0.4f ? 4 : 0;
            else if (anda) {
                const float fps = voa ? 7.0f : (medo ? 6.0f : 10.0f);
                static const int CICLO[4] = {0, 1, 0, 2};   // parado, passo A, parado, passo B
                col = CICLO[static_cast<int>(i.animTempo * fps) % 4];
                sobe = voa ? 0.0f : 1.6f * std::fabs(std::sin(i.animTempo * fps * 1.5708f));
            }
            desenharFramePesEstica(canvas, oito, col, LINHA_OCTANTE[oct], xb, yb - altura - sobe, escala * ex, escala * ey,
                                   ESPELHO_OCTANTE[oct], opacidade, 0xFFFFFF, branco);
            return;
        }
    }
    const bool deslocando = i.transicao <= 0.0f && (i.estado == EstadoIA::Perseguindo || i.estado == EstadoIA::Buscando ||
                                                    i.estado == EstadoIA::Vagando);
    const bool voando = medo && i.tipo == TipoInimigo::Passaro;
    if (deslocando && (i.andando || voando)) {
        const Sprite andar = medo ? spriteDe(r, folhaAndarChefe(i)) : spriteDoEstagio(r, i.tipo, "_walk");
        if (spriteValido(andar)) {
            const int dir = static_cast<int>(i.direcao);
            const float fps = voando ? 7.0f : (medo ? 6.0f : 10.0f);
            const Animacao anim{dir, 0, 4, fps, true};
            // A passada pesa: o corpo sobe e desce a cada passo e alarga um pouco ao pisar.
            const float passo = std::fabs(std::sin(i.animTempo * fps * 1.5708f));
            const float sobe = voando ? 0.0f : 1.6f * passo;
            desenharFramePesEstica(canvas, andar, colunaAnimacao(anim, i.animTempo), dir, x, y - altura - sobe,
                                   escala * (1.0f + 0.025f * (1.0f - passo)), escala * (1.0f - 0.02f * (1.0f - passo)), false,
                                   opacidade, 0xFFFFFF, branco);
            return;
        }
    }
    desenharFramePesEstica(canvas, s, coluna, linha, xb, yb - altura, escala * ex, escala * ey, false, opacidade, 0xFFFFFF, branco);
}

// Nuvem pútrida do Leproso: névoa verde-pálida translúcida (forma: manchas que ondulam), por cima
// dos atores; some no último segundo.
void desenharNuvens(Canvas& canvas, const EstadoPartida& p, float tempo, float camX, float camY) {
    for (const NuvemPutrida& n : p.nuvens) {
        const float vida = std::min(1.0f, n.vida);
        for (int k = 0; k < 6; ++k) {
            const float a = 1.047f * static_cast<float>(k) + tempo * 0.6f;
            const float x = n.centro.x + 0.45f * n.raio * std::cos(a) - camX;
            const float y = n.centro.y + 0.30f * n.raio * std::sin(a) - camY - 6.0f;
            elipseAlpha(canvas, x, y, 0.55f * n.raio, 0.35f * n.raio, rgb(185, 205, 120), static_cast<std::uint8_t>(70 * vida));
        }
        // Base mais forte e uma borda clara: a nuvem DEIXA LENTO, então tem de ser lida no escuro
        // (na 1ª captura ela quase sumia).
        elipseAlpha(canvas, n.centro.x - camX, n.centro.y - camY, n.raio, 0.55f * n.raio, rgb(140, 165, 85), static_cast<std::uint8_t>(95 * vida));
        for (int k = 0; k < 16; ++k) {
            const float a = 0.3927f * static_cast<float>(k) + tempo * 0.4f;
            elipseAlpha(canvas, n.centro.x + n.raio * std::cos(a) - camX, n.centro.y + 0.55f * n.raio * std::sin(a) - camY, 2.2f, 1.4f,
                        rgb(215, 235, 150), static_cast<std::uint8_t>(150 * vida));
        }
    }
}

// Festa dos Balões: o sprite do balão tingido (amarelo, azul, verde); o vermelho é o original,
// com a cara do mal (armadilha). Uma corda fina embaixo.
void desenharBaloesDaFesta(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    const Sprite s = spriteDe(r, "balao");
    if (!spriteValido(s)) return;
    static const std::uint32_t CORES[3] = {0xFFD84A, 0x5AA8FF, 0x6AD36A};
    for (const BalaoFesta& b : p.baloes) {
        const float x = b.pos.x + 3.0f * std::sin(b.fase) - camX, y = b.pos.y - camY;
        retanguloAlpha(canvas, x, y - 6, 1, 8, rgb(220, 220, 220), 200);   // corda
        if (b.vermelho) desenharFramePes(canvas, s, 0, 0, x, y - 4);
        else desenharFramePesTinta(canvas, s, 0, 0, x, y - 4, false, 255, CORES[b.cor % 3], 200);
    }
}

// Covil (Etapa 15): a ilusão de Georgie (sempre à vista: ela CHAMA) e as Luzes Mortas, por cima da escuridão.
void desenharCovil(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    if (p.luzesMortas) {
        const Sprite luz = spriteDe(r, "fx_luzes_mortas");
        for (int i = 0; i < p.quantasLuzes; ++i) {
            const Vec2 c = centroLuzMorta(p, i);
            const float x = c.x - camX, y = c.y - camY;
            const float pulso = 0.85f + 0.15f * std::sin(tempo * 3.0f + static_cast<float>(i));
            elipseAlpha(canvas, x, y, RAIO_LUZ_MORTA * pulso, RAIO_LUZ_MORTA * 0.62f * pulso, rgb(255, 214, 140), 46);
            elipseAlpha(canvas, x, y, RAIO_LUZ_MORTA * 0.6f, RAIO_LUZ_MORTA * 0.38f, rgb(255, 236, 190), 40);
            if (spriteValido(luz)) desenharFramePes(canvas, luz, static_cast<int>(tempo * 6.0f + static_cast<float>(i)) % 4, 0, x, y + 28.0f);
        }
    }
    if (p.georgie.ativo) {
        const Sprite s = spriteDe(r, "georgie_walk");
        if (!spriteValido(s)) return;
        const float x = p.georgie.pes.x - camX, y = p.georgie.pes.y - camY;
        elipseAlpha(canvas, x, y - 2, 12.0f, 5.0f, rgb(255, 220, 90), 70);   // o brilho amarelado da ilusão
        const int dir = static_cast<int>(p.georgie.direcao);
        const Animacao anim{dir, 0, 4, 6.0f, true};
        desenharFramePesTinta(canvas, s, colunaAnimacao(anim, p.georgie.anim), dir, x, y, false, 225, 0xFFE07A, 40);
    }
}

void desenharDispersoes(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    for (const Dispersao& d : p.dispersoes) {
        const Sprite s = spriteDe(r, spriteDispersao(d.chefe));
        if (!spriteValido(s)) continue;
        const float t = 1.0f - d.vida / DURACAO_DISPERSAO;
        const int coluna = std::min(3, static_cast<int>(t * 4.0f));
        desenharFramePes(canvas, s, coluna, 0, d.pos.x - camX, d.pos.y - camY, false,
                         static_cast<std::uint8_t>(255.0f * std::min(1.0f, d.vida / 0.4f)));
    }
}

// Barra do chefe (PRD §15.2): nome e vida; a marca dos 50% avisa a troca de fase.
void desenharBarraDoChefe(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    const Inimigo* ch = chefeNaSala(p);
    if (!ch || ch->surgindo > 0.0f) return;
    const float w = 300, x = LARGURA_LOGICA / 2.0f - w / 2, y = 52;
    desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, y - 19, paraWide(nomeDoChefe(*ch)),
                              ch->fase == 1 ? rgb(230, 214, 160) : rgb(255, 120, 110));
    const float fracao = static_cast<float>(std::max(0, ch->hp)) / static_cast<float>(std::max(1, ch->hpMax));
    retanguloAlpha(canvas, x, y, w, 9, rgb(8, 10, 16), 210);
    retanguloCheio(canvas, x + 1, y + 1, (w - 2) * fracao, 7, ch->fase == 1 ? rgb(196, 170, 96) : rgb(200, 40, 48));
    contornoRetangulo(canvas, x, y, w, 9, rgb(150, 140, 120));
    if (ch->tipo == TipoInimigo::PennywiseIV) {   // as duas viradas do Covil: Georgie (60%) e Luzes Mortas (25%)
        if (ch->fase <= 1) retanguloCheio(canvas, x + w * 0.60f, y - 2, 1, 13, rgb(255, 230, 200));
        if (ch->fase <= 2) retanguloCheio(canvas, x + w * 0.25f, y - 2, 1, 13, rgb(255, 230, 200));
    } else if (ch->fase == 1) {
        retanguloCheio(canvas, x + w / 2, y - 2, 1, 13, rgb(255, 230, 200));   // aqui o medo se rasga
    }
}

// Cabelo do ralo (Etapa 12.2): torreta sem direção. Recolhido = o ralo quase vazio (linha 3, col. 0);
// preparando = os cabelos subindo (linha 3, col. 1→3); chicoteando = linha 1; de pé = linha 0.
void desenharCabelo(Canvas& canvas, const Recursos& r, const Inimigo& i, float camX, float camY) {
    const Sprite s = spriteDe(r, "cabelo_ralo");
    if (!spriteValido(s)) return;
    int linha = 0, coluna = static_cast<int>(i.animTempo * 5.0f) % 4;
    if (i.submerso > 0.0f) { linha = 3; coluna = 0; }
    else if (i.estado == EstadoIA::Preparando) { linha = 3; coluna = 1 + std::min(2, static_cast<int>((1.0f - i.timer / 0.7f) * 3.0f)); }
    else if (i.estado == EstadoIA::Atacando) { linha = 1; }
    const std::uint8_t opacidade = i.surgindo > 0.0f ? static_cast<std::uint8_t>(255.0f * (1.0f - i.surgindo / 0.7f)) : 255;
    const std::uint32_t branco = i.flash > 0.0f ? static_cast<std::uint32_t>(230.0f * std::min(1.0f, i.flash / 0.1f)) : 0;
    desenharFramePesTinta(canvas, s, coluna, linha, i.pes.x - camX, i.pes.y - camY + 6.0f, false, opacidade, 0xFFFFFF, branco);
}

void desenharInimigo(Canvas& canvas, const Recursos& r, const Inimigo& i, float camX, float camY) {
    if (i.tipo == TipoInimigo::Cabelo) { desenharCabelo(canvas, r, i, camX, camY); return; }
    if (i.tipo == TipoInimigo::Espectro && i.teleporte > 0.0f) {
        // Teleporte (13.2): a fuligem se desfaz onde ele estava e se ADENSA onde vai aparecer — o
        // aviso de 0,5 s. Manchas cinzentas girando; a do destino cresce e escurece.
        const float t = 1.0f - i.teleporte / AVISO_TELEPORTE;   // 0 → 1
        for (int lado = 0; lado < 2; ++lado) {
            const Vec2 c = lado == 0 ? i.pes : i.destinoTeleporte;
            const float forca = lado == 0 ? 1.0f - t : 0.35f + 0.65f * t;
            for (int k = 0; k < 6; ++k) {
                const float a = 1.047f * static_cast<float>(k) + i.animTempo * 3.0f * (lado == 0 ? 1.0f : -1.0f);
                const float raio = 6.0f + 10.0f * (lado == 0 ? t : 1.0f - 0.5f * t);
                // Bordas claras (fuligem iluminada) por cima de um miolo escuro: no piso escuro da
                // Siderúrgica, a fumaça só cinza-escura não se lia (1ª captura da 13.2).
                const float fx = c.x + raio * std::cos(a) - camX, fy = c.y - 14.0f + 0.6f * raio * std::sin(a) - camY;
                elipseAlpha(canvas, fx, fy, 8.0f, 6.0f, rgb(40, 36, 34), static_cast<std::uint8_t>(150 * forca));
                elipseAlpha(canvas, fx - 1.0f, fy - 1.5f, 5.5f, 3.5f, rgb(165, 155, 145), static_cast<std::uint8_t>(170 * forca));
            }
            elipseAlpha(canvas, c.x - camX, c.y - 14.0f - camY, 7.0f, 6.0f, rgb(255, 140, 50), static_cast<std::uint8_t>(150 * forca));
            elipseAlpha(canvas, c.x - camX, c.y - 2.0f - camY, 14.0f, 5.0f, rgb(255, 120, 40), static_cast<std::uint8_t>(120 * forca));
        }
        return;
    }
    const ParametrosInimigo& p = parametrosDe(i.tipo);
    if (p.chefe) {
        desenharChefe(canvas, r, i, camX, camY);
        return;
    }
    const float x = i.pes.x - camX, y = i.pes.y - camY;
    int coluna = 0;
    if (i.estado == EstadoIA::Preparando || i.estado == EstadoIA::Atacando) coluna = 3;
    else if (i.andando) coluna = 1 + static_cast<int>(i.animTempo * 8.0f) % 2;
    // Surgindo: aparece aos poucos (o spawn é visível e justo, nunca "do nada").
    const std::uint8_t opacidade = i.surgindo > 0.0f ? static_cast<std::uint8_t>(255.0f * (1.0f - i.surgindo / 0.7f)) : 255;
    elipseAlpha(canvas, x, y - 1, 9, 3.5f, rgb(0, 0, 0), static_cast<std::uint8_t>(opacidade / 2));
    const float flutuar = p.voa ? 3.0f * std::sin(i.animTempo * 3.0f) - 4.0f : 0.0f;
    // Acerto: o sprite fica BRANCO por um instante (tinta), sem sumir nem piscar.
    const std::uint32_t branco = i.flash > 0.0f ? static_cast<std::uint32_t>(230.0f * std::min(1.0f, i.flash / 0.1f)) : 0;
    if (i.aliado > 0.0f) {   // Imitação (Richie): aura dourada no chão + tinta dourada (forma + cor)
        const float pulso = 0.5f + 0.5f * std::sin(i.animTempo * 6.0f);
        elipseAlpha(canvas, x, y - 1, 15.0f + 3.0f * pulso, 6.0f + 1.5f * pulso, rgb(255, 205, 90), static_cast<std::uint8_t>(120 * opacidade / 255));
    }
    if (i.elite) {   // Assombrado (PRD §9): aura violeta pulsando no chão (forma + cor: não depende só da cor)
        const float pulso = 0.5f + 0.5f * std::sin(i.animTempo * 4.0f);
        elipseAlpha(canvas, x, y - 1, 15.0f + 3.0f * pulso, 6.0f + 1.5f * pulso, rgb(150, 70, 220), static_cast<std::uint8_t>(110 * opacidade / 255));
    }
    const bool aliado = i.aliado > 0.0f && branco == 0;
    desenharFramePesTinta(canvas, spriteDe(r, p.atlas), coluna, static_cast<int>(i.direcao), x, y + flutuar, false, opacidade,
                          aliado ? 0xFFD070 : (i.elite && branco == 0 ? 0xB070FF : 0xFFFFFF),
                          aliado ? 80 : (i.elite && branco == 0 ? 70 : branco));
    if (i.itemCarregado >= 0) {   // rato ladrão: o item na boca, balançando
        const float balanco = 1.5f * std::sin(i.animTempo * 14.0f);
        desenharIcone(canvas, r, static_cast<TipoItem>(i.itemCarregado), x - 8.0f + balanco, y - p.alturaCorpo - 12.0f);
    }
    if (i.atordoado > 0.0f) {   // atordoado pelo clarão: estrelinhas girando
        for (int k = 0; k < 3; ++k) {
            const float a = i.animTempo * 6.0f + static_cast<float>(k) * 2.094f;
            elipseAlpha(canvas, x + 8.0f * std::cos(a), y - p.alturaCorpo - 6.0f + 3.0f * std::sin(a), 1.6f, 1.6f,
                        rgb(255, 230, 120), 230);
        }
    }
}

// Companheiro (SDD §24.6). Anda em 4 direções com a folha <amigo>_walk (Etapa 9b, P151–P156: o Diego
// viu os amigos "se arrastando" de frente); parado e de frente, respira com o _idle. Sem a folha, o
// _idle com um balanço de passo. Ben: barreira de tijolos em volta de Bill. Beverly: traço do
// estilingue. O resto (luz do Mike, armadilhas do Stan, provocação do Richie) fica no mundo.
void desenharCompanheiro(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    const Companheiro& c = p.companheiro;
    if (!c.ativo) return;
    const Sprite s = spriteDe(r, std::string(codigoAmigo(c.quem)) + "_idle");
    float x = c.pes.x - camX, y = c.pes.y - camY;
    // Arremesso/poder (24/11): um pulinho na direção do alvo. Com medo: treme e o ícone de medo sobre a cabeça.
    if (c.ataque.ativo && c.ataque.tempo < 0.25f) {
        const Vec2 rumo = normalizar(subtrair(c.ataque.para, c.ataque.de));
        const float k = std::sin(c.ataque.tempo / 0.25f * 3.14159f);
        x += rumo.x * 4.0f * k;
        y += rumo.y * 2.0f * k - 3.0f * k;
    }
    if (c.comMedo) {
        x += std::sin(tempo * 47.0f) * 0.9f;
        if (const Sprite m = spriteDe(r, "fx_medo"); spriteValido(m)) {
            const float meio = static_cast<float>(m.frameW) / (2.0f * static_cast<float>(canvas.escala));
            desenharFrame(canvas, m, static_cast<int>(tempo * 6.0f) % std::max(1, m.colunas), 0, x - meio, y - 62.0f - meio);
        }
    }
    elipseAlpha(canvas, x, y - 2, 8, 3.5f, rgb(0, 0, 0), 110);
    const Sprite andar = spriteDe(r, std::string(codigoAmigo(c.quem)) + "_walk");
    if (spriteValido(andar) && (c.andando || c.direcao != Direcao::Baixo || c.emLuta || !spriteValido(s))) {
        const int linha = static_cast<int>(c.direcao);
        const Animacao anim{linha, 0, 4, 8.0f, true};
        if (c.andando) {
            desenharFramePes(canvas, andar, colunaAnimacao(anim, c.tempoAnimacao), linha, x, y);
        } else {   // parado: respira (e, em luta, fica em guarda, com o peso mudando de um pé para o outro)
            const float resp = std::sin(c.relogio * (c.emLuta ? 5.0f : 2.4f));
            desenharFramePesEstica(canvas, andar, c.emLuta && resp > 0.6f ? 1 : 0, linha, x, y - (c.emLuta ? 0.8f * std::fabs(resp) : 0.0f),
                                   1.0f - 0.012f * resp, 1.0f + 0.022f * resp, false, 255, 0xFFFFFF, 0);
        }
        return;
    }
    const float passo = c.andando ? -std::fabs(std::sin(c.tempoAnimacao * 10.0f)) * 2.5f : 0.0f;
    if (spriteValido(s)) {
        const int quadro = static_cast<int>(tempo * 1.6f) % std::max(1, s.colunas);
        desenharFramePes(canvas, s, quadro, 0, x, y + passo);
    }
}

void desenharEfeitosDoCompanheiro(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    const Companheiro& c = p.companheiro;
    if (!c.ativo) return;
    if (barreiraAtiva(p)) {   // Ben: anel de tijolos girando em volta de Bill (sprites; nada de retângulo: 24/11)
        const float bx = p.bill.pes.x - camX, by = p.bill.pes.y - camY;
        const Sprite tijolo = spriteDe(r, "fx_barreira_ben");
        for (int k = 0; k < 10; ++k) {
            const float a = static_cast<float>(k) * 0.6283f + tempo * 0.8f;
            const float tx = bx + 24.0f * std::cos(a), ty = by - 14.0f + 14.0f * std::sin(a);
            if (spriteValido(tijolo)) {
                desenharFrameGirado(canvas, tijolo, (k + static_cast<int>(tempo * 5.0f)) % std::max(1, tijolo.colunas), 0, tx, ty,
                                    a * 0.5f, 0.55f);
            } else {
                elipseAlpha(canvas, tx, ty, 4.0f, 3.0f, rgb(150, 70, 50), 220);
            }
        }
    }
    if (c.ataque.ativo) {   // Arma (voa girando até o alvo) e poder (surge no alvo): sprites da lore de cada amigo
        const std::string quem = codigoAmigo(c.quem);
        const AtaqueDoAmigo& a = c.ataque;
        if (!a.poder && !a.resolvido) {
            const Sprite arma = spriteDe(r, "fx_arma_" + quem);
            const float k = std::clamp(a.tempo / std::max(0.01f, a.duracao), 0.0f, 1.0f);
            const Vec2 q = somar(a.de, escalar(subtrair(a.para, a.de), k));
            const float arco = -14.0f * std::sin(k * 3.14159f);   // sobe e desce (arremesso)
            if (spriteValido(arma)) {
                // Os 4 quadros já são o objeto girando (0°, 90°, 180°, 270°): só os percorre, sem girar por cima.
                desenharFrameGirado(canvas, arma, static_cast<int>(a.tempo * 20.0f) % arma.colunas, 0, q.x - camX, q.y + arco - camY,
                                    0.0f, 0.75f);
            }
        } else if (a.poder || a.resolvido) {
            const Sprite fx = spriteDe(r, a.poder ? "fx_poder_" + quem : "fx_impacto");
            const float inicio = a.poder ? 0.0f : a.duracao;
            const float total = a.poder ? a.duracao + EFEITO_ATAQUE_AMIGO : EFEITO_ATAQUE_AMIGO;
            const float k = std::clamp((a.tempo - inicio) / total, 0.0f, 0.999f);
            if (spriteValido(fx) && a.tempo >= inicio) {
                const float meio = static_cast<float>(fx.frameW) / (2.0f * static_cast<float>(canvas.escala));
                const float meioH = static_cast<float>(fx.frameH) / (2.0f * static_cast<float>(canvas.escala));
                desenharFrame(canvas, fx, static_cast<int>(k * static_cast<float>(fx.colunas)), 0, a.para.x - meio - camX,
                              a.para.y - meioH - camY);
            }
        }
    }
    if (const IdEntidade id = itemMarcado(p)) {   // Mike: o item mais perto ganha um círculo no chão
        for (const ItemNoChao& it : p.itensChao) {
            if (it.id != id) continue;
            const float pulso = 0.5f + 0.5f * std::sin(tempo * 5.0f);
            elipseAlpha(canvas, it.pos.x - camX, it.pos.y + 2 - camY, 11 + 2 * pulso, 4.5f + pulso, rgb(120, 200, 255),
                        static_cast<std::uint8_t>(70 + 60 * pulso));
        }
    }
    if (revelaArmadilhas(p)) {   // Stan ("Olho atento"): um gaio-azul pousado em cada armadilha na luz
        // 23/11 era um brilho âmbar e, antes, um retângulo amarelo; Diego não entendia o que queriam dizer (24/11).
        // O passarinho diz quem avisou (o Stan, observador de pássaros) e onde. Pousado, chama e pula; quando o
        // Bill chega perto, levanta voo e paira mais alto e mais transparente, e volta a pousar quando ele sai.
        const Sprite passaro = spriteDe(r, "fx_passaro_stan");
        for (int l = 0; l < p.sala.lins; ++l) {
            for (int col = 0; col < p.sala.cols; ++col) {
                if (!stanApontaArmadilha(p, col, l)) continue;
                const float cx = static_cast<float>(col * TILE + TILE / 2), cy = static_cast<float>(l * TILE + TILE / 2);
                const float perto = std::clamp(1.0f - (comprimento(subtrair(p.bill.pes, Vec2{cx, cy})) - 0.6f * TILE) / (1.4f * TILE), 0.0f, 1.0f);
                const float fase = static_cast<float>(col * 7 + l * 3) * 0.37f;   // cada um no seu ritmo
                if (!spriteValido(passaro)) {
                    elipseAlpha(canvas, cx - camX, cy - camY, 10.0f, 7.0f, rgb(80, 150, 255), 70);
                    continue;
                }
                int quadro;
                float altura;
                if (perto > 0.05f) {   // voando: bate as asas (3 e 4)
                    quadro = 2 + static_cast<int>(tempo * 12.0f + fase) % 2;
                    altura = 6.0f + 18.0f * perto;
                } else {               // pousado: chama e dá pulinhos (1 e 2)
                    quadro = std::fmod(tempo * 1.6f + fase, 1.0f) < 0.75f ? 0 : 1;
                    altura = quadro == 1 ? 3.0f : 0.0f;
                }
                const bool espelhar = std::fmod(tempo * 0.25f + fase, 1.0f) < 0.5f;
                const auto opacidade = static_cast<std::uint8_t>(255.0f * (1.0f - 0.55f * perto));
                elipseAlpha(canvas, cx - camX, cy + 4 - camY, 5.0f - 2.0f * perto, 2.0f, rgb(0, 0, 0),
                            static_cast<std::uint8_t>(80.0f * (1.0f - perto)));   // sombra no chão
                desenharFramePesEstica(canvas, passaro, quadro, 0, cx - camX, cy + 4 - altura - camY, 1.0f, 1.0f, espelhar,
                                       opacidade, 0, 0);
            }
        }
    }
    for (const Inimigo& i : p.inimigos) {   // Richie: o provocado ganha um alvo laranja
        if (!provocado(p, i.id) || !estaVisivel(p.visao, i.pes)) continue;
        desenharTextoCentralizado(canvas, r.fonteMedia, i.pes.x - camX, i.pes.y - parametrosDe(i.tipo).alturaCorpo - 26 - camY, L"◎",
                                  rgb(255, 160, 60));
    }
}

void desenharAtores(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempoTotal, float camX, float camY) {
    std::vector<Desenhavel> lista{{p.bill.pes.y, TipoDesenho::Bill, 0}};
    if (p.companheiro.ativo) lista.push_back({p.companheiro.pes.y, TipoDesenho::Companheiro, 0});
    for (int i = 0; i < static_cast<int>(p.inimigos.size()); ++i) {
        // Todos os inimigos são desenhados, na luz ou não (Diego, 24/11: "os inimigos atrás de nós somem e aparecem
        // de novo sem terem saído da tela... melhor nem desaparecerem"). Antes só entrava quem estava numa célula com
        // luz, e na borda da lanterna o inimigo piscava. Agora a ESCURIDÃO (desenhada por cima) é que os esconde: longe
        // da lanterna eles escurecem junto com o chão, sem sumir de repente. As regras (piloto, amigos) não mudam.
        const Inimigo& ini = p.inimigos[static_cast<std::size_t>(i)];
        // O Espectro em fumaça é ordenado pelo DESTINO: a fumaça que se adensa ali é o aviso do teleporte.
        lista.push_back({ini.teleporte > 0.0f ? ini.destinoTeleporte.y : ini.pes.y, TipoDesenho::Inimigo, i});
    }
    for (int i = 0; i < static_cast<int>(p.sala.objetos.size()); ++i) {
        const ObjetoSala& o = p.sala.objetos[static_cast<std::size_t>(i)];
        // Caixa quebrada (entulho) e alçapão ficam no chão: sempre atrás de quem passa por cima.
        const float y = objetoNoPiso(o) ? -1.0f : static_cast<float>((o.celula.lin + 1) * TILE) - 2.0f;
        lista.push_back({y, TipoDesenho::Objeto, i});
    }
    for (int i = 0; i < static_cast<int>(p.itensChao.size()); ++i) {
        lista.push_back({p.itensChao[static_cast<std::size_t>(i)].pos.y - 6.0f, TipoDesenho::Item, i});
    }
    std::stable_sort(lista.begin(), lista.end(), [](const Desenhavel& a, const Desenhavel& b) { return a.y < b.y; });
    for (const Desenhavel& d : lista) {
        const std::size_t i = static_cast<std::size_t>(d.indice);
        switch (d.tipo) {
            case TipoDesenho::Bill:
                desenharBill(canvas, r, p.bill, p.inventario, tempoTotal, camX, camY,
                             chefeNaSala(p) && chefeNaSala(p)->tipo == TipoInimigo::PennywiseIV);
                desenharCorteDoTaco(canvas, r, p.bill, camX, camY);
                break;
            case TipoDesenho::Inimigo: {
                const Inimigo& ini = p.inimigos[i];
                desenharInimigo(canvas, r, ini, camX, camY);
                if (ini.queimando > 0.0f && ini.submerso <= 0.0f) {   // Eco "Fogo controlado": chamas no corpo
                    const Sprite fogo = spriteDe(r, "fx_fogo");
                    const Retangulo h = hurtboxInimigo(ini);
                    if (spriteValido(fogo))
                        desenharFrame(canvas, fogo, static_cast<int>(tempoTotal * 12.0f + static_cast<float>(ini.id)) % 4, 0,
                                      h.x + h.w / 2.0f - 16.0f - camX, h.y + h.h / 2.0f - 22.0f - camY);
                }
                break;
            }
            case TipoDesenho::Objeto: desenharObjeto(canvas, r, p, p.sala.objetos[i], tempoTotal, camX, camY); break;
            case TipoDesenho::Item: desenharItemChao(canvas, r, p.itensChao[i], tempoTotal, camX, camY); break;
            case TipoDesenho::Companheiro: desenharCompanheiro(canvas, r, p, tempoTotal, camX, camY); break;
        }
    }
}

// "E: abrir" sobre o baú ao alcance (ou o motivo de não abrir).
void desenharDicaInteracao(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    if (p.pendencia != Pendencia::Nenhuma) return;   // com menu ou diálogo aberto, "E: …" não vale
    if (const ObjetoSala* o = objetoDoClubeProximo(p)) {
        // Enquanto o amigo fala, o balão ocupa o lugar da dica.
        if (p.balao.vida > 0.0f && p.balao.objeto == static_cast<int>(o - p.sala.objetos.data())) return;
        std::wstring msg;
        switch (o->codigo) {
            case 'M': msg = ajuda("{INTERAGIR}: Mural dos Perdedores (talentos)"); break;
            case 'Q': msg = ajuda("{INTERAGIR}: quadro de recordes"); break;
            case 'H': msg = ajuda("{INTERAGIR}: descer pelo alçapão (começar a noite)"); break;
            default:
                msg = ajuda("{INTERAGIR}: falar com ") +
                      paraWide(nomeDoAmigo(p.sala.modelo->npcs[static_cast<std::size_t>(o->codigo - '0')]));
                break;
        }
        const float x = static_cast<float>(o->celula.col * TILE + TILE / 2) - camX;
        const float y = static_cast<float>(o->celula.lin * TILE) - (o->codigo >= '1' && o->codigo <= '9' ? 40.0f : 22.0f) - camY;
        const float w = 5.4f * static_cast<float>(msg.size()) + 12;
        const float xc = limitar(x, w / 2 + 4, LARGURA_LOGICA - w / 2 - 4);
        retanguloAlpha(canvas, xc - w / 2, y - 1, w, 14, rgb(8, 10, 16), 190);
        desenharTextoCentralizado(canvas, r.fontePequena, xc, y, msg, rgb(255, 236, 170));
        return;
    }
    const Celula t = portaDoMedoProxima(p);
    if (t.col >= 0) {
        const bool tem = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] > 0;
        const std::wstring msg = tem ? ajuda("{INTERAGIR}: usar a Chave do Medo") : L"Trancada: precisa da Chave do Medo";
        const float x = static_cast<float>(t.col * TILE + TILE / 2) - camX, y = static_cast<float>(t.lin * TILE) - 18.0f - camY;
        retanguloAlpha(canvas, x - 3.2f * static_cast<float>(msg.size()) - 6, y - 1, 6.4f * static_cast<float>(msg.size()) + 12, 14,
                       rgb(8, 10, 16), 190);
        desenharTextoCentralizado(canvas, r.fontePequena, x, y, msg, tem ? rgb(255, 236, 170) : rgb(255, 170, 150));
        return;
    }
    if (const ObjetoSala* amigo = amigoParaResgatar(p)) {
        const std::wstring msg = ajuda("{INTERAGIR}: resgatar ") + paraWide(nomeDoAmigo(p.sala.modelo->npcs[static_cast<std::size_t>(amigo->codigo - '0')]));
        const float x = static_cast<float>(amigo->celula.col * TILE + TILE / 2) - camX, y = static_cast<float>(amigo->celula.lin * TILE) - 40.0f - camY;
        const float w = 5.4f * static_cast<float>(msg.size()) + 12;
        retanguloAlpha(canvas, x - w / 2, y - 1, w, 14, rgb(8, 10, 16), 190);
        desenharTextoCentralizado(canvas, r.fontePequena, x, y, msg, rgb(255, 236, 170));
        return;
    }
    if (const ObjetoSala* elias = eliasProximo(p)) {
        const bool manivela = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Manivela)] > 0 && !p.manivelaEntregue;
        const std::wstring msg = ajuda(manivela ? "{INTERAGIR}: entregar a manivela ao Elias" : "{INTERAGIR}: falar com o Elias (loja)");
        const float x = static_cast<float>(elias->celula.col * TILE + TILE / 2) - camX, y = static_cast<float>(elias->celula.lin * TILE) - 40.0f - camY;
        const float w = 5.4f * static_cast<float>(msg.size()) + 12;
        retanguloAlpha(canvas, x - w / 2, y - 1, w, 14, rgb(8, 10, 16), 190);
        desenharTextoCentralizado(canvas, r.fontePequena, x, y, msg, rgb(255, 236, 170));
        return;
    }
    if (const ObjetoSala* altar = altarProximo(p)) {
        const Amigo dono = amigoDoBioma(p.emTentativa ? p.plano.bioma : p.sala.modelo->bioma);
        const std::wstring msg = ajuda("{INTERAGIR}: tocar o altar (Eco do ") + paraWide(nomeAmigo(dono)) + L")";
        const float x = static_cast<float>(altar->celula.col * TILE + TILE / 2) - camX, y = static_cast<float>(altar->celula.lin * TILE) - 22.0f - camY;
        const float w = 5.4f * static_cast<float>(msg.size()) + 12;
        retanguloAlpha(canvas, x - w / 2, y - 1, w, 14, rgb(8, 10, 16), 190);
        desenharTextoCentralizado(canvas, r.fontePequena, x, y, msg, rgb(200, 225, 255));
        return;
    }
    const ObjetoSala* bau = bauProximo(p);
    if (!bau) return;
    const bool semChave = bauTrancado(p) && p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveEnferrujada)] <= 0;
    const std::wstring dica = semChave ? L"Trancado: precisa de chave enferrujada" : ajuda("{INTERAGIR}: abrir o baú");
    const float x = static_cast<float>(bau->celula.col * TILE + TILE / 2) - camX, y = static_cast<float>(bau->celula.lin * TILE) - 22.0f - camY;
    retanguloAlpha(canvas, x - 3.2f * static_cast<float>(dica.size()) - 6, y - 1, 6.4f * static_cast<float>(dica.size()) + 12, 14,
                   rgb(8, 10, 16), 190);
    desenharTextoCentralizado(canvas, r.fontePequena, x, y, dica, semChave ? rgb(255, 170, 150) : rgb(255, 236, 170));
}

// ---------- portas da tentativa: o que cada uma promete ----------
struct IconePorta {
    const char* atlas;
    int quadro;
    std::wstring rotulo;
};

IconePorta iconeDaPorta(const OpcaoPorta* o, bool descida) {
    if (descida) return {"objetos_comuns", 14, L"Descer"};
    switch (o->tipo) {
        case TipoSala::Tesouro: return {"objetos_comuns", 2, L"Tesouro"};
        case TipoSala::Loja: return {"item_moeda", 0, L"Loja do Elias"};
        case TipoSala::Refugio: return {"icones", 9, L"Refúgio (cura)"};
        case TipoSala::Resgate: return {"item_chave_latao", 0, L"Resgate"};
        case TipoSala::Eco: return {"objetos_comuns", 6, L"Altar de Eco"};
        case TipoSala::Bonus: return {"balao", 0, L"Festa dos Balões"};
        case TipoSala::Chefe: return {"icones", 8, L"Chefe-medo"};
        default: break;
    }
    if (o->recompensa == Recompensa::PontoAtributo) return {"icones", 10, L"Ponto de atributo"};
    if (o->recompensa == Recompensa::Eco) return {"objetos_comuns", 6, L"Eco"};
    const DefinicaoRecompensa& d = definicaoRecompensa(o->recompensa);
    if (d.item == TipoItem::Total) return {"icones", 12, L"Combate"};
    return {definicaoDe(d.item).atlas, definicaoDe(d.item).quadro, paraWide(d.nome)};
}

// Pictograma de cadeado em pixels, preso ao canto da placa. O miolo e a haste têm
// silhuetas diferentes para continuar legíveis mesmo sobre uma recompensa dourada.
void desenharCadeadoPlaca(Canvas& canvas, float x, float y) {
    static const char* const linhas[] = {
        "...XXXXXX...", "..XssssssX..", ".Xss....ssX.", ".Xss....ssX.", ".Xss....ssX.",
        ".Xss....ssX.", ".XXXXXXXXXX.", "XbbbbbbbbbbX", "XbggggggggbX", "XbgggXXgggbX",
        "XbgggXXgggbX", "XbggggggggbX", "XbbbbbbbbbbX", ".XXXXXXXXXX."
    };
    for (int l = 0; l < 14; ++l) {
        for (int c = 0; c < 12; ++c) {
            const char p = linhas[l][c];
            if (p == '.') continue;
            const std::uint32_t cor = p == 'X' ? rgb(23, 18, 16) : p == 's' ? rgb(186, 190, 188) :
                                      p == 'b' ? rgb(91, 59, 34) : rgb(219, 153, 67);
            retanguloCheio(canvas, x + c, y + l, 1, 1, cor);
        }
    }
}

void brilhoDaPlaca(Canvas& canvas, float x, float y, float metade, float pulso) {
    const int margem = 9 * canvas.escala;
    const int cx = paraTela(canvas, x), cy = paraTela(canvas, y);
    const int h = paraTela(canvas, metade);
    const int x0 = std::max(0, cx - h - margem), x1 = std::min(canvas.w, cx + h + margem);
    const int y0 = std::max(0, cy - h - margem), y1 = std::min(canvas.h, cy + h + margem);
    for (int py = y0; py < y1; ++py) {
        for (int px = x0; px < x1; ++px) {
            const int fora = std::max(std::abs(px - cx) - h, std::abs(py - cy) - h);
            if (fora < 0 || fora >= margem) continue;
            const float t = 1.0f - static_cast<float>(fora) / margem;
            const int alfa = static_cast<int>((32.0f + 64.0f * pulso) * t * t);
            std::uint32_t& destino = canvas.px[static_cast<std::size_t>(py) * canvas.w + px];
            const int inv = 255 - alfa;
            const int vr = (((destino >> 16) & 255) * inv + 255 * alfa) / 255;
            const int vg = (((destino >> 8) & 255) * inv + 186 * alfa) / 255;
            const int vb = ((destino & 255) * inv + 75 * alfa) / 255;
            destino = rgb(static_cast<std::uint8_t>(vr), static_cast<std::uint8_t>(vg), static_cast<std::uint8_t>(vb));
        }
    }
}

// Placa sobre cada porta com opção: a recompensa da PRÓXIMA sala (só portas já vistas). Diego, 17/11:
// o ícone flutuava dentro da sala num círculo escuro e parecia um item no chão ("moeda e chave que não
// consigo pegar", "tesouro que não pego"). Agora é uma placa emoldurada PRESA à porta, com uma seta
// para ela e "Próxima sala: …" quando Bill chega perto. Em combate a placa fica apagada (portas fechadas).
void desenharPortasDaTentativa(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    if (!p.emTentativa) return;
    for (const PortaSala& porta : p.sala.portas) {
        if (porta.opcao < 0 || porta.selada || !visaoEm(p.visao, porta.celula.col, porta.celula.lin).explorada) continue;
        bool descida = false;
        const OpcaoPorta* o = destinoDaPorta(p, porta, descida);
        if (!o && !descida) continue;
        const IconePorta ic = iconeDaPorta(o, descida);
        // Sobre a própria porta (portas laterais: ao lado dela, para dentro só o bastante para caber).
        const Vec2 d = vetorDaDirecao(porta.lado);
        const bool lateral = std::fabs(d.x) > 0.5f;
        const Vec2 c = centroCelula(porta.celula);
        const float x = c.x - (lateral ? d.x * 18.0f : 0.0f) - camX;
        const float y = c.y - (lateral ? 30.0f : (d.y > 0.0f ? 40.0f : -6.0f)) - camY;
        const bool aberta = p.salaConcluida;
        const float pulso = 0.5f + 0.5f * std::sin(tempo * 5.0f);
        const std::uint32_t moldura = aberta ? rgb(255, static_cast<std::uint8_t>(192 + 45 * pulso),
                                                   static_cast<std::uint8_t>(95 + 55 * pulso)) : rgb(110, 106, 100);
        // Placa: fundo escuro, moldura dupla e uma seta apontando para a porta (cresce com o ícone:
        // os objetos de 32 px — baú, altar, poço — não cabem na de 16).
        const Sprite icone = spriteDe(r, ic.atlas);
        const float m = spriteValido(icone) ? static_cast<float>(icone.frameW) / (2.0f * static_cast<float>(canvas.escala)) + 4.0f : 12.0f;
        const float h = std::max(14.0f, m);
        if (aberta) brilhoDaPlaca(canvas, x, y, h, pulso);
        retanguloAlpha(canvas, x - h, y - h, 2 * h, 2 * h, rgb(14, 12, 10), aberta ? 230 : 160);
        contornoRetangulo(canvas, x - h, y - h, 2 * h, 2 * h, moldura);
        contornoRetangulo(canvas, x - h + 2, y - h + 2, 2 * h - 4, 2 * h - 4, aberta ? rgb(150, 110, 50) : rgb(70, 68, 64));
        const float sx = lateral ? x + d.x * (h + 3.0f) : x, sy = lateral ? y : y + (d.y > 0.0f ? h + 3.0f : -h - 3.0f);
        for (int k = 0; k < 4; ++k) {   // seta (triângulo de 4 degraus) para o lado da porta
            const float t = static_cast<float>(k);
            if (lateral) retanguloAlpha(canvas, sx + d.x * t * 1.5f, sy - 4.0f + t, 2.0f, 8.0f - 2.0f * t, moldura, 220);
            else retanguloAlpha(canvas, sx - 4.0f + t, sy + (d.y > 0.0f ? t : -t) * 1.5f, 8.0f - 2.0f * t, 2.0f, moldura, 220);
        }
        const Sprite spr = spriteDe(r, ic.atlas);
        if (spriteValido(spr)) {
            const float meio = static_cast<float>(spr.frameW) / (2.0f * static_cast<float>(canvas.escala));
            desenharFrame(canvas, spr, ic.quadro % spr.colunas, ic.quadro / spr.colunas, x - meio, y - meio);
        }
        if (!aberta) desenharCadeadoPlaca(canvas, x + h - 8.0f, y + h - 8.0f);
        // Nome quando Bill chega perto (decidir sem adivinhar).
        if (comprimento(subtrair(p.bill.pes, c)) < 4.5f * TILE) {
            const std::wstring rotulo = (aberta ? L"ABERTA · Próxima sala: " : L"FECHADA · Próxima sala: ") + ic.rotulo;
            const float w = 6.0f * static_cast<float>(rotulo.size()) + 10.0f;
            const float ty = y - 32 + 2.0f * std::sin(tempo * 2.5f);
            const float tx = limitar(x, w / 2 + 4.0f, LARGURA_LOGICA - w / 2 - 4.0f);
            retanguloAlpha(canvas, tx - w / 2, ty, w, 14, rgb(8, 10, 16), 210);
            desenharTextoCentralizado(canvas, r.fontePequena, tx, ty, rotulo, rgb(255, 236, 170));
        }
    }
}

// ---------- pendência: subir de nível (com prévia dos valores derivados) ----------
// ---------- menus: pausa e inventário (painel e linhaMenu vêm de cenas/ui) ----------

const wchar_t* const OPCOES_ASSISTIR[TOTAL_OPCOES_ASSISTIR] = {L"Continuar assistindo", L"Assumir o controle", L"Voltar ao menu"};

void desenharMenuAssistir(Canvas& canvas, const Recursos& r, int cursor) {
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 140);
    const float x = LARGURA_LOGICA / 2.0f - 130, y = 96;
    painel(canvas, x, y, 260, 150);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, y + 6, L"Assistindo", rgb(255, 228, 150));
    for (int i = 0; i < TOTAL_OPCOES_ASSISTIR; ++i)
        linhaMenu(canvas, r, x + 14, y + 52 + 26.0f * static_cast<float>(i), 232, OPCOES_ASSISTIR[i], cursor == i);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 132,
                              ajuda("{CONFIRMAR} escolher · {VOLTAR} continuar"), rgb(150, 146, 136));
}

void desenharPausa(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 140);
    const float x = LARGURA_LOGICA / 2.0f - 130, y = 60;   // 6 opções (Salvar jogo, 23/11)
    painel(canvas, x, y, 260, 226);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, y + 6, L"Pausado", rgb(255, 228, 150));
    for (int i = 0; i < TOTAL_OPCOES_PAUSA; ++i) {
        std::wstring t = paraWide(nomeOpcaoPausa(static_cast<OpcaoPausa>(i), p.noClube || p.noPrologo));
        if (static_cast<OpcaoPausa>(i) == OpcaoPausa::Ficha && p.bill.progresso.pontosPendentes > 0) {
            t += L" (+" + std::to_wstring(p.bill.progresso.pontosPendentes) + L")";
        }
        linhaMenu(canvas, r, x + 14, y + 52 + 26.0f * static_cast<float>(i), 232, t,
                  p.confirmando == OpcaoPausa::Total && p.cursor == i);
    }
    const int seg = segundosDeJogo(p.stats);
    wchar_t info[96];
    std::swprintf(info, 96, L"%02d:%02d · %ls · seed %llu", seg / 60, seg % 60, paraWide(nomeDificuldade(p.dificuldade)).c_str(),
                  static_cast<unsigned long long>(p.seed));
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 208, info, rgb(150, 146, 136));
    if (p.tempoAvisoMenu > 0.0f && !p.avisoMenu.empty())
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 248, paraWide(p.avisoMenu), rgb(255, 200, 160));
    if (p.confirmando != OpcaoPausa::Total) {   // "Tem certeza?"
        const float cx = LARGURA_LOGICA / 2.0f - 120, cy = 120;
        painel(canvas, cx, cy, 240, 104);
        const bool abandonar = p.confirmando == OpcaoPausa::Abandonar;
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, cy + 8,
                                  abandonar ? L"Abandonar a tentativa?" : L"Sair do jogo?", rgb(255, 200, 180));
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, cy + 30,
                                  abandonar ? L"A pontuação desta tentativa não é gravada." : L"O progresso desta tentativa se perde.",
                                  rgb(170, 166, 156));
        linhaMenu(canvas, r, cx + 40, cy + 52, 70, L"Sim", p.cursor == 0);
        linhaMenu(canvas, r, cx + 130, cy + 52, 70, L"Não", p.cursor == 1);
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 232,
                              ajuda("←→ escolher · {CONFIRMAR} confirmar · {VOLTAR} continuar"), rgb(170, 166, 156));
}

// Cor de cada raridade (texto e moldura das cartas): cinza, azul, roxo, dourado.
std::uint32_t corDaRaridade(Raridade r) {
    switch (r) {
        case Raridade::Raro: return rgb(120, 175, 255);
        case Raridade::Epico: return rgb(205, 130, 255);
        case Raridade::Lendario: return rgb(255, 205, 90);
        default: return rgb(215, 210, 200);
    }
}

// ---------- resultado (fim da tentativa) ----------
void desenharResultado(Canvas& canvas, const Recursos& r, const EstadoSala& s, bool fluxoClube) {
    const EstadoPartida& p = s.partida;
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, p.fimDoConteudo ? rgb(4, 10, 20) : rgb(20, 4, 6), 215);
    std::wstring titulo = L"Você caiu";
    if (p.abandonou) titulo = L"Tentativa abandonada";
    else if (p.fimDoConteudo) titulo = paraWide(std::string(nomeBioma(p.plano.bioma)) + " concluída");
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 8, titulo,
                              p.fimDoConteudo ? rgb(255, 228, 150) : rgb(240, 200, 196));
    // Esquerda: de onde veio cada ponto (a MESMA função do HUD: detalharPontuacao).
    const ParcelasPontuacao d = detalharPontuacao(p.stats, p.ajustes.multiplicadorPontos);
    const Estatisticas& e = p.stats;
    painel(canvas, 14, 46, 300, 278);
    struct Linha { const wchar_t* nome; long long valor; int qtd; };
    const Linha linhas[] = {
        {L"Inimigos (K) ×100", d.inimigos, e.K},  {L"Elites (E) ×300", d.elites, e.E},
        {L"Itens (I) ×50", d.itens, e.I},         {L"Resgates (R) ×500", d.resgates, e.R},
        {L"Chefes (B) ×1000", d.chefes, e.B},     {L"Salas (C) ×250", d.salas, e.C},
        {L"Vitória (V) ×5000", d.vitoria, e.V},   {L"Vitória rápida", d.vitoriaRapida, -1},
        {L"Movimento (M/2)", -d.movimento, e.M},  {L"Vidas usadas (U) ×1000", -d.vidas, e.U},
        {L"Tempo (T/5)", -d.tempo, segundosDeJogo(e)},
    };
    wchar_t t[160];
    float ly = 54;
    for (const Linha& l : linhas) {
        desenharTexto(canvas, r.fontePequena, 24, ly, l.nome, rgb(200, 194, 180));
        if (l.qtd >= 0) {
            std::swprintf(t, 160, L"%d", l.qtd);
            desenharTexto(canvas, r.fontePequena, 190, ly, t, rgb(150, 146, 136));
        }
        const std::wstring v = (l.valor < 0 ? L"−" : L"+") + comMilhar(l.valor < 0 ? -l.valor : l.valor);
        desenharTexto(canvas, r.fontePequena, 244, ly, v, l.valor < 0 ? rgb(255, 160, 150) : rgb(160, 235, 170));
        ly += 17;
    }
    std::swprintf(t, 160, L"× %d%% (%ls)", d.multiplicadorPct, paraWide(nomeDificuldade(p.dificuldade)).c_str());
    desenharTexto(canvas, r.fontePequena, 24, ly + 2, t, rgb(200, 225, 255));
    desenharTexto(canvas, r.fonteMedia, 24, ly + 22, L"Pontuação: " + comMilhar(d.total), rgb(255, 228, 150));
    const int seg = segundosDeJogo(e);
    std::swprintf(t, 160, L"tempo %02d:%02d · seed %llu", seg / 60, seg % 60, static_cast<unsigned long long>(p.seed));
    desenharTexto(canvas, r.fontePequena, 24, ly + 46, t, rgb(150, 146, 136));
    // Direita: recordes (top 10) com filtro (a mesma tabela do menu: cenas/ui).
    painel(canvas, 326, 46, 300, 278);
    Recorde minha;
    minha.seed = p.seed;
    minha.pontos = pontuacaoAtual(p);
    minha.piloto = s.pilotoJogou;
    desenharRecordes(canvas, r, 326, 50, 300, s.recordes, s.filtro, s.colocacao > 0 ? &minha : nullptr);
    if (s.lembrancasGanhas > 0) {
        std::swprintf(t, 160, L"+%d Lembranças (total %d)", s.lembrancasGanhas, s.lembrancasTotal);
        desenharTextoCentralizado(canvas, r.fontePequena, 476, 276, t, rgb(200, 225, 255));
    }
    std::wstring col = p.abandonou ? (s.chefesNoRecorde > 0 ? L"Abandono: vale o recorde do último chefão."
                                                            : L"Abandono: pontuação não gravada.")
                                   : (s.colocacao > 0 ? std::to_wstring(s.colocacao) + L"º lugar (" + (s.pilotoJogou ? L"computador" : L"jogador") + L")"
                                                      : L"Fora do top 10 desta vez.");
    desenharTextoCentralizado(canvas, r.fonteMedia, 476, 296, col, rgb(255, 228, 150));
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 22,
                              fluxoClube ? (p.vitoria ? ajuda("← → filtro dos recordes · {CONFIRMAR}: ver créditos")
                                                       : ajuda("← → filtro dos recordes · {CONFIRMAR}: voltar ao Clube · {VOLTAR}: menu"))
                                         : ajuda("← → filtro dos recordes · {CONFIRMAR}: nova tentativa · {VOLTAR}: menu"),
                              rgb(170, 166, 156));
}

// "+ 3 Moeda de prata" com ícone, no centro inferior (critério M09).
void desenharNotificacoes(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    float y = ALTURA_LOGICA - 112.0f;
    for (const Notificacao& n : p.notificacoes) {
        const std::uint8_t alfa = static_cast<std::uint8_t>(255.0f * std::min(1.0f, n.vida / 0.4f));
        std::wstring t = L"+ ";
        if (n.quantidade > 1) t += std::to_wstring(n.quantidade) + L" ";
        t += paraWide(definicaoDe(n.tipo).nome);
        const float largura = 22.0f + 5.2f * static_cast<float>(t.size());
        const float x = LARGURA_LOGICA / 2.0f - largura / 2.0f;
        retanguloAlpha(canvas, x, y, largura, 18, rgb(8, 10, 16), static_cast<std::uint8_t>(alfa * 3 / 4));
        desenharIcone(canvas, r, n.tipo, x + 2, y + 1);
        desenharTexto(canvas, r.fontePequena, x + 20, y + 2, t, rgb(255, 236, 170));
        y += 20.0f;
    }
}

// Brasas do Espectro no chão: brilham e apagam (perigo para os itens; o jogador vê onde estão).
void desenharBrasas(Canvas& canvas, const EstadoPartida& p, float tempo, float camX, float camY) {
    for (const Brasa& b : p.brasas) {
        const float vida = b.vida / VIDA_BRASA;
        const float pisca = 0.75f + 0.25f * std::sin(tempo * 13.0f + b.pos.x);
        const float x = b.pos.x - camX, y = b.pos.y - camY;
        elipseAlpha(canvas, x, y, 9.0f * vida + 3.0f, 4.0f * vida + 2.0f, rgb(255, 110, 30), static_cast<std::uint8_t>(110 * vida * pisca));
        elipseAlpha(canvas, x, y, 3.0f, 1.6f, rgb(255, 220, 120), static_cast<std::uint8_t>(220 * vida));
    }
}

// Armadilhas (Etapa 11.1, PRD §14.2): pregos com "textura própria" (cabeças de prego na célula) e
// piso cedendo com rachaduras DISCRETAS (quem presta atenção vê; o Stan e os Óculos contornam).
// Rachando, o aviso hachurado normal aparece por cima (desenharAvisos).
void desenharArmadilhas(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            const TipoTile t = tileEm(p.sala, c, l).tipo;
            if ((t != TipoTile::Espinhos && t != TipoTile::PisoCedendo && t != TipoTile::Jato && t != TipoTile::PocaSangue &&
                 t != TipoTile::Fogo && t != TipoTile::Esteira && t != TipoTile::Labareda) ||
                !visaoEm(p.visao, c, l).explorada)
                continue;
            const float x = static_cast<float>(c * TILE) - camX, y = static_cast<float>(l * TILE) - camY;
            if (t == TipoTile::Esteira) {   // faixa de metal com setas que andam na direção do empurrão
                const Vec2 d = direcaoDaEsteira(tileEm(p.sala, c, l));
                retanguloCheio(canvas, x + 1, y + 1, 30, 30, rgb(46, 42, 40));
                retanguloAlpha(canvas, x + 1, y + 1, 30, 2, rgb(120, 100, 80), 160);
                retanguloAlpha(canvas, x + 1, y + 29, 30, 2, rgb(20, 18, 16), 200);
                const float fase = std::fmod(tempo * VELOCIDADE_ESTEIRA, 16.0f);
                for (int k = -1; k < 2; ++k) {   // duas setas por célula, deslizando
                    const float s = static_cast<float>(k) * 16.0f + fase - 8.0f;
                    const float cx = x + 16 + d.x * s, cy = y + 16 + d.y * s;
                    if (cx < x + 3 || cx > x + 29 || cy < y + 3 || cy > y + 29) continue;
                    for (int q = 0; q < 4; ++q) {   // "›": dois traços em V apontando para d
                        const float o = static_cast<float>(q) * 1.5f;
                        const float px = -d.y, py = d.x;   // perpendicular
                        retanguloAlpha(canvas, cx - d.x * o + px * o - 1, cy - d.y * o + py * o - 1, 2, 2, rgb(230, 150, 60), 220);
                        retanguloAlpha(canvas, cx - d.x * o - px * o - 1, cy - d.y * o - py * o - 1, 2, 2, rgb(230, 150, 60), 220);
                    }
                }
                continue;
            }
            if (t == TipoTile::Fogo) {   // a grelha (objetos_siderurgica 5/6) e, acesa, as chamas de fx_fogo
                const EstadoGrelha e = estadoDaGrelha(p, {c, l});
                const Sprite grelha = spriteDe(r, "objetos_siderurgica");
                if (spriteValido(grelha)) desenharFrame(canvas, grelha, e == EstadoGrelha::Chamas ? 2 : 1, 1, x, y);
                if (e == EstadoGrelha::Brasas) {   // brasas acendendo: o aviso também vem pela hachura
                    const float pulso = 0.5f + 0.5f * std::sin(tempo * 18.0f);
                    elipseAlpha(canvas, x + 16, y + 18, 11, 7, rgb(255, 120, 30), static_cast<std::uint8_t>(90 + 80 * pulso));
                } else if (e == EstadoGrelha::Chamas) {
                    const Sprite fogo = spriteDe(r, "fx_fogo");
                    if (spriteValido(fogo)) desenharFrame(canvas, fogo, static_cast<int>(tempo * 12.0f + c + l) % 4, 0, x, y - 6);
                }
                continue;
            }
            if (t == TipoTile::Labareda) {   // o fogo da chuva de brasas (13.3): chão chamuscado, brilho e chamas
                const float pulso = 0.6f + 0.4f * std::sin(tempo * 9.0f + static_cast<float>(c * 3 + l));
                elipseAlpha(canvas, x + 16, y + 18, 15, 11, rgb(40, 22, 14), 200);
                elipseAlpha(canvas, x + 16, y + 18, 13, 9, rgb(255, 110, 20), static_cast<std::uint8_t>(110 * pulso));
                const Sprite fogo = spriteDe(r, "fx_fogo");
                if (spriteValido(fogo)) desenharFrame(canvas, fogo, static_cast<int>(tempo * 12.0f + c * 2 + l) % 4, 0, x, y - 6);
                continue;
            }
            if (t == TipoTile::PocaSangue) {   // brilho ritmado (PRD §14.2): vermelho é perigo
                const float pulso = 0.6f + 0.4f * std::sin(tempo * 3.0f + static_cast<float>(c + l));
                // Poças vizinhas viram UM lençol (a inundação do Ralo, 12.3): cada célula pinta só o
                // próprio quadrado, recuado nos lados sem vizinha (camadas sobrepostas faziam costuras).
                // A poça solta continua redonda.
                const std::uint32_t sangue = rgb(90, 8, 12);
                auto poca = [&](int dc, int dl) { return tileEm(p.sala, c + dc, l + dl).tipo == TipoTile::PocaSangue; };
                const bool e = poca(-1, 0), d = poca(1, 0), cima = poca(0, -1), baixo = poca(0, 1);
                if (e || d || cima || baixo) {
                    const float x0 = x + (e ? 0.0f : 3.0f), x1 = x + 32.0f - (d ? 0.0f : 3.0f);
                    const float y0 = y + (cima ? 0.0f : 6.0f), y1 = y + 32.0f - (baixo ? 0.0f : 3.0f);
                    retanguloAlpha(canvas, x0, y0, x1 - x0, y1 - y0, sangue, 210);
                } else {
                    elipseAlpha(canvas, x + 16, y + 17, 14, 10, sangue, 210);
                }
                elipseAlpha(canvas, x + 15, y + 15, 9, 6, rgb(170, 20, 28), static_cast<std::uint8_t>(140 * pulso));
                continue;
            }
            if (t == TipoTile::Jato) {   // o cano saindo da parede, com a boca para onde ele dispara
                const Vec2 d = direcaoDoJato(p.sala, {c, l});
                const float bx = x + 16 - d.x * 10, by = y + 16 - d.y * 10;
                retanguloCheio(canvas, bx - 5, by - 5, 10, 10, rgb(92, 70, 52));
                retanguloCheio(canvas, bx + d.x * 6 - 3, by + d.y * 6 - 3, 6, 6, rgb(40, 34, 30));
                continue;
            }
            if (t == TipoTile::Espinhos) {
                for (int k = 0; k < 9; ++k) {   // 3×3 pregos: sombra, haste e cabeça clara
                    const float px = x + 6.0f + 10.0f * static_cast<float>(k % 3) + ((k / 3) % 2 ? 4.0f : 0.0f);
                    const float py = y + 8.0f + 9.0f * static_cast<float>(k / 3);
                    elipseAlpha(canvas, px + 1.0f, py + 3.0f, 2.5f, 1.2f, rgb(0, 0, 0), 120);
                    retanguloAlpha(canvas, px - 0.5f, py - 4.0f, 1.5f, 6.0f, rgb(120, 116, 110), 230);
                    elipseAlpha(canvas, px, py - 4.0f, 1.8f, 1.2f, rgb(200, 196, 186), 240);
                }
            } else {
                const std::uint32_t risco = rgb(20, 16, 12);
                retanguloAlpha(canvas, x + 9, y + 10, 9, 1, risco, 90);
                retanguloAlpha(canvas, x + 17, y + 11, 1, 7, risco, 90);
                retanguloAlpha(canvas, x + 12, y + 20, 10, 1, risco, 80);
            }
        }
    }
}

// Poças do Afogado (Etapa 10.2): água escura e lisa, com um brilho que ondula; somem no fim.
// Bill agarrado: anéis de água em volta dos pés enquanto dura a lentidão.
void desenharPocas(Canvas& canvas, const EstadoPartida& p, float tempo, float camX, float camY) {
    for (const Poca& o : p.pocas) {
        const float vida = std::min(1.0f, o.vida / 1.5f);   // some no último 1,5 s
        const float x = o.pos.x - camX, y = o.pos.y - camY;
        elipseAlpha(canvas, x, y, 13.0f, 6.0f, rgb(20, 46, 58), static_cast<std::uint8_t>(170 * vida));
        const float brilho = 0.5f + 0.5f * std::sin(tempo * 3.0f + o.pos.x * 0.1f);
        elipseAlpha(canvas, x - 3.0f, y - 1.0f, 5.0f, 1.5f, rgb(150, 190, 200), static_cast<std::uint8_t>(90 * vida * brilho));
    }
    if (p.bill.lento > 0.0f) {
        const float x = p.bill.pes.x - camX, y = p.bill.pes.y - camY;
        const float fase = std::fmod(tempo * 2.0f, 1.0f);
        elipseAlpha(canvas, x, y, 8.0f + 8.0f * fase, 3.0f + 3.0f * fase, rgb(120, 180, 200), static_cast<std::uint8_t>(160 * (1.0f - fase)));
    }
}

// Óculos do Richie: inimigos FORA da luz aparecem como silhueta com contorno branco.
// Desenhado DEPOIS da escuridão (senão ela os apagaria junto).
// Parede rachada: rachaduras DISCRETAS na face (quem presta atenção vê; segredo, SDD §24.5).
// Com os Óculos do Richie ou o Stan acompanhando, ganha um contorno claro (PRD §11.1, §24.6).
// (Desenho provisório até a arte própria da parede rachada: lote da Etapa 9.)
void desenharParedesRachadas(Canvas& canvas, const EstadoPartida& p, float tempo, float camX, float camY) {
    const bool destacar = p.bill.oculos > 0.0f || revelaArmadilhas(p);
    for (int l = 0; l < p.sala.lins; ++l) {
        for (int c = 0; c < p.sala.cols; ++c) {
            if (tileEm(p.sala, c, l).tipo != TipoTile::ParedeRachada || !visaoEm(p.visao, c, l).explorada) continue;
            const float x = static_cast<float>(c * TILE) - camX, y = static_cast<float>(l * TILE) - camY;
            const std::uint32_t risco = rgb(26, 24, 22);
            retanguloAlpha(canvas, x + 14, y + 6, 2, 9, risco, 170);
            retanguloAlpha(canvas, x + 11, y + 14, 4, 2, risco, 170);
            retanguloAlpha(canvas, x + 16, y + 14, 2, 8, risco, 170);
            retanguloAlpha(canvas, x + 18, y + 21, 5, 2, risco, 170);
            if (destacar) {   // brilho frio pulsando nas rachaduras (sem moldura retangular, 23/11)
                const float pulso = 0.5f + 0.5f * std::sin(tempo * 4.0f);
                elipseAlpha(canvas, x + TILE / 2.0f, y + TILE / 2.0f, 12.0f + 3.0f * pulso, 12.0f + 3.0f * pulso, rgb(215, 230, 255),
                            static_cast<std::uint8_t>(40 + 45 * pulso));
            }
        }
    }
}

void desenharRevelados(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    // O item Óculos revela todos; o Eco "Óculos" (Richie, 14.3) só em células já exploradas, até N células além da luz.
    const bool item = aparenciaDe(p.bill, p.inventario, tempo).revelaInimigos;
    const int eco = valorEco(p, EfeitoEco::OculosRichie);
    if (!item && eco <= 0) return;
    const float alcance = (raioDaLuz(p) + static_cast<float>(eco)) * static_cast<float>(TILE);
    for (const Inimigo& i : p.inimigos) {
        if (!i.vivo || i.surgindo > 0.0f || estaVisivel(p.visao, i.pes)) continue;
        if (!item) {
            const Celula c = celulaDe(i.pes);
            if (!visaoEm(p.visao, c.col, c.lin).explorada || comprimento(subtrair(i.pes, p.bill.pes)) > alcance) continue;
        }
        const ParametrosInimigo& par = parametrosDe(i.tipo);
        const Sprite s = spriteDe(r, par.atlas);
        const float x = i.pes.x - camX, y = i.pes.y - camY;
        contornoFramePes(canvas, s, 0, static_cast<int>(i.direcao), x, y, rgb(240, 240, 255));
        desenharFramePesTinta(canvas, s, 0, static_cast<int>(i.direcao), x, y, false, 90, 0xC8D2FF, 160);
    }
}

// Golpes do Bill: rastro claro da área (feedback do alcance real do golpe).
// Pontos espalhados pela área (ao longo da linha, pela boca do arco, em volta do anel, no círculo):
// onde os sprites de aviso e de ataque são desenhados. Sem formas geométricas (Diego, 17/11: "não quero
// mais esses retângulos vermelhos, esses semicírculos").
template <typename F>
void paraCadaPontoDaArea(const Area& a, float passo, F&& f) {
    switch (a.forma) {
        case FormaArea::Linha: {
            const int n = std::max(1, static_cast<int>(a.comprimento / passo));
            for (int k = 0; k <= n; ++k) {
                const float d = a.comprimento * static_cast<float>(k) / static_cast<float>(n);
                f(a.centro.x + a.direcao.x * d, a.centro.y + a.direcao.y * d);
            }
            break;
        }
        case FormaArea::Arco: {
            const float meia = a.anguloGraus * 0.5f * 0.0174533f;
            const float base = std::atan2(a.direcao.y, a.direcao.x);
            const int n = std::max(2, static_cast<int>(a.anguloGraus * 0.0174533f * a.raio * 0.65f / passo));
            for (int k = 0; k <= n; ++k) {
                const float ang = base - meia * 0.8f + 1.6f * meia * static_cast<float>(k) / static_cast<float>(n);
                f(a.centro.x + std::cos(ang) * a.raio * 0.65f, a.centro.y + std::sin(ang) * a.raio * 0.65f);
            }
            break;
        }
        case FormaArea::Anel: {
            const float rr = (a.raio + a.raioInterno) * 0.5f;
            const int n = std::max(6, static_cast<int>(6.2832f * rr / passo));
            for (int k = 0; k < n; ++k) {
                const float ang = 6.2832f * static_cast<float>(k) / static_cast<float>(n);
                f(a.centro.x + std::cos(ang) * rr, a.centro.y + std::sin(ang) * rr);
            }
            break;
        }
        default: {
            f(a.centro.x, a.centro.y);
            for (float rr = passo; rr <= a.raio * 0.85f; rr += passo) {   // anéis concêntricos até a borda
                const int n = std::max(5, static_cast<int>(6.2832f * rr / passo));
                for (int k = 0; k < n; ++k) {
                    const float ang = 6.2832f * static_cast<float>(k) / static_cast<float>(n) + rr * 0.1f;
                    f(a.centro.x + std::cos(ang) * rr, a.centro.y + std::sin(ang) * rr);
                }
            }
            break;
        }
    }
}

// Golpes de Bill (Diego, 17/11: o clarão "um retângulo sem graça"; nada de semicírculos): o corte do taco
// é o impacto em meia-lua (fx_impacto) na boca do arco; o clarão é um FEIXE de luz — brilhos que se
// encadeiam ao longo do caminho, faíscas e um clarão na ponta.
void desenharGolpes(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    const Sprite impacto = spriteDe(r, "fx_impacto");
    for (const Golpe& g : p.golpes) {
        if (g.equipe != Equipe::Jogador || g.queima) continue;
        const Area& a = g.area;
        if (g.atordoa > 0.0f && a.forma == FormaArea::Linha) {   // clarão
            const float vida = std::clamp(g.duracao / 0.12f, 0.0f, 1.0f);
            for (float d = 0.0f; d <= a.comprimento; d += 5.0f) {
                const float x = a.centro.x + a.direcao.x * d - camX, y = a.centro.y + a.direcao.y * d - camY;
                const float onda = 0.75f + 0.25f * std::sin(d * 0.35f - tempo * 40.0f);
                elipseAlpha(canvas, x, y, 11.0f * onda, 9.0f * onda, rgb(255, 230, 140), static_cast<std::uint8_t>(55 * vida));
                elipseAlpha(canvas, x, y, 5.0f, 4.0f, rgb(255, 250, 225), static_cast<std::uint8_t>(200 * vida));
            }
            for (int k = 0; k < 10; ++k) {   // faíscas
                const float d = std::fmod(static_cast<float>(k) * 37.0f + tempo * 300.0f, std::max(1.0f, a.comprimento));
                const float lado = (k % 2 == 0 ? 1.0f : -1.0f) * (4.0f + static_cast<float>(k % 3) * 3.0f);
                elipseAlpha(canvas, a.centro.x + a.direcao.x * d - a.direcao.y * lado - camX,
                            a.centro.y + a.direcao.y * d + a.direcao.x * lado - camY, 1.6f, 1.6f, rgb(255, 255, 255),
                            static_cast<std::uint8_t>(230 * vida));
            }
            const float fx = a.centro.x + a.direcao.x * a.comprimento - camX, fy = a.centro.y + a.direcao.y * a.comprimento - camY;
            elipseAlpha(canvas, fx, fy, 16.0f, 13.0f, rgb(255, 240, 180), static_cast<std::uint8_t>(110 * vida));
            continue;
        }
        // Corte do taco (24/11): o rastro desenhado apontando para a direita, girado para o rumo do golpe. A
        // diagonal fica igual ao eixo (Diego: "melhorar os golpes diagonais"). O 3º golpe do combo é maior.
        // (o corte do taco é desenhado com o Bill, pelo tempo do golpe: desenharCorteDoTaco)
        if (spriteValido(spriteDe(r, "fx_corte_taco")) && a.forma == FormaArea::Arco) continue;
        if (!spriteValido(impacto)) continue;
        // Corte do taco (arco) ou o giro do especial (círculo): meias-luas de impacto pela boca/volta.
        const int quadro = std::clamp(static_cast<int>((1.0f - std::clamp(g.duracao / 0.12f, 0.0f, 1.0f)) * 3.0f), 0, 3);
        const float meio = static_cast<float>(impacto.frameW) / (2.0f * static_cast<float>(canvas.escala));
        paraCadaPontoDaArea(a, a.forma == FormaArea::Arco ? 26.0f : 30.0f, [&](float x, float y) {
            desenharFrame(canvas, impacto, quadro, 0, x - meio - camX, y - meio - camY);
        });
    }
}

// Traços dos Ecos da Beverly: o tiro de prata (cinza-azulado, com a pedra na ponta) e o salto do
// clarão do Ricochete (amarelo). Somem em ~0,15 s.
void desenharTracos(Canvas& canvas, const EstadoPartida& p, float camX, float camY) {
    for (const Traco& t : p.tracos) {
        const std::uint8_t a = static_cast<std::uint8_t>(255.0f * std::min(1.0f, t.vida / 0.15f));
        const std::uint32_t cor = t.prata ? rgb(215, 225, 240) : rgb(255, 240, 160);
        const Vec2 d = subtrair(t.para, t.de);
        const float n = std::max(1.0f, comprimento(d));
        for (float k = 0.0f; k <= n; k += 2.0f) {
            const float x = t.de.x + d.x * k / n - camX, y = t.de.y + d.y * k / n - camY;
            retanguloAlpha(canvas, x - 1.0f, y - 1.0f, 2.0f, 2.0f, cor, a);
        }
        if (t.prata) elipseAlpha(canvas, t.para.x - camX, t.para.y - camY, 3.0f, 3.0f, rgb(235, 240, 250), a);
        else elipseAlpha(canvas, t.para.x - camX, t.para.y - camY, 6.0f, 6.0f, rgb(255, 245, 190), static_cast<std::uint8_t>(a / 2));
    }
}

// Folha do ataque de verdade de cada inimigo (Diego, 17/11). Sem a folha nova, cai no impacto comum.
const char* folhaDoAtaque(const EfeitoAtaque& e) {
    if (e.fogo) return "fx_fogo";
    if (e.nuvem) return "fx_nuvem";
    if (e.poca) return "fx_agua";
    switch (e.atacante) {
        case TipoInimigo::Balao: return "fx_estouro";
        case TipoInimigo::Rato: case TipoInimigo::Sombra: case TipoInimigo::Sanguessuga: return "fx_garra";
        case TipoInimigo::Espectro: return "fx_fuligem";
        case TipoInimigo::Afogado: return "fx_agua";
        case TipoInimigo::Boneco: return "fx_impacto";
        case TipoInimigo::Cabelo: return "fx_cabelo";
        case TipoInimigo::Mumia: return e.fase == 1 ? "fx_bandagem" : "fx_golpe_sombrio";
        case TipoInimigo::MedoAfogado: return e.fase == 1 ? "fx_agua" : "fx_golpe_sombrio";
        case TipoInimigo::Leproso: return "fx_golpe_sombrio";
        case TipoInimigo::RaloSangue: return e.fase == 1 ? "fx_cabelo" : "fx_golpe_sombrio";
        case TipoInimigo::Passaro: return "fx_fogo";
        case TipoInimigo::Lobisomem: return e.fase == 1 ? "fx_garra" : "fx_golpe_sombrio";
        case TipoInimigo::PennywiseIV: {   // o palhaço-aranha (24/11): cada poder com o seu efeito
            int quantos = 0;
            const PadraoChefe* ps = padroesDoChefe(TipoInimigo::PennywiseIV, quantos);
            if (e.padrao < 0 || e.padrao >= quantos) return e.fase == 3 ? "fx_luzes_mortas" : "fx_penny_linha";
            switch (ps[e.padrao].forma) {
                case FormaPadrao::Chuva: return "fx_teia";
                case FormaPadrao::Leque: case FormaPadrao::Salto: return "fx_patas";
                case FormaPadrao::Circulo: return e.fase == 3 ? "fx_patas" : "fx_penny_pulso";
                case FormaPadrao::Ondas: return "fx_penny_pulso";
                case FormaPadrao::CruzAlvo: return "fx_luzes_mortas";
                default: return e.fase == 3 ? "fx_luzes_mortas" : "fx_penny_linha";
            }
        }
        default: return "fx_impacto";
    }
}

// O ataque de verdade: a folha do atacante espalhada pela área que acertou (ao longo da linha, pela
// boca do arco, em volta do anel, no centro do círculo), animada do começo ao fim do efeito.
void desenharEfeitosDeAtaque(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    for (const EfeitoAtaque& e : p.efeitosAtaque) {
        Sprite s = spriteDe(r, folhaDoAtaque(e));
        if (!spriteValido(s)) s = spriteDe(r, "fx_impacto");
        if (!spriteValido(s)) continue;
        const float t = 1.0f - e.vida / DURACAO_EFEITO_ATAQUE;
        const int quadro = std::min(s.colunas - 1, static_cast<int>(t * static_cast<float>(s.colunas)));
        const float meio = static_cast<float>(s.frameW) / (2.0f * static_cast<float>(canvas.escala));
        auto em = [&](float x, float y) { desenharFrame(canvas, s, quadro, 0, x - meio - camX, y - meio - camY); };
        paraCadaPontoDaArea(e.area, 24.0f, em);
    }
}

// Avisos: o chão racha até o golpe sair, sem hachura nem marcadores geométricos.
void desenharAvisos(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo, float camX, float camY) {
    for (const Aviso& a : p.avisos) {
        const float progresso = 1.0f - a.tempoRestante / a.tempoTotal;
        // O chão racha onde o golpe vai cair (fx_piso), com um brilho avermelhado suave em cada ponto;
        // as rachaduras crescem até o golpe sair. Lê-se ONDE e QUANDO sem nenhuma forma geométrica.
        const Sprite piso = spriteDe(r, "fx_piso");
        const int quadro = std::min(2, static_cast<int>(progresso * 3.0f));
        const float pulso = 0.7f + 0.3f * std::sin(tempo * 16.0f);
        const std::uint8_t alfa = static_cast<std::uint8_t>(110.0f + 145.0f * progresso);
        paraCadaPontoDaArea(a.golpe.area, 22.0f, [&](float x, float y) {
            elipseAlpha(canvas, x - camX, y - camY, 9.0f + 4.0f * progresso, 5.0f + 2.0f * progresso, rgb(200, 40, 30),
                        static_cast<std::uint8_t>((40.0f + 90.0f * progresso) * pulso));
            if (spriteValido(piso)) desenharFramePes(canvas, piso, quadro, 0, x - camX, y - camY + 8.0f, false, alfa);
        });
        if (a.projetil) {   // o pino do Boneco voa em arco até o aviso
            const Vec2 fim = a.golpe.area.centro;
            const float t = progresso;
            const float px = a.lancadoDe.x + (fim.x - a.lancadoDe.x) * t - camX;
            const float py = a.lancadoDe.y + (fim.y - a.lancadoDe.y) * t - 46.0f * 4.0f * t * (1.0f - t) - camY;
            elipseAlpha(canvas, a.lancadoDe.x + (fim.x - a.lancadoDe.x) * t - camX, a.lancadoDe.y + (fim.y - a.lancadoDe.y) * t - camY,
                        4.0f, 1.6f, rgb(0, 0, 0), 110);   // a sombra no chão
            retanguloCheio(canvas, px - 1.5f, py - 5.0f, 3, 7, rgb(225, 40, 40));   // o pino vermelho
            retanguloCheio(canvas, px - 1.0f, py - 6.5f, 2, 2, rgb(245, 235, 225));
        }
    }
}

void desenharTextos(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    for (const TextoFlutuante& t : p.textos) {
        std::uint32_t cor = COR_TEXTO;
        switch (t.tipo) {
            case TipoTexto::DanoInimigo: cor = rgb(255, 250, 235); break;
            case TipoTexto::Critico: cor = rgb(255, 205, 70); break;
            case TipoTexto::DanoBill: cor = rgb(255, 90, 90); break;
            case TipoTexto::Errou: case TipoTexto::Esquivou: cor = rgb(170, 200, 230); break;
            case TipoTexto::Info: cor = rgb(160, 235, 170); break;
        }
        const Fonte& f = (t.tipo == TipoTexto::Critico || t.tipo == TipoTexto::Info) ? r.fonteMedia : r.fontePequena;
        desenharTextoCentralizado(canvas, f, t.pos.x - camX, t.pos.y - camY, paraWide(t.texto), cor);
    }
}

// ---------- HUD ----------
void barra(Canvas& canvas, float x, float y, float w, float h, float fracao, std::uint32_t cor) {
    retanguloAlpha(canvas, x, y, w, h, rgb(8, 10, 16), 200);
    retanguloCheio(canvas, x + 1, y + 1, (w - 2) * limitar(fracao, 0.0f, 1.0f), h - 2, cor);
    contornoRetangulo(canvas, x, y, w, h, rgb(120, 116, 110));
}

void desenharHud(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const Jogo& jogo) {
    const Jogador& b = p.bill;
    wchar_t t[160];
    // Superior esquerdo: vida, pactos e lanterna.
    barra(canvas, 8, 8, 130, 12, static_cast<float>(b.hp) / static_cast<float>(b.stats.hpMax), rgb(190, 40, 48));
    std::swprintf(t, 160, L"%d / %d", b.hp, b.stats.hpMax);
    desenharTexto(canvas, r.fontePequena, 12, 8, t, COR_TEXTO);
    for (int i = 0; i < b.pactos; ++i) elipseAlpha(canvas, 148.0f + 10.0f * static_cast<float>(i), 14, 3.5f, 4.5f, rgb(170, 20, 30), 255);
    desenharTexto(canvas, r.fontePequena, 8, 23, L"Lanterna:", rgb(200, 194, 180));
    for (int i = 0; i < std::max(CARGAS_LANTERNA_INICIAIS, b.cargasLanterna); ++i) {
        retanguloCheio(canvas, 58.0f + 9.0f * static_cast<float>(i), 26, 6, 7,
                       i < b.cargasLanterna ? rgb(250, 220, 110) : rgb(60, 58, 52));
    }
    // Slot de missão, ao lado da vida (SDD §24.4): chaves do Medo e manivela.
    float xm = 150.0f + 10.0f * static_cast<float>(b.pactos) + 4.0f;
    for (TipoItem t : {TipoItem::ChaveDoMedo, TipoItem::Manivela}) {
        if (p.inventario.quantidades[static_cast<std::size_t>(t)] <= 0) continue;
        retanguloAlpha(canvas, xm - 1, 5, 18, 18, rgb(60, 48, 16), 200);
        contornoRetangulo(canvas, xm - 1, 5, 18, 18, rgb(230, 190, 90));
        desenharIcone(canvas, r, t, xm, 6);
        xm += 21.0f;
    }
    // Power-ups ativos: ícone + segundos restantes, abaixo da lanterna.
    float xp = 8.0f;
    const struct { TipoItem tipo; float resta; } ativos[] = {
        {TipoItem::FichaCoragem, b.coragem}, {TipoItem::Tonico, b.tonico}, {TipoItem::Oculos, b.oculos}};
    for (const auto& a : ativos) {
        if (a.resta <= 0.0f) continue;
        desenharIcone(canvas, r, a.tipo, xp, 38);
        std::swprintf(t, 160, L"%d", static_cast<int>(std::ceil(a.resta)));
        desenharTexto(canvas, r.fontePequena, xp + 17, 39, t, rgb(255, 200, 120));
        xp += 38.0f;
    }
    // Companheiro: quem é, a habilidade e a recarga (SDD §24.6).
    if (p.companheiro.ativo) {
        const Companheiro& c = p.companheiro;
        const HabilidadeCompanheiro& h = habilidadeDe(c.quem);
        const CombateAmigo& cb = combateDe(c.quem);
        std::swprintf(t, 160, L"%ls · %ls", paraWide(nomeAmigo(c.quem)).c_str(), paraWide(h.nome).c_str());
        desenharTexto(canvas, r.fontePequena, 8, 58, t, rgb(200, 225, 255));
        // Arma e poder (24/11): o próximo ataque aceso; com medo, tudo apagado.
        std::swprintf(t, 160, L"%ls  ·  %ls", paraWide(cb.arma).c_str(), paraWide(cb.poder).c_str());
        desenharTexto(canvas, r.fontePequena, 8, 72, c.comMedo ? L"Com medo: não ajuda nesta luta" : std::wstring(t),
                      c.comMedo ? rgb(255, 150, 140) : rgb(255, 214, 150));
        const float pronto = 1.0f - std::clamp(c.recargaAtaque / INTERVALO_ATAQUE_AMIGO, 0.0f, 1.0f);
        barra(canvas, 8, 86, 80, 4, c.comMedo ? 0.0f : pronto, pronto >= 1.0f ? rgb(160, 235, 170) : rgb(220, 160, 90));
        if (h.recarga > 0.0f) {
            const float apoio = 1.0f - std::max(0.0f, c.recarga) / h.recarga;
            barra(canvas, 92, 86, 40, 4, apoio, apoio >= 1.0f ? rgb(160, 235, 170) : rgb(120, 150, 200));
        }
    }
    // Inferior esquerdo: nível e XP (+ pontos pendentes).
    const int nivel = b.progresso.nivel;
    const int base = LIMIARES_XP[nivel], prox = nivel < NIVEL_MAXIMO ? LIMIARES_XP[nivel + 1] : base + 1;
    std::swprintf(t, 160, L"Nível %d", nivel);
    desenharTexto(canvas, r.fonteMedia, 8, ALTURA_LOGICA - 46.0f, t, COR_TEXTO);
    barra(canvas, 8, ALTURA_LOGICA - 28.0f, 110, 6,
          static_cast<float>(b.progresso.xp - base) / static_cast<float>(prox - base), rgb(120, 190, 240));
    if (b.progresso.pontosPendentes > 0) {
        std::swprintf(t, 160, L"+%d pontos", b.progresso.pontosPendentes);
        desenharTexto(canvas, r.fontePequena, 124, ALTURA_LOGICA - 31.0f, t, rgb(255, 214, 90));
    }
    // Centro superior: cronômetro SEMPRE visível (critério M03). Só conta tempo de jogo.
    const int seg = segundosDeJogo(p.stats);
    std::swprintf(t, 160, L"%02d:%02d", seg / 60, seg % 60);
    retanguloAlpha(canvas, LARGURA_LOGICA / 2.0f - 26, 4, 52, 16, rgb(8, 10, 16), 170);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 5, t, COR_TEXTO);
    if (p.emTentativa) {   // onde estou na tentativa
        std::swprintf(t, 160, L"%ls · sala %d de %d", paraWide(nomeBioma(p.plano.bioma)).c_str(), p.etapa + 1,
                      static_cast<int>(p.plano.etapas.size()));
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 20, t, rgb(170, 166, 156));
    }
    // Superior direito: pontuação (mesma fórmula do resultado), dificuldade, abates e a sala.
    desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 110.0f, 8, L"Pontos: " + comMilhar(pontuacaoAtual(p)),
                  rgb(255, 228, 150));
    std::swprintf(t, 160, L"Abates: %d · %ls", p.stats.K, paraWide(nomeDificuldade(p.dificuldade)).c_str());
    desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 110.0f, 21, t, COR_TEXTO);
    if (p.emTentativa) {   // Lembranças que esta tentativa já rendeu (PRD §15.2)
        std::swprintf(t, 160, L"Lembranças +%d", lembrancasDaTentativa(p));
        desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 110.0f, 47, t, rgb(200, 225, 255));
    }
    const int totalOndas = p.sala.modelo->ondas;
    if (totalOndas > 0 && !p.salaConcluida) {
        std::swprintf(t, 160, L"Onda %d de %d", std::max(1, p.ondaAtual), totalOndas);
        desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 110.0f, 34, t, rgb(255, 160, 150));
    } else if (totalOndas > 0) {
        desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 110.0f, 34, L"Portas abertas", rgb(160, 235, 170));
    }
    // Inferior direito: faixa de itens (M09). O kit (Q) sempre aparece; os outros, se houver.
    {
        const TipoItem faixa[] = {TipoItem::Kit, TipoItem::Barquinho, TipoItem::FichaCoragem, TipoItem::Tonico,
                                  TipoItem::Oculos, TipoItem::ChaveEnferrujada, TipoItem::Escudo, TipoItem::MelhoriaArma,
                                  TipoItem::Moeda};
        float x = LARGURA_LOGICA - 8.0f;
        const float y = ALTURA_LOGICA - 62.0f;   // acima da barra do medidor (F1)
        for (int k = static_cast<int>(sizeof faixa / sizeof faixa[0]) - 1; k >= 0; --k) {
            const TipoItem it = faixa[k];
            const int q = quantidadeDe(b, p.inventario, it);
            if (q <= 0 && it != TipoItem::Kit) continue;
            std::swprintf(t, 160, L"%d", q);
            const float w = it == TipoItem::Moeda ? 40.0f : 30.0f;
            x -= w;
            retanguloAlpha(canvas, x, y, w - 3, 20, rgb(8, 10, 16), 180);
            desenharIcone(canvas, r, it, x + 2, y + 2);
            desenharTexto(canvas, r.fontePequena, x + 19, y + 3, t, q > 0 ? COR_TEXTO : rgb(120, 116, 110));
            if (it == TipoItem::Kit) desenharTexto(canvas, r.fontePequena, x + 2, y - 12, ajuda("{KIT}"), rgb(170, 166, 156));
        }
    }
    // Atalhos (rodapé). Com menu ou diálogo aberto, somem (a caixa ocupa a parte de baixo).
    if (p.pendencia == Pendencia::Nenhuma) desenharTexto(canvas, r.fontePequena, 150, ALTURA_LOGICA - 16.0f,
                  ajuda("{ATACAR} atacar · {ESPECIAL} especial · {CLARAO} clarão · {DASH} dash · {KIT} kit · {INTERAGIR} interagir · {FICHA} ficha"),
                  rgb(150, 146, 136));
    if (jogo.mostrarMedidor) {
        std::swprintf(t, 160, L"quadro: média %.2f ms · p95 %.2f ms · escala %d× · hostis %d · seed %llu",
                      mediaMs(jogo.medidor), percentil95Ms(jogo.medidor), jogo.escalaAtual, hostisVivos(p),
                      static_cast<unsigned long long>(p.seed));
        retanguloAlpha(canvas, 150, ALTURA_LOGICA - 34.0f, 470, 15, rgb(8, 10, 16), 190);
        desenharTexto(canvas, r.fontePequena, 154, ALTURA_LOGICA - 33.0f, t, rgb(150, 230, 160));
    }
}

// ---------- caixa de diálogo (Etapa 7.3, PRD §15.3) ----------
std::string emMinusculas(std::string s) {
    for (char& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return s;
}

void desenharDialogo(Canvas& canvas, const Recursos& r, const EstadoPartida& p, bool controle, float tempo) {
    const EstadoDialogo& d = p.dialogo;
    if (p.pendencia != Pendencia::Dialogo || !d.atual) return;
    const Dialogo& dl = *d.atual;
    const LinhaDialogo& l = dl.linhas[static_cast<std::size_t>(d.linha)];
    const float x = 12, w = LARGURA_LOGICA - 24, h = 100, y = ALTURA_LOGICA - h - 10;
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 70);   // o mundo parou
    painel(canvas, x, y, w, h);
    // Retrato 64×64: Bill à direita (ele responde), os outros à esquerda.
    const bool bill = l.falante == "Bill";
    const float rx = bill ? x + w - 70 : x + 6, ry = y + (h - 64) / 2;
    const Sprite retrato = spriteDe(r, "retrato_" + emMinusculas(l.falante));
    retanguloAlpha(canvas, rx, ry, 64, 64, rgb(30, 32, 44), 255);
    if (spriteValido(retrato)) desenharFrame(canvas, retrato, 0, 0, rx, ry);
    contornoRetangulo(canvas, rx, ry, 64, 64, rgb(255, 214, 120));
    const float tx = bill ? x + 12 : x + 80, tw = w - 92;
    desenharTexto(canvas, r.fonteMedia, tx, y + 6, paraWide(l.falante), COR_UI_DESTAQUE);
    // Texto com digitação. As letras contam no texto original; com os nomes de botão trocados
    // o tamanho muda, então a parte visível é proporcional (termina junto).
    const std::wstring texto = paraWide(trocarBotoes(l.texto, controle));
    const float totalOriginal = static_cast<float>(std::max(1, contarCaracteres(l.texto)));
    int visiveis = static_cast<int>(std::ceil(static_cast<float>(texto.size()) * std::min(1.0f, d.letras / totalOriginal)));
    if (d.escolhendo) visiveis = static_cast<int>(texto.size());
    float ly = y + 28;
    for (const std::wstring& linha : quebrarLinhas(canvas, r.fonteMedia, texto, tw)) {
        if (visiveis <= 0) break;
        desenharTexto(canvas, r.fonteMedia, tx, ly, linha.substr(0, static_cast<std::size_t>(visiveis)), COR_UI_TEXTO);
        visiveis -= static_cast<int>(linha.size()) + 1;   // +1: o espaço da quebra
        ly += 19;
    }
    const bool completa = d.letras >= totalOriginal;
    if (completa && !d.escolhendo && std::fmod(tempo, 0.8f) < 0.5f) {   // ▼ = pode avançar
        desenharTexto(canvas, r.fontePequena, bill ? x + w - 88 : x + w - 18, y + h - 16, L"▼", COR_UI_DESTAQUE);
    }
    desenharTexto(canvas, r.fontePequena, tx, y + h - 15,
                  ajuda(controle ? "A: avançar · B: pular" : "Enter ou J: avançar · Esc: pular"), COR_UI_APAGADO);
    if (d.escolhendo) {   // escolhas (só no Clube): painel acima da caixa
        const float n = static_cast<float>(dl.escolhas.size());
        const float ex = LARGURA_LOGICA / 2.0f - 170, eh = 12 + 26 * n, ey = y - eh - 6;
        painel(canvas, ex, ey, 340, eh);
        for (std::size_t i = 0; i < dl.escolhas.size(); ++i) {
            linhaMenu(canvas, r, ex + 8, ey + 8 + 26.0f * static_cast<float>(i), 324, paraWide(dl.escolhas[i].texto),
                      p.cursor == static_cast<int>(i));
        }
    }
}

// ---------- escolher Eco: 3 cartas (Etapa 8.1, PRD §12) ----------
void desenharEscolhaEco(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    if (p.pendencia != Pendencia::EscolherEco || p.ofertasEco.empty()) return;
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 160);
    const Amigo dono = definicaoEco(p.ofertasEco[0].definicao).amigo;
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 12,
                              L"Eco do " + paraWide(nomeAmigo(dono)), COR_UI_DESTAQUE);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 42,
                              L"Uma lembrança que vale até o fim da noite. Escolha uma.", COR_UI_APAGADO);
    const int n = static_cast<int>(p.ofertasEco.size());
    const float w = 180, h = 236, gap = 14;
    const float x0 = LARGURA_LOGICA / 2.0f - (w * static_cast<float>(n) + gap * static_cast<float>(n - 1)) / 2.0f;
    const Sprite retrato = spriteDe(r, std::string("retrato_") + codigoAmigo(dono));
    for (int i = 0; i < n; ++i) {
        const OfertaEco& o = p.ofertasEco[static_cast<std::size_t>(i)];
        const DefinicaoEco& d = definicaoEco(o.definicao);
        const bool sel = p.cursor == i;
        const float x = x0 + (w + gap) * static_cast<float>(i), y = sel ? 62.0f - 4.0f * std::fabs(std::sin(tempo * 3.0f)) : 66.0f;
        const std::uint32_t cor = corDaRaridade(o.raridade);
        retanguloAlpha(canvas, x, y, w, h, rgb(14, 16, 26), 250);
        contornoRetangulo(canvas, x, y, w, h, cor);
        if (sel) contornoRetangulo(canvas, x - 2, y - 2, w + 4, h + 4, COR_UI_DESTAQUE);
        if (spriteValido(retrato)) desenharFrame(canvas, retrato, 0, 0, x + w / 2 - 32, y + 12);
        desenharTextoCentralizado(canvas, r.fonteMedia, x + w / 2, y + 84, paraWide(d.nome), sel ? rgb(255, 240, 200) : COR_TEXTO);
        desenharTextoCentralizado(canvas, r.fontePequena, x + w / 2, y + 106, paraWide(nomeRaridade(o.raridade)), cor);
        float ly = y + 132;
        for (const std::wstring& linha : quebrarLinhas(canvas, r.fontePequena, paraWide(descricaoEco(o.definicao, o.raridade)), w - 20)) {
            desenharTextoCentralizado(canvas, r.fontePequena, x + w / 2, ly, linha, rgb(200, 194, 180));
            ly += 16;
        }
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 30,
                              ajuda("← → escolher · {CONFIRMAR} ficar com este Eco"), rgb(170, 166, 156));
}

// ---------- Loja do Elias (Etapa 8.2, PRD §11.3) ----------
void desenharLoja(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    if (p.pendencia != Pendencia::Loja) return;
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 150);
    const float x = 60, y = 24, w = LARGURA_LOGICA - 120, h = 312;
    painel(canvas, x, y, w, h);
    const Sprite retrato = spriteDe(r, "retrato_elias");
    if (spriteValido(retrato)) desenharFrame(canvas, retrato, 0, 0, x + 10, y + 10);
    contornoRetangulo(canvas, x + 10, y + 10, 64, 64, rgb(255, 214, 120));
    desenharTexto(canvas, r.fonteGrande, x + 86, y + 8, L"Loja do Elias", COR_UI_DESTAQUE);
    desenharTexto(canvas, r.fontePequena, x + 86, y + 44, paraWide(falaDoElias(p.assassinoAnterior, p.conversasElias)),
                  rgb(200, 194, 180));
    wchar_t t[96];
    const int moedas = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)];
    desenharIcone(canvas, r, TipoItem::Moeda, x + w - 96, y + 12);
    std::swprintf(t, 96, L"%d", moedas);
    desenharTexto(canvas, r.fonteMedia, x + w - 74, y + 10, t, COR_UI_DESTAQUE);
    const Sprite altar = spriteDe(r, "objetos_comuns");
    for (int i = 0; i < static_cast<int>(p.loja.size()); ++i) {
        const OfertaLoja& o = p.loja[static_cast<std::size_t>(i)];
        const float ly = y + 90 + 34.0f * static_cast<float>(i);
        const bool pode = !o.vendida && o.preco <= moedas;
        linhaMenu(canvas, r, x + 10, ly, w - 20, L"", p.cursor == i, o.vendida);
        if (o.item == TipoItem::Total) {   // Eco: o altar como ícone
            if (spriteValido(altar)) desenharFramePes(canvas, altar, 2, 1, x + 44, ly + 18);
        } else {
            desenharIcone(canvas, r, o.item, x + 36, ly);
        }
        desenharTexto(canvas, r.fonteMedia, x + 62, ly, paraWide(nomeDaOferta(p, o)), o.vendida ? COR_UI_APAGADO : COR_TEXTO);
        desenharTexto(canvas, r.fontePequena, x + 62, ly + 18,
                      o.item != TipoItem::Total ? paraWide(definicaoDe(o.item).descricao)
                                                : L"Um Eco que você ainda não tem (Comum, Raro ou Épico), na hora.",
                      rgb(150, 146, 136));
        std::swprintf(t, 96, o.vendida ? L"vendido" : L"%d moedas", o.preco);
        desenharTexto(canvas, r.fonteMedia, x + w - 110, ly, t,
                      o.vendida ? COR_UI_APAGADO : (pode ? rgb(255, 236, 170) : rgb(255, 160, 150)));
    }
    if (p.tempoAvisoMenu > 0.0f) {   // compra em verde; recusa em tom de alerta
        const bool comprou = p.avisoMenu.rfind("Comprou", 0) == 0;
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, y + h - 42, paraWide(p.avisoMenu),
                                  comprou ? rgb(160, 235, 170) : rgb(255, 170, 150));
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + h - 18,
                              ajuda("↑↓ escolher · {CONFIRMAR} comprar · {VOLTAR} sair da loja"), rgb(170, 166, 156));
}

// Barreira de tijolos do Eco "Ponte firme" (onde o dash terminou).
void desenharBarreiras(Canvas& canvas, const EstadoPartida& p, float camX, float camY) {
    for (const BarreiraTijolos& t : p.barreiras) {
        const std::uint8_t alfa = static_cast<std::uint8_t>(255.0f * std::min(1.0f, t.vida / 0.3f));
        for (int k = 0; k < 10; ++k) {
            const float a = static_cast<float>(k) * 0.6283f;
            const float bx = t.centro.x + RAIO_BARREIRA_DASH * std::cos(a) - camX, by = t.centro.y - 8 + 0.6f * RAIO_BARREIRA_DASH * std::sin(a) - camY;
            retanguloAlpha(canvas, bx - 4, by - 3, 8, 6, rgb(160, 80, 55), alfa);
        }
    }
}

// ---------- Clube (Etapa 7.2) ----------
// Balão de fala sobre o amigo (até a caixa de diálogo com retrato da 7.3).
void desenharBalao(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float camX, float camY) {
    if (p.balao.vida <= 0.0f || p.balao.objeto < 0 || p.balao.objeto >= static_cast<int>(p.sala.objetos.size())) return;
    const ObjetoSala& o = p.sala.objetos[static_cast<std::size_t>(p.balao.objeto)];
    const std::wstring t = paraWide(p.balao.texto);
    const float w = 5.2f * static_cast<float>(t.size()) + 16;
    const float x = limitar(static_cast<float>(o.celula.col * TILE + TILE / 2) - camX, w / 2 + 4, LARGURA_LOGICA - w / 2 - 4);
    const float y = static_cast<float>(o.celula.lin * TILE) - 44.0f - camY;
    const std::uint8_t alfa = static_cast<std::uint8_t>(235.0f * std::min(1.0f, p.balao.vida / 0.4f));
    retanguloAlpha(canvas, x - w / 2, y - 3, w, 18, rgb(250, 244, 228), alfa);
    contornoRetangulo(canvas, x - w / 2, y - 3, w, 18, rgb(60, 52, 40));
    const float bx = static_cast<float>(o.celula.col * TILE + TILE / 2) - camX;
    retanguloAlpha(canvas, bx - 2, y + 15, 4, 5, rgb(250, 244, 228), alfa);   // "rabinho" do balão
    desenharTextoCentralizado(canvas, r.fontePequena, x, y, t, rgb(40, 34, 28));
}

void desenharHudClube(Canvas& canvas, const Recursos& r, const Jogo& jogo) {
    const Perfil& pf = jogo.perfil;
    const EstadoPartida& p = jogo.salaAtual.partida;
    wchar_t t[160];
    if (p.noPrologo) {   // prólogo: só o objetivo (ainda não há Lembranças nem Clube)
        retanguloAlpha(canvas, 4, 4, 300, 18, rgb(8, 10, 16), 170);
        desenharTexto(canvas, r.fontePequena, 10, 7,
                      p.salaConcluida ? L"Siga pela rua até a entrada da galeria (→)." : L"O Mike está ali. Vá falar com ele.",
                      COR_TEXTO);
        if (p.pendencia == Pendencia::Nenhuma) {
            desenharTexto(canvas, r.fontePequena, 150, ALTURA_LOGICA - 16.0f, ajuda("{MOVER}: andar · {INTERAGIR}: interagir · {PAUSA}: pausa"),
                          rgb(150, 146, 136));
        }
        return;
    }
    retanguloAlpha(canvas, LARGURA_LOGICA - 150.0f, 4, 146, 32, rgb(8, 10, 16), 180);
    std::swprintf(t, 160, L"Lembranças: %d", pf.lembrancas);
    desenharTexto(canvas, r.fonteMedia, LARGURA_LOGICA - 144.0f, 5, t, rgb(200, 225, 255));
    std::swprintf(t, 160, L"tentativas %d · vitórias %d", pf.tentativas, pf.vitorias);
    desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 144.0f, 22, t, COR_UI_APAGADO);
    // Estante dos Perdedores (PRD §13.2): uma Lembrancinha por bioma; as achadas aparecem.
    retanguloAlpha(canvas, LARGURA_LOGICA - 150.0f, 40, 146, 26, rgb(8, 10, 16), 180);
    desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 144.0f, 46, L"Estante:", COR_UI_APAGADO);
    for (int b = 1; b <= 6; ++b) {
        const float x = LARGURA_LOGICA - 98.0f + 15.0f * static_cast<float>(b - 1);
        if ((pf.lembrancinhas >> b) & 1u) desenharIcone(canvas, r, TipoItem::Lembrancinha, x - 2, 44);
        else retanguloAlpha(canvas, x + 2, 48, 9, 9, rgb(60, 58, 52), 200);
    }
    retanguloAlpha(canvas, 4, 4, 236, 18, rgb(8, 10, 16), 170);
    desenharTexto(canvas, r.fontePequena, 10, 7, L"Desça pelo alçapão quando estiver pronto.", COR_TEXTO);
    if (jogo.salaAtual.partida.pendencia == Pendencia::Nenhuma) {
        desenharTexto(canvas, r.fontePequena, 150, ALTURA_LOGICA - 16.0f,
                      ajuda("{MOVER}: andar · {INTERAGIR}: interagir · {PAUSA}: pausa"), rgb(150, 146, 136));
    }
}

// Mural dos Perdedores: os 9 talentos do PRD §13.1 com nível, efeito e preço do próximo nível.
void desenharQuadro(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const std::vector<Recorde>& recordes) {
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 140);
    painel(canvas, 150, 24, 340, 300);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 30, L"Quadro de recordes", COR_UI_DESTAQUE);
    desenharRecordes(canvas, r, 170, 66, 300, recordes, static_cast<FiltroRecordes>(p.filtroQuadro), nullptr);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 304, ajuda("← → filtro · {VOLTAR} fechar"),
                              rgb(170, 166, 156));
}

void desenharAlcapao(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const TabelaDificuldade& tabela) {
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 150);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 20, L"Descer pelo alçapão", COR_UI_DESTAQUE);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 46,
                              L"Escolha a dificuldade desta noite (muda probabilidades e o multiplicador de pontos).",
                              COR_UI_APAGADO);
    desenharOpcoesDificuldade(canvas, r, tabela, p.cursor, 66);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 286,
                              ajuda("↑↓ escolher · {CONFIRMAR} descer · {VOLTAR} ficar no Clube"), rgb(170, 166, 156));
}

// Bill insistiu numa porta fechada (23/11): um painel no alto explica por que ela não abre. Entra
// deslizando 0,25 s.
void desenharAvisoPorta(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    if (p.tempoAvisoPorta <= 0.0f || p.avisoPorta.empty() || p.pendencia != Pendencia::Nenhuma) return;
    const float largura = 420.0f;
    const std::vector<std::wstring> linhas = quebrarLinhas(canvas, r.fonteMedia, paraWide(p.avisoPorta), largura - 28);
    const float altura = 30.0f + 16.0f * static_cast<float>(linhas.size());
    const float entrada = std::min(1.0f, (5.0f - p.tempoAvisoPorta) / 0.25f);
    const float x = (LARGURA_LOGICA - largura) / 2.0f, y = 64.0f - 10.0f * (1.0f - entrada);
    painel(canvas, x, y, largura, altura);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 6, L"PORTA FECHADA", COR_UI_DESTAQUE);
    for (std::size_t i = 0; i < linhas.size(); ++i) {
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, y + 22 + 16.0f * static_cast<float>(i), linhas[i], COR_TEXTO);
    }
}

// Faixa "COMPUTADOR JOGANDO"; com F1, o objetivo atual e o caminho planejado no chão.
void desenharPiloto(Canvas& canvas, const Recursos& r, const MemoriaPiloto& m, bool detalhes, float camX, float camY,
                    float y = 38) {   // desce quando há a barra do chefe
    retanguloAlpha(canvas, LARGURA_LOGICA / 2.0f - 120, y, 240, 20, rgb(10, 30, 60), 200);
    contornoRetangulo(canvas, LARGURA_LOGICA / 2.0f - 120, y, 240, 20, rgb(110, 170, 255));
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 4, ajuda("COMPUTADOR JOGANDO · {PAUSA}: opções"),
                              rgb(200, 225, 255));
    if (!detalhes) return;
    for (const Celula& c : m.caminho) {
        const Vec2 centro = centroCelula(c);
        elipseAlpha(canvas, centro.x - camX, centro.y - camY, 3, 3, rgb(110, 170, 255), 200);
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + 24, paraWide("Objetivo: " + m.descricao),
                              rgb(170, 205, 255));
}

}  // namespace

void retomarSomDaSala(Jogo& jogo) {
    const EstadoPartida& p = jogo.salaAtual.partida;
    if (p.terminou || !p.sala.modelo) {   // tela de resultado: silêncio
        definirMusica(IdSom::Total);
        ligarAmbiente(IdSom::Total);
        return;
    }
    definirMusica(IdSom::MusicaJogo);
    ligarAmbiente(IdSom::AmbienteMasmorra);
    // Sala marcada com musica=tensao: a trilha de tensão entra em crossfade.
    definirTensao(p.sala.modelo->musicaTensao ? 1.0f : 0.0f);
}

// ---------- resultado: grava o recorde (uma vez) e carrega a lista ----------
std::string dataDeHoje() {
    const std::time_t agora = std::time(nullptr);
    const std::tm* t = std::localtime(&agora);
    char texto[32] = "";
    if (t) std::strftime(texto, sizeof texto, "%Y-%m-%d %H:%M", t);
    return texto;
}

// Telemetria (PRD §18): começa sozinha quando há tentativa e acompanha cada passo de lógica.
// O passo que COMEÇOU a medir não conta (a troca de sala dele é a entrada na 1ª sala).
void acompanharTelemetria(EstadoSala& s, float dtReal) {
    const EstadoPartida& p = s.partida;
    if (!p.emTentativa) { s.telemetria.ativa = false; return; }
    if (!s.telemetria.ativa) {
        if (!p.terminou && !s.resultadoPronto) iniciarTelemetria(s.telemetria, p);
        return;
    }
    passoTelemetria(s.telemetria, p, dtReal);
}

// Acrescenta a tentativa a saves/telemetria.csv (pessoa e computador; a coluna "quem" separa).
void gravarTelemetria(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    const EstadoPartida& p = s.partida;
    if (!s.telemetria.ativa) return;
    encerrarTelemetria(s.telemetria, p);
    const char* fim = p.abandonou ? "abandonou" : p.vitoria ? "vitoria" : p.fimDoConteudo ? "completou" : "morreu";
    const std::filesystem::path arquivo = std::filesystem::path(jogo.pastaSaves) / L"telemetria.csv";
    std::string atual;
    lerArquivoTexto(arquivo, atual);   // não existe ainda: começa vazio (com cabeçalho)
    gravarArquivoTexto(arquivo, anexarTelemetria(atual, csvTelemetria(s.telemetria, dataDeHoje(), p.seed, s.pilotoJogou,
                                                                        p.dificuldade, fim)));
}

// O recorde desta tentativa como está agora (no fim ou ao derrotar um chefão).
Recorde recordeAtual(const Jogo& jogo) {
    const EstadoSala& s = jogo.salaAtual;
    const EstadoPartida& p = s.partida;
    Recorde r;
    r.pontos = pontuacaoAtual(p);
    r.data = dataDeHoje();
    r.dificuldade = p.dificuldade;
    r.tempo = segundosDeJogo(p.stats);
    r.bioma = p.emTentativa ? p.plano.bioma : p.sala.modelo->bioma;
    r.vitoria = p.vitoria;
    r.seed = p.seed;
    r.piloto = s.pilotoJogou;
    return r;
}

// Chefão derrotado (Diego, 24/11): grava o recorde da tentativa já, sem esperar o fim.
void gravarRecordeDoChefe(Jogo& jogo) {
    const std::filesystem::path arquivo = std::filesystem::path(jogo.pastaSaves) / L"recordes.txt";
    std::vector<Recorde> lista = carregarRecordes(arquivo);
    registrarRecordeDaTentativa(lista, recordeAtual(jogo));
    gravarRecordes(arquivo, lista);
}

void prepararResultado(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    const EstadoPartida& p = s.partida;
    gravarTelemetria(jogo);
    s.resultadoPronto = true;
    s.colocacao = 0;
    const std::filesystem::path arquivo = std::filesystem::path(jogo.pastaSaves) / L"recordes.txt";
    s.recordes = carregarRecordes(arquivo);
    s.lembrancasGanhas = 0;
    if (p.abandonou) {   // abandonar não grava pontuação (SDD §16); as Lembranças ficam
        if (p.emTentativa) {
            s.lembrancasGanhas = registrarFimDeTentativa(jogo.perfil, p, s.pilotoJogou);
            s.lembrancasTotal = jogo.perfil.lembrancas;
            gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
        }
        return;
    }
    const Recorde r = recordeAtual(jogo);
    s.colocacao = registrarRecordeDaTentativa(s.recordes, r);   // substitui a linha gravada nos chefões
    gravarRecordes(arquivo, s.recordes);
    if (p.emTentativa) {   // Lembranças e contadores no perfil (o piloto não ganha para você)
        s.lembrancasGanhas = registrarFimDeTentativa(jogo.perfil, p, s.pilotoJogou);
        s.lembrancasTotal = jogo.perfil.lembrancas;
        gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
    }
    s.filtro = r.piloto ? FiltroRecordes::Computador : FiltroRecordes::Jogador;
}

// Começa uma partida: a tentativa gerada (padrão) ou uma sala avulsa (--sala N, testes).
void comecar(Jogo& jogo, std::uint64_t seed, Dificuldade dificuldade) {
    EstadoPartida& p = jogo.salaAtual.partida;
    std::string erro;
    if (jogo.salaInicial >= 0 || !iniciarTentativa(p, *jogo.conteudo, seed, dificuldade, erro, jogo.biomaInicial)) {
        iniciarPartida(p, *jogo.conteudo, seed, std::max(0, jogo.salaInicial), dificuldade);
    } else if (!jogo.pilotoAtivo) {
        aplicarTalentos(jogo.perfil, p);   // o Mural vale para quem joga (o computador joga "limpo")
        p.pontoDaSala = capturarPonto(p, p.indiceSala);   // o ponto da 1ª sala com os talentos
    }
    if (jogo.forte && jogo.salaInicial >= 0) chegarComoNaTentativa(p);   // Modo Defesa (--forte)
    if (jogo.salaInicial >= 0 && p.sala.modelo->tipo == TipoSala::Combate && !jogo.inimigoDefesa.empty()) {
        for (int i = 0; i < TOTAL_TIPOS_INIMIGO; ++i) {
            const TipoInimigo tipo = static_cast<TipoInimigo>(i);
            if (jogo.inimigoDefesa != parametrosDe(tipo).atlas || parametrosDe(tipo).chefe) continue;
            const Celula b = celulaDe(p.bill.pes);
            const Celula posicoes[] = {{b.col + 2, b.lin}, {b.col - 2, b.lin}, {b.col, b.lin + 2}, {b.col, b.lin - 2}};
            for (Celula cel : posicoes) {
                if (!dentroDaSala(p.sala, cel.col, cel.lin) || tileEm(p.sala, cel.col, cel.lin).bloqueiaMovimento) continue;
                p.inimigos.clear();
                p.inimigos.push_back(criarInimigo(tipo, p.proximoId++, centroCelula(cel)));
                p.ondaAtual = 1;
                p.timerOnda = 1.5f;
                break;
            }
            break;
        }
    }
    if (amigoDoCodigo(jogo.amigoDefesa) != Amigo::Total) {   // Modo Defesa (--amigo stan), também na tentativa
        entrarCompanheiro(p, amigoDoCodigo(jogo.amigoDefesa));
    }
    p.chefeNaFase2 = jogo.pennywise && jogo.salaInicial >= 0;           // Modo Defesa (--pennywise)
    p.faseCovilInicial = jogo.salaInicial >= 0 ? jogo.faseCovil : 1;   // Modo Defesa (--fase-covil N)
    p.bill.progresso.pontosPendentes += jogo.pontosIniciais;   // Modo Defesa (--pontos)
    if (jogo.itensDefesa) {   // Modo Defesa (--itens): um de cada, para demonstrar M08/M09
        for (TipoItem t : {TipoItem::Escudo, TipoItem::Kit, TipoItem::FichaCoragem, TipoItem::Tonico, TipoItem::Oculos}) {
            coletarNoInventario(p.bill, p.inventario, t, 1);
        }
        coletarNoInventario(p.bill, p.inventario, TipoItem::MelhoriaArma, 2);
    }
    for (const std::string& nome : jogo.ecosDefesa) {   // --eco: o Eco Épico (o lendário, Lendário) já ativo
        for (int i = 0; i < totalEcos(); ++i) {
            if (nome != definicaoEco(i).nome) continue;
            aplicarEco(p, i, definicaoEco(i).lendario ? Raridade::Lendario : Raridade::Epico);
        }
    }
    jogo.salaAtual.piloto = MemoriaPiloto{};
    jogo.salaAtual.pilotoJogou = jogo.pilotoAtivo;
    jogo.salaAtual.resultadoPronto = false;
    jogo.salaAtual.chefesNoRecorde = 0;
    jogo.salaAtual.colocacao = 0;
    aoEntrarNaSala(jogo);
}

// Clube dos Perdedores (hub): começo de cada noite. Seed nova a cada visita (a tentativa usa a
// seed do Clube). O perfil é gravado ao entrar (SDD §16), menos assistindo o computador.
void entrarClube(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    jogo.seed = jogo.seed * 6364136223846793005ULL + 1442695040888963407ULL;
    const Dificuldade sugerida = jogo.pilotoAtivo ? jogo.dificuldade : jogo.perfil.ultimaDificuldade;
    if (!iniciarClube(s.partida, *jogo.conteudo, jogo.seed, sugerida)) {   // conteúdo sem Clube: direto à tentativa
        comecar(jogo, jogo.seed, jogo.dificuldade);
        return;
    }
    prepararVisitaAoClube(s.partida, *jogo.conteudo, jogo.perfil, jogo.pilotoAtivo);   // "!" e dicas
    s.piloto = MemoriaPiloto{};
    s.piloto.dificuldadeDesejada = sugerida;
    s.pilotoJogou = jogo.pilotoAtivo;
    s.resultadoPronto = false;
    s.chefesNoRecorde = 0;
    s.colocacao = 0;
    s.recordes = carregarRecordes(std::filesystem::path(jogo.pastaSaves) / L"recordes.txt");
    if (!jogo.pilotoAtivo) gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
    aoEntrarNaSala(jogo);
}

// Prólogo (PRD §4.3): só na 1ª vez, para quem joga (assistir o computador começa no Clube).
void entrarPrologo(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    jogo.seed = jogo.seed * 6364136223846793005ULL + 1442695040888963407ULL;
    if (!iniciarPrologo(s.partida, *jogo.conteudo, jogo.seed, jogo.perfil.ultimaDificuldade)) {
        entrarClube(jogo);   // conteúdo sem prólogo: direto ao Clube
        return;
    }
    prepararPrologo(s.partida, jogo.perfil, jogo.pilotoAtivo);
    s.piloto = MemoriaPiloto{};
    s.piloto.dificuldadeDesejada = jogo.perfil.ultimaDificuldade;
    s.pilotoJogou = jogo.pilotoAtivo;
    s.resultadoPronto = false;
    s.chefesNoRecorde = 0;
    s.colocacao = 0;
    aoEntrarNaSala(jogo);
}

// "Carregar jogo" (cartão de memória, 23/11): a tentativa volta à entrada da sala salva.
void carregarJogoSalvo(Jogo& jogo) {
    jogo.carregarPonto = false;
    EstadoSala& s = jogo.salaAtual;
    jogo.salaInicial = -1;
    jogo.fluxoClube = true;   // morrer volta ao Clube, como numa tentativa comum
    jogo.pilotoAtivo = false;
    jogo.velocidade = 1;
    std::string erro;
    if (!restaurarPonto(s.partida, *jogo.conteudo, jogo.pontoParaCarregar, erro)) {
        mostrarAviso(jogo, L"Não foi possível carregar: " + paraWide(erro), 4.0f);
        entrarClube(jogo);
        return;
    }
    jogo.dificuldade = s.partida.dificuldade;
    jogo.seed = s.partida.seed;
    s.piloto = MemoriaPiloto{};
    s.pilotoJogou = false;
    s.resultadoPronto = false;
    s.chefesNoRecorde = 0;
    s.colocacao = 0;
    s.biomaDoCartao = -1;   // o cartão do bioma aparece de novo: "você está aqui"
    aoEntrarNaSala(jogo);
}

void entrarSala(Jogo& jogo) {
    if (jogo.conteudo->salas.empty()) return;
    if (jogo.carregarPonto) {
        carregarJogoSalvo(jogo);
        return;
    }
    if (jogo.salaInicial >= 0 || !jogo.fluxoClube) comecar(jogo, jogo.seed, jogo.dificuldade);
    else if (!jogo.pilotoAtivo && !jogo.perfil.prologoVisto) entrarPrologo(jogo);
    else entrarClube(jogo);
}

// Pedidos do Clube (Mural, alçapão, voltar ao menu). Devolve false se a cena foi trocada.
bool atenderPedidosDoClube(Jogo& jogo) {
    EstadoSala& s = jogo.salaAtual;
    EstadoPartida& p = s.partida;
    // Falas lidas e dicas vistas vão para o perfil assim que acontecem (Clube ou tentativa).
    if (sincronizarPerfil(jogo.perfil, p, jogo.pilotoAtivo)) {
        gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
    }
    if (!p.noClube && !p.noPrologo) return true;
    if (p.pedidoMenu) {
        trocarCenaBase(jogo, IdCena::Menu);
        return false;
    }
    const RespostaClube rc = atenderClube(jogo.perfil, p, *jogo.conteudo, jogo.pilotoAtivo);
    if (rc.perfilMudou && !jogo.pilotoAtivo) gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
    if (rc.comecouTentativa) {
        jogo.dificuldade = p.dificuldade;
        const Dificuldade desejada = s.piloto.dificuldadeDesejada;
        s.piloto = MemoriaPiloto{};
        s.piloto.dificuldadeDesejada = desejada;
        s.pilotoJogou = jogo.pilotoAtivo;
        s.resultadoPronto = false;
        s.chefesNoRecorde = 0;
        s.colocacao = 0;
    }
    return true;
}

void atualizarSala(Jogo& jogo, const EstadoEntrada& entrada, float dt) {
    EstadoSala& s = jogo.salaAtual;
    EstadoPartida& p = s.partida;
    if (s.telaCiclo > 0.0f) {   // ilustração na frente; o Clube espera (Confirmar pula depois de 1 s)
        s.telaCiclo -= dt;
        const bool pular = pressionou(entrada, Comando::Confirmar) || pressionou(entrada, Comando::Atacar) ||
                           pressionou(entrada, Comando::Voltar);
        if (pular && s.telaCiclo < DURACAO_TELA_CICLO - 1.0f) s.telaCiclo = 0.0f;
        return;
    }
    // Cartão do bioma (Etapa 10.5): ao chegar a um bioma da tentativa, a ilustração e o nome, como o
    // nome da região em Hades. A cena espera (o cronômetro não conta; ninguém anda às cegas).
    if (!p.emTentativa) s.biomaDoCartao = -1;
    else if (!p.terminou && p.plano.bioma != s.biomaDoCartao) {
        s.biomaDoCartao = p.plano.bioma;
        s.cartaoBioma = DURACAO_CARTAO_BIOMA;
        if (abrirCenaDaChegada(jogo, p.plano.bioma)) return;   // a cena da história e, depois, o cartão
    }
    if (s.cartaoBioma > 0.0f) {
        s.cartaoBioma -= dt;
        const bool pular = pressionou(entrada, Comando::Confirmar) || pressionou(entrada, Comando::Atacar) ||
                           pressionou(entrada, Comando::Voltar);
        if (pular && s.cartaoBioma < DURACAO_CARTAO_BIOMA - 0.8f) s.cartaoBioma = 0.0f;
        return;
    }
    if (p.terminou) {
        // Tela de resultado. Filtro dos recordes com < > (ou ← →); Enter/J: nova tentativa
        // (na Etapa 7, volta ao Clube). Com o piloto ligado, recomeça sozinho depois de 8 s.
        int f = static_cast<int>(s.filtro);
        if (pressionou(entrada, Comando::AbaDireita) || pressionou(entrada, Comando::Direita)) f = (f + 1) % 3;
        if (pressionou(entrada, Comando::AbaEsquerda) || pressionou(entrada, Comando::Esquerda)) f = (f + 2) % 3;
        s.filtro = static_cast<FiltroRecordes>(f);
        if (pressionou(entrada, Comando::Voltar) && jogo.salaInicial < 0 && !p.vitoria) {   // Esc: volta ao menu
            trocarCenaBase(jogo, IdCena::Menu);
            return;
        }
        if (pressionou(entrada, Comando::Confirmar) || pressionou(entrada, Comando::Atacar) ||
            (p.vitoria && pressionou(entrada, Comando::Voltar)) ||
            (jogo.pilotoAtivo && s.piloto.quadro++ % 480 == 479)) {
            if (jogo.fluxoClube) {   // Etapa 7.2: Bill acorda no Clube
                if (p.vitoria && !jogo.pilotoAtivo) {
                    trocarCenaBase(jogo, IdCena::Menu);
                    jogo.menu.tela = TelaMenu::Creditos;
                    jogo.menu.creditosDaVitoria = true;
                    return;
                }
                // 1ª morte: a tela "Bill acorda no Clube" revela o ciclo (PRD §4.3).
                const bool primeiraMorte = !s.pilotoJogou && !p.vitoria && !p.abandonou && !p.fimDoConteudo &&
                                           jogo.perfil.mortes == 1;
                entrarClube(jogo);
                if (primeiraMorte) s.telaCiclo = DURACAO_TELA_CICLO;
                return;
            }
            if (!p.emTentativa) jogo.salaInicial = p.indiceSala;   // avulsa: recomeça na mesma sala
            comecar(jogo, p.seed + 1, p.dificuldade);
            return;
        }
        atualizarPartida(p, *jogo.conteudo, entrada, dt);   // só anima os textos
        return;
    }
    // Computador jogando (Diego, 24/11: "não consigo voltar pro menu, nem Start nem Esc"): os comandos do piloto
    // substituem os da pessoa, então a pausa nunca abria. Start/Esc da PESSOA abrem um menu próprio, com o jogo parado.
    if (jogo.pilotoAtivo && !s.menuAssistir &&
        (pressionou(entrada, Comando::Pausa) || pressionou(entrada, Comando::Voltar))) {
        s.menuAssistir = true;
        s.cursorAssistir = 0;
        return;
    }
    if (s.menuAssistir) {
        if (!jogo.pilotoAtivo) {   // F10 desligou o piloto com o menu aberto
            s.menuAssistir = false;
        } else {
            if (pressionou(entrada, Comando::Cima)) s.cursorAssistir = (s.cursorAssistir + TOTAL_OPCOES_ASSISTIR - 1) % TOTAL_OPCOES_ASSISTIR;
            if (pressionou(entrada, Comando::Baixo)) s.cursorAssistir = (s.cursorAssistir + 1) % TOTAL_OPCOES_ASSISTIR;
            if (pressionou(entrada, Comando::Pausa) || pressionou(entrada, Comando::Voltar)) {
                s.menuAssistir = false;   // continuar assistindo
            } else if (pressionou(entrada, Comando::Confirmar) || pressionou(entrada, Comando::Atacar)) {
                s.menuAssistir = false;
                if (s.cursorAssistir == 1) {   // assumir o controle: a pessoa joga daqui; a tentativa segue sendo "computador"
                    jogo.pilotoAtivo = false;
                    jogo.velocidade = 1;
                    jogo.menu.assistir = false;
                    mostrarAviso(jogo, L"Você assumiu o controle", 2.5f);
                } else if (s.cursorAssistir == 2) {
                    trocarCenaBase(jogo, IdCena::Menu);
                }
            }
            return;
        }
    }
    // (Removido em 17/11: um atalho de teste trocava de sala com AbaDireita/AbaEsquerda fora da
    // tentativa. No controle, RB é TAMBÉM Interagir: no Clube, no prólogo e nas salas avulsas da
    // demonstração, apertar RB para interagir pulava para o próximo arquivo de sala — o Diego caiu em
    // salas "sem saída" e nunca chegou ao alçapão, onde se escolhe a dificuldade.)

    // Piloto automático: as MESMAS regras recebem os comandos do computador em vez dos do jogador.
    // Assistindo o computador: ×1, ×2 ou ×4 passos de lógica por passo (o jogo é o mesmo, só mais rápido).
    const int passos = jogo.pilotoAtivo ? std::max(1, jogo.velocidade) : 1;
    for (int k = 0; k < passos && !p.terminou; ++k) {
        const EstadoEntrada comandos = jogo.pilotoAtivo ? pilotar(p, s.piloto, dt) : entrada;
        atualizarPartida(p, *jogo.conteudo, comandos, dt);
        if (!atenderPedidosDoClube(jogo)) return;
        acompanharTelemetria(s, k == 0 ? dt : 0.0f);   // tempo real: uma vez por quadro, mesmo no ×4
        for (IdSom som : p.sons) tocarEfeito(som);
        p.sons.clear();
        if (p.trocouDeSala && k + 1 < passos) aoEntrarNaSala(jogo);
    }
    if (jogo.pilotoAtivo) s.pilotoJogou = true;
    if (p.emTentativa && !p.terminou && p.stats.B > s.chefesNoRecorde) {   // um chefão caiu: o recorde já fica gravado
        s.chefesNoRecorde = p.stats.B;
        gravarRecordeDoChefe(jogo);
    }
    if (p.pedidoSalvar) {   // "Salvar jogo" na pausa: o cartão de memória por cima (a pausa continua por baixo)
        p.pedidoSalvar = false;
        abrirCartao(jogo, true);
        return;
    }
    if (p.pedidoSair) jogo.rodando = false;
    if (p.terminou && !s.resultadoPronto) {
        prepararResultado(jogo);
        // Vitória no Covil: a cena final da história (também quando o computador vence) antes do resultado.
        if (p.vitoria && abrirCenaHistoria(jogo, "final")) return;
    }
    if (p.terminou) {
        definirMusica(IdSom::Total);
        if (!p.vitoria) tocarEfeito(IdSom::VinhetaDerrota);
    }
    if (p.trocouDeSala) aoEntrarNaSala(jogo);
    if (s.tempoNome > 0.0f) s.tempoNome -= dt;
    s.camera = somar(s.camera, escalar(subtrair(alvoCamera(p), s.camera), SUAVIZACAO_CAMERA));
}

void desenharSala(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const EstadoSala& s = jogo.salaAtual;
    const EstadoPartida& p = s.partida;
    // Arredondar a câmera para pixel de tela evita que o terreno "trema" 1 px (F02).
    const float camX = static_cast<float>(paraTela(canvas, s.camera.x)) / static_cast<float>(canvas.escala);
    const float camY = static_cast<float>(paraTela(canvas, s.camera.y)) / static_cast<float>(canvas.escala);

    definirControleNaAjuda(jogo.entrada.ultimoUsado == Dispositivo::Gamepad);
    limpar(canvas, rgb(0, 0, 0));
    prepararCacheTerreno(s.cache, p.sala, r, canvas.escala);
    desenharTerreno(canvas, s.cache, p.sala, r, camX, camY);
    desenharArmadilhas(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharPocas(canvas, p, jogo.tempoTotal, camX, camY);
    desenharBrasas(canvas, p, jogo.tempoTotal, camX, camY);
    desenharGolpes(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharTracos(canvas, p, camX, camY);
    desenharEstilhacos(canvas, p, camX, camY);
    desenharAtores(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharNuvens(canvas, p, jogo.tempoTotal, camX, camY);
    desenharNevoasDash(canvas, p, camX, camY);
    if (jogo.mostrarMedidor) {   // F1: caixas de colisão (verde) e de dano (amarelo)
        const Retangulo cx = caixaNosPes(p.bill.pes, CAIXA_BILL);
        contornoRetangulo(canvas, cx.x - camX, cx.y - camY, cx.w, cx.h, rgb(120, 255, 140));
        for (const Inimigo& i : p.inimigos) {
            const Retangulo h = hurtboxInimigo(i);
            contornoRetangulo(canvas, h.x - camX, h.y - camY, h.w, h.h, rgb(255, 220, 90));
        }
    }
    // Escuridão: depois do terreno e dos atores (SDD §7.4, passo 7). Avisos e números ficam
    // por cima dela: todo ataque é anunciado, mesmo vindo da penumbra.
    aplicarEscuridao(canvas, p.visao, camX, camY);
    desenharRevelados(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharCovil(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharParedesRachadas(canvas, p, jogo.tempoTotal, camX, camY);
    desenharEfeitosDoCompanheiro(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharDispersoes(canvas, r, p, camX, camY);
    desenharBaloesDaFesta(canvas, r, p, camX, camY);
    desenharBarreiras(canvas, p, camX, camY);
    if (p.noPrologo) {   // "Um balão vermelho sobe contra o vento" (PRD §4.3), em ciclo sobre o bueiro
        const Sprite balao = spriteDe(r, "balao");
        for (const ObjetoSala& o : p.sala.objetos) {
            if (o.codigo != 'e' || o.quadro != 14 || !spriteValido(balao)) continue;
            const float ciclo = std::fmod(jogo.tempoTotal, 9.0f) / 9.0f;
            const float bx = static_cast<float>(o.celula.col * TILE + TILE / 2) + 10.0f * std::sin(jogo.tempoTotal * 0.9f) - camX;
            const float by = static_cast<float>(o.celula.lin * TILE) - 20.0f - 300.0f * ciclo - camY;
            desenharFramePes(canvas, balao, 0, 0, bx, by, false, static_cast<std::uint8_t>(255.0f * std::min(1.0f, (1.0f - ciclo) * 3.0f)));
        }
    }
    desenharAvisos(canvas, r, p, jogo.tempoTotal, camX, camY);
    desenharEfeitosDeAtaque(canvas, r, p, camX, camY);
    desenharTextos(canvas, r, p, camX, camY);
    desenharDicaInteracao(canvas, r, p, camX, camY);
    desenharBalao(canvas, r, p, camX, camY);
    desenharPortasDaTentativa(canvas, r, p, jogo.tempoTotal, camX, camY);

    if (s.tempoNome > 0.0f) {
        const std::wstring nome = paraWide(p.sala.modelo->nome.empty() ? p.sala.modelo->id : p.sala.modelo->nome);
        // Com o piloto, a faixa "COMPUTADOR JOGANDO" e o objetivo ocupam essa altura e cobriam o meio
        // do título (o travessão "sumia"; capturas da 13.1): o título desce para baixo deles.
        desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, jogo.pilotoAtivo ? 82.0f : 40.0f, nome, COR_TEXTO);
    }
    if (p.noClube || p.noPrologo) desenharHudClube(canvas, r, jogo);
    else desenharHud(canvas, r, p, jogo);
    desenharBarraDoChefe(canvas, r, p);
    if (p.festaAtiva) {   // o relógio da festa e os estourados
        wchar_t t[96];
        std::swprintf(t, 96, L"Festa dos Balões · %d s · %d estourados", static_cast<int>(std::ceil(p.tempoFesta)), p.baloesEstourados);
        retanguloAlpha(canvas, LARGURA_LOGICA / 2.0f - 120, 34, 240, 18, rgb(40, 8, 12), 210);
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 37, t, rgb(255, 214, 120));
    }
    desenharNotificacoes(canvas, r, p);
    desenharAvisoPorta(canvas, r, p);
    if (p.pendencia == Pendencia::SubirNivel) desenharTelaNivel(canvas, r, p, jogo.tempoTotal);   // menus repaginados (24/11)
    if (jogo.pilotoAtivo) desenharPiloto(canvas, r, s.piloto, jogo.mostrarMedidor, camX, camY, (chefeNaSala(p) || p.festaAtiva) ? 66.0f : 38.0f);
    if (p.pendencia == Pendencia::Pausa) desenharPausa(canvas, r, p);
    if (s.menuAssistir) desenharMenuAssistir(canvas, r, s.cursorAssistir);
    if (p.pendencia == Pendencia::Inventario) desenharTelaInventario(canvas, r, p, jogo.tempoTotal);
    desenharDialogo(canvas, r, p, jogo.entrada.ultimoUsado == Dispositivo::Gamepad, jogo.tempoTotal);
    desenharEscolhaEco(canvas, r, p, jogo.tempoTotal);
    desenharLoja(canvas, r, p);
    if (p.pendencia == Pendencia::Mural) desenharTelaMural(canvas, r, p, jogo.perfil, jogo.tempoTotal);
    if (p.pendencia == Pendencia::Quadro) desenharQuadro(canvas, r, p, s.recordes);
    if (p.pendencia == Pendencia::Alcapao) desenharAlcapao(canvas, r, p, jogo.conteudo->dificuldades);
    if (p.terminou) desenharResultado(canvas, r, s, jogo.fluxoClube);
    if (s.telaCiclo > 0.0f) {   // "Bill acorda no Clube" (tela_ciclo): entra e sai em fade
        const float t = s.telaCiclo;
        const float fade = std::min(1.0f, std::min((DURACAO_TELA_CICLO - t) / 0.8f, t / 0.8f));
        retanguloCheio(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0));
        const Sprite tela = spriteDe(r, "tela_ciclo");
        if (spriteValido(tela)) desenharFrame(canvas, tela, 0, 0, 0, 0);
        retanguloAlpha(canvas, 0, ALTURA_LOGICA - 56, LARGURA_LOGICA, 56, rgb(0, 0, 0), 170);
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 48,
                                  L"Bill acorda no Clube, ao entardecer. Os amigos estão todos ali.", COR_TEXTO);
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 24,
                                  L"A mesma tarde. De novo.", rgb(255, 200, 190));
        retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), static_cast<std::uint8_t>(255.0f * (1.0f - fade)));
    }
    if (s.cartaoBioma > 0.0f) desenharCartaoBioma(canvas, r, p.plano.bioma, s.cartaoBioma);
    if (jogo.pausadoPorFoco) {
        retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 150);
        desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 150, L"Pausado", COR_TEXTO);
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, 190, L"Clique na janela para continuar",
                                  rgb(200, 192, 176));
    }
}
