#include "jogo/recordes.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "core/arquivo.h"

namespace {

std::vector<std::string> separar(const std::string& linha, char sep) {
    std::vector<std::string> partes;
    std::string atual;
    for (char c : linha) {
        if (c == sep) {
            partes.push_back(atual);
            atual.clear();
        } else {
            atual += c;
        }
    }
    partes.push_back(atual);
    return partes;
}

bool lerLongo(const std::string& s, long long& v) {
    if (s.empty()) return false;
    char* fim = nullptr;
    v = std::strtoll(s.c_str(), &fim, 10);
    return *fim == '\0';
}

bool lerSemSinal(const std::string& s, std::uint64_t& v) {
    if (s.empty() || s[0] == '-') return false;
    char* fim = nullptr;
    v = std::strtoull(s.c_str(), &fim, 10);
    return *fim == '\0';
}

bool lerBool(const std::string& s, bool& v) {
    if (s != "0" && s != "1") return false;
    v = s == "1";
    return true;
}

bool lerLinha(const std::string& linha, Recorde& r) {
    const std::vector<std::string> c = separar(linha, ';');
    if (c.size() != 8) return false;
    long long tempo = 0, bioma = 0;
    if (!lerLongo(c[0], r.pontos) || r.pontos < 0) return false;
    r.data = c[1];
    if (!dificuldadeDoCodigo(c[2], r.dificuldade)) return false;
    if (!lerLongo(c[3], tempo) || tempo < 0) return false;
    if (!lerLongo(c[4], bioma) || bioma < 0 || bioma > 7) return false;
    r.tempo = static_cast<int>(tempo);
    r.bioma = static_cast<int>(bioma);
    return lerBool(c[5], r.vitoria) && lerSemSinal(c[6], r.seed) && lerBool(c[7], r.piloto);
}

}  // namespace

bool vemAntes(const Recorde& a, const Recorde& b) {
    if (a.pontos != b.pontos) return a.pontos > b.pontos;
    return a.tempo < b.tempo;
}

std::vector<Recorde> lerRecordes(const std::string& texto, int* ignoradas) {
    std::vector<Recorde> lista;
    int ruins = 0;
    std::istringstream entrada(texto);
    std::string linha;
    while (std::getline(entrada, linha)) {
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();
        if (linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);
        if (linha.empty() || linha[0] == '#') continue;
        Recorde r;
        if (lerLinha(linha, r)) lista.push_back(r);
        else ++ruins;
    }
    std::stable_sort(lista.begin(), lista.end(), vemAntes);
    if (ignoradas) *ignoradas = ruins;
    return lista;
}

std::string escreverRecordes(const std::vector<Recorde>& lista) {
    std::string s = "# pontos;data;dificuldade;tempo_s;bioma;vitoria;seed;piloto\n";
    for (const Recorde& r : lista) {
        s += std::to_string(r.pontos) + ';' + r.data + ';' + codigoDificuldade(r.dificuldade) + ';' +
             std::to_string(r.tempo) + ';' + std::to_string(r.bioma) + ';' + (r.vitoria ? "1" : "0") + ';' +
             std::to_string(r.seed) + ';' + (r.piloto ? "1" : "0") + '\n';
    }
    return s;
}

int inserirRecorde(std::vector<Recorde>& lista, const Recorde& novo) {
    // A data não pode ter ';' (quebraria a linha do arquivo).
    Recorde r = novo;
    std::replace(r.data.begin(), r.data.end(), ';', ',');
    std::stable_sort(lista.begin(), lista.end(), vemAntes);
    // Entra DEPOIS dos empates exatos: quem chegou antes ao mesmo placar fica na frente.
    const auto posicao = std::upper_bound(lista.begin(), lista.end(), r, vemAntes);
    const std::size_t indiceNovo = static_cast<std::size_t>(posicao - lista.begin());
    lista.insert(posicao, r);
    // Corta cada categoria em 10 e descobre a colocação do novo dentro da categoria dele.
    std::vector<Recorde> cortada;
    int porCategoria[2] = {0, 0};
    int colocacao = 0;
    for (std::size_t i = 0; i < lista.size(); ++i) {
        int& n = porCategoria[lista[i].piloto ? 1 : 0];
        if (n >= RECORDES_POR_CATEGORIA) continue;
        ++n;
        if (i == indiceNovo) colocacao = n;
        cortada.push_back(lista[i]);
    }
    lista = cortada;
    return colocacao;
}

int registrarRecordeDaTentativa(std::vector<Recorde>& lista, const Recorde& novo) {
    const auto mesmaTentativa = [&](const Recorde& r) {
        return r.seed == novo.seed && r.dificuldade == novo.dificuldade && r.piloto == novo.piloto && !r.vitoria;
    };
    const Recorde* melhorAnterior = nullptr;
    for (const Recorde& r : lista)
        if (mesmaTentativa(r) && (!melhorAnterior || r.pontos > melhorAnterior->pontos)) melhorAnterior = &r;
    const Recorde fica = (melhorAnterior && !novo.vitoria && melhorAnterior->pontos > novo.pontos) ? *melhorAnterior : novo;
    lista.erase(std::remove_if(lista.begin(), lista.end(), mesmaTentativa), lista.end());
    return inserirRecorde(lista, fica);
}

std::vector<Recorde> recordesFiltrados(const std::vector<Recorde>& lista, FiltroRecordes filtro) {
    std::vector<Recorde> saida;
    for (const Recorde& r : lista) {
        if (filtro == FiltroRecordes::Jogador && r.piloto) continue;
        if (filtro == FiltroRecordes::Computador && !r.piloto) continue;
        saida.push_back(r);
        if (static_cast<int>(saida.size()) == RECORDES_POR_CATEGORIA) break;
    }
    return saida;
}

std::vector<Recorde> carregarRecordes(const std::filesystem::path& caminho) {
    std::string texto;
    if (!lerArquivoTexto(caminho, texto)) return {};
    return lerRecordes(texto);
}

bool gravarRecordes(const std::filesystem::path& caminho, const std::vector<Recorde>& lista) {
    return gravarArquivoTexto(caminho, escreverRecordes(lista));
}
