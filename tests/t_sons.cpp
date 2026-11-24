// Testes da parte pura do áudio (core/sons): leitura de WAV e da tabela data/sons.txt.
#include <filesystem>

#include "core/arquivo.h"
#include "core/sons.h"
#include "teste.h"

namespace {

void poeU32(std::vector<std::uint8_t>& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}
void poeU16(std::vector<std::uint8_t>& b, std::uint16_t v) {
    b.push_back(static_cast<std::uint8_t>(v));
    b.push_back(static_cast<std::uint8_t>(v >> 8));
}
void poeTexto(std::vector<std::uint8_t>& b, const char* s) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(s[i]));
}

// WAV mínimo: canais, bits, um bloco extra antes do 'data' (como o "LIST" de editores).
std::vector<std::uint8_t> wavSintetico(std::uint16_t canais, std::uint16_t bits, const std::vector<std::int16_t>& amostras) {
    std::vector<std::uint8_t> b;
    poeTexto(b, "RIFF");
    poeU32(b, 0);   // tamanho total (o leitor não depende dele)
    poeTexto(b, "WAVE");
    poeTexto(b, "fmt ");
    poeU32(b, 16);
    poeU16(b, 1);
    poeU16(b, canais);
    poeU32(b, 44100);
    poeU32(b, 44100u * canais * 2u);
    poeU16(b, static_cast<std::uint16_t>(canais * 2));
    poeU16(b, bits);
    poeTexto(b, "LIST");
    poeU32(b, 3);
    b.push_back(1); b.push_back(2); b.push_back(3); b.push_back(0);   // ímpar + preenchimento
    poeTexto(b, "data");
    poeU32(b, static_cast<std::uint32_t>(amostras.size() * 2));
    for (std::int16_t a : amostras) poeU16(b, static_cast<std::uint16_t>(a));
    return b;
}

}  // namespace

TESTE(sons_le_wav_mono_e_estereo) {
    SomPcm s;
    std::string erro;
    VERIFICAR(lerWav(wavSintetico(1, 16, {0, 1000, -1000, 32767}), s, erro));
    VERIFICAR(s.canais == 1 && s.taxa == 44100 && s.amostras.size() == 4);
    VERIFICAR(s.amostras[2] == -1000 && s.amostras[3] == 32767);
    VERIFICAR(lerWav(wavSintetico(2, 16, {1, 2, 3, 4}), s, erro));
    VERIFICAR(s.canais == 2 && s.amostras.size() == 4);
}

TESTE(sons_recusa_wav_invalido) {
    SomPcm s;
    std::string erro;
    VERIFICAR(!lerWav(wavSintetico(1, 8, {0, 0}), s, erro));   // 8 bits: não suportado
    VERIFICAR(!lerWav({'R', 'I', 'F', 'F'}, s, erro));
    VERIFICAR(!lerWav(std::vector<std::uint8_t>(64, 0), s, erro));
}

TESTE(sons_tabela_le_ids_e_volumes) {
    TabelaSons t;
    std::string erro;
    VERIFICAR(lerTabelaSons("# comentário\ngolpe;sfx/a.wav;0.5\nmusica_jogo;m.mp3\n", t, erro));
    VERIFICAR(t[static_cast<int>(IdSom::Golpe)].arquivo == "sfx/a.wav");
    VERIFICAR_PERTO(t[static_cast<int>(IdSom::Golpe)].volume, 0.5f, 1e-6f);
    VERIFICAR_PERTO(t[static_cast<int>(IdSom::MusicaJogo)].volume, 1.0f, 1e-6f);
    VERIFICAR(t[static_cast<int>(IdSom::Kit)].arquivo.empty());   // ausente = mudo, sem erro
    VERIFICAR(!lerTabelaSons("trovao;x.wav;1\n", t, erro));        // id desconhecido
    VERIFICAR(erro.find("trovao") != std::string::npos);
}

TESTE(sons_tabela_real_aponta_para_arquivos_validos) {
    const std::filesystem::path raiz(ECOS_PASTA_DADOS);
    std::string texto, erro;
    VERIFICAR(lerArquivoTexto(raiz / "data/sons.txt", texto));
    TabelaSons t;
    VERIFICAR(lerTabelaSons(texto, t, erro));
    for (int i = 0; i < TOTAL_SONS; ++i) {
        const DefinicaoSom& d = t[static_cast<std::size_t>(i)];
        if (d.arquivo.empty()) continue;
        const std::filesystem::path arq = raiz / "assets" / std::filesystem::u8path(d.arquivo);
        const bool existe = std::filesystem::exists(arq);
        if (!existe) std::printf("    falta: %s\n", d.arquivo.c_str());
        VERIFICAR(existe);
        if (!existe || ehMusica(static_cast<IdSom>(i))) continue;
        std::string bytes;
        lerArquivoTexto(arq, bytes);
        SomPcm s;
        const bool ok = lerWav(std::vector<std::uint8_t>(bytes.begin(), bytes.end()), s, erro);
        if (!ok) std::printf("    %s: %s\n", d.arquivo.c_str(), erro.c_str());
        VERIFICAR(ok && s.taxa == 44100);
    }
}
