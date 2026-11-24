// Testes da visÃ£o (SDD Â§7.5 e Â§19.1, critÃ©rio F11).
#include <filesystem>

#include "mundo/visao.h"
#include "teste.h"

namespace {

//   0123456789
// 0 ##########
// 1 #E...#...#      parede em (5,1)-(5,3) separa a sala em dois lados
// 2 #....#...#
// 3 #....#...#
// 4 #........#
// 5 ##########
SalaViva salaDeTeste(SalaModelo& m) {
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n##########\n#E...#...#\n#....#...#\n#....#...#\n#........#\n##########\n", m, erro);
    return criarSalaViva(m);
}

}  // namespace

TESTE(visao_sala_comeca_toda_desconhecida) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    MapaVisao v;
    iniciarVisao(v, sala);
    for (int l = 0; l < sala.lins; ++l)
        for (int c = 0; c < sala.cols; ++c) VERIFICAR(!visaoEm(v, c, l).explorada && visaoEm(v, c, l).luz == 0);
}

TESTE(visao_parede_bloqueia_e_ela_mesma_e_vista) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    MapaVisao v;
    iniciarVisao(v, sala);
    atualizarVisao(v, sala, centroCelula({2, 1}), RAIO_LANTERNA);
    VERIFICAR(visaoEm(v, 2, 1).luz == 255 || visaoEm(v, 2, 1).luz > 240);   // a prÃ³pria cÃ©lula
    VERIFICAR(visaoEm(v, 4, 2).luz > 0);                                     // mesmo lado
    VERIFICAR(visaoEm(v, 5, 1).explorada);                                   // a parede Ã© vista
    VERIFICAR(visaoEm(v, 6, 1).luz == 0 && !visaoEm(v, 6, 1).explorada);     // atrÃ¡s da parede
    VERIFICAR(estaVisivel(v, centroCelula({3, 3})));
    VERIFICAR(!estaVisivel(v, centroCelula({7, 2})));
}

TESTE(visao_luz_cai_com_a_distancia_e_respeita_o_raio) {
    SalaModelo m;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n################\n#E.............#\n################\n", m, erro);
    const SalaViva sala = criarSalaViva(m);
    MapaVisao v;
    iniciarVisao(v, sala);
    atualizarVisao(v, sala, centroCelula({1, 1}), 6.0f);
    VERIFICAR(visaoEm(v, 2, 1).luz > visaoEm(v, 5, 1).luz);
    VERIFICAR(visaoEm(v, 5, 1).luz > visaoEm(v, 7, 1).luz);
    VERIFICAR(visaoEm(v, 7, 1).luz > 0);       // dentro do raio (6 cÃ©lulas)
    VERIFICAR(visaoEm(v, 9, 1).luz == 0);      // fora do raio
    VERIFICAR(!visaoEm(v, 9, 1).explorada);
}

TESTE(visao_explorada_continua_explorada) {
    SalaModelo m;
    const SalaViva sala = salaDeTeste(m);
    MapaVisao v;
    iniciarVisao(v, sala);
    atualizarVisao(v, sala, centroCelula({1, 1}), RAIO_LANTERNA);
    VERIFICAR(visaoEm(v, 3, 2).explorada);
    // Bill dÃ¡ a volta pelo corredor de baixo e vai para o outro lado.
    atualizarVisao(v, sala, centroCelula({8, 1}), 2.0f);
    VERIFICAR(visaoEm(v, 3, 2).explorada);       // memÃ³ria
    VERIFICAR(visaoEm(v, 3, 2).luz == 0);        // mas nÃ£o estÃ¡ iluminada agora
    VERIFICAR(visaoEm(v, 7, 1).luz > 0);
    revelarTudo(v);
    VERIFICAR(visaoEm(v, 6, 3).explorada);
}

TESTE(visao_e_simetrica_em_todas_as_salas) {
    // Se A vÃª B, entÃ£o B vÃª A, para todo par de cÃ©lulas de chÃ£o de cada sala real.
    std::vector<SalaModelo> salas;
    std::string erro;
    VERIFICAR(carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", salas, erro));
    salas.push_back(SalaModelo{});
    salaDeTeste(salas.back());
    int assimetrias = 0, pares = 0;
    for (const SalaModelo& modelo : salas) {
        const SalaViva sala = criarSalaViva(modelo);
        std::vector<Celula> chao;
        for (int l = 0; l < sala.lins; ++l)
            for (int c = 0; c < sala.cols; ++c)
                if (!tileEm(sala, c, l).bloqueiaVisao) chao.push_back({c, l});
        // Amostra determinÃ­stica: cada cÃ©lula contra uma em cada 7 (cobre todas as distÃ¢ncias).
        for (std::size_t i = 0; i < chao.size(); ++i) {
            for (std::size_t j = i % 7; j < chao.size(); j += 7) {
                ++pares;
                if (linhaDeVisao(sala, chao[i], chao[j]) != linhaDeVisao(sala, chao[j], chao[i])) ++assimetrias;
            }
        }
    }
    if (assimetrias) std::printf("    %d assimetrias em %d pares\n", assimetrias, pares);
    VERIFICAR(pares > 1000);
    VERIFICAR(assimetrias == 0);
}
