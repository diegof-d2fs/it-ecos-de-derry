// Testes do leitor e do validador de salas (SDD §8). O teste principal carrega TODAS as salas
// de data/salas: um erro de digitação num mapa quebra o build de testes, não a partida.
#include <cstdio>
#include <filesystem>

#include "mundo/sala.h"
#include "teste.h"

namespace {

const char* const SALA_MINIMA =
    "id=t1\n"
    "bioma=1\n"
    "tipo=teste\n"
    "mapa:\n"
    "#######\n"
    "#..E..L\n"
    "#.~w,.#\n"
    "#######\n";

}  // namespace

TESTE(salas_todas_as_salas_de_data_sao_validas) {
    std::vector<SalaModelo> salas;
    std::string erro;
    const bool ok = carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", salas, erro);
    if (!ok) std::printf("    %s\n", erro.c_str());
    VERIFICAR(ok);
    VERIFICAR(!salas.empty());
    for (const SalaModelo& s : salas) {
        const std::vector<std::string> problemas = validarSala(s);
        for (const std::string& p : problemas) std::printf("    %s: %s\n", s.id.c_str(), p.c_str());
        VERIFICAR(problemas.empty());
    }
}

TESTE(salas_leitura_basica) {
    SalaModelo s;
    std::string erro;
    VERIFICAR(lerSala(SALA_MINIMA, s, erro));
    VERIFICAR(s.cols == 7 && s.lins == 4);
    VERIFICAR(s.entradaJogador.col == 3 && s.entradaJogador.lin == 1);
    VERIFICAR(s.portas.size() == 1 && s.portas[0].lado == Direcao::Direita);
    VERIFICAR(s.ambiente == "galeria");   // padrão do bioma 1
    VERIFICAR(tileEm(s, 2, 2).tipo == TipoTile::AguaRasa && !tileEm(s, 2, 2).bloqueiaMovimento);
    VERIFICAR(tileEm(s, 3, 2).tipo == TipoTile::AguaFunda && tileEm(s, 3, 2).bloqueiaMovimento);
    VERIFICAR(tileEm(s, -1, 0).bloqueiaMovimento);   // fora do mapa = parede
}

TESTE(salas_erros_de_leitura_dizem_onde) {
    SalaModelo s;
    std::string erro;
    VERIFICAR(!lerSala("id=x\nmapa:\n####\n#.Z#\n####\n", s, erro));
    VERIFICAR(erro.find("coluna 3") != std::string::npos);
    VERIFICAR(!lerSala("id=x\nmapa:\n####\n#..\n####\n", s, erro));   // linha curta
    VERIFICAR(erro.find("largura") != std::string::npos);
    VERIFICAR(!lerSala("id=x\ncor=azul\nmapa:\n###\n", s, erro));
    VERIFICAR(erro.find("chave desconhecida") != std::string::npos);
    VERIFICAR(!lerSala("id=x\nmapa:\n#####\n#E.E#\n#####\n", s, erro));
    VERIFICAR(erro.find("mais de uma entrada") != std::string::npos);
}

TESTE(salas_validador_pega_os_defeitos) {
    SalaModelo s;
    std::string erro;
    // Borda aberta e objeto do outro lado de uma parede (inalcançável).
    VERIFICAR(lerSala("id=x\ntipo=teste\nmapa:\n######\n#E.#B#\n#...#.\n######\n", s, erro));
    const std::vector<std::string> p = validarSala(s);
    bool bordaAberta = false, inalcancavel = false;
    for (const std::string& m : p) {
        bordaAberta |= m.find("borda aberta") != std::string::npos;
        inalcancavel |= m.find("inalcançável") != std::string::npos;
    }
    VERIFICAR(bordaAberta);
    VERIFICAR(inalcancavel);
    // Sala de combate precisa de 4 spawns longe da entrada.
    VERIFICAR(lerSala("id=x\ntipo=combate\nmapa:\n#######\n#E.s..#\n#######\n", s, erro));
    const std::vector<std::string> p2 = validarSala(s);
    VERIFICAR(p2.size() >= 2);   // spawn perto + menos de 4 spawns
}

TESTE(salas_grade_e_detrito_contam_como_passaveis_na_validacao) {
    SalaModelo s;
    std::string erro;
    // A Chave do Medo atrás de uma grade (Elias abre) continua "alcançável".
    VERIFICAR(lerSala("id=x\ntipo=teste\nmapa:\n#######\n#E.g.K#\n#######\n", s, erro));
    VERIFICAR(validarSala(s).empty());
    // Atrás da porta trancada, não: a chave do medo não pode depender da própria porta.
    VERIFICAR(lerSala("id=x\ntipo=teste\nmapa:\n#######\n#E.T.K#\n#######\n", s, erro));
    VERIFICAR(!validarSala(s).empty());
}
