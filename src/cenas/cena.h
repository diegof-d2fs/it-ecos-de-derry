#pragma once
// Máquina de cenas (SDD §5). Cada cena é uma linha numa tabela de PONTEIROS PARA FUNÇÃO
// (técnica T03): entrar, atualizar e desenhar. Overlays (diálogo, inventário, pausa…)
// são cenas empilhadas por cima; uma cena modal congela as de baixo.
#include "cenas/jogo.h"
#include "render/canvas.h"

struct DefinicaoCena {
    const char* nome;
    bool modal;
    void (*entrar)(Jogo& jogo);
    void (*atualizar)(Jogo& jogo, const EstadoEntrada& entrada, float dt);
    void (*desenhar)(const Jogo& jogo, Canvas& canvas);
};

const DefinicaoCena& definicaoCena(IdCena id);

void trocarCenaBase(Jogo& jogo, IdCena id);   // esvazia a pilha e entra em `id`
void empilharCena(Jogo& jogo, IdCena id);
void desempilharCena(Jogo& jogo);
void atualizarCenas(Jogo& jogo, float dt);
void desenharCenas(const Jogo& jogo, Canvas& canvas);
IdCena cenaDoTopo(const Jogo& jogo);   // IdCena::Total se a pilha estiver vazia
void mostrarAviso(Jogo& jogo, const std::wstring& texto, float segundos);

// ---------- cenas (cada uma no seu arquivo) ----------
void entrarVitrine(Jogo& jogo);
void atualizarVitrine(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharVitrine(const Jogo& jogo, Canvas& canvas);

void entrarSala(Jogo& jogo);
void atualizarSala(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharSala(const Jogo& jogo, Canvas& canvas);

void entrarMenu(Jogo& jogo);
void atualizarMenu(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharMenu(const Jogo& jogo, Canvas& canvas);

// Cenas da história (cut scenes): abre `id` (data/cenas.txt) por cima da cena atual, que fica congelada.
// false = a cena não existe (nada muda). Ao terminar, volta à cena de baixo e retoma o som dela.
bool abrirCenaHistoria(Jogo& jogo, const std::string& id);
void entrarHistoria(Jogo& jogo);
void atualizarHistoria(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharHistoria(const Jogo& jogo, Canvas& canvas);
// Música e ambiente da sala atual (ou silêncio, se a partida terminou). Usada na volta de uma cena da história.
void retomarSomDaSala(Jogo& jogo);

// Cartão de memória (15 blocos): `salvar` = pela pausa; senão, carregar (menu).
void abrirCartao(Jogo& jogo, bool salvar);
void entrarCartao(Jogo& jogo);
void atualizarCartao(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharCartao(const Jogo& jogo, Canvas& canvas);

void entrarTesteControle(Jogo& jogo);
void atualizarTesteControle(Jogo& jogo, const EstadoEntrada& entrada, float dt);
void desenharTesteControle(const Jogo& jogo, Canvas& canvas);
