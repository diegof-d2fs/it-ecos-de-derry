#pragma once
// Sons do jogo (SDD §24.8). Parte PURA: IDs, leitura de WAV e da tabela data/sons.txt.
// As regras nunca tocam som: pedem por ID (eventos), e a plataforma toca.
#include <array>
#include <cstdint>
#include <string>
#include <vector>

enum class IdSom : std::uint8_t {
    // efeitos (WAV, mixer)
    Golpe, AcertoInimigo, MorteInimigo, BillFerido, BillCaiu, Kit, Moeda, Bau, Nivel, Armadilha,
    TrocaSala, VinhetaDerrota, AmbienteMasmorra,
    // músicas (MP3 em streaming)
    MusicaMenu, MusicaJogo, MusicaTensao,
    Total
};
constexpr int TOTAL_SONS = static_cast<int>(IdSom::Total);

// Nome do ID usado em data/sons.txt (ex.: "golpe").
const char* nomeSom(IdSom id);
bool ehMusica(IdSom id);

struct DefinicaoSom {
    std::string arquivo;     // relativo à pasta assets/ (ex.: "musics/sword-swing.wav")
    float volume = 1.0f;     // 0..1
};
using TabelaSons = std::array<DefinicaoSom, TOTAL_SONS>;

// Lê data/sons.txt ("id;arquivo;volume", '#' = comentário). IDs ausentes ficam sem arquivo
// (o jogo fica mudo naquele evento, sem erro). Devolve false só se uma linha for inválida.
bool lerTabelaSons(const std::string& texto, TabelaSons& tabela, std::string& erro);

// Áudio PCM de 16 bits já decodificado.
struct SomPcm {
    int canais = 0;               // 1 ou 2
    int taxa = 0;                 // amostras por segundo (o mixer usa 44100)
    std::vector<std::int16_t> amostras;   // intercaladas (L R L R… no estéreo)
};

// Decodifica um arquivo WAV PCM 16 bits (mono ou estéreo) a partir dos bytes. Pura e testável.
bool lerWav(const std::vector<std::uint8_t>& bytes, SomPcm& som, std::string& erro);
