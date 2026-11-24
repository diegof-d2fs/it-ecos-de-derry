#include "cenas/ui.h"

#include <cstdio>

#include "jogo/dialogo.h"
#include "render/sprite.h"
#include "render/texto.h"

std::wstring comMilhar(long long v) {
    std::wstring s = std::to_wstring(v < 0 ? -v : v);
    for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<std::size_t>(i), 1, L'.');
    return v < 0 ? L"-" + s : s;
}

void painel(Canvas& canvas, float x, float y, float w, float h) {
    retanguloAlpha(canvas, x, y, w, h, rgb(14, 16, 26), 252);   // quase opaco: nada vaza de trás
    contornoRetangulo(canvas, x, y, w, h, rgb(255, 214, 120));
}

void linhaMenu(Canvas& canvas, const Recursos& r, float x, float y, float w, const std::wstring& t, bool sel, bool apagada) {
    if (sel) {
        retanguloAlpha(canvas, x, y - 3, w, 22, rgb(255, 214, 120), 40);
        contornoRetangulo(canvas, x, y - 3, w, 22, rgb(255, 214, 120));
    }
    const std::uint32_t cor = apagada ? rgb(120, 116, 110) : (sel ? rgb(255, 240, 200) : COR_UI_TEXTO);
    desenharTexto(canvas, r.fonteMedia, x + 8, y, (sel ? L"▶ " : L"  ") + t, cor);
}

void desenharIcone(Canvas& canvas, const Recursos& r, TipoItem tipo, float x, float y) {
    const DefinicaoItem& d = definicaoDe(tipo);
    const Sprite s = spriteDe(r, d.atlas);
    if (!spriteValido(s)) return;
    desenharFrame(canvas, s, d.quadro % s.colunas, d.quadro / s.colunas, x, y);
}

void desenharRecordes(Canvas& canvas, const Recursos& r, float x, float y, float w, const std::vector<Recorde>& todos,
                      FiltroRecordes filtro, const Recorde* destaque) {
    const wchar_t* filtros[] = {L"Todas", L"Jogador", L"Computador"};
    const float larguraAba = (w - 12) / 3;
    for (int f = 0; f < 3; ++f) {
        const bool sel = static_cast<int>(filtro) == f;
        const float ax = x + 6 + larguraAba * static_cast<float>(f);
        retanguloAlpha(canvas, ax, y, larguraAba - 4, 18, sel ? rgb(255, 214, 120) : rgb(30, 32, 44), sel ? 70 : 200);
        desenharTextoCentralizado(canvas, r.fontePequena, ax + (larguraAba - 4) / 2, y + 2, filtros[f],
                                  sel ? rgb(255, 240, 200) : COR_UI_APAGADO);
    }
    const std::vector<Recorde> lista = recordesFiltrados(todos, filtro);
    wchar_t t[160];
    float ly = y + 26;
    for (std::size_t i = 0; i < lista.size(); ++i) {
        const Recorde& rc = lista[i];
        std::swprintf(t, 160, L"%2d. %ls  %02d:%02d  %ls · bioma %d%ls%ls", static_cast<int>(i + 1), comMilhar(rc.pontos).c_str(),
                      rc.tempo / 60, rc.tempo % 60, paraWide(nomeDificuldade(rc.dificuldade)).c_str(), rc.bioma,
                      rc.vitoria ? L" · vitória" : L"", rc.piloto ? L" · CPU" : L"");
        const bool minha = destaque && rc.seed == destaque->seed && rc.pontos == destaque->pontos && rc.piloto == destaque->piloto;
        desenharTexto(canvas, r.fontePequena, x + 10, ly, t, minha ? COR_UI_DESTAQUE : COR_UI_TEXTO);
        ly += 20;
    }
    if (lista.empty()) desenharTextoCentralizado(canvas, r.fontePequena, x + w / 2, y + 70, L"Nenhum recorde ainda.", COR_UI_APAGADO);
}

namespace {
bool controleNaAjuda = false;   // dispositivo do quadro que está sendo desenhado
}  // namespace

void definirControleNaAjuda(bool controle) { controleNaAjuda = controle; }

std::wstring ajuda(const std::string& utf8) { return paraWide(trocarBotoes(utf8, controleNaAjuda)); }

void desenharOpcoesDificuldade(Canvas& canvas, const Recursos& r, const TabelaDificuldade& tabela, int cursor, float y0) {
    const wchar_t* resumos[] = {L"Mais kits e itens, inimigos fracos, +1 Pacto de Sangue",
                                L"A experiência pensada para o jogo",
                                L"Mais inimigos e mais fortes, menos itens, ladrões espertos"};
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        const ParametrosDificuldade& par = parametros(tabela, static_cast<Dificuldade>(d));
        const float y = y0 + 68.0f * static_cast<float>(d);
        painel(canvas, 100, y, 440, 62);
        linhaMenu(canvas, r, 106, y + 8, 428, paraWide(nomeDificuldade(static_cast<Dificuldade>(d))), cursor == d);
        desenharTexto(canvas, r.fontePequena, 130, y + 30, resumos[d], COR_UI_TEXTO);
        wchar_t t[160];
        std::swprintf(t, 160, L"inimigos ×%d%% · dano recebido ×%d%% · item ao derrotar %d%% · pontos ×%d%%", par.orcamentoOnda,
                      par.danoRecebido, par.chanceDrop, par.multiplicadorPontos);
        desenharTexto(canvas, r.fontePequena, 130, y + 43, t, COR_UI_APAGADO);
    }
}

void desenharParagrafo(Canvas& canvas, const Fonte& fonte, float x, float y, float entrelinha, const std::wstring& texto,
                       std::uint32_t cor) {
    std::size_t inicio = 0;
    float ly = y;
    while (inicio <= texto.size()) {
        std::size_t fim = texto.find(L'\n', inicio);
        if (fim == std::wstring::npos) fim = texto.size();
        desenharTexto(canvas, fonte, x, ly, texto.substr(inicio, fim - inicio), cor);
        ly += entrelinha;
        inicio = fim + 1;
    }
}

// Quebra o texto em linhas que cabem em `largura` (por palavras). Quebrar o texto INTEIRO,
// e não só o que já apareceu, impede que uma palavra pule de linha no meio da digitação.
std::vector<std::wstring> quebrarLinhas(Canvas& canvas, const Fonte& fonte, const std::wstring& texto, float largura) {
    std::vector<std::wstring> linhas;
    std::wstring atual, palavra;
    auto fecharPalavra = [&]() {
        if (palavra.empty()) return;
        const std::wstring tentativa = atual.empty() ? palavra : atual + L" " + palavra;
        if (!atual.empty() && larguraTexto(canvas, fonte, tentativa) > largura) {
            linhas.push_back(atual);
            atual = palavra;
        } else {
            atual = tentativa;
        }
        palavra.clear();
    };
    for (wchar_t ch : texto) {
        if (ch == L' ') fecharPalavra();
        else palavra += ch;
    }
    fecharPalavra();
    if (!atual.empty()) linhas.push_back(atual);
    return linhas;
}
