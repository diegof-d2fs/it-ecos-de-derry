#pragma once
// Recursos visuais. Dono: main.cpp. Ciclo de vida:
//   carregarRecursos   lê o manifesto e os PNG (HD, em assets/jogo_hd) uma única vez → `fontes`
//   prepararEscala     reduz cada fonte para a escala do canvas → `imagens` e recria as fontes
//                      de texto. Só roda de novo quando a janela muda de escala.
//   liberarRecursos    devolve tudo.
#include <map>
#include <string>
#include <vector>

#include "core/manifesto.h"
#include "render/imagem.h"
#include "render/sprite.h"
#include "render/texto.h"

struct Recursos {
    std::vector<EntradaManifesto> manifesto;
    std::map<std::string, Imagem> fontes;    // como estão no disco (medida lógica × fator)
    std::map<std::string, Imagem> imagens;   // preparadas para a escala atual. std::map não move valores.
    // Quadros das cenas da história (cena_*): grandes demais para ficar na memória o jogo todo. Ficam só
    // no manifesto e são lidos do disco quando a cena abre (carregarSobDemanda).
    std::map<std::string, EntradaManifesto> sobDemanda;
    std::wstring pasta;                      // assets/jogo_hd (ou assets/jogo), terminada em barra
    int escala = 0;                          // escala em que `imagens` e as fontes de texto estão
    Fonte fontePequena;   // HUD, mensagens
    Fonte fonteMedia;     // diálogos, menus
    Fonte fonteGrande;    // títulos
};

// Procura `pastaHd` (assets/jogo_hd); se não existir, usa `pastaPadrao` (assets/jogo).
bool carregarRecursos(Recursos& recursos, const std::wstring& pastaHd, const std::wstring& pastaPadrao,
                      std::string& erro);
// Prepara imagens e fontes para `escala` (1..6). Retorna false se faltar memória para fontes.
bool prepararEscala(Recursos& recursos, int escala);
void liberarRecursos(Recursos& recursos);

// Lê do disco uma imagem "sob demanda" (cena_*), no tamanho do arquivo (medida lógica × fator). false = não há.
bool carregarSobDemanda(const Recursos& recursos, const std::string& nome, Imagem& saida);
// Busca uma imagem preparada pelo nome. Retorna nullptr se não existir (T03).
const Imagem* buscarImagem(const Recursos& recursos, const std::string& nome);
// Monta um Sprite a partir do manifesto; inválido (atlas nulo) se o nome não existir.
Sprite spriteDe(const Recursos& recursos, const std::string& nome);
