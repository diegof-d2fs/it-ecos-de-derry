#pragma once
// Leitura e escrita de arquivos de texto (dados do jogo, manifesto, perfil).
#include <filesystem>
#include <string>

// Lê o arquivo inteiro. Retorna false se não abrir; `conteudo` fica vazio.
bool lerArquivoTexto(const std::filesystem::path& caminho, std::string& conteudo);

// Grava de forma atômica: escreve "<caminho>.tmp" e depois troca pelo definitivo,
// para que uma queda no meio da gravação não corrompa o arquivo anterior.
bool gravarArquivoTexto(const std::filesystem::path& caminho, const std::string& conteudo);
