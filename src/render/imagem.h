#pragma once
// Imagem em memória: pixels ARGB de 32 bits com alfa PRÉ-MULTIPLICADO.
// O GDI+ é usado apenas para decodificar o PNG no carregamento (lição do Bomberman:
// desenhar com Gdiplus::Graphics a cada quadro era o gargalo).
#include <string>

#include "core/pixels.h"   // struct Imagem e reamostrar (puros, testáveis)

// Inicia e encerra o GDI+ (uma vez no programa).
bool iniciarDecodificadorPng();
void encerrarDecodificadorPng();

// Carrega um PNG. Retorna false (e deixa `saida` vazia) se o arquivo não abrir.
bool carregarPng(const std::wstring& caminho, Imagem& saida);
