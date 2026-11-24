// Menu principal (PRD §15.1; critério F04): Novo jogo, Carregar jogo, Assistir o computador jogar,
// Como jogar, Itens e efeitos, Pontuação, Controles, Opções, Recordes, Créditos, Sair.
// Repaginado em 24/11 (Diego): menu inicial, dificuldade sem explicações, Como jogar ilustrado, Pontuação e
// Créditos refeitos; "Continuar" saiu (Novo jogo mantém o progresso do perfil; Carregar usa o cartão de memória).
// Navegável por teclado e joystick (↑↓ escolhe, ← → muda valor/página, Enter/J/A confirma,
// Esc/Backspace/B volta). Cada tela é uma função de desenhar; a navegação é uma máquina de
// estados pequena (TelaMenu) guardada em Jogo::menu.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "cenas/cena.h"
#include "cenas/ui.h"
#include "cenas/ui_menus.h"
#include "core/pixels.h"
#include "plataforma/audio.h"
#include "render/sprite.h"
#include "render/texto.h"

namespace {

enum class AcaoMenu : std::uint8_t {
    Continuar, NovoJogo, Carregar, Assistir, ComoJogar, Itens, Pontuacao, Controles, Opcoes, Recordes, Creditos, Sair
#ifdef __EMSCRIPTEN__
    , LerLore, Feedback
#endif
};

struct ItemPrincipal {
    const wchar_t* texto;
    AcaoMenu acao;
};

std::vector<ItemPrincipal> itensPrincipais(const Jogo& jogo) {
    (void)jogo;
    std::vector<ItemPrincipal> v;
    v.push_back({L"Novo jogo", AcaoMenu::NovoJogo});
    v.push_back({L"Carregar jogo", AcaoMenu::Carregar});   // o cartão de memória (15 blocos, 23/11)
    v.push_back({L"Assistir o computador jogar", AcaoMenu::Assistir});
    v.push_back({L"Como jogar", AcaoMenu::ComoJogar});
    v.push_back({L"Itens e efeitos", AcaoMenu::Itens});
    v.push_back({L"Pontuação", AcaoMenu::Pontuacao});
    v.push_back({L"Controles", AcaoMenu::Controles});
    v.push_back({L"Opções", AcaoMenu::Opcoes});
    v.push_back({L"Recordes", AcaoMenu::Recordes});
#ifdef __EMSCRIPTEN__
    v.push_back({L"Ler lore", AcaoMenu::LerLore});
    v.push_back({L"Feedback", AcaoMenu::Feedback});
#endif
    v.push_back({L"Créditos", AcaoMenu::Creditos});
#ifndef __EMSCRIPTEN__   // no navegador não se fecha a aba: sair é voltar a este menu (Diego, 24/11)
    v.push_back({L"Sair", AcaoMenu::Sair});
#endif
    return v;
}

// ---------- Como jogar: páginas ilustradas ----------
// Cada página: uma ilustração animada feita com os sprites do próprio jogo (à esquerda) e tópicos curtos (à direita).
// Os botões vão como marcadores ({ATACAR}…) e passam por ajuda(): teclado ou controle, conforme o que se usa.
using Ilustracao = void (*)(Canvas&, const Recursos&, float x, float y, float w, float h, float t);

void chao(Canvas& canvas, float x, float y, float w, float h) {
    retanguloAlpha(canvas, x, y, w, h, rgb(10, 12, 18), 235);
    for (int k = 1; k < 6; ++k)   // ladrilhos apagados
        retanguloAlpha(canvas, x, y + h * static_cast<float>(k) / 6.0f, w, 1, rgb(40, 46, 58), 120);
}

void ilustrarObjetivo(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const char* amigos[] = {"retrato_ben", "retrato_stan", "retrato_eddie", "retrato_beverly", "retrato_mike", "retrato_richie"};
    for (int k = 0; k < 6; ++k) {
        const float cx = x + w * (0.2f + 0.3f * static_cast<float>(k % 3)), cy = y + h * (k < 3 ? 0.3f : 0.68f);
        const float bob = 2.0f * std::sin(t * 1.6f + static_cast<float>(k));
        const Sprite s = spriteDe(r, amigos[k]);
        elipseAlpha(canvas, cx, cy + 4, 30, 30, rgb(120, 20, 30), 70);
        if (spriteValido(s)) desenharFrameEm(canvas, s, 0, 0, cx - 26, cy - 26 + bob, 52, 52, 225);
    }
}

void ilustrarMover(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const Sprite bill = spriteDe(r, "bill_walk");
    if (!spriteValido(bill)) return;
    const float ciclo = std::fmod(t, 3.0f) / 3.0f;   // atravessa o quadro a cada 3 s; o dash no meio
    const bool dash = ciclo > 0.45f && ciclo < 0.6f;
    const float px = x + 30 + (w - 60) * ciclo, py = y + h * 0.62f;
    if (dash)
        for (int k = 1; k <= 3; ++k)   // rastro do dash
            desenharFramePesEstica(canvas, bill, 1, 2, px - 14.0f * static_cast<float>(k), py, 1.6f, 1.6f, false,
                                   static_cast<std::uint8_t>(120 - 30 * k), 0xB4D2FF, 200);
    elipseAlpha(canvas, px, py - 1, 12, 4, rgb(0, 0, 0), 120);
    desenharFramePesEstica(canvas, bill, static_cast<int>(t * 8.0f) % 4, 2, px, py, 1.6f, 1.6f, false, 255, 0, 0);
}

void ilustrarCombate(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const Sprite bill = spriteDe(r, "bill_attack"), rato = spriteDe(r, "rato"), corte = spriteDe(r, "fx_corte_taco");
    const float bx = x + w * 0.38f, by = y + h * 0.64f;
    const float fase = std::fmod(t, 1.2f) / 1.2f;
    elipseAlpha(canvas, bx, by - 1, 12, 4, rgb(0, 0, 0), 120);
    if (spriteValido(bill)) desenharFramePesEstica(canvas, bill, fase < 0.5f ? 0 : 1, 2, bx, by, 1.6f, 1.6f, false, 255, 0, 0);
    if (spriteValido(corte) && fase > 0.3f && fase < 0.75f)
        desenharFrameEm(canvas, corte, std::min(3, static_cast<int>((fase - 0.3f) / 0.45f * 4.0f)), 0, bx + 4, by - 52, 56, 56);
    if (spriteValido(rato)) {
        const bool apanha = fase > 0.4f && fase < 0.6f;
        desenharFramePesEstica(canvas, rato, static_cast<int>(t * 6.0f) % 4, 1, x + w * 0.74f + (apanha ? 4.0f : 0.0f), by, 1.6f,
                               1.6f, false, 255, 0xFFFFFF, apanha ? 200u : 0u);
    }
}

void ilustrarLanterna(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    retanguloAlpha(canvas, x, y, w, h, rgb(2, 3, 6), 250);
    const float cx = x + w * 0.5f, cy = y + h * 0.58f, pulso = 1.0f + 0.04f * std::sin(t * 3.0f);
    for (int k = 6; k >= 1; --k) {   // a luz da lanterna, mais forte no centro
        const float rr = 14.0f * static_cast<float>(k) * pulso;
        elipseAlpha(canvas, cx, cy, rr, rr * 0.7f, rgb(255, 214, 140), static_cast<std::uint8_t>(26));
    }
    const Sprite bill = spriteDe(r, "bill_walk"), sombra = spriteDe(r, "sombra");
    if (spriteValido(sombra))   // um inimigo na borda da luz, quase engolido pelo escuro
        desenharFramePesEstica(canvas, sombra, static_cast<int>(t * 5.0f) % 4, 1, x + w * 0.86f, cy + 6, 1.5f, 1.5f, false, 90, 0, 0);
    if (spriteValido(bill)) desenharFramePesEstica(canvas, bill, 0, 0, cx, cy + 10, 1.6f, 1.6f, false, 255, 0, 0);
}

void ilustrarRecompensas(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const Sprite s = spriteDe(r, "ui_recompensas");
    if (!spriteValido(s)) return;
    for (int k = 0; k < 8; ++k) {
        const float cx = x + w * (0.17f + 0.22f * static_cast<float>(k % 4)), cy = y + h * (k < 4 ? 0.33f : 0.7f);
        const float bob = 2.5f * std::sin(t * 2.0f + static_cast<float>(k) * 0.8f);
        elipseAlpha(canvas, cx, cy + 18, 14, 4, rgb(0, 0, 0), 110);
        desenharFrameEm(canvas, s, k, 0, cx - 18, cy - 18 + bob, 36, 36);
    }
}

void ilustrarCrescer(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const wchar_t* nomes[] = {L"Força", L"Foco", L"Agilidade", L"Coragem"};
    for (int k = 0; k < 4; ++k) {
        const float cx = x + w * (k % 2 == 0 ? 0.3f : 0.7f), cy = y + h * (k < 2 ? 0.3f : 0.68f);
        brilhoDeSelecao(canvas, cx, cy, 26, 22, t + static_cast<float>(k));
        iconeAtributo(canvas, r, k, cx - 22, cy - 26, 44);
        desenharTextoCentralizado(canvas, r.fontePequena, cx, cy + 22, nomes[k], COR_UI_DESTAQUE);
    }
}

void ilustrarAmigos(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const Sprite ret = spriteDe(r, "retrato_mike"), arma = spriteDe(r, "fx_arma_mike"), poder = spriteDe(r, "fx_poder_mike");
    const Sprite rato = spriteDe(r, "rato");
    if (spriteValido(ret)) desenharFrameEm(canvas, ret, 0, 0, x + 14, y + h * 0.5f - 34, 68, 68);
    const float fase = std::fmod(t, 2.0f) / 2.0f;
    const float ax = x + 86 + (w - 150) * std::min(1.0f, fase * 2.0f), ay = y + h * 0.5f - 10 - 18.0f * std::sin(std::min(1.0f, fase * 2.0f) * 3.14159f);
    if (spriteValido(rato)) desenharFramePesEstica(canvas, rato, static_cast<int>(t * 6.0f) % 4, 1, x + w - 40, y + h * 0.5f + 14, 1.5f, 1.5f, false, 255, 0, 0);
    if (fase < 0.5f && spriteValido(arma)) desenharFrameEm(canvas, arma, static_cast<int>(t * 16.0f) % 4, 0, ax - 14, ay - 14, 28, 28);
    if (fase >= 0.5f && spriteValido(poder))
        desenharFrameEm(canvas, poder, std::min(3, static_cast<int>((fase - 0.5f) * 8.0f)), 0, x + w - 72, y + h * 0.5f - 34, 64, 64);
}

void ilustrarPacto(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, float t) {
    chao(canvas, x, y, w, h);
    const Sprite pol = spriteDe(r, "ui_polaroid"), tal = spriteDe(r, "ui_talentos");
    for (int k = 0; k < 3; ++k) {   // polaroides do Mural, levemente em leque
        const float cx = x + w * (0.25f + 0.25f * static_cast<float>(k)), cy = y + h * 0.48f + (k == 1 ? -8.0f : 4.0f);
        if (spriteValido(pol)) desenharFrameEm(canvas, pol, k, 0, cx - 30, cy - 34, 60, 68);
        if (spriteValido(tal)) desenharFrameEm(canvas, tal, k * 3, 0, cx - 18, cy - 24, 36, 36);
    }
    const float pulso = 0.5f + 0.5f * std::sin(t * 2.5f);   // a gota do Pacto de Sangue
    elipseAlpha(canvas, x + w * 0.5f, y + h * 0.86f, 10 + 2 * pulso, 10 + 2 * pulso, rgb(170, 16, 28), static_cast<std::uint8_t>(170 + 60 * pulso));
}

struct PaginaComo {
    const wchar_t* titulo;
    Ilustracao ilustrar;
    const char* topicos[4];   // UTF-8 com marcadores de botão; "" = sem
};

const PaginaComo COMO_JOGAR[] = {
    {L"O objetivo", ilustrarObjetivo,
     {"Seus amigos sumiram nos esgotos de Derry, cada um preso por um medo.",
      "Atravesse os biomas, ache a Chave do Medo e liberte cada amigo.",
      "Cada noite é diferente: as salas e as recompensas mudam a cada tentativa.",
      "Morrer não é o fim: as Lembranças ficam e o Mural do Clube deixa o Bill mais forte."}},
    {L"Mover e desviar", ilustrarMover,
     {"{MOVER}: anda em 8 direções.",
      "{DASH}: avanço rápido e invulnerável. Atravessa golpes e inimigos.",
      "Água rasa e lama deixam o Bill mais lento.", ""}},
    {L"Combate", ilustrarCombate,
     {"{ATACAR}: três golpes seguidos fazem um combo.",
      "{ESPECIAL}: golpe em círculo. {CLARAO}: o clarão da lanterna atordoa.",
      "Todo ataque forte é anunciado: o chão racha antes do golpe. Saia dali ou dê dash.",
      "Derrubar quem está preparando o golpe cancela o ataque."}},
    {L"Lanterna e escuridão", ilustrarLanterna,
     {"As salas são escuras: a lanterna ilumina em volta do Bill.",
      "O que você já viu fica na memória, mais escuro.",
      "O clarão gasta uma carga da lanterna; as baterias recarregam.", ""}},
    {L"Salas e recompensas", ilustrarRecompensas,
     {"Numa sala de combate, as portas fecham até a última onda cair.",
      "Cada porta mostra o que a próxima sala promete. Escolha andando até ela.",
      "Itens no chão são pegos só de passar por cima.",
      "{INTERAGIR}: abrir baús, falar e usar o alçapão."}},
    {L"Ficar mais forte", ilustrarCrescer,
     {"Inimigos e salas dão XP. Cada nível dá 2 pontos de atributo.",
      "{FICHA}: a ficha de atributos. {INVENTARIO}: o inventário e os itens.",
      "{KIT}: usa um kit de cura na hora.", ""}},
    {L"Os amigos", ilustrarAmigos,
     {"Um amigo acompanha o Bill, com uma arma e um poder da sua história.",
      "Ele só ajuda com 2 ou mais inimigos na tela e nunca derruba o último.",
      "Cada um tem também um apoio: o Stan aponta armadilhas, o Ben ergue barreiras.", ""}},
    {L"Pacto e Lembranças", ilustrarPacto,
     {"O Pacto de Sangue é uma vida extra: o Bill levanta com metade da vida.",
      "Sem pacto, a noite termina e a pontuação vai para os recordes.",
      "As Lembranças compram talentos no Mural do Clube, para sempre.", ""}},
};
constexpr int TOTAL_PAGINAS_COMO = static_cast<int>(sizeof COMO_JOGAR / sizeof COMO_JOGAR[0]);
constexpr int ITENS_POR_PAGINA = 7;

// ---------- ações ----------
std::filesystem::path arquivoPerfil(const Jogo& jogo) { return std::filesystem::path(jogo.pastaSaves) / L"perfil.txt"; }

void salvarPerfil(Jogo& jogo) {
    gravarPerfil(arquivoPerfil(jogo), jogo.perfil);
    jogo.menu.temPerfil = true;
}

void aplicarVolumes(const Jogo& jogo) {
    const OpcoesJogo& o = jogo.perfil.opcoes;
    definirVolumes(o.volumeGeral / 100.0f, o.volumeMusica / 100.0f, o.volumeEfeitos / 100.0f);
}

void irPara(Jogo& jogo, TelaMenu tela, int cursor = 0) {
    jogo.menu.tela = tela;
    jogo.menu.cursor = cursor;
    jogo.menu.pagina = 0;
}

// Vai ao Clube dos Perdedores (Etapa 7.2): a dificuldade se escolhe no alçapão de lá.
// Assistindo, o computador anda até o alçapão e escolhe `d` (a dificuldade pedida aqui).
void comecarJogo(Jogo& jogo, Dificuldade d) {
    jogo.dificuldade = d;
    // Escolhida no MENU (Diego, 17/11): o alçapão do Clube já abre com ela marcada.
    if (!jogo.menu.assistir) jogo.perfil.ultimaDificuldade = d;
    jogo.pilotoAtivo = jogo.menu.assistir;
    jogo.velocidade = jogo.menu.assistir ? jogo.menu.velocidade : 1;
    jogo.salaInicial = -1;    // tentativa gerada
    jogo.fluxoClube = true;   // morte/fim voltam ao Clube
    if (!jogo.menu.assistir) salvarPerfil(jogo);
    trocarCenaBase(jogo, IdCena::Sala);
    // Novo jogo (23/11): a abertura da história (~1 min 30 s) passa antes do prólogo, sempre.
    if (jogo.menu.novoJogo && !jogo.menu.assistir) abrirCenaHistoria(jogo, "abertura");
    jogo.menu.novoJogo = false;
}

void executar(Jogo& jogo, AcaoMenu a) {
    EstadoMenu& m = jogo.menu;
    switch (a) {
        // Novo jogo passa pela tela de DIFICULDADE. O progresso do perfil (Lembranças, Mural) fica: "Continuar" saiu do
        // menu (Diego, 24/11: "só Novo jogo ou Carregar um jogo do cartão de memória").
        case AcaoMenu::Continuar:
        case AcaoMenu::NovoJogo:
            m.assistir = false;
            m.novoJogo = true;
            irPara(jogo, TelaMenu::Dificuldade, static_cast<int>(m.temPerfil ? jogo.perfil.ultimaDificuldade : Dificuldade::Normal));
            break;
        case AcaoMenu::Carregar:
            abrirCartao(jogo, false);
            break;
        case AcaoMenu::Assistir:
            m.assistir = true;
            m.novoJogo = false;
            irPara(jogo, TelaMenu::Dificuldade, static_cast<int>(Dificuldade::Normal));
            break;
        case AcaoMenu::ComoJogar: irPara(jogo, TelaMenu::ComoJogar); break;
        case AcaoMenu::Itens: irPara(jogo, TelaMenu::Itens); break;
        case AcaoMenu::Pontuacao: irPara(jogo, TelaMenu::Pontuacao); break;
        case AcaoMenu::Controles: irPara(jogo, TelaMenu::Controles); break;
        case AcaoMenu::Opcoes: irPara(jogo, TelaMenu::Opcoes); break;
        case AcaoMenu::Recordes:
            m.recordes = carregarRecordes(std::filesystem::path(jogo.pastaSaves) / L"recordes.txt");
            irPara(jogo, TelaMenu::Recordes);
#ifdef __EMSCRIPTEN__
            EM_ASM({ if (window.ecosAcesso) window.ecosAcesso.abrirRanking(); });
#endif
            break;
#ifdef __EMSCRIPTEN__
        case AcaoMenu::LerLore:
            EM_ASM({ if (window.ecosAcesso) window.ecosAcesso.abrirLore(); });
            break;
        case AcaoMenu::Feedback:
            EM_ASM({ if (window.ecosAcesso) window.ecosAcesso.abrirFeedback(); });
            break;
#endif
        case AcaoMenu::Creditos: irPara(jogo, TelaMenu::Creditos); break;
        case AcaoMenu::Sair: jogo.rodando = false; break;
    }
}

bool confirmou(const EstadoEntrada& e) { return pressionou(e, Comando::Confirmar) || pressionou(e, Comando::Atacar); }
bool voltou(const EstadoEntrada& e) { return pressionou(e, Comando::Voltar); }
int horizontal(const EstadoEntrada& e) {
    return (pressionou(e, Comando::Direita) || pressionou(e, Comando::AbaDireita) ? 1 : 0) -
           (pressionou(e, Comando::Esquerda) || pressionou(e, Comando::AbaEsquerda) ? 1 : 0);
}
void moverCursor(EstadoMenu& m, const EstadoEntrada& e, int total) {
    if (pressionou(e, Comando::Cima)) m.cursor = (m.cursor + total - 1) % total;
    if (pressionou(e, Comando::Baixo)) m.cursor = (m.cursor + 1) % total;
}

// Opções: 5 linhas; ← → muda o valor.
constexpr int TOTAL_OPCOES_MENU = 6;
void mudarOpcao(Jogo& jogo, int linha, int passo) {
    OpcoesJogo& o = jogo.perfil.opcoes;
    auto volume = [&](int& v) { v = std::max(0, std::min(100, v + 10 * passo)); };
    switch (linha) {
        case 0: o.dicas = !o.dicas; break;
        case 1: volume(o.volumeGeral); break;
        case 2: volume(o.volumeMusica); break;
        case 3: volume(o.volumeEfeitos); break;
        case 4:
            o.telaCheia = !o.telaCheia;
            jogo.pedidoTelaCheia = true;   // a janela é da plataforma: o main alterna
            break;
        default: o.cenasSempre = !o.cenasSempre; break;   // cenas da história: só na 1ª vez ou sempre
    }
    aplicarVolumes(jogo);
}

// ---------- desenho ----------
void fundo(const Jogo& jogo, Canvas& canvas, std::uint8_t escurecer) {
    if (const Imagem* capa = buscarImagem(*jogo.recursos, "capa_menu")) desenharImagem(canvas, *capa, 0, 0);
    else limpar(canvas, rgb(8, 10, 16));
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(4, 6, 12), escurecer);
}

void rodape(Canvas& canvas, const Recursos& r, const std::wstring& texto) {
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 18, texto, rgb(190, 184, 170));
}

void tituloTela(Canvas& canvas, const Recursos& r, const std::wstring& t) {
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 14, t, COR_UI_DESTAQUE);
}

// Fontes próprias do menu (o logo e os títulos), recriadas quando a escala da tela muda.
struct FontesMenu {
    Fonte logo, nome, item, titulo;
    int escala = 0;
};
const FontesMenu& fontesMenu(int escala) {
    static FontesMenu f;
    if (f.escala != escala) {
        for (Fonte* fo : {&f.logo, &f.nome, &f.item, &f.titulo}) liberarFonte(*fo);
        criarFonte(f.logo, L"Georgia", 50, escala, true);
        criarFonte(f.nome, L"Georgia", 19, escala, true);
        criarFonte(f.item, L"Georgia", 15, escala, false);
        criarFonte(f.titulo, L"Georgia", 24, escala, true);
        f.escala = escala;
    }
    return f;
}

const std::uint32_t VERMELHO_IT = rgb(196, 28, 40);

// A tela de título (cena_menu_inicial, 2560 × 1440, pedido do Diego em 24/11: "imagem inédita e em altíssima
// resolução"): lida do disco uma vez e reduzida para o tamanho da TELA pelo pixel mais próximo (é pixel art: a
// média borraria). Desenhada 1:1. Sem ela, a capa antiga.
void fundoDoTitulo(const Jogo& jogo, Canvas& canvas) {
    static Imagem preparada;
    static int largura = -1, altura = -1;
    if (largura != canvas.w || altura != canvas.h) {
        Imagem lida;
        preparada = Imagem{};
        if (carregarSobDemanda(*jogo.recursos, "cena_menu_inicial", lida)) preparada = reduzirVizinho(lida, canvas.w, canvas.h);
        largura = canvas.w;
        altura = canvas.h;
    }
    if (preparada.w > 0) desenharImagemAmostrada(canvas, preparada, 0.0f, 0.0f, 1.0f, 255);
    else if (const Imagem* capa = buscarImagem(*jogo.recursos, "capa_menu")) desenharImagem(canvas, *capa, 0, 0);
    else limpar(canvas, rgb(8, 10, 16));
}

// Véu: a faixa do menu escura por inteiro até `solido`, e um degradê até `ate` (a ilustração livre à direita).
void veuLateral(Canvas& canvas, float solido, float ate, std::uint8_t maximo) {
    retanguloAlpha(canvas, 0, 0, solido, ALTURA_LOGICA, rgb(4, 5, 10), maximo);
    const int passos = 28;
    const float largura = (ate - solido) / static_cast<float>(passos);
    for (int k = 0; k < passos; ++k) {
        const float f = 1.0f - (static_cast<float>(k) + 0.5f) / static_cast<float>(passos);
        retanguloAlpha(canvas, solido + largura * static_cast<float>(k), 0, largura + 0.5f, ALTURA_LOGICA, rgb(4, 5, 10),
                       static_cast<std::uint8_t>(static_cast<float>(maximo) * f * std::sqrt(f) + 0.5f));
    }
}

// Vinheta: topo e base escurecem um pouco (dá profundidade).
void vinheta(Canvas& canvas) {
    for (int k = 0; k < 10; ++k) {
        const std::uint8_t a = static_cast<std::uint8_t>(16 * (10 - k));
        retanguloAlpha(canvas, 0, static_cast<float>(k) * 3.0f, LARGURA_LOGICA, 3, rgb(0, 0, 0), static_cast<std::uint8_t>(a / 2));
        retanguloAlpha(canvas, 0, ALTURA_LOGICA - static_cast<float>(k + 1) * 3.0f, LARGURA_LOGICA, 3, rgb(0, 0, 0), a);
    }
}

// Filete dourado com um losango no meio.
void filete(Canvas& canvas, float x, float y, float w) {
    retanguloAlpha(canvas, x, y, w, 1, rgb(214, 172, 92), 170);
    retanguloAlpha(canvas, x + w / 2 - 2, y - 2, 4, 4, rgb(240, 200, 120), 220);
}

// O balão vermelho (o marcador do item escolhido), flutuando.
void balaoMarcador(Canvas& canvas, const Recursos& r, float x, float y, float tempo, float lado = 14.0f) {
    const Sprite b = spriteDe(r, "balao");
    const float bob = 1.5f * std::sin(tempo * 3.0f);
    if (spriteValido(b)) desenharFrameEm(canvas, b, 0, 0, x, y + bob, lado, lado);
    else elipseAlpha(canvas, x + lado / 2, y + lado / 2 + bob, lado / 3, lado / 2.6f, VERMELHO_IT, 230);
}

// O logo: "IT" grande em vermelho e "ECOS DE DERRY" ao lado.
void logo(Canvas& canvas, const Recursos& r, float x, float y) {
    const FontesMenu& f = fontesMenu(canvas.escala);
    desenharTexto(canvas, f.logo, x, y, L"IT", VERMELHO_IT);
    const float w = larguraTexto(canvas, f.logo, L"IT");
    desenharTexto(canvas, f.nome, x + w + 12, y + 10, L"ECOS DE DERRY", rgb(240, 226, 200));
    desenharTexto(canvas, r.fontePequena, x + w + 13, y + 34, L"um roguelike nos esgotos de Derry", rgb(176, 168, 156));
}

void desenharPrincipal(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const FontesMenu& f = fontesMenu(canvas.escala);
    fundoDoTitulo(jogo, canvas);
    veuLateral(canvas, 250, 430, 225);
    vinheta(canvas);
    logo(canvas, r, 42, 18);
    filete(canvas, 42, 84, 250);
    // Os três primeiros (jogar) maiores; o resto (ver e ajustar) menor; Sair separado no fim.
    const std::vector<ItemPrincipal> itens = itensPrincipais(jogo);
    float y = 98;
    for (int i = 0; i < static_cast<int>(itens.size()); ++i) {
        const bool principal = i < 3, sair = itens[static_cast<std::size_t>(i)].acao == AcaoMenu::Sair;
        if (i == 3) y += 8;
        if (sair) y += 6;
        const float alto = principal ? 22.0f : 17.0f;
        const bool sel = jogo.menu.cursor == i;
        if (sel) {
            brilhoDeSelecao(canvas, 150, y + alto / 2, 130, alto * 0.8f, jogo.tempoTotal);
            placaDeLinha(canvas, r, 34, y - 2, 262, alto + 2, true);
            balaoMarcador(canvas, r, 40, y + (alto - 16) / 2 - 1, jogo.tempoTotal, 15);
        }
        const std::uint32_t cor = sel ? COR_UI_DESTAQUE : (principal ? rgb(238, 230, 214) : rgb(196, 190, 178));
        desenharTexto(canvas, principal ? f.item : r.fonteMedia, sel ? 60.0f : 52.0f, y + (principal ? 2.0f : 1.0f),
                      itens[static_cast<std::size_t>(i)].texto, cor);
        y += alto;
    }
    if (jogo.menu.temPerfil && jogo.perfil.lembrancas + jogo.perfil.melhorPontuacao > 0) {   // canto de baixo, discreto
        wchar_t t[120];
        std::swprintf(t, 120, L"Lembranças %d · melhor pontuação %ls", jogo.perfil.lembrancas, comMilhar(jogo.perfil.melhorPontuacao).c_str());
        const float w = larguraTexto(canvas, r.fontePequena, t);
        desenharTexto(canvas, r.fontePequena, LARGURA_LOGICA - 16 - w, ALTURA_LOGICA - 36, t, rgb(200, 220, 245));
    }
    rodape(canvas, r, ajuda("↑↓ escolher · {CONFIRMAR} confirmar"));
}

// Dificuldade (menu): três cartas lado a lado, sem explicação (Diego, 24/11). Os balões dizem o peso: 1, 2, 3.
void desenharDificuldade(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const FontesMenu& f = fontesMenu(canvas.escala);
    fundoDoTitulo(jogo, canvas);
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(4, 5, 10), 200);
    vinheta(canvas);
    const std::wstring titulo = jogo.menu.assistir ? L"Assistir o computador jogar" : L"Escolha a dificuldade";
    desenharTextoCentralizado(canvas, f.titulo, LARGURA_LOGICA / 2.0f, 34, titulo, rgb(240, 226, 200));
    filete(canvas, LARGURA_LOGICA / 2.0f - 120, 70, 240);
    constexpr float W = 150, H = 168, ESPACO = 22;
    const float x0 = LARGURA_LOGICA / 2.0f - (3 * W + 2 * ESPACO) / 2.0f;
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        const bool sel = jogo.menu.cursor == d;
        const float x = x0 + static_cast<float>(d) * (W + ESPACO), y = sel ? 94.0f : 100.0f;
        if (sel) brilhoDeSelecao(canvas, x + W / 2, y + H / 2, W * 0.7f, H * 0.6f, jogo.tempoTotal);
        painelOrnado(canvas, r, x, y, W, H);
        for (int b = 0; b <= d; ++b) {   // 1, 2 ou 3 balões
            const float bx = x + W / 2 + (static_cast<float>(b) - static_cast<float>(d) / 2.0f) * 30.0f - 15;
            balaoMarcador(canvas, r, bx, y + 34 + 4.0f * std::sin(jogo.tempoTotal * 2.0f + static_cast<float>(b)), jogo.tempoTotal + b, 30);
        }
        placaDeLinha(canvas, r, x + 14, y + H - 56, W - 28, 28, sel);
        desenharTextoCentralizado(canvas, f.item, x + W / 2, y + H - 50, paraWide(nomeDificuldade(static_cast<Dificuldade>(d))),
                                  sel ? COR_UI_DESTAQUE : rgb(214, 206, 192));
        if (!sel) retanguloAlpha(canvas, x, y, W, H, rgb(4, 5, 10), 90);
    }
    if (jogo.menu.assistir) {
        wchar_t t[64];
        std::swprintf(t, 64, L"Velocidade ×%d   (↑ ↓)", jogo.menu.velocidade);
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, 290, t, COR_UI_DESTAQUE);
    }
    rodape(canvas, r, ajuda("← → escolher · {CONFIRMAR} começar · {VOLTAR} voltar"));
}

void desenharConfirmarNovo(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    fundo(jogo, canvas, 170);
    painel(canvas, 150, 110, 340, 120);
    desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, 122, L"Começar um jogo novo?", rgb(255, 200, 180));
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 148,
                              L"O perfil atual (Lembranças, talentos e contadores) será apagado.", COR_UI_TEXTO);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 164, L"Os recordes continuam guardados.", COR_UI_APAGADO);
    linhaMenu(canvas, r, 230, 192, 70, L"Sim", jogo.menu.cursor == 0);
    linhaMenu(canvas, r, 340, 192, 70, L"Não", jogo.menu.cursor == 1);
}

// Fundo das telas de leitura: a capa bem escurecida.
void fundoLeitura(const Jogo& jogo, Canvas& canvas) {
    fundoDoTitulo(jogo, canvas);
    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(4, 5, 10), 215);
    vinheta(canvas);
}

// Pontinhos de página (o atual aceso).
void pontosDePagina(Canvas& canvas, float cx, float y, int total, int atual) {
    for (int k = 0; k < total; ++k) {
        const float x = cx + (static_cast<float>(k) - static_cast<float>(total - 1) / 2.0f) * 12.0f;
        elipseAlpha(canvas, x, y, k == atual ? 3.5f : 2.5f, k == atual ? 3.5f : 2.5f, k == atual ? COR_UI_DESTAQUE : rgb(110, 104, 96), 230);
    }
}

void desenharComoJogar(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const FontesMenu& f = fontesMenu(canvas.escala);
    fundoLeitura(jogo, canvas);
    const int total = TOTAL_PAGINAS_COMO + 1;
    const int pg = jogo.menu.pagina;
    const std::wstring titulo = pg >= TOTAL_PAGINAS_COMO ? L"Dicas do tutorial" : COMO_JOGAR[pg].titulo;
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 10, L"COMO JOGAR", rgb(176, 168, 156));
    desenharTextoCentralizado(canvas, f.titulo, LARGURA_LOGICA / 2.0f, 22, titulo, rgb(240, 226, 200));
    pontosDePagina(canvas, LARGURA_LOGICA / 2.0f, 60, total, pg);
    if (pg >= TOTAL_PAGINAS_COMO) {   // última página: as dicas do tutorial (PRD §15.4)
        painelOrnado(canvas, r, 24, 72, 592, 248);
        wchar_t t[64];
        for (int n = 1; n <= TOTAL_DICAS; ++n) {
            const bool vista = dicaVista(jogo.perfil.dicasVistas, n);
            const float x = n <= 10 ? 48.0f : 334.0f, y = 90 + 21.0f * static_cast<float>((n - 1) % 10);
            std::swprintf(t, 64, L"%ls", vista ? L"✓" : L"·");
            desenharTexto(canvas, r.fonteMedia, x, y, t, vista ? rgb(160, 235, 170) : COR_UI_APAGADO);
            desenharTexto(canvas, r.fonteMedia, x + 18, y, paraWide(tituloDaDica(n)), vista ? COR_UI_TEXTO : COR_UI_APAGADO);
        }
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 302,
                                  jogo.perfil.opcoes.dicas ? L"As dicas aparecem uma vez, no momento certo. Dá para desligar em Opções."
                                                           : L"As dicas estão desligadas (Opções).",
                                  COR_UI_APAGADO);
    } else {
        const PaginaComo& p = COMO_JOGAR[pg];
        // Ilustração à esquerda, numa moldura.
        painelOrnado(canvas, r, 24, 72, 250, 248);
        p.ilustrar(canvas, r, 40, 88, 218, 216, jogo.tempoTotal);
        // Tópicos à direita, cada um com um balão como marcador.
        float y = 90;
        for (const char* topico : p.topicos) {
            if (!topico[0]) continue;
            balaoMarcador(canvas, r, 292, y + 1, jogo.tempoTotal + y, 12);
            for (const std::wstring& linha : quebrarLinhas(canvas, r.fonteMedia, ajuda(topico), 304)) {
                desenharTexto(canvas, r.fonteMedia, 310, y, linha, COR_UI_TEXTO);
                y += 17;
            }
            y += 14;
        }
    }
    wchar_t t[96];
    std::swprintf(t, 96, ajuda("%d de %d · ← → páginas · {VOLTAR} voltar").c_str(), pg + 1, total);
    rodape(canvas, r, t);
}

void desenharItens(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    fundo(jogo, canvas, 190);
    tituloTela(canvas, r, L"Itens e efeitos");
    painel(canvas, 30, 64, 580, 250);
    const int primeiro = jogo.menu.pagina * ITENS_POR_PAGINA;
    wchar_t t[160];
    for (int i = 0; i < ITENS_POR_PAGINA && primeiro + i < TOTAL_ITENS; ++i) {
        const TipoItem it = static_cast<TipoItem>(primeiro + i);
        const DefinicaoItem& d = definicaoDe(it);
        const float y = 76 + 33.0f * static_cast<float>(i);
        desenharIcone(canvas, r, it, 44, y + 2);
        desenharTexto(canvas, r.fonteMedia, 68, y, paraWide(d.nome), COR_UI_DESTAQUE);
        if (d.limite > 0) std::swprintf(t, 160, L"limite %d", d.limite);
        else std::swprintf(t, 160, L"sem limite");
        desenharTexto(canvas, r.fontePequena, 230, y + 3, t, COR_UI_APAGADO);
        desenharTexto(canvas, r.fontePequena, 68, y + 17, paraWide(d.descricao), COR_UI_TEXTO);
    }
    const int paginas = (TOTAL_ITENS + ITENS_POR_PAGINA - 1) / ITENS_POR_PAGINA;
    std::swprintf(t, 160, ajuda("página %d de %d · %d itens · ← → páginas · {VOLTAR} voltar").c_str(), jogo.menu.pagina + 1, paginas, TOTAL_ITENS);
    rodape(canvas, r, t);
}

// Pontuação (refeita em 24/11: "horrível e com muita informação"): o que soma, o que tira e a dificuldade.
// Os valores são os da fórmula de calcularPontuacao (jogo/pontuacao).
void linhaDePonto(Canvas& canvas, const Recursos& r, const Sprite& icone, int col, int lin, float x, float y,
                  const wchar_t* nome, const wchar_t* valor, std::uint32_t corValor) {
    if (spriteValido(icone)) desenharFrameEm(canvas, icone, col, lin, x, y - 3, 20, 20);
    desenharTexto(canvas, r.fonteMedia, x + 28, y, nome, COR_UI_TEXTO);
    const float w = larguraTexto(canvas, r.fonteMedia, valor);
    desenharTexto(canvas, r.fonteMedia, x + 250 - w, y, valor, corValor);
}

void desenharPontuacao(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const FontesMenu& f = fontesMenu(canvas.escala);
    fundoLeitura(jogo, canvas);
    desenharTextoCentralizado(canvas, f.titulo, LARGURA_LOGICA / 2.0f, 16, L"Pontuação", rgb(240, 226, 200));
    filete(canvas, LARGURA_LOGICA / 2.0f - 110, 52, 220);
    const std::uint32_t verde = rgb(150, 230, 160), vermelho = rgb(255, 140, 130);
    const Sprite rec = spriteDe(r, "ui_recompensas"), rato = spriteDe(r, "rato"), sombra = spriteDe(r, "sombra");
    const Sprite mike = spriteDe(r, "retrato_mike"), penny = spriteDe(r, "retrato_pennywise"), balao = spriteDe(r, "balao");
    const Sprite bill = spriteDe(r, "bill_hurt");
    // Soma
    painelOrnado(canvas, r, 24, 66, 292, 232);
    desenharTexto(canvas, f.item, 44, 78, L"Soma pontos", verde);
    float y = 104;
    linhaDePonto(canvas, r, rato, 0, 0, 44, y, L"Inimigo derrotado", L"+100", verde); y += 25;
    linhaDePonto(canvas, r, sombra, 0, 0, 44, y, L"Inimigo assombrado", L"+300", verde); y += 25;
    linhaDePonto(canvas, r, rec, 1, 0, 44, y, L"Item coletado", L"+50", verde); y += 25;
    linhaDePonto(canvas, r, rec, 7, 0, 44, y, L"Sala concluída", L"+250", verde); y += 25;
    linhaDePonto(canvas, r, mike, 0, 0, 44, y, L"Amigo resgatado", L"+500", verde); y += 25;
    linhaDePonto(canvas, r, penny, 0, 0, 44, y, L"Medo dispersado", L"+1.000", verde); y += 25;
    linhaDePonto(canvas, r, balao, 0, 0, 44, y, L"Vencer o Pennywise", L"+5.000", verde);
    desenharTexto(canvas, r.fontePequena, 72, y + 18, L"e até +9.000 se vencer rápido", COR_UI_APAGADO);
    // Tira
    painelOrnado(canvas, r, 324, 66, 292, 150);
    desenharTexto(canvas, f.item, 344, 78, L"Tira pontos", vermelho);
    y = 104;
    linhaDePonto(canvas, r, bill, 0, 0, 344, y, L"Vida usada", L"−1.000", vermelho); y += 25;
    linhaDePonto(canvas, r, spriteDe(r, "bill_walk"), 1, 2, 344, y, L"Andar à toa (2 passos)", L"−1", vermelho); y += 25;
    linhaDePonto(canvas, r, rec, 2, 0, 344, y, L"Tempo (a cada 5 s)", L"−1", vermelho);
    // Dificuldade
    painelOrnado(canvas, r, 324, 222, 292, 76);
    desenharTexto(canvas, f.item, 344, 234, L"A dificuldade multiplica", COR_UI_DESTAQUE);
    const wchar_t* nomes[] = {L"Fácil ×0,75", L"Normal ×1", L"Difícil ×1,5"};
    for (int d = 0; d < 3; ++d) {
        const float cx = 344 + 42 + 86.0f * static_cast<float>(d);
        desenharTextoCentralizado(canvas, r.fonteMedia, cx, 262, nomes[d], d == 2 ? rgb(255, 190, 150) : COR_UI_TEXTO);
    }
    rodape(canvas, r, ajuda("{VOLTAR} voltar"));
}

void desenharControles(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    fundo(jogo, canvas, 190);
    tituloTela(canvas, r, L"Controles");
    painel(canvas, 30, 60, 580, 262);
    const wchar_t* linhas[][3] = {
        {L"Mover", L"WASD ou setas", L"analógico / direcional"},
        {L"Atacar (combo)", L"J", L"X"},
        {L"Especial", L"K", L"Y"},
        {L"Clarão da lanterna", L"L", L"B"},
        {L"Dash (invulnerável)", L"Espaço", L"A"},
        {L"Interagir", L"E", L"RB"},
        {L"Usar kit", L"Q", L"LB"},
        {L"Inventário", L"I ou Tab", L"Back"},
        {L"Ficha de atributos", L"C", L"LT + ↑"},
        {L"Pausa", L"Esc ou P", L"Start"},
        {L"Computador joga (liga/desliga)", L"F10", L"nenhum"},
        {L"Tela cheia · depuração · teste do controle", L"F11 · F1 · F2", L"nenhum"},
    };
    desenharTexto(canvas, r.fontePequena, 44, 68, L"Ação", COR_UI_APAGADO);
    desenharTexto(canvas, r.fontePequena, 330, 68, L"Teclado", COR_UI_APAGADO);
    desenharTexto(canvas, r.fontePequena, 470, 68, L"Controle", COR_UI_APAGADO);
    for (int i = 0; i < 12; ++i) {
        const float y = 86 + 18.0f * static_cast<float>(i);
        desenharTexto(canvas, r.fontePequena, 44, y, linhas[i][0], COR_UI_TEXTO);
        desenharTexto(canvas, r.fontePequena, 330, y, linhas[i][1], COR_UI_DESTAQUE);
        desenharTexto(canvas, r.fontePequena, 470, y, linhas[i][2], rgb(200, 225, 255));
    }
    desenharTexto(canvas, r.fontePequena, 44, 304,
                  jogo.entrada.gamepadConectado ? L"Controle conectado." : L"Nenhum controle conectado (o teclado funciona junto).",
                  COR_UI_APAGADO);
    rodape(canvas, r, ajuda("{VOLTAR} voltar · F2 abre o teste de botões"));
}

void desenharOpcoes(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    fundo(jogo, canvas, 190);
    tituloTela(canvas, r, L"Opções");
    const OpcoesJogo& o = jogo.perfil.opcoes;
    const std::wstring valores[TOTAL_OPCOES_MENU] = {
        o.dicas ? L"ligadas" : L"desligadas", std::to_wstring(o.volumeGeral) + L"%", std::to_wstring(o.volumeMusica) + L"%",
        std::to_wstring(o.volumeEfeitos) + L"%", o.telaCheia ? L"sim" : L"não", o.cenasSempre ? L"sempre" : L"1ª vez"};
    const wchar_t* nomes[TOTAL_OPCOES_MENU] = {L"Dicas do tutorial", L"Volume geral", L"Volume da música", L"Volume dos efeitos",
                                               L"Tela cheia", L"Cenas da história"};
    painel(canvas, 120, 70, 400, 188);
    for (int i = 0; i < TOTAL_OPCOES_MENU; ++i) {
        const float y = 84 + 28.0f * static_cast<float>(i);
        linhaMenu(canvas, r, 126, y, 388, nomes[i], jogo.menu.cursor == i);
        desenharTexto(canvas, r.fonteMedia, 400, y, L"◀ " + valores[i] + L" ▶", COR_UI_DESTAQUE);
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 266,
                              L"O cronômetro fica sempre visível: o tempo faz parte da pontuação.", COR_UI_APAGADO);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 282,
                              L"A abertura da história passa a cada Novo jogo; as outras cenas, conforme a opção.", COR_UI_APAGADO);
    rodape(canvas, r, ajuda("↑↓ escolher · ← → mudar · {VOLTAR} salvar e voltar"));
}

void desenharRecordesMenu(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    fundo(jogo, canvas, 190);
    tituloTela(canvas, r, L"Recordes");
    painel(canvas, 90, 60, 460, 262);
    desenharRecordes(canvas, r, 90, 66, 460, jogo.menu.recordes, jogo.menu.filtro, nullptr);
    rodape(canvas, r, ajuda("← → filtro (todas, jogador, computador) · {VOLTAR} voltar"));
}

// Créditos (refeitos em 24/11): os sete Perdedores ao fundo e o nome de quem fez o jogo em destaque.
void desenharCreditos(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const FontesMenu& f = fontesMenu(canvas.escala);
    fundoDeMenu(canvas, r, "tela_perdedores", 170);
    vinheta(canvas);
    const float cx = LARGURA_LOGICA / 2.0f;
    // O logo centralizado.
    const float wIt = larguraTexto(canvas, f.logo, L"IT"), wNome = larguraTexto(canvas, f.nome, L"ECOS DE DERRY");
    const float x0 = cx - (wIt + 12 + wNome) / 2.0f;
    desenharTexto(canvas, f.logo, x0, 14, L"IT", VERMELHO_IT);
    desenharTexto(canvas, f.nome, x0 + wIt + 12, 32, L"ECOS DE DERRY", rgb(240, 226, 200));
    filete(canvas, cx - 120, 78, 240);
    desenharTextoCentralizado(canvas, r.fontePequena, cx, 92, L"um jogo de", rgb(190, 182, 168));
    desenharTextoCentralizado(canvas, f.titulo, cx, 104, L"Diego Silva", COR_UI_DESTAQUE);
    desenharTextoCentralizado(canvas, r.fonteMedia, cx, 138, L"criação, programação, design e direção", rgb(214, 206, 192));
    // Os detalhes, em blocos curtos.
    struct Bloco { const wchar_t* titulo; const wchar_t* texto; };
    const Bloco blocos[] = {
        {L"O TRABALHO", L"T3 de Algoritmos e Programação II · UNIVALI"},
        {L"O MOTOR", L"C++17 puro, sem engine: render, áudio, IA e piloto automático próprios"},
        {L"ARTE E SOM", L"pixel art gerada e revisada para o projeto · músicas e sons de bancos livres"},
        {L"GERAÇÃO DE IMAGENS", L"Gemini Pro (Google) e Codex (OpenAI)"},
        {L"INSPIRAÇÃO", L"IT, de Stephen King"},
        {L"ASSISTÊNCIA DE PROGRAMAÇÃO", L"Claude (Anthropic) e Codex (OpenAI)"},
    };
    float y = 160;   // 6 blocos de 28: o último termina em ~326, antes do rodapé (342)
    for (const Bloco& b : blocos) {
        desenharTextoCentralizado(canvas, r.fontePequena, cx, y, b.titulo, rgb(214, 172, 92));
        desenharTextoCentralizado(canvas, r.fonteMedia, cx, y + 12, b.texto, COR_UI_TEXTO);
        y += 28;
    }
    rodape(canvas, r, jogo.menu.creditosDaVitoria ? ajuda("{CONFIRMAR} ou {VOLTAR} voltar ao Clube") : ajuda("{VOLTAR} voltar"));
}

}  // namespace

void entrarMenu(Jogo& jogo) {
    jogo.menu.temPerfil = perfilExiste(arquivoPerfil(jogo));
    jogo.menu.tela = TelaMenu::Principal;
    jogo.menu.creditosDaVitoria = false;
    jogo.menu.cursor = 0;
    jogo.pilotoAtivo = false;
    jogo.velocidade = 1;
    aplicarVolumes(jogo);
    definirMusica(IdSom::MusicaMenu);
    ligarAmbiente(IdSom::Total);
    definirTensao(0.0f);
}

void atualizarMenu(Jogo& jogo, const EstadoEntrada& e, float) {
    EstadoMenu& m = jogo.menu;
    switch (m.tela) {
        case TelaMenu::Principal: {
            const std::vector<ItemPrincipal> itens = itensPrincipais(jogo);
            moverCursor(m, e, static_cast<int>(itens.size()));
            if (confirmou(e)) executar(jogo, itens[static_cast<std::size_t>(m.cursor)].acao);
            break;
        }
        case TelaMenu::ConfirmarNovo:
            moverCursor(m, e, 2);
            if (horizontal(e) != 0) m.cursor = 1 - m.cursor;   // Sim e Não ficam lado a lado
            if (voltou(e)) irPara(jogo, TelaMenu::Principal, 1);
            else if (confirmou(e)) {
                if (m.cursor == 0) {
                    const OpcoesJogo opcoes = jogo.perfil.opcoes;   // as opções não são "progresso"
                    jogo.perfil = Perfil{};
                    jogo.perfil.opcoes = opcoes;
                    irPara(jogo, TelaMenu::Dificuldade, static_cast<int>(Dificuldade::Normal));
                } else {
                    irPara(jogo, TelaMenu::Principal, 1);
                }
            }
            break;
        case TelaMenu::Dificuldade: {
            // As cartas ficam lado a lado (24/11): ← → escolhem. Assistindo, ↑ ↓ mudam a velocidade; jogando, também
            // escolhem a carta (quem está acostumado com a lista).
            m.cursor = (m.cursor + TOTAL_DIFICULDADES + horizontal(e)) % TOTAL_DIFICULDADES;
            const int vertical = (pressionou(e, Comando::Baixo) ? 1 : 0) - (pressionou(e, Comando::Cima) ? 1 : 0);
            if (m.assistir && vertical != 0) {   // ×1 → ×2 → ×4
                const int opcoes[] = {1, 2, 4};
                int i = m.velocidade == 4 ? 2 : (m.velocidade == 2 ? 1 : 0);
                i = std::max(0, std::min(2, i - vertical));
                m.velocidade = opcoes[i];
            } else if (vertical != 0) {
                m.cursor = (m.cursor + TOTAL_DIFICULDADES + vertical) % TOTAL_DIFICULDADES;
            }
            if (voltou(e)) irPara(jogo, TelaMenu::Principal);
            else if (confirmou(e)) comecarJogo(jogo, static_cast<Dificuldade>(m.cursor));
            break;
        }
        case TelaMenu::ComoJogar:
        case TelaMenu::Itens: {
            const int paginas = m.tela == TelaMenu::ComoJogar ? TOTAL_PAGINAS_COMO + 1 : (TOTAL_ITENS + ITENS_POR_PAGINA - 1) / ITENS_POR_PAGINA;
            m.pagina = (m.pagina + paginas + horizontal(e)) % paginas;
            if (voltou(e) || confirmou(e)) irPara(jogo, TelaMenu::Principal);
            break;
        }
        case TelaMenu::Opcoes:
            moverCursor(m, e, TOTAL_OPCOES_MENU);
            if (horizontal(e) != 0) mudarOpcao(jogo, m.cursor, horizontal(e));
            else if (confirmou(e)) mudarOpcao(jogo, m.cursor, 1);
            if (voltou(e)) {
                salvarPerfil(jogo);
                irPara(jogo, TelaMenu::Principal);
            }
            break;
        case TelaMenu::Recordes:
            if (horizontal(e) != 0) m.filtro = static_cast<FiltroRecordes>((static_cast<int>(m.filtro) + 3 + horizontal(e)) % 3);
            if (voltou(e) || confirmou(e)) irPara(jogo, TelaMenu::Principal);
            break;
        default:   // Pontuação, Controles, Créditos: só leitura
            if (voltou(e) || confirmou(e)) {
                if (m.tela == TelaMenu::Creditos && m.creditosDaVitoria) {
                    m.creditosDaVitoria = false;
                    trocarCenaBase(jogo, IdCena::Sala);
                } else irPara(jogo, TelaMenu::Principal);
            }
            break;
    }
}

void desenharMenu(const Jogo& jogo, Canvas& canvas) {
    definirControleNaAjuda(jogo.entrada.ultimoUsado == Dispositivo::Gamepad);   // rodapés pelo dispositivo em uso
    // T03: a tela atual escolhe a função de desenho por uma tabela de ponteiros para função.
    using Desenho = void (*)(const Jogo&, Canvas&);
    static const Desenho TELAS[] = {desenharPrincipal, desenharConfirmarNovo, desenharDificuldade, desenharComoJogar,
                                    desenharItens,     desenharPontuacao,     desenharControles,   desenharOpcoes,
                                    desenharRecordesMenu, desenharCreditos};
    TELAS[static_cast<int>(jogo.menu.tela)](jogo, canvas);
}
