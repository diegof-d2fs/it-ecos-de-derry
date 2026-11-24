#pragma once
// Armadilhas de terreno (PRD §14.2, critério F14 do enunciado: "armadilha atinge Bill e inimigo").
//
// Uma armadilha dispara um Golpe de TERRENO (Golpe::terreno): ele é julgado pelos PÉS (quem está na
// célula, não quem tem o corpo por cima dela), não erra, não rola esquiva nem crítico, atinge Bill e
// inimigos terrestres, e nunca voadores, chefes ou NPCs. Assim "a mesma função de dano de terreno
// atende a qualquer criatura elegível" e um ator não sofre o mesmo disparo duas vezes (atingidos).
//
//   Pregos / espinheiro '^': 6 de dano ao pisar, no máximo 1× por segundo por célula.
//   Piso cedendo 'X': ao ser pisado abre um AVISO de 0,5 s (rachaduras); fere 8 quem continua em
//     cima e vira buraco (bloqueia). Quem fica em cima do buraco é posto na célula livre mais perto.
//   opcional=<col> <lin> na sala: a armadilha só fica ativa pela chance da dificuldade (SDD §24.2).
#include "jogo/partida.h"

constexpr int DANO_ESPINHOS = 6;
constexpr float RECARGA_ESPINHOS = 1.0f;
constexpr int DANO_PISO_CEDENDO = 8;
constexpr float AVISO_PISO_CEDENDO = 0.5f;
constexpr float QUEDA_DEPOIS_DO_AVISO = 0.12f;   // o golpe acontece ANTES de o piso virar buraco
// Jato da tubulação 'J' (Etapa 12.1): gotejamento (aviso 0,8 s) → jato numa linha de 3 células para
// LONGE da parede onde o cano está → recarga; ciclo de 3 s com fase própria por cano.
constexpr int DANO_JATO = 10;
constexpr float AVISO_JATO = 0.8f;
constexpr float PERIODO_JATO = 3.0f;
constexpr float ALCANCE_JATO = 3.0f * TILE;
// Poça de sangue 'b': 4 de dano por segundo a quem está nela (Bill e inimigos terrestres).
constexpr int DANO_POCA_SANGUE = 4;
constexpr float RECARGA_POCA_SANGUE = 1.0f;
// Grelha de fogo 'f' (Etapa 13.1, PRD §14.2): em ciclo, brasas (aviso 0,8 s) → chamas por 1 s
// (12 de dano a quem estiver ou entrar na célula, uma vez por acesa) → apagada. Fase própria por célula.
constexpr int DANO_GRELHA = 12;
constexpr float AVISO_GRELHA = 0.8f;
constexpr float CHAMAS_GRELHA = 1.0f;
constexpr float PERIODO_GRELHA = 3.6f;
// Labareda (Etapa 13.3): o fogo que a chuva de brasas do Pássaro acende no piso por alguns segundos.
// Fere como as brasas do Espectro (6 por segundo) a quem está nela: Bill e inimigos terrestres.
constexpr int DANO_LABAREDA = 6;
constexpr float RECARGA_LABAREDA = 1.0f;
enum class EstadoGrelha : std::uint8_t { Apagada, Brasas, Chamas };
EstadoGrelha estadoDaGrelha(const EstadoPartida& p, Celula c);
// Esteira '>' '<' 'v' 'A': empurra Bill (fora do dash) e os inimigos terrestres que não são fixos
// (VELOCIDADE_ESTEIRA e direcaoDaEsteira moram em mundo/tile.h: a IA dos inimigos também usa).
// Para onde o jato da célula `c` dispara (longe da parede vizinha; sem parede, para baixo).
Vec2 direcaoDoJato(const SalaViva& sala, Celula c);

bool ehArmadilha(TipoTile t);
// Ao entrar na sala: as opcionais ficam ou viram chão (dificuldade); recargas zeradas.
void prepararArmadilhas(EstadoPartida& p);
// A cada passo: quem pisa dispara, os pisos rachando caem, ninguém fica dentro de um buraco.
void atualizarArmadilhas(EstadoPartida& p, float dt);
