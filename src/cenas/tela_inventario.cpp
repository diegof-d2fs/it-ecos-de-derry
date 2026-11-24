#include "cenas/tela_inventario.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "cenas/ui.h"
#include "cenas/ui_menus.h"
#include "jogo/companheiro.h"
#include "jogo/ecos.h"
#include "render/sprite.h"
#include "render/texto.h"

namespace {

constexpr std::uint32_t COR_VALOR = rgb(200, 225, 255);
constexpr std::uint32_t COR_DESCRICAO = rgb(206, 198, 182);
constexpr std::uint32_t COR_FRACA = rgb(128, 122, 112);

// Áreas fixas da tela do inventário (lógicas, 640 × 360).
constexpr float ABA_X = 40, ABA_Y = 8, ABA_W = 112, ABA_H = 30;
constexpr float ESQ_X = 14, ESQ_Y = 46, ESQ_W = 172, ESQ_H = 284;
constexpr float MEIO_X = 194, MEIO_W = 214;
constexpr float DIR_X = 416, DIR_W = 210;
constexpr float CORPO_Y = 46, CORPO_H = 284;

std::uint32_t corDaRaridadeEco(Raridade r) {
    switch (r) {
        case Raridade::Raro: return rgb(120, 175, 255);
        case Raridade::Epico: return rgb(205, 130, 255);
        case Raridade::Lendario: return rgb(255, 205, 90);
        default: return rgb(215, 210, 200);
    }
}

void iconeDoAtlas(Canvas& canvas, const Recursos& r, const char* nome, float x, float y, float lado, std::uint8_t op = 255) {
    desenharFrameEm(canvas, spriteDe(r, nome), 0, 0, x, y, lado, lado, op);
}

// Parágrafo quebrado na largura; devolve o y depois da última linha.
float paragrafo(Canvas& canvas, const Fonte& f, float x, float y, float largura, const std::wstring& t, std::uint32_t cor,
                float entrelinha = 14.0f, int maxLinhas = 8) {
    int n = 0;
    for (const std::wstring& linha : quebrarLinhas(canvas, f, t, largura)) {
        if (n++ >= maxLinhas) break;
        desenharTexto(canvas, f, x, y, linha, cor);
        y += entrelinha;
    }
    return y;
}

void textoADireita(Canvas& canvas, const Fonte& f, float xDireita, float y, const std::wstring& t, std::uint32_t cor) {
    desenharTexto(canvas, f, xDireita - larguraTexto(canvas, f, t), y, t, cor);
}

// ---------- coluna da esquerda: o Bill (Hades) ----------
void painelDoBill(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    const Jogador& b = p.bill;
    painelOrnado(canvas, r, ESQ_X, ESQ_Y, ESQ_W, ESQ_H);
    const float cx = ESQ_X + ESQ_W / 2.0f;
    brilhoDeSelecao(canvas, cx, ESQ_Y + 120.0f, 58.0f, 78.0f, tempo * 0.5f);
    elipseAlpha(canvas, cx, ESQ_Y + 186.0f, 34.0f, 6.0f, rgb(0, 0, 0), 120);
    const Sprite corpo = spriteDe(r, "retrato_bill_corpo");
    if (spriteValido(corpo)) {
        desenharFrameEm(canvas, corpo, 0, 0, cx - 54.0f, ESQ_Y + 24.0f, 108.0f, 162.0f);
    } else {
        desenharFrameEm(canvas, spriteDe(r, "retrato_bill"), 0, 0, cx - 50.0f, ESQ_Y + 60.0f, 100.0f, 100.0f);
    }
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, ESQ_Y + 192.0f, L"Bill Denbrough", COR_UI_DESTAQUE);
    wchar_t t[96];
    const int nivel = b.progresso.nivel;
    std::swprintf(t, 96, L"Nível %d  ·  %d pacto%ls", nivel, b.pactos, b.pactos == 1 ? L"" : L"s");
    desenharTextoCentralizado(canvas, r.fontePequena, cx, ESQ_Y + 210.0f, t, COR_UI_APAGADO);
    const float bx = ESQ_X + 14.0f, bw = ESQ_W - 28.0f;
    std::swprintf(t, 96, L"Vida  %d / %d", b.hp, b.stats.hpMax);
    desenharTexto(canvas, r.fontePequena, bx, ESQ_Y + 226.0f, t, rgb(255, 196, 188));
    barraOrnada(canvas, bx, ESQ_Y + 240.0f, bw, 5.0f, static_cast<float>(b.hp) / static_cast<float>(std::max(1, b.stats.hpMax)),
                rgb(190, 40, 46));
    const int base = LIMIARES_XP[nivel], prox = nivel < NIVEL_MAXIMO ? LIMIARES_XP[nivel + 1] : base + 1;
    const float fxp = nivel < NIVEL_MAXIMO ? static_cast<float>(b.progresso.xp - base) / static_cast<float>(std::max(1, prox - base)) : 1.0f;
    barraOrnada(canvas, bx, ESQ_Y + 250.0f, bw, 3.0f, fxp, rgb(120, 170, 255));
    for (int i = 0; i < TOTAL_ATRIBUTOS; ++i) {   // os quatro atributos em medalhões
        const float ix = bx + static_cast<float>(i) * (bw / 4.0f);
        iconeAtributo(canvas, r, i, ix, ESQ_Y + 258.0f, 18.0f);
        desenharTexto(canvas, r.fonteMedia, ix + 20.0f, ESQ_Y + 258.0f, std::to_wstring(atributoDoBill(b, i)), COR_UI_TEXTO);
    }
}

// ---------- abas no alto ----------
void abas(Canvas& canvas, const Recursos& r, const EstadoPartida& p) {
    for (int a = 0; a < TOTAL_ABAS; ++a) {
        abaOrnada(canvas, r, ABA_X + ABA_W * static_cast<float>(a), ABA_Y, ABA_W - 4.0f, ABA_H, static_cast<int>(p.aba) == a, a,
                  paraWide(nomeAba(static_cast<AbaInventario>(a))));
    }
}

// Nível da moldura do item (0 comum, 1 raro, 2 épico): os de efeito forte ganham latão melhor.
int nivelDoItem(TipoItem t) {
    switch (t) {
        case TipoItem::FichaCoragem: case TipoItem::Tonico: return 1;
        case TipoItem::Oculos: return 2;
        default: return 0;
    }
}

// ---------- aba Itens (Final Fantasy + Resident Evil) ----------
void abaItens(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    const Jogador& b = p.bill;
    painelOrnado(canvas, r, MEIO_X, CORPO_Y, MEIO_W, CORPO_H);
    tituloDeSecao(canvas, r, MEIO_X + MEIO_W / 2.0f, CORPO_Y + 12.0f, MEIO_W - 60.0f, L"Mochila");
    wchar_t t[64];
    for (int i = 0; i < TOTAL_ITENS_USAVEIS; ++i) {
        const TipoItem it = ITENS_USAVEIS[i];
        const int q = quantidadeDe(b, p.inventario, it);
        const bool sel = p.cursor == i;
        const float ly = CORPO_Y + 44.0f + 38.0f * static_cast<float>(i);
        placaDeLinha(canvas, r, MEIO_X + 10.0f, ly, MEIO_W - 20.0f, 30.0f, sel);
        iconeAmpliado(canvas, r, it, MEIO_X + 34.0f, ly + 5.0f, 20.0f, q > 0 ? 255 : 90);   // depois da seta da placa
        desenharTexto(canvas, r.fontePequena, MEIO_X + 60.0f, ly + 8.0f, paraWide(definicaoDe(it).nome),
                      q == 0 ? COR_FRACA : (sel ? rgb(255, 240, 200) : COR_UI_TEXTO));
        std::swprintf(t, 64, L"×%d", q);
        textoADireita(canvas, r.fontePequena, MEIO_X + MEIO_W - 26.0f, ly + 8.0f, t, q == 0 ? COR_FRACA : COR_UI_DESTAQUE);
    }
    // O escolhido em destaque (Resident Evil): moldura de latão, o ícone grande e a descrição.
    painelOrnado(canvas, r, DIR_X, CORPO_Y, DIR_W, CORPO_H);
    const int i = std::clamp(p.cursor, 0, TOTAL_ITENS_USAVEIS - 1);
    const TipoItem it = ITENS_USAVEIS[i];
    const DefinicaoItem& d = definicaoDe(it);
    const int q = quantidadeDe(b, p.inventario, it);
    const float cx = DIR_X + DIR_W / 2.0f, lado = 88.0f;
    brilhoDeSelecao(canvas, cx, CORPO_Y + 64.0f, 56.0f, 46.0f, tempo);
    molduraDeItem(canvas, r, nivelDoItem(it), cx - lado / 2.0f, CORPO_Y + 20.0f, lado);
    const float flutua = 1.5f * std::sin(tempo * 2.2f);
    iconeAmpliado(canvas, r, it, cx - 26.0f, CORPO_Y + 38.0f + flutua, 52.0f, q > 0 ? 255 : 120);
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, CORPO_Y + 118.0f, paraWide(d.nome), COR_UI_DESTAQUE);
    std::swprintf(t, 64, q > 0 ? L"Na mochila: %d" : L"Nenhum na mochila", q);
    desenharTextoCentralizado(canvas, r.fontePequena, cx, CORPO_Y + 138.0f, t, q > 0 ? COR_VALOR : COR_FRACA);
    paragrafo(canvas, r.fontePequena, DIR_X + 16.0f, CORPO_Y + 162.0f, DIR_W - 32.0f, paraWide(d.descricao), COR_DESCRICAO);
    if (q > 0) desenharTextoCentralizado(canvas, r.fontePequena, cx, CORPO_Y + CORPO_H - 26.0f, ajuda("{CONFIRMAR}: usar agora"),
                                          rgb(160, 235, 170));
}

// ---------- aba Equipamento ----------
void abaEquipamento(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    const Jogador& b = p.bill;
    painelOrnado(canvas, r, MEIO_X, CORPO_Y, MEIO_W, CORPO_H);
    tituloDeSecao(canvas, r, MEIO_X + MEIO_W / 2.0f, CORPO_Y + 12.0f, MEIO_W - 60.0f, L"Equipado");
    struct Linha { const char* icone; std::wstring nome, detalhe; };
    wchar_t a[96], e[96], l[96], m[96];
    std::swprintf(a, 96, L"dano %d · melhorias %d/3", b.stats.danoBase, b.equip.melhorias);
    std::swprintf(e, 96, b.equip.armaduraEscudo > 0 ? L"armadura +%d" : L"sem escudo", b.equip.armaduraEscudo);
    std::swprintf(l, 96, L"%d de %d cargas de clarão", b.cargasLanterna, b.cargasMaximo);
    std::swprintf(m, 96, L"%d moedas de prata", p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)]);
    const Linha linhas[] = {
        {"item_taco", L"Taco de beisebol", a},
        {"item_escudo", b.equip.armaduraEscudo > 0 ? L"Tampa de lixeira" : L"Escudo", e},
        {"item_bateria", L"Lanterna", l},
        {"item_moeda", L"Bolso", m},
    };
    for (int i = 0; i < 4; ++i) {
        const float ly = CORPO_Y + 44.0f + 50.0f * static_cast<float>(i);
        placaDeLinha(canvas, r, MEIO_X + 10.0f, ly, MEIO_W - 20.0f, 42.0f, false);
        molduraDeItem(canvas, r, i == 0 && b.equip.melhorias > 0 ? 1 : 0, MEIO_X + 18.0f, ly + 4.0f, 34.0f);
        iconeDoAtlas(canvas, r, linhas[i].icone, MEIO_X + 25.0f, ly + 11.0f, 20.0f);
        desenharTexto(canvas, r.fontePequena, MEIO_X + 60.0f, ly + 7.0f, linhas[i].nome, COR_UI_TEXTO);
        desenharTexto(canvas, r.fontePequena, MEIO_X + 60.0f, ly + 22.0f, linhas[i].detalhe, COR_VALOR);
    }
    // À direita: quem luta ao lado do Bill (arma e poder da lore, 24/11).
    painelOrnado(canvas, r, DIR_X, CORPO_Y, DIR_W, CORPO_H);
    tituloDeSecao(canvas, r, DIR_X + DIR_W / 2.0f, CORPO_Y + 12.0f, DIR_W - 60.0f, L"Ao seu lado");
    const Companheiro& c = p.companheiro;
    if (!c.ativo) {
        paragrafo(canvas, r.fontePequena, DIR_X + 16.0f, CORPO_Y + 50.0f, DIR_W - 32.0f,
                  L"Ninguém acompanha o Bill agora. Resgate um amigo para lutarem juntos.", COR_FRACA);
        return;
    }
    const std::string quem = codigoAmigo(c.quem);
    const float cx = DIR_X + DIR_W / 2.0f;
    brilhoDeSelecao(canvas, cx, CORPO_Y + 72.0f, 40.0f, 34.0f, tempo);
    molduraDeItem(canvas, r, 1, cx - 32.0f, CORPO_Y + 40.0f, 64.0f);
    desenharFrameEm(canvas, spriteDe(r, "retrato_" + quem), 0, 0, cx - 26.0f, CORPO_Y + 46.0f, 52.0f, 52.0f);
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, CORPO_Y + 108.0f, paraWide(nomeAmigo(c.quem)), COR_UI_DESTAQUE);
    const CombateAmigo& cb = combateDe(c.quem);
    float y = CORPO_Y + 132.0f;
    iconeDoAtlas(canvas, r, ("fx_arma_" + quem).c_str(), DIR_X + 14.0f, y - 2.0f, 18.0f);
    desenharTexto(canvas, r.fontePequena, DIR_X + 36.0f, y, paraWide(cb.arma), rgb(255, 214, 150));
    y = paragrafo(canvas, r.fontePequena, DIR_X + 36.0f, y + 14.0f, DIR_W - 50.0f, paraWide(cb.descricaoArma), COR_DESCRICAO, 13.0f, 2);
    y += 4.0f;
    iconeDoAtlas(canvas, r, ("fx_poder_" + quem).c_str(), DIR_X + 14.0f, y - 2.0f, 18.0f);
    desenharTexto(canvas, r.fontePequena, DIR_X + 36.0f, y, paraWide(cb.poder), rgb(255, 214, 150));
    y = paragrafo(canvas, r.fontePequena, DIR_X + 36.0f, y + 14.0f, DIR_W - 50.0f, paraWide(cb.descricaoPoder), COR_DESCRICAO, 13.0f, 3);
    paragrafo(canvas, r.fontePequena, DIR_X + 14.0f, CORPO_Y + CORPO_H - 40.0f, DIR_W - 28.0f,
              L"Ajuda com 2 ou mais inimigos, um ataque a cada 3 s.", COR_FRACA, 13.0f, 2);
}

// ---------- aba Missão ----------
void abaMissao(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    painelOrnado(canvas, r, MEIO_X, CORPO_Y, MEIO_W, CORPO_H);
    tituloDeSecao(canvas, r, MEIO_X + MEIO_W / 2.0f, CORPO_Y + 12.0f, MEIO_W - 60.0f, L"Objetivo");
    float y = paragrafo(canvas, r.fonteMedia, MEIO_X + 18.0f, CORPO_Y + 44.0f, MEIO_W - 36.0f, paraWide(objetivoAtual(p)), COR_UI_TEXTO,
                        18.0f, 5);
    y = std::max(y + 10.0f, CORPO_Y + 140.0f);
    wchar_t t[64];
    for (TipoItem it : {TipoItem::ChaveDoMedo, TipoItem::Manivela, TipoItem::ChaveEnferrujada}) {
        const int q = p.inventario.quantidades[static_cast<std::size_t>(it)];
        placaDeLinha(canvas, r, MEIO_X + 10.0f, y, MEIO_W - 20.0f, 30.0f, false);
        iconeAmpliado(canvas, r, it, MEIO_X + 22.0f, y + 5.0f, 20.0f, q > 0 ? 255 : 80);
        desenharTexto(canvas, r.fontePequena, MEIO_X + 50.0f, y + 8.0f, paraWide(definicaoDe(it).nome), q > 0 ? COR_UI_TEXTO : COR_FRACA);
        std::swprintf(t, 64, L"×%d", q);
        textoADireita(canvas, r.fontePequena, MEIO_X + MEIO_W - 26.0f, y + 8.0f, t, q > 0 ? COR_UI_DESTAQUE : COR_FRACA);
        y += 36.0f;
    }
    // À direita: os Perdedores desta noite (resgatados acesos; os outros, ainda perdidos).
    painelOrnado(canvas, r, DIR_X, CORPO_Y, DIR_W, CORPO_H);
    tituloDeSecao(canvas, r, DIR_X + DIR_W / 2.0f, CORPO_Y + 12.0f, DIR_W - 60.0f, L"Os Perdedores");
    for (int a = 0; a < TOTAL_AMIGOS; ++a) {
        const Amigo amigo = static_cast<Amigo>(a);
        const bool salvo = std::find(p.resgatados.begin(), p.resgatados.end(), amigo) != p.resgatados.end() ||
                           (p.companheiro.ativo && p.companheiro.quem == amigo);
        const float x = DIR_X + 16.0f + static_cast<float>(a % 3) * 62.0f, y2 = CORPO_Y + 42.0f + static_cast<float>(a / 3) * 96.0f;
        if (salvo) brilhoDeSelecao(canvas, x + 27.0f, y2 + 27.0f, 30.0f, 30.0f, tempo + static_cast<float>(a));
        molduraDeItem(canvas, r, salvo ? 1 : 0, x, y2, 54.0f);
        desenharFrameEm(canvas, spriteDe(r, std::string("retrato_") + codigoAmigo(amigo)), 0, 0, x + 6.0f, y2 + 6.0f, 42.0f, 42.0f,
                        salvo ? 255 : 70);
        desenharTextoCentralizado(canvas, r.fontePequena, x + 27.0f, y2 + 58.0f, paraWide(nomeAmigo(amigo)), salvo ? COR_UI_TEXTO : COR_FRACA);
        desenharTextoCentralizado(canvas, r.fontePequena, x + 27.0f, y2 + 72.0f, salvo ? L"a salvo" : L"perdido",
                                  salvo ? rgb(160, 235, 170) : rgb(200, 120, 110));
    }
}

// ---------- aba Ecos: cartas em duas colunas ----------
void abaEcos(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    const float x = MEIO_X, w = DIR_X + DIR_W - MEIO_X;
    painelOrnado(canvas, r, x, CORPO_Y, w, CORPO_H);
    tituloDeSecao(canvas, r, x + w / 2.0f, CORPO_Y + 12.0f, w - 140.0f, L"Ecos dos amigos");
    if (p.ecos.empty()) {
        paragrafo(canvas, r.fontePequena, x + 30.0f, CORPO_Y + 60.0f, w - 60.0f,
                  L"Nenhum Eco ainda. Altares de Eco e portas com o ícone do altar oferecem 3 Ecos; você escolhe 1.", COR_FRACA);
        return;
    }
    const float cw = (w - 30.0f) / 2.0f, ch = 54.0f;
    int n = 0;
    for (const EcoAtivo& eco : p.ecos) {
        if (n >= 8) break;
        const DefinicaoEco& d = definicaoEco(eco.definicao);
        const float cx = x + 10.0f + static_cast<float>(n % 2) * (cw + 10.0f), cy = CORPO_Y + 40.0f + static_cast<float>(n / 2) * (ch + 6.0f);
        placaDeLinha(canvas, r, cx, cy, cw, ch, eco.raridade >= Raridade::Epico);
        const int nivel = eco.raridade == Raridade::Comum ? 0 : eco.raridade == Raridade::Raro ? 1 : 2;
        molduraDeItem(canvas, r, nivel, cx + 6.0f, cy + 6.0f, 42.0f);
        desenharFrameEm(canvas, spriteDe(r, std::string("retrato_") + codigoAmigo(d.amigo)), 0, 0, cx + 11.0f, cy + 11.0f, 32.0f, 32.0f);
        desenharTexto(canvas, r.fontePequena, cx + 54.0f, cy + 6.0f, paraWide(d.nome), corDaRaridadeEco(eco.raridade));
        desenharTexto(canvas, r.fontePequena, cx + cw - 70.0f, cy + 6.0f, paraWide(nomeRaridade(eco.raridade)), corDaRaridadeEco(eco.raridade));
        paragrafo(canvas, r.fontePequena, cx + 54.0f, cy + 21.0f, cw - 80.0f, paraWide(descricaoEco(eco.definicao, eco.raridade)),
                  COR_DESCRICAO, 13.0f, 2);
        ++n;
    }
    (void)tempo;
    if (static_cast<int>(p.ecos.size()) > n) {
        wchar_t t[48];
        std::swprintf(t, 48, L"+%d", static_cast<int>(p.ecos.size()) - n);
        textoADireita(canvas, r.fontePequena, x + w - 16.0f, CORPO_Y + CORPO_H - 22.0f, t, COR_UI_DESTAQUE);
    }
}

// ---------- aba Ficha: atributos e números derivados (Final Fantasy) ----------
void abaFicha(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float) {
    const Jogador& b = p.bill;
    painelOrnado(canvas, r, MEIO_X, CORPO_Y, MEIO_W, CORPO_H);
    tituloDeSecao(canvas, r, MEIO_X + MEIO_W / 2.0f, CORPO_Y + 12.0f, MEIO_W - 60.0f, L"Atributos");
    for (int i = 0; i < TOTAL_ATRIBUTOS; ++i) {
        const float ly = CORPO_Y + 44.0f + 44.0f * static_cast<float>(i);
        placaDeLinha(canvas, r, MEIO_X + 10.0f, ly, MEIO_W - 20.0f, 36.0f, false);
        iconeAtributo(canvas, r, i, MEIO_X + 18.0f, ly + 4.0f, 28.0f);
        desenharTexto(canvas, r.fonteMedia, MEIO_X + 54.0f, ly + 9.0f, paraWide(nomeAtributo(i)), COR_UI_TEXTO);
        textoADireita(canvas, r.fonteGrande, MEIO_X + MEIO_W - 24.0f, ly + 4.0f, std::to_wstring(atributoDoBill(b, i)), COR_UI_DESTAQUE);
    }
    if (b.progresso.pontosPendentes > 0) {
        wchar_t t[96];
        std::swprintf(t, 96, ajuda("{CONFIRMAR}: distribuir %d ponto(s)").c_str(), b.progresso.pontosPendentes);
        desenharTextoCentralizado(canvas, r.fontePequena, MEIO_X + MEIO_W / 2.0f, CORPO_Y + CORPO_H - 30.0f, t, rgb(255, 214, 90));
    }
    painelOrnado(canvas, r, DIR_X, CORPO_Y, DIR_W, CORPO_H);
    tituloDeSecao(canvas, r, DIR_X + DIR_W / 2.0f, CORPO_Y + 12.0f, DIR_W - 60.0f, L"Em combate");
    const StatsDerivados& d = b.stats;
    wchar_t v[10][32];
    std::swprintf(v[0], 32, L"%d / %d", b.hp, d.hpMax);
    std::swprintf(v[1], 32, L"%d", d.danoBase);
    std::swprintf(v[2], 32, L"%.0f%%", d.critico);
    std::swprintf(v[3], 32, L"%.0f%%", d.acertoBase);
    std::swprintf(v[4], 32, L"%.0f%%", d.esquiva);
    std::swprintf(v[5], 32, L"%d", d.armadura);
    std::swprintf(v[6], 32, L"%.0f", d.velocidade);
    std::swprintf(v[7], 32, L"%d", d.danoClarao);
    std::swprintf(v[8], 32, L"%d", b.progresso.xp);
    const wchar_t* nomes[] = {L"Vida", L"Dano do taco", L"Crítico", L"Acerto", L"Esquiva", L"Armadura", L"Velocidade",
                              L"Dano do clarão", L"Experiência"};
    for (int i = 0; i < 9; ++i) {
        const float ly = CORPO_Y + 42.0f + 24.0f * static_cast<float>(i);
        if (i % 2 == 0) retanguloAlpha(canvas, DIR_X + 12.0f, ly - 3.0f, DIR_W - 24.0f, 22.0f, rgb(255, 230, 180), 10);
        desenharTexto(canvas, r.fontePequena, DIR_X + 20.0f, ly + 2.0f, nomes[i], COR_UI_TEXTO);
        textoADireita(canvas, r.fontePequena, DIR_X + DIR_W - 22.0f, ly + 2.0f, v[i], COR_VALOR);
    }
}

// ---------- Mural: posição de cada polaroide ----------
const wchar_t* const COLUNAS[COLUNAS_MURAL] = {L"Coragem", L"Preparo", L"Os amigos"};
constexpr float MURAL_X0 = 58.0f, MURAL_DX = 134.0f, MURAL_Y0 = 78.0f, MURAL_DY = 88.0f;
constexpr float POLA_W = 56.0f, POLA_H = 63.0f;

Vec2 posicaoNoMural(int i) {
    const int col = i / 3, lin = i % 3;
    // Um leve desalinhamento, como fotos presas à mão (fixo por talento).
    const float torto = static_cast<float>((i * 7) % 5) - 2.0f;
    return {MURAL_X0 + MURAL_DX * static_cast<float>(col) + torto, MURAL_Y0 + MURAL_DY * static_cast<float>(lin) + torto * 0.5f};
}

// Fio vermelho entre dois pontos, com barriga (pontos de lã, sem linha geométrica).
void fioVermelho(Canvas& canvas, Vec2 a, Vec2 b) {
    const float d = comprimento(subtrair(b, a));
    const int n = std::max(4, static_cast<int>(d / 2.0f));
    for (int k = 0; k <= n; ++k) {
        const float t = static_cast<float>(k) / static_cast<float>(n);
        const float barriga = 6.0f * std::sin(t * 3.14159f);
        const float x = a.x + (b.x - a.x) * t + barriga * 0.3f, y = a.y + (b.y - a.y) * t + barriga;
        elipseAlpha(canvas, x, y, 1.1f, 1.1f, rgb(150, 20, 24), 235);
        elipseAlpha(canvas, x - 0.3f, y - 0.4f, 0.5f, 0.5f, rgb(220, 70, 60), 200);
    }
}

void polaroide(Canvas& canvas, const Recursos& r, int i, float x, float y, float escala, int estado, std::uint8_t opIcone) {
    const Sprite pol = spriteDe(r, "ui_polaroid");
    const float w = POLA_W * escala, h = POLA_H * escala;
    elipseAlpha(canvas, x + w / 2.0f + 2.0f, y + h - 2.0f, w * 0.5f, 4.0f * escala, rgb(0, 0, 0), 90);   // sombra na cortiça
    if (spriteValido(pol)) desenharFrameEm(canvas, pol, std::clamp(estado, 0, pol.colunas - 1), 0, x, y, w, h);
    else retanguloAlpha(canvas, x, y, w, h, rgb(236, 228, 210), 255);
    const Sprite ic = spriteDe(r, "ui_talentos");
    // A janela da foto: 48 × 48 de 64 × 72, a 8 da esquerda e 6 do alto.
    if (spriteValido(ic)) {
        desenharFrameEm(canvas, ic, i % ic.colunas, i / ic.colunas, x + w * 8.0f / 64.0f, y + h * 6.0f / 72.0f, w * 48.0f / 64.0f,
                        h * 48.0f / 72.0f, opIcone);
    }
}

}  // namespace

void desenharTelaInventario(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    fundoDeMenu(canvas, r, "tela_fundo_mochila", 70);
    abas(canvas, r, p);
    painelDoBill(canvas, r, p, tempo);
    switch (p.aba) {
        case AbaInventario::Itens: abaItens(canvas, r, p, tempo); break;
        case AbaInventario::Equipamento: abaEquipamento(canvas, r, p, tempo); break;
        case AbaInventario::Missao: abaMissao(canvas, r, p, tempo); break;
        case AbaInventario::Ecos: abaEcos(canvas, r, p, tempo); break;
        case AbaInventario::Ficha: abaFicha(canvas, r, p, tempo); break;
        default: break;
    }
    if (p.tempoAvisoMenu > 0.0f) {
        retanguloAlpha(canvas, MEIO_X + 20.0f, CORPO_Y + CORPO_H - 54.0f, DIR_X + DIR_W - MEIO_X - 40.0f, 20.0f, rgb(10, 30, 16), 220);
        desenharTextoCentralizado(canvas, r.fontePequena, (MEIO_X + DIR_X + DIR_W) / 2.0f, CORPO_Y + CORPO_H - 51.0f,
                                  paraWide(p.avisoMenu), rgb(160, 235, 170));
    }
    retanguloAlpha(canvas, 0, ALTURA_LOGICA - 24.0f, LARGURA_LOGICA, 24.0f, rgb(4, 5, 10), 200);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 19.0f,
                              ajuda("← → abas · ↑↓ escolher · {CONFIRMAR} usar · {INVENTARIO} ou {VOLTAR} fechar"), rgb(190, 184, 170));
}

void desenharTelaNivel(Canvas& canvas, const Recursos& r, const EstadoPartida& p, float tempo) {
    const Jogador& b = p.bill;
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(4, 5, 10), 150);
    const float w = 380.0f, h = 236.0f, x = (LARGURA_LOGICA - w) / 2.0f, y = 62.0f;
    painelOrnado(canvas, r, x, y, w, h);
    wchar_t t[120];
    std::swprintf(t, 120, L"Nível %d  ·  %d ponto%ls para distribuir", b.progresso.nivel, b.progresso.pontosPendentes,
                  b.progresso.pontosPendentes == 1 ? L"" : L"s");
    tituloDeSecao(canvas, r, LARGURA_LOGICA / 2.0f, y + 14.0f, w - 100.0f, t);
    for (int i = 0; i < TOTAL_ATRIBUTOS; ++i) {
        const float ly = y + 46.0f + 42.0f * static_cast<float>(i);
        const bool sel = i == p.cursor;
        if (sel) brilhoDeSelecao(canvas, x + w / 2.0f, ly + 17.0f, w * 0.45f, 22.0f, tempo);
        placaDeLinha(canvas, r, x + 14.0f, ly, w - 28.0f, 34.0f, sel);
        iconeAtributo(canvas, r, i, x + 24.0f, ly + 4.0f, 26.0f);
        std::swprintf(t, 120, L"%ls  %d", paraWide(nomeAtributo(i)).c_str(), atributoDoBill(b, i));
        desenharTexto(canvas, r.fonteMedia, x + 58.0f, ly + 8.0f, t, sel ? rgb(255, 240, 200) : COR_UI_TEXTO);
        // a prévia do que o ponto muda (a mesma conta das regras)
        Atributos a = b.attr;
        int* campos[TOTAL_ATRIBUTOS] = {&a.forca, &a.foco, &a.agilidade, &a.coragem};
        ++*campos[i];
        const StatsDerivados agora = b.stats, depois = calcularStats(a, b.equip);
        switch (i) {
            case 0: std::swprintf(t, 120, L"dano %d → %d", agora.danoBase, depois.danoBase); break;
            case 1: std::swprintf(t, 120, L"acerto %.0f%% → %.0f%% · clarão %d → %d", agora.acertoBase, depois.acertoBase,
                                  agora.danoClarao, depois.danoClarao); break;
            case 2: std::swprintf(t, 120, L"esquiva %.0f%% → %.0f%% · veloc. %.0f → %.0f", agora.esquiva, depois.esquiva,
                                  agora.velocidade, depois.velocidade); break;
            default: std::swprintf(t, 120, L"vida %d → %d · armadura %d → %d", agora.hpMax, depois.hpMax, agora.armadura,
                                   depois.armadura); break;
        }
        textoADireita(canvas, r.fontePequena, x + w - 26.0f, ly + 11.0f, t, sel ? rgb(160, 235, 170) : COR_FRACA);
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, y + h - 22.0f,
                              ajuda("↑↓ escolher · {CONFIRMAR} aplicar · {FICHA} guardar para depois"), rgb(190, 184, 170));
}

void desenharTelaMural(Canvas& canvas, const Recursos& r, const EstadoPartida& p, const Perfil& pf, float tempo) {
    fundoDeMenu(canvas, r, "tela_mural_cortica", 60);
    // Cabeçalho: o título num bilhete e as Lembranças.
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, 34.0f, rgb(6, 6, 10), 170);
    desenharTexto(canvas, r.fonteGrande, 18.0f, 4.0f, L"Mural dos Perdedores", COR_UI_DESTAQUE);
    wchar_t t[120];
    std::swprintf(t, 120, L"%d Lembranças", pf.lembrancas);
    desenharFrameEm(canvas, spriteDe(r, "item_lembranca"), 0, 0, LARGURA_LOGICA - 150.0f, 8.0f, 18.0f, 18.0f);
    desenharTexto(canvas, r.fonteMedia, LARGURA_LOGICA - 128.0f, 9.0f, t, rgb(200, 225, 255));
    // Colunas (ramos da árvore) com os fios vermelhos ligando cada nível ao seguinte.
    for (int col = 0; col < COLUNAS_MURAL; ++col) {
        const float cx = MURAL_X0 + MURAL_DX * static_cast<float>(col) + POLA_W / 2.0f;
        retanguloAlpha(canvas, cx - 46.0f, 42.0f, 92.0f, 18.0f, rgb(236, 226, 200), 235);   // a etiqueta de papel
        desenharTextoCentralizado(canvas, r.fontePequena, cx, 44.0f, COLUNAS[col], rgb(70, 40, 24));
        fioVermelho(canvas, {cx, 60.0f}, {cx, MURAL_Y0 + 4.0f});
        for (int lin = 0; lin + 1 < 3; ++lin) {
            const int i = col * 3 + lin;
            if (i + 1 >= TOTAL_TALENTOS) break;
            const Vec2 a = posicaoNoMural(i), b = posicaoNoMural(i + 1);
            fioVermelho(canvas, {a.x + POLA_W / 2.0f, a.y + POLA_H - 4.0f}, {b.x + POLA_W / 2.0f, b.y + 4.0f});
        }
    }
    for (int i = 0; i < TOTAL_TALENTOS; ++i) {
        const DefinicaoTalento& d = definicaoTalento(static_cast<Talento>(i));
        const int nivel = pf.talentos[static_cast<std::size_t>(i)];
        const bool sel = p.cursor == i;
        const Vec2 pos = posicaoNoMural(i);
        const int estado = !d.disponivel ? 0 : nivel > 0 ? 2 : sel ? 3 : 1;
        const float escala = sel ? 1.12f : 1.0f;
        const float x = pos.x - (escala - 1.0f) * POLA_W / 2.0f, y = pos.y - (escala - 1.0f) * POLA_H / 2.0f;
        if (sel) brilhoDeSelecao(canvas, pos.x + POLA_W / 2.0f, pos.y + POLA_H / 2.0f, 44.0f, 46.0f, tempo);
        polaroide(canvas, r, i, x, y, escala, estado, d.disponivel ? 255 : 110);
        for (int n = 0; n < d.niveis; ++n) {   // níveis: tachinhas de latão acesas = compradas
            const float bx = pos.x + POLA_W / 2.0f + (static_cast<float>(n) - (static_cast<float>(d.niveis) - 1.0f) / 2.0f) * 8.0f;
            elipseAlpha(canvas, bx, pos.y + POLA_H + 19.0f, 2.4f, 2.4f, n < nivel ? rgb(255, 210, 110) : rgb(60, 50, 44), 255);
        }
        desenharTextoCentralizado(canvas, r.fontePequena, pos.x + POLA_W / 2.0f, pos.y + POLA_H + 1.0f, paraWide(d.nome),
                                  d.disponivel ? (sel ? rgb(255, 240, 200) : COR_UI_TEXTO) : COR_FRACA);
    }
    // Detalhe do escolhido (à direita), como a ficha de uma pista no quadro.
    const int i = std::clamp(p.cursor, 0, TOTAL_TALENTOS - 1);
    const DefinicaoTalento& d = definicaoTalento(static_cast<Talento>(i));
    const int nivel = pf.talentos[static_cast<std::size_t>(i)];
    const int custo = custoProximoNivel(pf, static_cast<Talento>(i));
    const float px = 452.0f, py = 44.0f, pw = 176.0f, ph = 286.0f;
    painelOrnado(canvas, r, px, py, pw, ph);
    const float cx = px + pw / 2.0f;
    brilhoDeSelecao(canvas, cx, py + 70.0f, 50.0f, 52.0f, tempo);
    polaroide(canvas, r, i, cx - 42.0f, py + 22.0f, 1.5f, !d.disponivel ? 0 : nivel > 0 ? 2 : 1, d.disponivel ? 255 : 110);
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, py + 124.0f, paraWide(d.nome), COR_UI_DESTAQUE);
    float y = paragrafo(canvas, r.fontePequena, px + 14.0f, py + 146.0f, pw - 28.0f, paraWide(d.efeito), COR_DESCRICAO, 14.0f, 3);
    std::swprintf(t, 120, L"Nível %d de %d", nivel, d.niveis);
    desenharTextoCentralizado(canvas, r.fontePequena, cx, y + 6.0f, t, COR_VALOR);
    std::wstring preco;
    std::uint32_t cor = COR_FRACA;
    if (!d.disponivel) preco = L"Em breve";
    else if (custo < 0) { preco = L"Completo"; cor = rgb(160, 235, 170); }
    else {
        preco = std::to_wstring(custo) + L" Lembranças";
        cor = custo <= pf.lembrancas ? rgb(255, 236, 170) : rgb(255, 160, 150);
    }
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, y + 26.0f, preco, cor);
    if (d.disponivel && custo >= 0) {
        desenharTextoCentralizado(canvas, r.fontePequena, cx, py + ph - 26.0f,
                                  custo <= pf.lembrancas ? ajuda("{CONFIRMAR}: comprar") : std::wstring(L"Faltam Lembranças"),
                                  custo <= pf.lembrancas ? rgb(160, 235, 170) : COR_FRACA);
    }
    if (p.tempoAvisoMenu > 0.0f) {
        retanguloAlpha(canvas, 60.0f, ALTURA_LOGICA - 50.0f, 360.0f, 20.0f, rgb(10, 30, 16), 220);
        desenharTextoCentralizado(canvas, r.fontePequena, 240.0f, ALTURA_LOGICA - 47.0f, paraWide(p.avisoMenu), rgb(160, 235, 170));
    }
    retanguloAlpha(canvas, 0, ALTURA_LOGICA - 24.0f, LARGURA_LOGICA, 24.0f, rgb(4, 5, 10), 200);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 19.0f,
                              ajuda("↑↓ ← → escolher · {CONFIRMAR} comprar o próximo nível · {VOLTAR} fechar"), rgb(190, 184, 170));
}
