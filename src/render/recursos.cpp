#include "render/recursos.h"

#include <filesystem>

#include "core/arquivo.h"

namespace {

void liberarFontes(Recursos& recursos) {
    liberarFonte(recursos.fontePequena);
    liberarFonte(recursos.fonteMedia);
    liberarFonte(recursos.fonteGrande);
}

}  // namespace

bool carregarRecursos(Recursos& recursos, const std::wstring& pastaHd, const std::wstring& pastaPadrao,
                      std::string& erro) {
    const std::wstring pasta = std::filesystem::exists(pastaHd + L"manifesto.txt") ? pastaHd : pastaPadrao;
    std::string texto;
    if (!lerArquivoTexto(pasta + L"manifesto.txt", texto)) {
        erro = "não encontrei assets/jogo_hd nem assets/jogo (manifesto.txt) ao lado do executável";
        return false;
    }
    if (!lerManifesto(texto, recursos.manifesto, erro)) {
        return false;
    }
    recursos.pasta = pasta;
    // cena_*: sai da lista residente (não carrega agora nem é preparado por escala); confere só se existe.
    std::vector<EntradaManifesto> residentes;
    for (const EntradaManifesto& entrada : recursos.manifesto) {
        if (entrada.nome.rfind("cena_", 0) != 0) {
            residentes.push_back(entrada);
            continue;
        }
        if (!std::filesystem::exists(pasta + paraWide(entrada.arquivo))) {
            erro = "falta " + entrada.arquivo;
            return false;
        }
        recursos.sobDemanda[entrada.nome] = entrada;
    }
    recursos.manifesto = std::move(residentes);
    for (const EntradaManifesto& entrada : recursos.manifesto) {
        Imagem imagem;
        if (!carregarPng(pasta + paraWide(entrada.arquivo), imagem)) {
            erro = "falha ao carregar " + entrada.arquivo;
            return false;
        }
        if (imagem.w != entrada.largura * entrada.fator || imagem.h != entrada.altura * entrada.fator) {
            erro = entrada.arquivo + ": tamanho no disco diferente de medida lógica × fator do manifesto";
            return false;
        }
        recursos.fontes[entrada.nome] = std::move(imagem);
    }
    return true;
}

bool prepararEscala(Recursos& recursos, int escala) {
    if (escala == recursos.escala) return true;
    recursos.imagens.clear();
    for (const EntradaManifesto& entrada : recursos.manifesto) {
        const Imagem& fonte = recursos.fontes.at(entrada.nome);
        // Reduz cada FRAME separadamente quando há grade: assim um frame nunca "vaza" pixels
        // para o vizinho na redução (importante em escalas não múltiplas, como 3 a partir de 4).
        const int fwFonte = entrada.frameLargura * entrada.fator, fhFonte = entrada.frameAltura * entrada.fator;
        const int fwDestino = entrada.frameLargura * escala, fhDestino = entrada.frameAltura * escala;
        Imagem destino;
        destino.w = entrada.largura * escala;
        destino.h = entrada.altura * escala;
        destino.px.assign(static_cast<size_t>(destino.w) * destino.h, 0u);
        for (int lin = 0; lin < entrada.linhas; ++lin) {
            for (int col = 0; col < entrada.colunas; ++col) {
                Imagem frame;
                frame.w = fwFonte;
                frame.h = fhFonte;
                frame.px.resize(static_cast<size_t>(fwFonte) * fhFonte);
                for (int y = 0; y < fhFonte; ++y) {
                    const std::uint32_t* origem = &fonte.px[static_cast<size_t>(lin * fhFonte + y) * fonte.w + col * fwFonte];
                    std::copy(origem, origem + fwFonte, &frame.px[static_cast<size_t>(y) * fwFonte]);
                }
                const Imagem reduzido = reamostrar(frame, fwDestino, fhDestino);
                for (int y = 0; y < fhDestino; ++y) {
                    std::copy(&reduzido.px[static_cast<size_t>(y) * fwDestino],
                              &reduzido.px[static_cast<size_t>(y) * fwDestino] + fwDestino,
                              &destino.px[static_cast<size_t>(lin * fhDestino + y) * destino.w + col * fwDestino]);
                }
            }
        }
        recursos.imagens[entrada.nome] = std::move(destino);
    }
    liberarFontes(recursos);
    const wchar_t* familia = L"Segoe UI";
    const bool ok = criarFonte(recursos.fontePequena, familia, 10, escala, false) &&
                    criarFonte(recursos.fonteMedia, familia, 12, escala, false) &&
                    criarFonte(recursos.fonteGrande, L"Georgia", 24, escala, true);
    recursos.escala = escala;
    return ok;
}

void liberarRecursos(Recursos& recursos) {
    liberarFontes(recursos);
    recursos.imagens.clear();
    recursos.fontes.clear();
    recursos.manifesto.clear();
    recursos.escala = 0;
}

const Imagem* buscarImagem(const Recursos& recursos, const std::string& nome) {
    const auto it = recursos.imagens.find(nome);
    return it == recursos.imagens.end() ? nullptr : &it->second;
}

Sprite spriteDe(const Recursos& recursos, const std::string& nome) {
    Sprite sprite;
    const EntradaManifesto* entrada = buscarManifesto(recursos.manifesto, nome);
    const Imagem* imagem = buscarImagem(recursos, nome);
    if (entrada == nullptr || imagem == nullptr) {
        return sprite;   // atlas nulo: spriteValido() == false, desenhar não faz nada
    }
    sprite.atlas = imagem;
    sprite.frameW = entrada->frameLargura * recursos.escala;
    sprite.frameH = entrada->frameAltura * recursos.escala;
    sprite.colunas = entrada->colunas;
    sprite.linhas = entrada->linhas;
    return sprite;
}

bool carregarSobDemanda(const Recursos& recursos, const std::string& nome, Imagem& saida) {
    const auto it = recursos.sobDemanda.find(nome);
    if (it == recursos.sobDemanda.end()) return false;
    if (!carregarPng(recursos.pasta + paraWide(it->second.arquivo), saida)) return false;
    return saida.w == it->second.largura * it->second.fator && saida.h == it->second.altura * it->second.fator;
}
