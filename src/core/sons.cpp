#include "core/sons.h"

#include <cstdlib>
#include <cstring>
#include <sstream>

namespace {

const char* const NOMES[TOTAL_SONS] = {
    "golpe", "acerto_inimigo", "morte_inimigo", "bill_ferido", "bill_caiu", "kit", "moeda", "bau", "nivel",
    "armadilha", "troca_sala", "vinheta_derrota", "ambiente_masmorra",
    "musica_menu", "musica_jogo", "musica_tensao",
};

std::uint32_t lerU32(const std::vector<std::uint8_t>& b, std::size_t i) {
    return static_cast<std::uint32_t>(b[i]) | (static_cast<std::uint32_t>(b[i + 1]) << 8) |
           (static_cast<std::uint32_t>(b[i + 2]) << 16) | (static_cast<std::uint32_t>(b[i + 3]) << 24);
}
std::uint16_t lerU16(const std::vector<std::uint8_t>& b, std::size_t i) {
    return static_cast<std::uint16_t>(b[i] | (b[i + 1] << 8));
}

std::string aparar(const std::string& s) {
    const auto a = s.find_first_not_of(" \t\r");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r") - a + 1);
}

}  // namespace

const char* nomeSom(IdSom id) {
    const int i = static_cast<int>(id);
    return (i >= 0 && i < TOTAL_SONS) ? NOMES[i] : "?";
}

bool ehMusica(IdSom id) { return id >= IdSom::MusicaMenu && id < IdSom::Total; }

bool lerTabelaSons(const std::string& texto, TabelaSons& tabela, std::string& erro) {
    tabela = TabelaSons{};
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    while (std::getline(entrada, linha)) {
        ++numero;
        linha = aparar(linha);
        if (numero == 1 && linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);
        if (linha.empty() || linha[0] == '#') continue;
        const auto p1 = linha.find(';');
        const auto p2 = p1 == std::string::npos ? p1 : linha.find(';', p1 + 1);
        if (p1 == std::string::npos) {
            erro = "sons.txt linha " + std::to_string(numero) + ": esperado 'id;arquivo;volume'";
            return false;
        }
        const std::string id = aparar(linha.substr(0, p1));
        const std::string arquivo = aparar(linha.substr(p1 + 1, p2 == std::string::npos ? std::string::npos : p2 - p1 - 1));
        float volume = 1.0f;
        if (p2 != std::string::npos) volume = std::strtof(linha.c_str() + p2 + 1, nullptr);
        int achado = -1;
        for (int i = 0; i < TOTAL_SONS; ++i) {
            if (id == NOMES[i]) achado = i;
        }
        if (achado < 0) {
            erro = "sons.txt linha " + std::to_string(numero) + ": id desconhecido '" + id + "'";
            return false;
        }
        tabela[static_cast<std::size_t>(achado)] = {arquivo, volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume)};
    }
    return true;
}

bool lerWav(const std::vector<std::uint8_t>& b, SomPcm& som, std::string& erro) {
    som = SomPcm{};
    if (b.size() < 12 || std::memcmp(b.data(), "RIFF", 4) != 0 || std::memcmp(b.data() + 8, "WAVE", 4) != 0) {
        erro = "não é um WAV (RIFF/WAVE)";
        return false;
    }
    bool temFormato = false;
    std::size_t i = 12;
    while (i + 8 <= b.size()) {
        const std::uint32_t tamanho = lerU32(b, i + 4);
        const std::size_t corpo = i + 8;
        if (corpo + tamanho > b.size()) break;
        if (std::memcmp(b.data() + i, "fmt ", 4) == 0 && tamanho >= 16) {
            const std::uint16_t formato = lerU16(b, corpo), bits = lerU16(b, corpo + 14);
            som.canais = lerU16(b, corpo + 2);
            som.taxa = static_cast<int>(lerU32(b, corpo + 4));
            if ((formato != 1 && formato != 0xFFFE) || bits != 16 || som.canais < 1 || som.canais > 2) {
                erro = "WAV precisa ser PCM 16 bits mono ou estéreo";
                return false;
            }
            temFormato = true;
        } else if (std::memcmp(b.data() + i, "data", 4) == 0 && temFormato) {
            som.amostras.resize(tamanho / 2);
            std::memcpy(som.amostras.data(), b.data() + corpo, som.amostras.size() * 2);
            return true;
        }
        i = corpo + tamanho + (tamanho & 1u);   // blocos têm tamanho par
    }
    erro = temFormato ? "WAV sem bloco 'data'" : "WAV sem bloco 'fmt '";
    return false;
}
