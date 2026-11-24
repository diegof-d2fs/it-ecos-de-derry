#pragma once
// Peças de interface compartilhadas pelas cenas (sala, menu, Clube): painéis, linhas de menu,
// números com separador de milhar, ícones de item e a tabela de recordes. Uma só aparência
// para todos os menus do jogo.
#include <string>
#include <vector>

#include "jogo/dificuldade.h"
#include "jogo/itens.h"
#include "jogo/recordes.h"
#include "render/canvas.h"
#include "render/recursos.h"

constexpr std::uint32_t COR_UI_TEXTO = rgb(240, 232, 214);
constexpr std::uint32_t COR_UI_DESTAQUE = rgb(255, 228, 150);
constexpr std::uint32_t COR_UI_APAGADO = rgb(150, 146, 136);

// 37400 → "37.400" (separador de milhar do português).
std::wstring comMilhar(long long v);
// Painel quase opaco com moldura dourada.
void painel(Canvas& canvas, float x, float y, float w, float h);
// Linha de menu com cursor (▶) e destaque; `apagada` = opção indisponível.
void linhaMenu(Canvas& canvas, const Recursos& r, float x, float y, float w, const std::wstring& t, bool sel,
               bool apagada = false);
// Ícone de item (16×16 lógico) com o canto superior esquerdo em (x, y).
void desenharIcone(Canvas& canvas, const Recursos& r, TipoItem tipo, float x, float y);
// Abas do filtro + top 10. `destaque` (pode ser nullptr) acende a linha da partida que acabou.
void desenharRecordes(Canvas& canvas, const Recursos& r, float x, float y, float w, const std::vector<Recorde>& todos,
                      FiltroRecordes filtro, const Recorde* destaque);
// As 3 dificuldades com resumo e números lidos da tabela (menu "Assistir" e alçapão do Clube).
// Caixas de 62 de altura a cada 68, a partir de `y`.
void desenharOpcoesDificuldade(Canvas& canvas, const Recursos& r, const TabelaDificuldade& tabela, int cursor, float y);
// Textos de ajuda ("{INTERAGIR}: abrir o baú") com o botão do dispositivo usado por último
// (teclado ou controle), como nas falas (PRD §15.4). A cena informa o dispositivo no começo
// de cada desenho, porque as funções de desenho só recebem dados constantes.
void definirControleNaAjuda(bool controle);
std::wstring ajuda(const std::string& utf8);
// Texto em várias linhas ('\n'), a partir de (x, y), com `entrelinha` unidades lógicas.
void desenharParagrafo(Canvas& canvas, const Fonte& fonte, float x, float y, float entrelinha, const std::wstring& texto,
                       std::uint32_t cor);

// Quebra o texto em linhas que cabem em `largura` (por palavras). Quebre o texto INTEIRO (e não só o
// que já apareceu): assim uma palavra não pula de linha no meio da digitação.
std::vector<std::wstring> quebrarLinhas(Canvas& canvas, const Fonte& fonte, const std::wstring& texto, float largura);
