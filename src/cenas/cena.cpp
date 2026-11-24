#include "cenas/cena.h"

#include "render/texto.h"

namespace {

// A ordem segue IdCena. Para criar uma cena: acrescentar o Id e uma linha aqui.
const DefinicaoCena TABELA_CENAS[static_cast<int>(IdCena::Total)] = {
    {"Vitrine", false, entrarVitrine, atualizarVitrine, desenharVitrine},
    {"TesteControle", true, entrarTesteControle, atualizarTesteControle, desenharTesteControle},
    {"Sala", false, entrarSala, atualizarSala, desenharSala},
    {"Menu", false, entrarMenu, atualizarMenu, desenharMenu},
    {"Historia", true, entrarHistoria, atualizarHistoria, desenharHistoria},
    {"Cartao", true, entrarCartao, atualizarCartao, desenharCartao},
};

// Aviso global (conexão do controle), por cima de qualquer cena.
void desenharAviso(const Jogo& jogo, Canvas& canvas) {
    if (jogo.aviso.restante <= 0.0f || jogo.aviso.texto.empty()) return;
    const Recursos& r = *jogo.recursos;
    const float largura = larguraTexto(canvas, r.fonteMedia, jogo.aviso.texto) + 24;
    const float x = (LARGURA_LOGICA - largura) / 2;
    // Some aos poucos no último meio segundo.
    const float opacidade = jogo.aviso.restante < 0.5f ? jogo.aviso.restante / 0.5f : 1.0f;
    retanguloAlpha(canvas, x, 8, largura, 22, rgb(8, 10, 16), static_cast<std::uint8_t>(210 * opacidade));
    contornoRetangulo(canvas, x, 8, largura, 22, rgb(179, 32, 42));
    desenharTextoCentralizado(canvas, r.fonteMedia, LARGURA_LOGICA / 2.0f, 11, jogo.aviso.texto, rgb(240, 232, 214));
}

}  // namespace

IdCena cenaDoTopo(const Jogo& jogo) {
    return jogo.cenas.quantidade > 0 ? jogo.cenas.itens[jogo.cenas.quantidade - 1] : IdCena::Total;
}

void mostrarAviso(Jogo& jogo, const std::wstring& texto, float segundos) {
    jogo.aviso.texto = texto;
    jogo.aviso.restante = segundos;
}

const DefinicaoCena& definicaoCena(IdCena id) { return TABELA_CENAS[static_cast<int>(id)]; }

void trocarCenaBase(Jogo& jogo, IdCena id) {
    jogo.cenas.quantidade = 0;
    empilharCena(jogo, id);
}

void empilharCena(Jogo& jogo, IdCena id) {
    PilhaCenas& pilha = jogo.cenas;
    if (pilha.quantidade >= static_cast<int>(pilha.itens.size())) {
        return;   // limite de segurança: nunca acontece em uso normal
    }
    pilha.itens[pilha.quantidade++] = id;
    limparEntrada(jogo.entrada);   // a tecla que abriu a cena não "vaza" para ela
    definicaoCena(id).entrar(jogo);
}

void desempilharCena(Jogo& jogo) {
    if (jogo.cenas.quantidade > 1) {
        --jogo.cenas.quantidade;
        limparEntrada(jogo.entrada);
    }
}

void atualizarCenas(Jogo& jogo, float dt) {
    if (jogo.aviso.restante > 0.0f) jogo.aviso.restante -= dt;
    const int topo = jogo.cenas.quantidade - 1;
    if (topo < 0) return;
    // Só o topo recebe entrada; as de baixo seguem o tempo se o topo não for modal.
    const EstadoEntrada vazia{};
    for (int i = 0; i <= topo; ++i) {
        const IdCena id = jogo.cenas.itens[i];
        bool congelada = false;
        for (int j = i + 1; j <= topo; ++j) {
            if (definicaoCena(jogo.cenas.itens[j]).modal) congelada = true;
        }
        if (!congelada) {
            definicaoCena(id).atualizar(jogo, i == topo ? jogo.entrada : vazia, dt);
        }
        if (jogo.cenas.quantidade - 1 != topo) return;   // a pilha mudou neste passo
    }
}

void desenharCenas(const Jogo& jogo, Canvas& canvas) {
    for (int i = 0; i < jogo.cenas.quantidade; ++i) {
        definicaoCena(jogo.cenas.itens[i]).desenhar(jogo, canvas);
    }
    desenharAviso(jogo, canvas);
}
