#pragma once
// Leitura de assets/jogo/manifesto.txt (gerado por tools/export_final.py).
// Cada linha: arquivo largura altura frame_largura frame_altura colunas linhas pivo [fator]
// As medidas são LÓGICAS (contrato do jogo). A imagem no disco tem medidas × fator
// (assets/jogo_hd usa fator 4; assets/jogo usa fator 1).
#include <string>
#include <vector>

struct EntradaManifesto {
    std::string arquivo;        // ex.: "bill_walk.png"
    std::string nome;           // ex.: "bill_walk" (sem extensão)
    int largura = 0, altura = 0;
    int frameLargura = 0, frameAltura = 0;
    int colunas = 1, linhas = 1;
    std::string pivo;           // "pes", "centro", "base" ou "-"
    int fator = 1;              // resolução da imagem no disco ÷ medida lógica
};

// Interpreta o texto inteiro do manifesto. Linhas vazias e iniciadas por '#' são ignoradas.
// Retorna false e descreve o problema em `erro` (com o número da linha) se algo estiver inválido.
bool lerManifesto(const std::string& texto, std::vector<EntradaManifesto>& saida, std::string& erro);

// Procura pelo nome sem extensão. Pode não existir: retorna nullptr (T03).
// O ponteiro só vale enquanto o vetor não for alterado.
const EntradaManifesto* buscarManifesto(const std::vector<EntradaManifesto>& entradas, const std::string& nome);
