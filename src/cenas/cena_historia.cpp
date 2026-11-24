// Cena da história (cut scene) — pedido do Diego em 23/11/2026. Só DESENHA o tocador das regras
// (jogo/historia): a ilustração passeia devagar (zoom ou panorâmica, pixel mais próximo: é pixel art), uma
// imagem se dissolve na outra, o texto aparece letra a letra sobre uma faixa escura, e um pó fino
// flutua na luz. A música é a de tensão. {CONFIRMAR} mostra o texto inteiro / passa ao próximo quadro;
// segurar {VOLTAR} por 1 s pula a cena. Modal: a cena de baixo (menu ou sala) fica congelada.
#include <algorithm>
#include <cmath>
#include <filesystem>

#include "cenas/cena.h"
#include "cenas/ui.h"
#include "plataforma/audio.h"
#include "render/texto.h"

namespace {

constexpr float ZOOM_FOLGA = 1.08f;   // preserva a panorâmica com ampliação menor
constexpr float LARGURA_TEXTO = 560.0f;
constexpr float ENTRELINHA = 19.0f;
const std::uint32_t COR_TEXTO = rgb(244, 236, 220);
const std::uint32_t COR_TITULO = rgb(240, 200, 196);
const std::uint32_t COR_FUNDO_TEXTO = rgb(8, 8, 12);

float suave(float t) {   // começa e termina devagar
    t = std::clamp(t, 0.0f, 1.0f);
    return 0.5f - 0.5f * std::cos(t * 3.14159265f);
}

// "Transparência" do texto do GDI: mistura a cor com o fundo escuro da faixa.
std::uint32_t esmaecer(std::uint32_t cor, float visivel) {
    const float k = std::clamp(visivel, 0.0f, 1.0f);
    auto canal = [&](int s) {
        const float a = static_cast<float>((cor >> s) & 0xFF), b = static_cast<float>((COR_FUNDO_TEXTO >> s) & 0xFF);
        return static_cast<std::uint8_t>(b + (a - b) * k);
    };
    return rgb(canal(16), canal(8), canal(0));
}

void liberarPreparo(const EstadoHistoria& h) {
    h.preparadas.clear();
    h.preparadas.shrink_to_fit();
    h.escalaPreparada = 0;
    liberarFonte(h.fonteTexto);
    liberarFonte(h.fonteTitulo);
}

// Reduz cada ilustração (HD, 2560×1440) uma única vez para (tela × folga): o desenho por quadro só amostra.
void preparar(const Jogo& jogo, const Canvas& canvas) {
    const EstadoHistoria& h = jogo.historia;
    if (h.escalaPreparada == canvas.escala || !h.tocador.cena) return;
    liberarPreparo(h);
    const int w = static_cast<int>(std::lround(static_cast<float>(canvas.w) * ZOOM_FOLGA));
    const int alt = static_cast<int>(std::lround(static_cast<float>(canvas.h) * ZOOM_FOLGA));
    for (const QuadroHistoria& q : h.tocador.cena->quadros) {
        // cena_*: lido do disco agora (não fica residente); as ilustrações antigas já estão em `fontes`.
        Imagem lida;
        const Imagem* fonte = nullptr;
        if (carregarSobDemanda(*jogo.recursos, q.imagem, lida)) fonte = &lida;
        else if (const auto it = jogo.recursos->fontes.find(q.imagem); it != jogo.recursos->fontes.end()) fonte = &it->second;
        if (!fonte) h.preparadas.emplace_back();                          // sem imagem: tela preta com o texto
        else if (fonte->w > w) {
            // As ilustrações são PIXEL ART (blocos de ~6 px no original): reduzir pela média transformava a borda de
            // cada bloco num degradê e a cena inteira ficava embaçada (Diego, 24/11). Pixel mais próximo: blocos nítidos.
            h.preparadas.push_back(reduzirVizinho(*fonte, w, alt));
        }
        else h.preparadas.push_back(fonte == &lida ? std::move(lida) : *fonte);     // menor: o bilinear amplia
    }
    criarFonte(h.fonteTexto, L"Georgia", 14, canvas.escala, false);
    criarFonte(h.fonteTitulo, L"Georgia", 22, canvas.escala, true);
    h.escalaPreparada = canvas.escala;
}

// A ilustração `i` com o movimento do quadro em `progresso` (0..1).
void desenharIlustracao(Canvas& canvas, const EstadoHistoria& h, int i, float progresso, std::uint8_t opacidade) {
    if (i < 0 || i >= static_cast<int>(h.preparadas.size())) return;
    const Imagem& img = h.preparadas[static_cast<std::size_t>(i)];
    if (img.w == 0) return;
    const float e = suave(progresso);
    // Contas na "tela com folga" (tela × ZOOM_FOLGA); `k` converte para pixels da imagem guardada, que pode
    // ser menor que isso (imagem de 1280 numa tela de 1920: o bilinear amplia).
    const float k = static_cast<float>(img.w) / (static_cast<float>(canvas.w) * ZOOM_FOLGA);
    const float largura = static_cast<float>(canvas.w) * ZOOM_FOLGA, altura = static_cast<float>(canvas.h) * ZOOM_FOLGA;
    const float folgaX = largura - static_cast<float>(canvas.w), folgaY = altura - static_cast<float>(canvas.h);
    float passo = 1.0f, ox = folgaX / 2.0f, oy = folgaY / 2.0f;
    switch (h.tocador.cena->quadros[static_cast<std::size_t>(i)].movimento) {
        case MovimentoQuadro::ZoomEntra:   // começa mostrando a ilustração inteira e se aproxima do centro
        case MovimentoQuadro::ZoomSai: {
            const float t = h.tocador.cena->quadros[static_cast<std::size_t>(i)].movimento == MovimentoQuadro::ZoomEntra ? e : 1.0f - e;
            passo = ZOOM_FOLGA + (1.0f - ZOOM_FOLGA) * t;
            ox = (largura - static_cast<float>(canvas.w) * passo) / 2.0f;
            oy = (altura - static_cast<float>(canvas.h) * passo) / 2.0f;
            break;
        }
        case MovimentoQuadro::Esquerda: ox = folgaX * (1.0f - e); break;
        case MovimentoQuadro::Direita: ox = folgaX * e; break;
        case MovimentoQuadro::Cima: oy = folgaY * (1.0f - e); break;
        case MovimentoQuadro::Baixo: oy = folgaY * e; break;
    }
    desenharImagemAmostrada(canvas, img, ox * k, oy * k, passo * k, opacidade);
}

// Pó fino flutuando na luz (determinístico pelo tempo; nada de estado).
void desenharPoeira(Canvas& canvas, float t, float opacidade) {
    for (int i = 0; i < 34; ++i) {
        const float a = std::fmod(static_cast<float>(i) * 0.6180339f, 1.0f), b = std::fmod(static_cast<float>(i) * 0.7548776f, 1.0f);
        const float x = std::fmod(a * 640.0f + t * (4.0f + 6.0f * b), 640.0f);
        const float y = std::fmod(b * 360.0f + 360.0f - t * (3.0f + 5.0f * a), 360.0f) + 3.0f * std::sin(t * 0.7f + static_cast<float>(i));
        const float brilho = 0.5f + 0.5f * std::sin(t * (0.8f + a) + static_cast<float>(i) * 1.7f);
        elipseAlpha(canvas, x, y, 0.7f + 0.5f * b, 0.7f + 0.5f * b, rgb(255, 226, 180),
                    static_cast<std::uint8_t>(opacidade * (10.0f + 26.0f * brilho)));
    }
}

// Mistura um pixel de tela com `cor` na força a/255.
inline std::uint32_t puxarPara(std::uint32_t p, std::uint32_t cor, std::uint32_t a) {
    const std::uint32_t u = 255 - a;
    const std::uint32_t rb = ((((p & 0x00FF00FFu) * u) + ((cor & 0x00FF00FFu) * a)) >> 8) & 0x00FF00FFu;
    const std::uint32_t g = ((((p & 0x0000FF00u) * u) + ((cor & 0x0000FF00u) * a)) >> 8) & 0x0000FF00u;
    return 0xFF000000u | rb | g;
}

// Degradê em PIXELS DE TELA (uma linha ou coluna por vez): sem as emendas que retângulos
// sobrepostos deixavam como riscos horizontais. Força = maximo × t·(2 − t), t de 0 (borda interna) a 1:
// escurece logo (onde o texto fica) e chega suave na borda da imagem.
void degradeVertical(Canvas& c, float yIni, float yFim, std::uint32_t cor, float maximo, bool escuroEmBaixo) {
    const int r0 = std::max(0, paraTela(c, yIni)), r1 = std::min(c.h, paraTela(c, yFim));
    for (int y = r0; y < r1; ++y) {
        float t = static_cast<float>(y - r0) / static_cast<float>(std::max(1, r1 - r0 - 1));
        if (!escuroEmBaixo) t = 1.0f - t;
        const auto a = static_cast<std::uint32_t>(maximo * t * (2.0f - t));
        std::uint32_t* linha = c.px + static_cast<std::size_t>(y) * static_cast<std::size_t>(c.w);
        for (int x = 0; x < c.w; ++x) linha[x] = puxarPara(linha[x], cor, a);
    }
}

void vinhetaLateral(Canvas& c, float largura, float maximo) {
    const int n = std::max(1, paraTela(c, largura));
    for (int k = 0; k < n; ++k) {
        const float t = 1.0f - static_cast<float>(k) / static_cast<float>(n);
        const auto a = static_cast<std::uint32_t>(maximo * t * t);
        for (int y = 0; y < c.h; ++y) {
            std::uint32_t* linha = c.px + static_cast<std::size_t>(y) * static_cast<std::size_t>(c.w);
            linha[k] = puxarPara(linha[k], 0, a);
            linha[c.w - 1 - k] = puxarPara(linha[c.w - 1 - k], 0, a);
        }
    }
}

// Escurece para baixo (o texto fica legível em qualquer ilustração), em cima (título) e nas bordas.
void desenharFaixas(Canvas& canvas, bool topo) {
    // 24/11: mais leve (a faixa cobria um terço da ilustração a 85% e a vinheta lavava as bordas): o escuro
    // só atrás do texto, e a ilustração aparece inteira e viva.
    degradeVertical(canvas, ALTURA_LOGICA - 100.0f, ALTURA_LOGICA, COR_FUNDO_TEXTO, 200.0f, true);
    if (topo) degradeVertical(canvas, 0, 50.0f, COR_FUNDO_TEXTO, 150.0f, false);
    vinhetaLateral(canvas, 18.0f, 45.0f);
}

// Texto do quadro, digitado: as linhas saem do texto INTEIRO (nenhuma palavra pula de linha) e cada uma
// fica parada no lugar enquanto as letras aparecem. '|' força a quebra.
void desenharTextoDoQuadro(Canvas& canvas, const EstadoHistoria& h, const QuadroHistoria& q, int letras, float visivel) {
    const std::wstring texto = paraWide(q.texto);
    std::vector<std::wstring> linhas;
    std::size_t inicio = 0;
    while (inicio <= texto.size()) {
        const std::size_t barra = texto.find(L'|', inicio);
        const std::wstring trecho = texto.substr(inicio, barra == std::wstring::npos ? std::wstring::npos : barra - inicio);
        for (const std::wstring& l : quebrarLinhas(canvas, h.fonteTexto, trecho, LARGURA_TEXTO)) linhas.push_back(l);
        if (barra == std::wstring::npos) break;
        inicio = barra + 1;
    }
    const float base = ALTURA_LOGICA - 30.0f - ENTRELINHA * static_cast<float>(linhas.size());
    int restantes = letras;
    for (std::size_t i = 0; i < linhas.size() && restantes > 0; ++i) {
        const std::wstring& l = linhas[i];
        const float x = (LARGURA_LOGICA - larguraTexto(canvas, h.fonteTexto, l)) / 2.0f;
        const int n = std::min(restantes, static_cast<int>(l.size()));
        desenharTexto(canvas, h.fonteTexto, x, base + ENTRELINHA * static_cast<float>(i), l.substr(0, static_cast<std::size_t>(n)),
                      esmaecer(COR_TEXTO, visivel));
        restantes -= n + 1;   // o espaço (ou a barra) que separava as linhas
    }
}

void terminar(Jogo& jogo) {
    const TocadorHistoria& t = jogo.historia.tocador;
    if (t.cena && !jogo.pilotoAtivo && jogo.perfil.cenasVistas.insert(t.cena->id).second) {
        gravarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt", jogo.perfil);
    }
    liberarPreparo(jogo.historia);
    jogo.historia.tocador = TocadorHistoria{};
    desempilharCena(jogo);
    const IdCena baixo = cenaDoTopo(jogo);
    if (baixo == IdCena::Sala) retomarSomDaSala(jogo);
    else if (baixo == IdCena::Menu) definirMusica(IdSom::MusicaMenu);
}

}  // namespace

bool abrirCenaHistoria(Jogo& jogo, const std::string& id) {
    const CenaHistoria* cena = buscarCenaHistoria(jogo.conteudo->cenas, id);
    if (!cena || cena->quadros.empty() || cenaDoTopo(jogo) == IdCena::Historia) return false;
    iniciarTocador(jogo.historia.tocador, cena);
    empilharCena(jogo, IdCena::Historia);
    return true;
}

void entrarHistoria(Jogo& jogo) {
    liberarPreparo(jogo.historia);   // prepara no 1º desenho, já na escala da tela
    definirMusica(IdSom::MusicaTensao);
    ligarAmbiente(IdSom::Total);
    definirTensao(0.0f);
}

void atualizarHistoria(Jogo& jogo, const EstadoEntrada& entrada, float dt) {
    TocadorHistoria& t = jogo.historia.tocador;
    const bool avancar = pressionou(entrada, Comando::Confirmar) || pressionou(entrada, Comando::Atacar);
    const bool pular = segurando(entrada, Comando::Voltar) || segurando(entrada, Comando::Pausa);
    atualizarTocador(t, avancar, pular, dt);
    if (t.terminou) terminar(jogo);
}

void desenharHistoria(const Jogo& jogo, Canvas& canvas) {
    const EstadoHistoria& h = jogo.historia;
    const TocadorHistoria& t = h.tocador;
    limpar(canvas, rgb(0, 0, 0));
    if (!t.cena || t.terminou) return;
    preparar(jogo, canvas);
    const std::vector<QuadroHistoria>& qs = t.cena->quadros;
    const QuadroHistoria& q = qs[static_cast<std::size_t>(t.quadro)];
    const float duracao = duracaoQuadro(q);

    // Ilustrações: a anterior continua se movendo enquanto a atual se dissolve por cima dela.
    float entrada = 1.0f;
    if (t.quadro == 0) {
        entrada = suave(t.tempoQuadro / HISTORIA_ENTRADA);
    } else if (t.tempoQuadro < HISTORIA_TRANSICAO) {
        const float dAnterior = duracaoQuadro(qs[static_cast<std::size_t>(t.quadro - 1)]);
        desenharIlustracao(canvas, h, t.quadro - 1, (dAnterior + t.tempoQuadro) / (dAnterior + HISTORIA_TRANSICAO), 255);
        entrada = suave(t.tempoQuadro / HISTORIA_TRANSICAO);
    }
    desenharIlustracao(canvas, h, t.quadro, progressoDoQuadro(t), static_cast<std::uint8_t>(255.0f * entrada));
    const float saida = t.saida > 0.0f ? 1.0f - std::clamp(t.saida / HISTORIA_SAIDA, 0.0f, 1.0f) : 1.0f;
    const bool preta = q.imagem == "-";
    if (!preta) desenharPoeira(canvas, t.tempoCena, saida);

    // Título da cena no começo (como o nome de um capítulo).
    const float tituloVisivel = std::min(std::clamp((t.tempoCena - 0.6f) / 1.0f, 0.0f, 1.0f), std::clamp((7.5f - t.tempoCena) / 1.2f, 0.0f, 1.0f));
    if (!preta) desenharFaixas(canvas, tituloVisivel > 0.0f);
    if (tituloVisivel > 0.0f && !t.cena->titulo.empty()) {
        desenharTextoCentralizado(canvas, h.fonteTitulo, LARGURA_LOGICA / 2.0f, 14, paraWide(t.cena->titulo), esmaecer(COR_TITULO, tituloVisivel));
    }

    // Texto: aparece letra a letra e some meio segundo antes da troca de quadro.
    const float textoVisivel = std::clamp((duracao - t.tempoQuadro) / 0.6f, 0.0f, 1.0f);
    if (preta) {   // cartão preto ("FIM"): texto grande no meio, entrando e saindo devagar
        const float v = std::min(std::clamp((t.tempoQuadro - 0.3f) / 1.2f, 0.0f, 1.0f), textoVisivel);
        desenharTextoCentralizado(canvas, h.fonteTitulo, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA / 2.0f - 16, paraWide(q.texto), esmaecer(COR_TITULO, v));
    } else if (textoVisivel > 0.0f) {
        desenharTextoDoQuadro(canvas, h, q, letrasVisiveis(t), textoVisivel);
    }

    // Quadro atual (pontinhos) e os botões, discretos; a dica volta quando alguém aperta algo.
    const float dica = std::max(std::clamp((8.0f - t.tempoCena) / 1.0f, 0.0f, 1.0f), std::clamp((3.0f - t.ultimoToque) / 0.8f, 0.0f, 1.0f));
    for (int i = 0; i < static_cast<int>(qs.size()); ++i) {
        const float x = LARGURA_LOGICA - 14.0f - 7.0f * static_cast<float>(static_cast<int>(qs.size()) - 1 - i);
        elipseAlpha(canvas, x, ALTURA_LOGICA - 10, 1.6f, 1.6f, rgb(240, 220, 200), i == t.quadro ? 200 : (i < t.quadro ? 110 : 45));
    }
    if (dica > 0.0f) {
        desenharTexto(canvas, jogo.recursos->fontePequena, 12, ALTURA_LOGICA - 16,
                      ajuda("{CONFIRMAR} avançar · segure {VOLTAR} para pular"), esmaecer(rgb(170, 166, 156), dica));
    }
    if (t.segurandoPular > 0.0f) {
        const float f = std::clamp(t.segurandoPular / HISTORIA_SEGURAR_PULAR, 0.0f, 1.0f);
        retanguloAlpha(canvas, 12, ALTURA_LOGICA - 4, 150, 1.5f, rgb(255, 255, 255), 50);
        retanguloAlpha(canvas, 12, ALTURA_LOGICA - 4, 150 * f, 1.5f, rgb(255, 214, 140), 220);
    }
    if (saida < 1.0f) retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), static_cast<std::uint8_t>(255.0f * (1.0f - saida)));
}
