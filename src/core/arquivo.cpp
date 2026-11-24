#include "core/arquivo.h"

#include <fstream>
#include <sstream>
#include <system_error>

bool lerArquivoTexto(const std::filesystem::path& caminho, std::string& conteudo) {
    conteudo.clear();
    std::ifstream arquivo(caminho, std::ios::binary);
    if (!arquivo) {
        return false;
    }
    std::ostringstream buffer;
    buffer << arquivo.rdbuf();
    conteudo = buffer.str();
    // Remove BOM UTF-8, se houver.
    if (conteudo.size() >= 3 && static_cast<unsigned char>(conteudo[0]) == 0xEF &&
        static_cast<unsigned char>(conteudo[1]) == 0xBB && static_cast<unsigned char>(conteudo[2]) == 0xBF) {
        conteudo.erase(0, 3);
    }
    return true;
}

bool gravarArquivoTexto(const std::filesystem::path& caminho, const std::string& conteudo) {
    std::error_code erro;
    if (caminho.has_parent_path()) {
        std::filesystem::create_directories(caminho.parent_path(), erro);
    }
    std::filesystem::path temporario = caminho;
    temporario += ".tmp";
    {
        std::ofstream arquivo(temporario, std::ios::binary | std::ios::trunc);
        if (!arquivo) {
            return false;
        }
        arquivo << conteudo;
        if (!arquivo) {
            return false;
        }
    }
    std::filesystem::rename(temporario, caminho, erro);
    return !erro;
}
