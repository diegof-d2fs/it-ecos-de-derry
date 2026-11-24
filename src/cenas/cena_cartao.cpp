// Cartão de memória (pedido do Diego em 23/11/2026): a tela dos 15 blocos, como nos cartões do PlayStation 1 e 2.
// Abre pela pausa ("Salvar jogo") ou pelo menu ("Carregar jogo"). Toda ação pergunta antes, com Sim/Não lado a
// lado: salvar num bloco livre, sobrescrever um ocupado, apagar e carregar. As regras do que se grava moram em
// jogo/salvamento (o ponto de entrada da sala atual); aqui só a tela e os arquivos.
#include <ctime>
#include <filesystem>

#include "cenas/cena.h"
#include "cenas/ui.h"
#include "plataforma/audio.h"
#include "render/texto.h"

namespace {

constexpr int COLUNAS = 5;
const std::uint32_t COR_FUNDO = rgb(10, 14, 34);
const std::uint32_t COR_BLOCO = rgb(26, 34, 70);
const std::uint32_t COR_BLOCO_LIVRE = rgb(18, 22, 40);
const std::uint32_t COR_TITULO = rgb(190, 210, 255);

std::filesystem::path pastaDosBlocos(const Jogo& jogo) { return std::filesystem::path(jogo.pastaSaves); }

std::string agora() {
    const std::time_t t = std::time(nullptr);
    char texto[32] = "";
    if (const std::tm* tm = std::localtime(&t)) std::strftime(texto, sizeof texto, "%d/%m/%Y %H:%M", tm);
    return texto;
}

void recarregarLista(Jogo& jogo) { jogo.cartao.blocos = listarBlocos(pastaDosBlocos(jogo), *jogo.conteudo); }

void avisar(EstadoCartao& c, const std::wstring& texto) {
    c.mensagem = texto;
    c.tempoMensagem = 2.5f;
}

const ResumoBloco& blocoAtual(const EstadoCartao& c) { return c.blocos[static_cast<std::size_t>(c.cursor)]; }

void executar(Jogo& jogo) {
    EstadoCartao& c = jogo.cartao;
    const int bloco = c.cursor + 1;
    switch (c.confirmando) {
        case AcaoCartao::Salvar:
            if (gravarBloco(pastaDosBlocos(jogo), bloco, jogo.salaAtual.partida.pontoDaSala, *jogo.conteudo, agora())) {
                avisar(c, L"Jogo salvo no bloco " + std::to_wstring(bloco) + L".");
                tocarEfeito(IdSom::Bau);
            } else {
                avisar(c, L"Não foi possível salvar.");
            }
            break;
        case AcaoCartao::Apagar:
            apagarBloco(pastaDosBlocos(jogo), bloco);
            avisar(c, L"Bloco " + std::to_wstring(bloco) + L" apagado.");
            break;
        case AcaoCartao::Carregar: {
            std::string erro;
            PontoSalvamento ponto;
            if (!carregarBloco(pastaDosBlocos(jogo), bloco, *jogo.conteudo, ponto, erro)) {
                avisar(c, L"Não foi possível carregar: " + paraWide(erro));
                break;
            }
            jogo.pontoParaCarregar = ponto;
            jogo.carregarPonto = true;
            c.confirmando = AcaoCartao::Nenhuma;
            trocarCenaBase(jogo, IdCena::Sala);   // entrarSala vê o ponto e recoloca Bill na sala salva
            return;
        }
        default: break;
    }
    c.confirmando = AcaoCartao::Nenhuma;
    recarregarLista(jogo);
}

}  // namespace

void abrirCartao(Jogo& jogo, bool salvar) {
    jogo.cartao = EstadoCartao{};
    jogo.cartao.salvar = salvar;
    empilharCena(jogo, IdCena::Cartao);
}

void entrarCartao(Jogo& jogo) { recarregarLista(jogo); }

void atualizarCartao(Jogo& jogo, const EstadoEntrada& e, float dt) {
    EstadoCartao& c = jogo.cartao;
    if (c.tempoMensagem > 0.0f) c.tempoMensagem -= dt;
    const bool confirmou = pressionou(e, Comando::Confirmar) || pressionou(e, Comando::Atacar);
    if (c.confirmando != AcaoCartao::Nenhuma) {   // "Tem certeza?": Sim/Não lado a lado (← → ou ↑ ↓)
        if (pressionou(e, Comando::Esquerda) || pressionou(e, Comando::Direita) || pressionou(e, Comando::Cima) ||
            pressionou(e, Comando::Baixo))
            c.simNao = 1 - c.simNao;
        if (pressionou(e, Comando::Voltar)) c.confirmando = AcaoCartao::Nenhuma;
        else if (confirmou) {
            if (c.simNao == 0) executar(jogo);
            else c.confirmando = AcaoCartao::Nenhuma;
        }
        return;
    }
    if (pressionou(e, Comando::Voltar) || pressionou(e, Comando::Pausa)) {
        desempilharCena(jogo);   // volta à pausa (salvar) ou ao menu (carregar)
        return;
    }
    const int linhas = TOTAL_BLOCOS / COLUNAS;
    int col = c.cursor % COLUNAS, lin = c.cursor / COLUNAS;
    if (pressionou(e, Comando::Direita)) col = (col + 1) % COLUNAS;
    if (pressionou(e, Comando::Esquerda)) col = (col + COLUNAS - 1) % COLUNAS;
    if (pressionou(e, Comando::Baixo)) lin = (lin + 1) % linhas;
    if (pressionou(e, Comando::Cima)) lin = (lin + linhas - 1) % linhas;
    c.cursor = lin * COLUNAS + col;
    const bool ocupado = blocoAtual(c).ocupado;
    if (pressionou(e, Comando::Interagir)) {   // apagar (sempre pergunta)
        if (ocupado) {
            c.confirmando = AcaoCartao::Apagar;
            c.simNao = 1;   // começa no "Não"
        } else {
            avisar(c, L"Este bloco já está livre.");
        }
        return;
    }
    if (!confirmou) return;
    if (c.salvar) {
        c.confirmando = AcaoCartao::Salvar;   // livre: "Salvar neste bloco?"; ocupado: "Sobrescrever?"
        c.simNao = ocupado ? 1 : 0;
    } else if (ocupado) {
        c.confirmando = AcaoCartao::Carregar;
        c.simNao = 0;
    } else {
        avisar(c, L"Bloco livre: não há jogo salvo aqui.");
    }
}

void desenharCartao(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const EstadoCartao& c = jogo.cartao;
    retanguloCheio(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, COR_FUNDO);
    // faixas suaves de "brilho" do cartão (como o fundo azul dos navegadores de cartão)
    for (int k = 0; k < 6; ++k)
        retanguloAlpha(canvas, 0, 60.0f * static_cast<float>(k), LARGURA_LOGICA, 30, rgb(40, 60, 120), static_cast<std::uint8_t>(10 + 4 * k));
    int ocupados = 0;
    for (const ResumoBloco& b : c.blocos) ocupados += b.ocupado ? 1 : 0;
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 8, c.salvar ? L"Salvar jogo" : L"Carregar jogo", COR_TITULO);
    wchar_t sub[96];
    std::swprintf(sub, 96, L"Cartão de memória · %d blocos · %d livres", TOTAL_BLOCOS, TOTAL_BLOCOS - ocupados);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 40, sub, rgb(150, 170, 210));

    // Grade 5 × 3 de blocos: ocupado = retrato de Bill e o bioma; livre = "Livre".
    constexpr float L = 92.0f, A = 64.0f, ESP = 10.0f, X0 = (LARGURA_LOGICA - (5 * L + 4 * ESP)) / 2.0f, Y0 = 60.0f;
    const Sprite retrato = spriteDe(r, "retrato_bill");
    for (int i = 0; i < TOTAL_BLOCOS; ++i) {
        const ResumoBloco& b = c.blocos[static_cast<std::size_t>(i)];
        const float x = X0 + static_cast<float>(i % COLUNAS) * (L + ESP), y = Y0 + static_cast<float>(i / COLUNAS) * (A + ESP);
        const bool sel = i == c.cursor;
        retanguloCheio(canvas, x, y, L, A, b.ocupado ? COR_BLOCO : COR_BLOCO_LIVRE);
        contornoRetangulo(canvas, x, y, L, A, sel ? COR_UI_DESTAQUE : rgb(70, 86, 140));
        if (sel) contornoRetangulo(canvas, x - 2, y - 2, L + 4, A + 4, COR_UI_DESTAQUE);
        wchar_t num[8];
        std::swprintf(num, 8, L"%02d", i + 1);
        desenharTexto(canvas, r.fontePequena, x + 4, y + 2, num, rgb(150, 170, 210));
        if (b.ocupado) {
            if (spriteValido(retrato)) {   // 64×64 do retrato, reduzido à metade: o "ícone" do bloco
                desenharFramePesEscala(canvas, retrato, 0, 0, x + 22, y + A - 6, 0.5f, false, 255, 0xFFFFFF, 0);
            }
            wchar_t t[48];
            std::swprintf(t, 48, L"Bioma %d", b.bioma);
            desenharTexto(canvas, r.fontePequena, x + 42, y + 18, t, COR_UI_TEXTO);
            std::swprintf(t, 48, L"Nv. %d", b.nivel);
            desenharTexto(canvas, r.fontePequena, x + 42, y + 34, t, rgb(200, 210, 230));
        } else {
            desenharTextoCentralizado(canvas, r.fontePequena, x + L / 2.0f, y + A / 2.0f - 6, L"Livre", rgb(110, 120, 150));
        }
    }

    // Detalhes do bloco escolhido.
    const ResumoBloco& b = blocoAtual(c);
    painel(canvas, 40, 284, LARGURA_LOGICA - 80, 44);
    if (b.ocupado) {
        const int s = b.segundos;
        wchar_t linha1[200], linha2[160];
        std::swprintf(linha1, 200, L"Bloco %02d · %ls · %ls", c.cursor + 1, paraWide(nomeBioma(b.bioma)).c_str(), paraWide(b.nomeSala).c_str());
        std::swprintf(linha2, 160, L"Nível %d · %02d:%02d · %ls · %ls pontos · salvo em %ls", b.nivel, s / 60, s % 60,
                      paraWide(nomeDificuldade(b.dificuldade)).c_str(), comMilhar(b.pontos).c_str(), paraWide(b.data).c_str());
        desenharTexto(canvas, r.fontePequena, 50, 290, linha1, COR_UI_TEXTO);
        desenharTexto(canvas, r.fontePequena, 50, 306, linha2, rgb(170, 180, 200));
    } else {
        wchar_t linha[64];
        std::swprintf(linha, 64, L"Bloco %02d · Livre", c.cursor + 1);
        desenharTexto(canvas, r.fontePequena, 50, 298, linha, rgb(150, 160, 190));
    }
    if (c.tempoMensagem > 0.0f) desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, 264, c.mensagem, rgb(160, 235, 170));

    // "Tem certeza?"
    if (c.confirmando != AcaoCartao::Nenhuma) {
        retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(0, 0, 0), 120);
        const float cx = LARGURA_LOGICA / 2.0f - 140, cy = 130;
        painel(canvas, cx, cy, 280, 96);
        std::wstring pergunta;
        const int n = c.cursor + 1;
        switch (c.confirmando) {
            case AcaoCartao::Salvar:
                pergunta = b.ocupado ? L"Sobrescrever o bloco " + std::to_wstring(n) + L"?" : L"Salvar no bloco " + std::to_wstring(n) + L"?";
                break;
            case AcaoCartao::Apagar: pergunta = L"Apagar o bloco " + std::to_wstring(n) + L"?"; break;
            default: pergunta = L"Carregar o jogo do bloco " + std::to_wstring(n) + L"?"; break;
        }
        desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, cy + 10, pergunta, rgb(255, 214, 160));
        const wchar_t* detalhe = c.confirmando == AcaoCartao::Apagar ? L"O jogo salvo nele se perde."
                               : c.confirmando == AcaoCartao::Carregar ? L"Você volta para a entrada da sala salva."
                               : b.ocupado ? L"O jogo salvo nele será substituído." : L"Salva a entrada da sala em que você está.";
        desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, cy + 34, detalhe, rgb(170, 166, 156));
        linhaMenu(canvas, r, cx + 50, cy + 60, 70, L"Sim", c.simNao == 0);
        linhaMenu(canvas, r, cx + 160, cy + 60, 70, L"Não", c.simNao == 1);
    }
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, ALTURA_LOGICA - 16,
                              c.confirmando != AcaoCartao::Nenhuma ? ajuda("← → Sim ou Não · {CONFIRMAR} confirmar · {VOLTAR} cancelar")
                              : c.salvar ? ajuda("setas: escolher bloco · {CONFIRMAR} salvar · {INTERAGIR} apagar · {VOLTAR} voltar")
                                         : ajuda("setas: escolher bloco · {CONFIRMAR} carregar · {INTERAGIR} apagar · {VOLTAR} voltar"),
                              rgb(170, 180, 200));
}
