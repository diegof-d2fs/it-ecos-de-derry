// Testes dos recordes (SDD §24.3; critério M04): ordem por pontos e tempo, 10 por categoria,
// filtro jogador/computador, ida e volta pelo arquivo e tolerância a linhas estragadas.
#include <filesystem>

#include "jogo/recordes.h"
#include "teste.h"

namespace {

Recorde rec(long long pontos, int tempo, bool piloto = false, std::uint64_t seed = 1) {
    Recorde r;
    r.pontos = pontos;
    r.tempo = tempo;
    r.piloto = piloto;
    r.seed = seed;
    r.data = "2026-11-16 10:00";
    return r;
}

}  // namespace

TESTE(recordes_ordem_por_pontos_e_depois_tempo) {
    std::vector<Recorde> l;
    VERIFICAR(inserirRecorde(l, rec(500, 100)) == 1);
    VERIFICAR(inserirRecorde(l, rec(900, 300)) == 1);
    VERIFICAR(inserirRecorde(l, rec(500, 50)) == 2);    // empate em pontos: o mais rápido na frente
    VERIFICAR(inserirRecorde(l, rec(500, 100, false, 7)) == 4);   // empate exato: quem chegou antes fica na frente
    VERIFICAR(l.size() == 4 && l[0].pontos == 900 && l[1].tempo == 50 && l[2].seed == 1 && l[3].seed == 7);
}

TESTE(recordes_dez_por_categoria_e_filtro) {
    std::vector<Recorde> l;
    for (int i = 0; i < 15; ++i) inserirRecorde(l, rec(1000 + i * 10, 60, true));   // o piloto joga muito
    for (int i = 0; i < 12; ++i) inserirRecorde(l, rec(100 + i, 60, false));
    VERIFICAR(l.size() == 20);
    // O filtro "Jogador" continua com 10 linhas, mesmo com o piloto à frente em pontos.
    const std::vector<Recorde> jog = recordesFiltrados(l, FiltroRecordes::Jogador);
    VERIFICAR(jog.size() == 10 && !jog[0].piloto && jog[0].pontos == 111 && jog[9].pontos == 102);
    const std::vector<Recorde> todas = recordesFiltrados(l, FiltroRecordes::Todas);
    VERIFICAR(todas.size() == 10 && todas[0].pontos == 1140 && todas[0].piloto);
    VERIFICAR(recordesFiltrados(l, FiltroRecordes::Computador).size() == 10);
    // Abaixo do 10º da categoria: não entra (colocação 0) e a lista não muda.
    VERIFICAR(inserirRecorde(l, rec(50, 60, false)) == 0 && l.size() == 20);
    VERIFICAR(inserirRecorde(l, rec(105, 10, false)) == 7);   // 111…106, e o 105 mais rápido
}

TESTE(recordes_ida_e_volta_pelo_texto) {
    std::vector<Recorde> l;
    Recorde r = rec(56100, 3000, true, 18446744073709551615ULL);
    r.dificuldade = Dificuldade::Dificil;
    r.bioma = 7;
    r.vitoria = true;
    r.data = "2026-11-16 21:30;x";   // ';' na data seria um campo a mais: vira ','
    inserirRecorde(l, r);
    inserirRecorde(l, rec(3220, 900));
    int ruins = -1;
    const std::vector<Recorde> lidos = lerRecordes(escreverRecordes(l), &ruins);
    VERIFICAR(ruins == 0 && lidos.size() == 2);
    VERIFICAR(lidos[0].pontos == 56100 && lidos[0].dificuldade == Dificuldade::Dificil && lidos[0].bioma == 7);
    VERIFICAR(lidos[0].vitoria && lidos[0].piloto && lidos[0].seed == 18446744073709551615ULL);
    VERIFICAR(lidos[0].data == "2026-11-16 21:30,x" && lidos[1].tempo == 900 && !lidos[1].piloto);
}

TESTE(recordes_linhas_estragadas_sao_ignoradas) {
    const std::string texto =
        "# comentário\n"
        "100;d;normal;10;1;0;5;0\r\n"
        "abc;d;normal;10;1;0;5;0\n"       // pontos inválidos
        "100;d;impossivel;10;1;0;5;0\n"   // dificuldade desconhecida
        "100;d;normal;10;9;0;5;0\n"       // bioma fora de 0..7
        "100;d;normal;10;1;0;5\n"         // falta um campo
        "\n"
        "300;d;facil;10;1;1;5;1\n";
    int ruins = 0;
    const std::vector<Recorde> l = lerRecordes(texto, &ruins);
    VERIFICAR(ruins == 4 && l.size() == 2 && l[0].pontos == 300 && l[0].dificuldade == Dificuldade::Facil);
}

TESTE(recordes_arquivo_ausente_e_gravacao) {
    const std::filesystem::path pasta = std::filesystem::temp_directory_path() / "ecos_teste_recordes";
    std::filesystem::remove_all(pasta);
    VERIFICAR(carregarRecordes(pasta / "recordes.txt").empty());
    std::vector<Recorde> l;
    inserirRecorde(l, rec(42, 7));
    VERIFICAR(gravarRecordes(pasta / "recordes.txt", l));   // cria a pasta
    const std::vector<Recorde> lidos = carregarRecordes(pasta / "recordes.txt");
    VERIFICAR(lidos.size() == 1 && lidos[0].pontos == 42);
    VERIFICAR(!std::filesystem::exists(pasta / "recordes.txt.tmp"));
    std::filesystem::remove_all(pasta);
}

// Diego, 24/11: o recorde é gravado a cada chefão derrotado. A MESMA tentativa ocupa uma linha só, que sobe a cada
// chefe; uma queda depois (morte com menos pontos) não apaga o que ela já tinha alcançado; a vitória substitui.
TESTE(recordes_uma_linha_por_tentativa_atualizada_a_cada_chefe) {
    std::vector<Recorde> l;
    registrarRecordeDaTentativa(l, rec(800, 100, false, 42));   // chefe 1
    registrarRecordeDaTentativa(l, rec(1500, 200, false, 42));  // chefe 2: a mesma linha sobe
    VERIFICAR(l.size() == 1);
    VERIFICAR(l[0].pontos == 1500);
    registrarRecordeDaTentativa(l, rec(1400, 260, false, 42));  // morreu depois, com menos: fica o melhor
    VERIFICAR(l.size() == 1);
    VERIFICAR(l[0].pontos == 1500);
    Recorde vitoria = rec(1300, 300, false, 42);
    vitoria.vitoria = true;
    registrarRecordeDaTentativa(l, vitoria);                    // vitória sempre substitui
    VERIFICAR(l.size() == 1);
    VERIFICAR(l[0].vitoria);
    // Outra tentativa (outra seed) e a mesma seed jogada pelo computador são linhas diferentes.
    registrarRecordeDaTentativa(l, rec(900, 100, false, 43));
    registrarRecordeDaTentativa(l, rec(900, 100, true, 43));
    VERIFICAR(l.size() == 3);
    // Uma vitória já gravada não é tocada por outra tentativa com a mesma seed.
    registrarRecordeDaTentativa(l, rec(2000, 100, false, 42));
    VERIFICAR(l.size() == 4);
}
