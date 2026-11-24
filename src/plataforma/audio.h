#pragma once
// Áudio (SDD §24.8). Nunca é obrigatório: sem placa de som ou sem arquivos, o jogo segue mudo.
//   Efeitos e ambiente: WAV na memória, mixer próprio com waveOut (8 vozes + 1 ambiente em loop),
//                      numa thread acordada pelo próprio waveOut (não depende do ritmo dos quadros).
//   Músicas: MP3 em streaming pelo MCI do Windows. A música de jogo e a de tensão tocam juntas
//            e o volume passa de uma para a outra (crossfade) conforme o nível de tensão.
#include <string>

#include "core/sons.h"

// `pastaAssets` termina com barra. `avisos` lista arquivos que faltaram (não é erro fatal).
void iniciarAudio(const std::wstring& pastaAssets, const TabelaSons& tabela, std::string& avisos);
void encerrarAudio();

void tocarEfeito(IdSom id, float volume = 1.0f);
void ligarAmbiente(IdSom id);   // IdSom::Total desliga
// Música de fundo: MusicaMenu ou MusicaJogo (que traz junto a de tensão); IdSom::Total = silêncio.
void definirMusica(IdSom musica);
// 0 = só a música de jogo; 1 = só a de tensão. O volume muda aos poucos (~1,5 s).
void definirTensao(float alvo);
// Uma vez por quadro: aplica o crossfade e o mudo por falta de foco.
void atualizarAudio(float dt, bool janelaFocada);
void definirVolumes(float geral, float musica, float efeitos);   // 0..1
// Diagnóstico (testes automáticos): dispositivo, efeitos carregados e estado real das músicas no MCI.
std::string diagnosticoAudio();
