#pragma once
// Peças dos menus repaginados (pedido do Diego, 24/11/2026: "repaginar todo o menu de inventário, itens e árvore de
// habilidades, inspire-se em Resident Evil, Final Fantasy e Hades; algo profissional e bonito").
//
// Tudo é sprite gerado para o jogo (pacote v9, P241–P251): moldura em 9 fatias, placas de linha, abas de caderno
// com ícones, molduras de item por raridade, polaroides do Mural, ícones de atributos e de talentos, e as
// ilustrações de fundo. Se um asset faltar, cada peça cai numa versão simples (o jogo nunca fica sem menu).
#include <cstdint>
#include <string>

#include "jogo/itens.h"
#include "render/canvas.h"
#include "render/recursos.h"

// Ilustração em tela cheia (tela_*) com um véu escuro por cima (0 = sem véu). Sem a imagem, só o véu.
void fundoDeMenu(Canvas& canvas, const Recursos& r, const char* tela, std::uint8_t veu);
// Painel com a moldura ornamentada (ui_painel, 9 fatias: os cantos não esticam).
void painelOrnado(Canvas& canvas, const Recursos& r, float x, float y, float w, float h);
// Placa de linha de menu (ui_placas): apagada ou acesa (selecionada), esticada só no meio.
void placaDeLinha(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, bool sel);
// Aba de caderno (ui_abas) com o ícone `icone` de ui_icones_abas (-1 = sem ícone) e o texto.
void abaOrnada(Canvas& canvas, const Recursos& r, float x, float y, float w, float h, bool sel, int icone,
               const std::wstring& texto);
// Medalhão do atributo i (Força, Foco, Agilidade, Coragem), `lado` × `lado` lógico.
void iconeAtributo(Canvas& canvas, const Recursos& r, int i, float x, float y, float lado);
// Moldura grande de item (0 comum, 1 raro, 2 épico) com o ícone ampliado dentro.
void molduraDeItem(Canvas& canvas, const Recursos& r, int nivel, float x, float y, float lado);
void iconeAmpliado(Canvas& canvas, const Recursos& r, TipoItem tipo, float x, float y, float lado, std::uint8_t opacidade = 255);
// Barra com fundo escuro, trilho e brilho no topo (vida, XP).
void barraOrnada(Canvas& canvas, float x, float y, float w, float h, float fracao, std::uint32_t cor);
// Título com um filete dourado embaixo, centralizado em `cx`.
void tituloDeSecao(Canvas& canvas, const Recursos& r, float cx, float y, float largura, const std::wstring& texto);
// Brilho quente atrás do que está selecionado (lanterna).
void brilhoDeSelecao(Canvas& canvas, float cx, float cy, float rx, float ry, float tempo);
// Qualquer frame de um sprite desenhado num retângulo lógico (escala livre, vizinho mais próximo).
void desenharFrameEm(Canvas& canvas, const Sprite& s, int coluna, int linha, float x, float y, float w, float h,
                     std::uint8_t opacidade = 255);
