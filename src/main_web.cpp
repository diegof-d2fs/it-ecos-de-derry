#ifdef __EMSCRIPTEN__
// IT: Ecos de Derry no NAVEGADOR (Etapa 18). Mesmo jogo, mesmas regras e cenas: só a plataforma muda.
//   * laço: emscripten_set_main_loop (ritmo do requestAnimationFrame) com o mesmo passo fixo de 1/60 s;
//   * vídeo: o canvas de software é copiado para o <canvas id="tela"> do HTML a cada quadro;
//   * teclado: KeyboardEvent.code → Comando (a mesma tabela do Windows); controle: Gamepad API ("standard");
//   * áudio: plataforma/audio_web.cpp; texto e PNG: render/texto_web.cpp e render/imagem_web.cpp;
//   * saves: /saves em memória; contas sincronizam com a API da AWS;
//   * opções pela URL, como na linha de comando: ?sala=b7_covil&forte=1&piloto=1&cena=final&bioma=3…
#include <emscripten.h>
#include <emscripten/html5.h>

#include <algorithm>
#include <array>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "cenas/cena.h"
#include "core/arquivo.h"
#include "core/controle.h"
#include "core/escala.h"
#include "core/sons.h"
#include "jogo/dialogo.h"
#include "jogo/dificuldade.h"
#include "jogo/historia.h"
#include "jogo/perfil.h"
#include "mundo/sala.h"
#include "plataforma/audio.h"
#include "render/canvas.h"
#include "render/imagem.h"
#include "render/recursos.h"
#include "render/texto.h"

namespace {

// Escala 3 = 1920×1080, a mesma do .exe numa tela 1080p. A 4 pediria ~600 MB de imagens prontas na memória.
constexpr int ESCALA_MAXIMA_WEB = 3;
// Se o desenho passar disto em média, o computador não acompanha: desce uma escala (só nesta sessão).
constexpr double QUADRO_LENTO_MS = 24.0;

// ---------- ponte com o JavaScript ----------
EM_JS(void, js_apresentar, (const unsigned char* rgba, int w, int h), {
    const c = document.getElementById('tela');
    if (c.width !== w || c.height !== h) { c.width = w; c.height = h; }
    const dpr = window.devicePixelRatio || 1;
    const caixa = c.getBoundingClientRect();   // object-fit: contain → a imagem ocupa o menor dos dois lados
    const razao = Math.min(caixa.width * dpr / w, caixa.height * dpr / h);
    const modo = (razao > 1.02 && Math.abs(razao - Math.round(razao)) < 0.02) ? 'pixelated' : 'auto';
    if (c.style.imageRendering !== modo) c.style.imageRendering = modo;
    const ctx = c.getContext('2d', {alpha : false});
    ctx.putImageData(new ImageData(new Uint8ClampedArray(HEAPU8.buffer, rgba, w * h * 4), w, h), 0, 0);
});
EM_JS(int, js_largura, (), { return Math.floor(window.innerWidth * (window.devicePixelRatio || 1)); });
EM_JS(int, js_altura, (), { return Math.floor(window.innerHeight * (window.devicePixelRatio || 1)); });
EM_JS(int, js_focada, (), { return (document.hasFocus() && document.visibilityState === 'visible') ? 1 : 0; });
EM_JS(void, js_tela_cheia, (), {
    if (document.fullscreenElement) document.exitFullscreen();
    else document.documentElement.requestFullscreen().catch(() => {});
});
EM_JS(char*, js_parametros, (), {
    const s = window.ecosAcesso ? window.ecosAcesso.parametrosDemo() : '';
    const n = lengthBytesUTF8(s) + 1;
    const p = _malloc(n);
    stringToUTF8(s, p, n);
    return p;
});
EM_JS(void, js_sincronizar_saves, (), {
    if (window.ecosAcesso) window.ecosAcesso.sincronizar();
});
EM_JS(void, js_carregou, (), { if (window.ecosCarregou) window.ecosCarregou(); });
// A capa ("clique para jogar") está fechada? Enquanto ela aparece, o jogo não ouve teclado nem controle.
EM_JS(int, js_liberado, (), { return window.ecosJogando ? 1 : 0; });


// ---------- teclado ----------
std::array<bool, TOTAL_COMANDOS> teclas{};
std::map<std::string, std::vector<Comando>> tabelaTeclas() {
    return {
        {"KeyW", {Comando::Cima}},          {"ArrowUp", {Comando::Cima}},
        {"KeyS", {Comando::Baixo}},         {"ArrowDown", {Comando::Baixo}},
        {"KeyA", {Comando::Esquerda}},      {"ArrowLeft", {Comando::Esquerda}},
        {"KeyD", {Comando::Direita}},       {"ArrowRight", {Comando::Direita}},
        {"KeyJ", {Comando::Atacar}},        {"KeyK", {Comando::Especial}},
        {"KeyL", {Comando::Clarao}},        {"Space", {Comando::Dash}},
        {"KeyE", {Comando::Interagir}},     {"KeyQ", {Comando::UsarKit}},
        {"KeyI", {Comando::Inventario}},    {"Tab", {Comando::Inventario}},
        {"KeyC", {Comando::Ficha}},         {"KeyM", {Comando::Mapa}},
        {"Escape", {Comando::Pausa, Comando::Voltar}}, {"KeyP", {Comando::Pausa}},
        {"Enter", {Comando::Confirmar}},    {"NumpadEnter", {Comando::Confirmar}},
        {"Backspace", {Comando::Voltar}},
        {"PageUp", {Comando::AbaEsquerda}}, {"Comma", {Comando::AbaEsquerda}},
        {"PageDown", {Comando::AbaDireita}}, {"Period", {Comando::AbaDireita}},
        {"F11", {Comando::TelaCheia}},      {"F1", {Comando::Debug}},
        {"F10", {Comando::Piloto}},         {"KeyO", {Comando::Piloto}},   // F10 pode ser do navegador: O também
    };
}

EM_BOOL tecla(int tipo, const EmscriptenKeyboardEvent* e, void*) {
    static const auto TABELA = tabelaTeclas();
    const auto it = TABELA.find(e->code);
    if (it == TABELA.end()) return EM_FALSE;
    for (Comando c : it->second) teclas[static_cast<int>(c)] = tipo == EMSCRIPTEN_EVENT_KEYDOWN;
    return EM_TRUE;   // o jogo usa a tecla: o navegador não rola a página nem muda de campo
}

// ---------- controle (Gamepad API, mapeamento "standard": por POSIÇÃO, como a SDL) ----------
void lerControle(InfoControle& info) {
    info.origem = OrigemControle::Nenhuma;
    info.leitura = LeituraControle{};
    if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS) return;
    for (int i = 0; i < emscripten_get_num_gamepads(); ++i) {
        EmscriptenGamepadEvent g;
        if (emscripten_get_gamepad_status(i, &g) != EMSCRIPTEN_RESULT_SUCCESS || !g.connected) continue;
        info.origem = OrigemControle::Sdl;
        std::snprintf(info.nome, sizeof info.nome, "%s", g.id);
        LeituraControle& l = info.leitura;
        static const int BOTOES[][2] = {   // BotaoControle ← índice "standard"
            {static_cast<int>(BotaoControle::Sul), 0},        {static_cast<int>(BotaoControle::Leste), 1},
            {static_cast<int>(BotaoControle::Oeste), 2},      {static_cast<int>(BotaoControle::Norte), 3},
            {static_cast<int>(BotaoControle::OmbroEsq), 4},   {static_cast<int>(BotaoControle::OmbroDir), 5},
            {static_cast<int>(BotaoControle::Selecionar), 8}, {static_cast<int>(BotaoControle::Iniciar), 9},
            {static_cast<int>(BotaoControle::AnalogicoEsq), 10}, {static_cast<int>(BotaoControle::AnalogicoDir), 11},
            {static_cast<int>(BotaoControle::Cima), 12},      {static_cast<int>(BotaoControle::Baixo), 13},
            {static_cast<int>(BotaoControle::Esquerda), 14},  {static_cast<int>(BotaoControle::Direita), 15},
            {static_cast<int>(BotaoControle::Guia), 16},
        };
        for (const auto& b : BOTOES)
            if (b[1] < g.numButtons) l.botoes[static_cast<std::size_t>(b[0])] = g.digitalButton[b[1]];
        if (g.numAxes >= 2) l.analogicoEsq = {static_cast<float>(g.axis[0]), static_cast<float>(g.axis[1])};
        if (g.numAxes >= 4) l.analogicoDir = {static_cast<float>(g.axis[2]), static_cast<float>(g.axis[3])};
        if (g.numButtons > 7) {
            l.gatilhoEsq = static_cast<float>(g.analogButton[6]);
            l.gatilhoDir = static_cast<float>(g.analogButton[7]);
        }
        return;   // o primeiro controle conectado
    }
}

// ---------- conteúdo ----------
bool carregarConteudo(Conteudo& conteudo, std::string& erro) {
    if (!carregarSalas(std::filesystem::path("/data/salas"), conteudo.salas, erro)) { erro = "salas: " + erro; return false; }
    if (!carregarDialogos(std::filesystem::path("/data/dialogos.txt"), conteudo.dialogos, erro)) { erro = "diálogos: " + erro; return false; }
    if (!carregarCenasHistoria(std::filesystem::path("/data/cenas.txt"), conteudo.cenas, erro)) return false;
    std::string texto;
    if (lerArquivoTexto(std::filesystem::path("/data/balanceamento.txt"), texto) && !lerBalanceamento(texto, conteudo.dificuldades, erro)) return false;
    return true;
}

std::map<std::string, std::string> lerParametros() {
    char* p = js_parametros();
    std::string s = p;
    std::free(p);
    std::map<std::string, std::string> r;
    if (!s.empty() && s[0] == '?') s.erase(0, 1);
    std::size_t i = 0;
    while (i < s.size()) {
        std::size_t fim = s.find('&', i);
        if (fim == std::string::npos) fim = s.size();
        const std::string par = s.substr(i, fim - i);
        const std::size_t igual = par.find('=');
        r[par.substr(0, igual)] = igual == std::string::npos ? "1" : par.substr(igual + 1);
        i = fim + 1;
    }
    return r;
}

// ---------- estado do laço ----------
struct Web {
    Recursos recursos;
    Conteudo conteudo;
    Jogo jogo;
    Canvas canvas;
    std::vector<unsigned char> rgba;
    double anterior = 0.0, acumulador = 0.0, ultimaSincronia = 0.0;
    int limiteEscala = ESCALA_MAXIMA_WEB;   // baixa se o computador não der conta (ver QUADRO_LENTO_MS)
    double somaQuadrosMs = 0.0;
    int quadrosMedidos = 0;
};
Web* web = nullptr;

// Escala para CIMA (Diego, 24/11: "resolução muito baixa no navegador"). Com o navegador em janela numa tela 1080p
// (~1920×960) a maior escala inteira que CABE é 2, e o navegador esticava 1280×720 até a tela. Agora desenha na
// escala acima (3) e o navegador reduz com suavização. Só arredonda para cima se faltar mais de 25% (evita desenhar
// 1920×1080 para uma janela de 1300×730). Só a web: o .exe continua com calcularApresentacao.
int escalaWeb(int larguraPx, int alturaPx, int limite) {
    const double razao = std::min(larguraPx / static_cast<double>(LARGURA_LOGICA), alturaPx / static_cast<double>(ALTURA_LOGICA));
    const int escala = static_cast<int>(std::ceil(razao - 0.25));
    return std::max(1, std::min(escala, limite));
}

bool ajustarEscala() {
    const int escala = escalaWeb(js_largura(), js_altura(), web->limiteEscala);
    if (escala == web->canvas.escala && web->canvas.px) return true;
    liberarCanvas(web->canvas);
    return criarCanvas(web->canvas, LARGURA_LOGICA, ALTURA_LOGICA, escala) && prepararEscala(web->recursos, escala);
}

// O toque que fecha a capa NÃO pode chegar ao jogo (24/11, Diego: "depois da tela de carregamento não foi para o
// menu"): o Enter/A que fechava a capa já confirmava o "Novo jogo". Depois da capa, o jogo só começa a ouvir quando
// todas as teclas e botões estiverem soltos.
bool travado = true;

bool algoSegurado(const Jogo& jogo) {
    for (bool t : teclas) if (t) return true;
    for (bool b : jogo.controle.leitura.botoes) if (b) return true;
    return false;
}

void lerEntrada(Jogo& jogo, bool focada) {
    const auto anterior = jogo.entrada.segurando;
    jogo.entrada.segurando.fill(false);
    const bool tinhaControle = jogo.controle.origem != OrigemControle::Nenhuma;
    lerControle(jogo.controle);
    const bool temControle = jogo.controle.origem != OrigemControle::Nenhuma;
    jogo.entrada.gamepadConectado = temControle;
    const std::string n = jogo.controle.nome;
    definirRotulosNintendo(temControle && (n.find("Nintendo") != std::string::npos || n.find("Switch") != std::string::npos ||
                                           n.find("Pro Controller") != std::string::npos));
    if (temControle && !tinhaControle) mostrarAviso(jogo, L"Controle conectado: " + paraWide(n), 3.0f);
    else if (!temControle && tinhaControle) mostrarAviso(jogo, L"Controle desconectado", 4.0f);
    if (!js_liberado()) travado = true;
    else if (travado && !algoSegurado(jogo)) travado = false;
    if (travado) focada = false;
    Vec2 analogico;
    if (focada) {
        bool alguma = false;
        for (int c = 0; c < TOTAL_COMANDOS; ++c) {
            if (teclas[static_cast<std::size_t>(c)]) { jogo.entrada.segurando[static_cast<std::size_t>(c)] = true; alguma = true; }
        }
        if (alguma) jogo.entrada.ultimoUsado = Dispositivo::Teclado;
        if (temControle) analogico = aplicarControle(jogo.controle.leitura, jogo.entrada);
    }
    combinarEixo(jogo.entrada, analogico);
    calcularBordas(jogo.entrada, anterior);
}

void quadro() {
    Jogo& jogo = web->jogo;
    const bool focada = js_focada() != 0;
    lerEntrada(jogo, focada);
    if (pressionou(jogo.entrada, Comando::TelaCheia)) js_tela_cheia();
    if (pressionou(jogo.entrada, Comando::Debug)) jogo.mostrarMedidor = !jogo.mostrarMedidor;
    if (pressionou(jogo.entrada, Comando::Piloto)) jogo.pilotoAtivo = !jogo.pilotoAtivo;
    for (Comando c : {Comando::TelaCheia, Comando::Debug, Comando::Piloto}) jogo.entrada.pressionou[static_cast<int>(c)] = false;
    if (jogo.pedidoTelaCheia) {
        jogo.pedidoTelaCheia = false;
        js_tela_cheia();
    }
    if (!ajustarEscala()) return;
    jogo.escalaAtual = web->canvas.escala;
    jogo.pausadoPorFoco = !focada;

    const double agora = emscripten_get_now() / 1000.0;
    double decorrido = agora - web->anterior;
    web->anterior = agora;
    if (decorrido > 0.25) decorrido = 0.25;
    if (jogo.pausadoPorFoco) {
        web->acumulador = 0.0;
        limparEntrada(jogo.entrada);
    } else {
        web->acumulador += decorrido;
        int passos = 0;
        while (web->acumulador >= DT && passos < 5) {
            atualizarCenas(jogo, DT);
            limparBordas(jogo.entrada);
            jogo.tempoTotal += DT;
            web->acumulador -= DT;
            ++passos;
        }
    }
    if (!jogo.rodando) {   // "Sair": no navegador não se fecha a aba
        jogo.rodando = true;
        jogo.salaAtual.partida.pedidoSair = false;   // "Sair do jogo" (pausa): volta ao menu inicial
        if (cenaDoTopo(jogo) != IdCena::Menu) trocarCenaBase(jogo, IdCena::Menu);
    }
    desenharCenas(jogo, web->canvas);
    registrarQuadro(jogo.medidor, static_cast<float>(emscripten_get_now() / 1000.0 - agora) * 1000.0f);
    // ARGB (B,G,R,A na memória) → RGBA para o ImageData.
    const std::size_t n = static_cast<std::size_t>(web->canvas.w) * static_cast<std::size_t>(web->canvas.h);
    web->rgba.resize(n * 4);
    for (std::size_t i = 0; i < n; ++i) {
        const std::uint32_t p = web->canvas.px[i];
        web->rgba[i * 4] = static_cast<unsigned char>(p >> 16);
        web->rgba[i * 4 + 1] = static_cast<unsigned char>(p >> 8);
        web->rgba[i * 4 + 2] = static_cast<unsigned char>(p);
        web->rgba[i * 4 + 3] = 255;
    }
    js_apresentar(web->rgba.data(), web->canvas.w, web->canvas.h);
    // Rede de segurança: mede desenho + conversão + envio; 90 quadros lentos em média → uma escala abaixo.
    if (focada && web->canvas.escala > 2) {
        web->somaQuadrosMs += emscripten_get_now() - agora * 1000.0;
        if (++web->quadrosMedidos >= 90) {
            if (web->somaQuadrosMs / web->quadrosMedidos > QUADRO_LENTO_MS) {
                web->limiteEscala = web->canvas.escala - 1;
                std::printf("Escala %d lenta (%.1f ms por quadro): usando %d.\n", web->canvas.escala,
                            web->somaQuadrosMs / web->quadrosMedidos, web->limiteEscala);
            }
            web->somaQuadrosMs = 0.0;
            web->quadrosMedidos = 0;
        }
    }
    atualizarAudio(static_cast<float>(decorrido), focada);
    if (agora - web->ultimaSincronia > 3.0) {   // perfil, recordes, cartão: para a conta AWS
        web->ultimaSincronia = agora;
        js_sincronizar_saves();
    }
}

}  // namespace

int main() {
    // UTF-8: sem isso o swprintf do navegador (musl, locale "C") para no primeiro acento ("Nível" saía "N").
    std::setlocale(LC_ALL, "C.UTF-8");
    web = new Web;
    std::string erro;
    if (!carregarRecursos(web->recursos, L"/assets/jogo_hd/", L"/assets/jogo/", erro)) {
        std::printf("Falha ao carregar os recursos: %s\n", erro.c_str());
        return 1;
    }
    if (!carregarConteudo(web->conteudo, erro)) {
        std::printf("Falha ao carregar o conteúdo: %s\n", erro.c_str());
        return 1;
    }
    Jogo& jogo = web->jogo;
    jogo.recursos = &web->recursos;
    jogo.conteudo = &web->conteudo;
    jogo.pastaSaves = L"/saves";
    jogo.perfil = carregarPerfil(std::filesystem::path("/saves/perfil.txt"));
    jogo.seed = static_cast<std::uint64_t>(emscripten_get_now() * 1000.0);

    // Opções pela URL (as mesmas da linha de comando do Windows, para a apresentação).
    const auto op = lerParametros();
    auto tem = [&](const char* k) { return op.count(k) != 0; };
    if (tem("sala")) {
        for (int i = 0; i < static_cast<int>(web->conteudo.salas.size()); ++i)
            if (web->conteudo.salas[static_cast<std::size_t>(i)].id == op.at("sala")) jogo.salaInicial = i;
    }
    if (tem("bioma")) jogo.biomaInicial = std::atoi(op.at("bioma").c_str());
    jogo.forte = tem("forte");
    jogo.pennywise = tem("pennywise");
    if (tem("fase")) jogo.faseCovil = std::atoi(op.at("fase").c_str());
    jogo.pilotoAtivo = tem("piloto");
    jogo.itensDefesa = tem("itens");
    if (tem("amigo")) jogo.amigoDefesa = op.at("amigo");
    if (tem("dificuldade")) dificuldadeDoCodigo(op.at("dificuldade"), jogo.dificuldade);
    if (tem("seed")) jogo.seed = std::strtoull(op.at("seed").c_str(), nullptr, 10);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, tecla);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, nullptr, true, tecla);

    std::string textoSons, avisos;
    TabelaSons tabelaSons;
    if (lerArquivoTexto(std::filesystem::path("/data/sons.txt"), textoSons) && lerTabelaSons(textoSons, tabelaSons, erro))
        iniciarAudio(L"assets/", tabelaSons, avisos);
    if (!avisos.empty()) std::printf("[audio] %s\n", avisos.c_str());
    {
        const OpcoesJogo& o = jogo.perfil.opcoes;
        definirVolumes(o.volumeGeral / 100.0f, o.volumeMusica / 100.0f, o.volumeEfeitos / 100.0f);
    }
    const bool direto = jogo.salaInicial >= 0 || tem("jogar") || tem("bioma") || jogo.pilotoAtivo;
    if (tem("bioma")) jogo.salaInicial = -1;
    trocarCenaBase(jogo, direto ? IdCena::Sala : IdCena::Menu);
    if (tem("cena")) abrirCenaHistoria(jogo, op.at("cena"));
    web->anterior = emscripten_get_now() / 1000.0;
    js_carregou();
    emscripten_set_main_loop(quadro, 0, false);
    return 0;
}
#endif
