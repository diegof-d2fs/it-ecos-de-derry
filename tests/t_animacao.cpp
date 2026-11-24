// Etapa 9b — personagens vivos (pedido do Diego, 17/11/2026): todo personagem que ANDA tem folha de
// andar em 4 direções × 4 quadros, como o Bill. Antes, os amigos tinham só 2 quadros de frente e
// "deslizavam" olhando para a câmera. Este teste barra personagem novo que nasça só com o _idle.
#include <filesystem>
#include <string>
#include <vector>

#include "core/arquivo.h"
#include "core/manifesto.h"
#include "entidades/inimigos.h"
#include "jogo/companheiro.h"
#include "teste.h"

namespace {
std::vector<EntradaManifesto> manifestoDoJogo() {
    std::string texto, erro;
    std::vector<EntradaManifesto> m;
    lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / "assets/jogo_hd/manifesto.txt", texto);
    lerManifesto(texto, m, erro);
    return m;
}

bool folhaDe4Direcoes(const std::vector<EntradaManifesto>& m, const std::string& nome) {
    const EntradaManifesto* e = buscarManifesto(m, nome);
    return e && e->colunas == 4 && e->linhas == 4 && e->pivo == "pes";
}
}  // namespace

TESTE(animacao_todo_personagem_que_anda_tem_folha_de_4_direcoes) {
    const std::vector<EntradaManifesto> m = manifestoDoJogo();
    VERIFICAR(!m.empty());
    VERIFICAR(folhaDe4Direcoes(m, "bill_walk"));
    // Os amigos que acompanham Bill (companheiro) e os NPCs que andam.
    for (int a = 0; a < static_cast<int>(Amigo::Total); ++a) {
        const std::string nome = std::string(codigoAmigo(static_cast<Amigo>(a))) + "_walk";
        VERIFICAR(folhaDe4Direcoes(m, nome));
    }
    for (const char* npc : {"elias_walk", "georgie_walk"}) VERIFICAR(folhaDe4Direcoes(m, npc));
    // Os chefes que andam: o medo (fase 1) e o Pennywise do estágio (fase 2). O Ralo é fixo.
    for (int t = 0; t < TOTAL_TIPOS_INIMIGO; ++t) {
        const ParametrosInimigo& p = parametrosDe(static_cast<TipoInimigo>(t));
        if (!p.chefe || static_cast<TipoInimigo>(t) == TipoInimigo::RaloSangue) continue;
        if (static_cast<TipoInimigo>(t) == TipoInimigo::PennywiseIV) {   // o palhaço-aranha GIRA em 360° (24/11)
            const EntradaManifesto* e = buscarManifesto(m, "pennywise_4_aranha");
            VERIFICAR(e && e->colunas == 4 && e->linhas == 2);
            continue;
        }
        VERIFICAR(folhaDe4Direcoes(m, std::string(p.atlas) + "_walk"));
    }
    for (const char* pw : {"pennywise_1_walk", "pennywise_2_walk", "pennywise_3_walk"})
        VERIFICAR(folhaDe4Direcoes(m, pw));
    // Inimigos comuns: a própria folha já é por direção (linhas = baixo, esquerda, direita, cima).
    for (int t = 0; t < TOTAL_TIPOS_INIMIGO; ++t) {
        const ParametrosInimigo& p = parametrosDe(static_cast<TipoInimigo>(t));
        if (p.chefe || p.velocidade <= 0.0f) continue;   // o Cabelo do ralo é fixo
        const EntradaManifesto* e = buscarManifesto(m, p.atlas);
        VERIFICAR(e && e->linhas == 4);
    }
}
