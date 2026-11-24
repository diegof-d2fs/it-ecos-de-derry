#include "core/manifesto.h"

#include <sstream>

namespace {

bool validarEntrada(const EntradaManifesto& e, std::string& motivo) {
    if (e.largura <= 0 || e.altura <= 0 || e.frameLargura <= 0 || e.frameAltura <= 0) {
        motivo = "dimensões precisam ser positivas";
        return false;
    }
    if (e.colunas <= 0 || e.linhas <= 0) {
        motivo = "colunas e linhas precisam ser positivas";
        return false;
    }
    if (e.frameLargura * e.colunas != e.largura || e.frameAltura * e.linhas != e.altura) {
        motivo = "frame × grade não bate com o tamanho da imagem";
        return false;
    }
    return true;
}

}  // namespace

bool lerManifesto(const std::string& texto, std::vector<EntradaManifesto>& saida, std::string& erro) {
    saida.clear();
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    while (std::getline(entrada, linha)) {
        ++numero;
        if (!linha.empty() && linha.back() == '\r') {
            linha.pop_back();
        }
        if (linha.empty() || linha[0] == '#') {
            continue;
        }
        std::istringstream campos(linha);
        EntradaManifesto e;
        if (!(campos >> e.arquivo >> e.largura >> e.altura >> e.frameLargura >> e.frameAltura >> e.colunas >> e.linhas >>
              e.pivo)) {
            erro = "manifesto, linha " + std::to_string(numero) + ": esperados 8 campos";
            return false;
        }
        int fator = 1;
        if (campos >> fator) {
            if (fator < 1 || fator > 8) {
                erro = "manifesto, linha " + std::to_string(numero) + ": fator inválido";
                return false;
            }
            e.fator = fator;
        }
        std::string motivo;
        if (!validarEntrada(e, motivo)) {
            erro = "manifesto, linha " + std::to_string(numero) + " (" + e.arquivo + "): " + motivo;
            return false;
        }
        const auto ponto = e.arquivo.rfind('.');
        e.nome = ponto == std::string::npos ? e.arquivo : e.arquivo.substr(0, ponto);
        saida.push_back(e);
    }
    if (saida.empty()) {
        erro = "manifesto vazio";
        return false;
    }
    return true;
}

const EntradaManifesto* buscarManifesto(const std::vector<EntradaManifesto>& entradas, const std::string& nome) {
    for (const EntradaManifesto& e : entradas) {
        if (e.nome == nome) {
            return &e;
        }
    }
    return nullptr;
}
