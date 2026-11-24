#include "plataforma/audio.h"

#include <windows.h>
#include <mmsystem.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <vector>

#include "core/arquivo.h"

namespace {

constexpr int TAXA = 44100;
constexpr int QUADROS_POR_BUFFER = 1024;   // ~23 ms
constexpr int TOTAL_BUFFERS = 4;           // ~93 ms de fila: folga para travadas curtas
constexpr int TOTAL_VOZES = 8;
constexpr float VELOCIDADE_CROSSFADE = 1.0f / 1.5f;   // tensão vai de 0 a 1 em 1,5 s
// Troca de música (24/11, Diego: "a transição entre músicas está muito abrupta"): nada corta. A que sai desce em
// DURACAO_SAIDA enquanto a que entra sobe em DURACAO_ENTRADA, ao mesmo tempo (crossfade); o ambiente também.
constexpr float DURACAO_ENTRADA = 2.0f;
constexpr float DURACAO_SAIDA = 1.6f;
constexpr float DURACAO_AMBIENTE = 1.0f;   // o ambiente desce e o novo sobe

struct Voz {
    const SomPcm* som = nullptr;   // nullptr = livre
    std::size_t quadro = 0;        // posição em quadros (1 quadro = 1 amostra por canal)
    float volume = 1.0f;
    bool loop = false;
};

struct EstadoAudio {
    TabelaSons tabela;
    std::array<SomPcm, TOTAL_SONS> sons;
    std::wstring pasta;
    // waveOut
    HWAVEOUT dispositivo = nullptr;
    HANDLE evento = nullptr;
    HANDLE thread = nullptr;
    volatile LONG parar = 0;
    std::array<WAVEHDR, TOTAL_BUFFERS> cabecalhos{};
    std::array<std::vector<std::int16_t>, TOTAL_BUFFERS> buffers;
    CRITICAL_SECTION trava;
    bool travaCriada = false;
    // estado do mixer (protegido por `trava`)
    std::array<Voz, TOTAL_VOZES> vozes{};
    Voz ambiente;
    const SomPcm* ambienteDesejado = nullptr;   // o ambiente troca aos poucos: desce o atual, sobe este
    float volumeAmbienteDesejado = 1.0f;
    float ganhoAmbiente = 0.0f;
    float volGeral = 1.0f, volMusica = 1.0f, volEfeitos = 1.0f;
    bool mudo = false;
    // músicas (MCI; só a thread principal mexe). Dois LUGARES (A e B), cada um com o canal base e o de tensão:
    // a música que sai desce num lugar enquanto a que entra sobe no outro.
    struct Lugar {
        IdSom musica = IdSom::Total;
        float fade = 0.0f, alvo = 0.0f;     // 0..1: volume de entrada/saída
        float tensao = 0.0f;                // a tensão congelada de quem está saindo
        int volumeAplicado[2] = {-1, -1};   // último volume MCI enviado (base, tensão)
    };
    std::array<Lugar, 2> lugares;
    int atual = 0;                       // o lugar da música que está (ou vai ficar) tocando
    IdSom musica = IdSom::Total;         // a música pedida
    float tensao = 0.0f, alvoTensao = 0.0f;
};
EstadoAudio a;

// ---------- mixer ----------
void misturar(std::int16_t* saida) {
    std::array<std::int32_t, QUADROS_POR_BUFFER * 2> soma{};
    EnterCriticalSection(&a.trava);
    const float mestre = a.mudo ? 0.0f : a.volGeral * a.volEfeitos;
    auto tocar = [&](Voz& v) {
        if (!v.som) return;
        const SomPcm& s = *v.som;
        const std::size_t total = s.amostras.size() / static_cast<std::size_t>(s.canais);
        const auto ganho = static_cast<std::int32_t>(v.volume * mestre * 256.0f);
        for (int q = 0; q < QUADROS_POR_BUFFER; ++q) {
            if (v.quadro >= total) {
                if (!v.loop || total == 0) { v.som = nullptr; return; }
                v.quadro = 0;
            }
            const std::int16_t* p = &s.amostras[v.quadro * static_cast<std::size_t>(s.canais)];
            const std::int32_t esq = p[0], dir = s.canais == 2 ? p[1] : p[0];
            soma[static_cast<std::size_t>(q) * 2] += (esq * ganho) >> 8;
            soma[static_cast<std::size_t>(q) * 2 + 1] += (dir * ganho) >> 8;
            ++v.quadro;
        }
    };
    for (Voz& v : a.vozes) tocar(v);
    // Ambiente: troca aos poucos (desce o que toca, troca, sobe o novo); nunca corta.
    const float passoAmb = static_cast<float>(QUADROS_POR_BUFFER) / static_cast<float>(TAXA) / DURACAO_AMBIENTE;
    if (a.ambiente.som != a.ambienteDesejado) {
        a.ganhoAmbiente -= passoAmb;
        if (a.ganhoAmbiente <= 0.0f || !a.ambiente.som) {
            a.ganhoAmbiente = 0.0f;
            a.ambiente.som = a.ambienteDesejado;
            a.ambiente.volume = a.volumeAmbienteDesejado;
            a.ambiente.quadro = 0;
            a.ambiente.loop = true;
        }
    } else if (a.ganhoAmbiente < 1.0f) {
        a.ganhoAmbiente = std::fmin(1.0f, a.ganhoAmbiente + passoAmb);
    }
    if (a.ambiente.som) {
        Voz amb = a.ambiente;
        amb.volume = a.ambiente.volume * a.ganhoAmbiente;
        tocar(amb);
        a.ambiente.quadro = amb.quadro;
    }
    LeaveCriticalSection(&a.trava);
    for (std::size_t i = 0; i < soma.size(); ++i) {
        const std::int32_t v = soma[i];
        saida[i] = static_cast<std::int16_t>(v > 32767 ? 32767 : (v < -32768 ? -32768 : v));
    }
}

DWORD WINAPI threadMixer(LPVOID) {
    while (!a.parar) {
        WaitForSingleObject(a.evento, 50);
        for (WAVEHDR& h : a.cabecalhos) {
            if (a.parar) break;
            if (!(h.dwFlags & WHDR_DONE)) continue;
            misturar(reinterpret_cast<std::int16_t*>(h.lpData));
            h.dwFlags &= ~static_cast<DWORD>(WHDR_DONE);
            waveOutWrite(a.dispositivo, &h, sizeof(WAVEHDR));
        }
    }
    return 0;
}

bool abrirDispositivo() {
    WAVEFORMATEX f{};
    f.wFormatTag = WAVE_FORMAT_PCM;
    f.nChannels = 2;
    f.nSamplesPerSec = TAXA;
    f.wBitsPerSample = 16;
    f.nBlockAlign = 4;
    f.nAvgBytesPerSec = TAXA * 4;
    a.evento = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!a.evento) return false;
    if (waveOutOpen(&a.dispositivo, WAVE_MAPPER, &f, reinterpret_cast<DWORD_PTR>(a.evento), 0, CALLBACK_EVENT) !=
        MMSYSERR_NOERROR) {
        a.dispositivo = nullptr;
        return false;
    }
    for (int i = 0; i < TOTAL_BUFFERS; ++i) {
        a.buffers[static_cast<std::size_t>(i)].assign(QUADROS_POR_BUFFER * 2, 0);
        WAVEHDR& h = a.cabecalhos[static_cast<std::size_t>(i)];
        h = WAVEHDR{};
        h.lpData = reinterpret_cast<LPSTR>(a.buffers[static_cast<std::size_t>(i)].data());
        h.dwBufferLength = QUADROS_POR_BUFFER * 4;
        waveOutPrepareHeader(a.dispositivo, &h, sizeof(WAVEHDR));
        waveOutWrite(a.dispositivo, &h, sizeof(WAVEHDR));   // começa com silêncio
    }
    a.thread = CreateThread(nullptr, 0, threadMixer, nullptr, 0, nullptr);
    if (a.thread) SetThreadPriority(a.thread, THREAD_PRIORITY_ABOVE_NORMAL);
    return a.thread != nullptr;
}

// ---------- músicas (MCI) ----------
std::wstring aliasDe(int lugar, int canal) {
    return std::wstring(L"ecos_m") + (lugar == 0 ? L"a" : L"b") + (canal == 0 ? L"_base" : L"_tensao");
}

void mci(const std::wstring& comando) { mciSendStringW(comando.c_str(), nullptr, 0, nullptr); }

void fecharLugar(int lugar) {
    EstadoAudio::Lugar& l = a.lugares[static_cast<std::size_t>(lugar)];
    for (int canal = 0; canal < 2; ++canal) {
        mci(L"close " + aliasDe(lugar, canal));
        l.volumeAplicado[canal] = -1;
    }
    l.musica = IdSom::Total;
    l.fade = l.alvo = 0.0f;
}

bool abrirCanal(int lugar, int canal, IdSom id) {
    const DefinicaoSom& d = a.tabela[static_cast<std::size_t>(id)];
    if (d.arquivo.empty()) return false;
    std::wstring caminho = a.pasta;
    for (char c : d.arquivo) caminho += (c == '/') ? L'\\' : static_cast<wchar_t>(static_cast<unsigned char>(c));
    const std::wstring alias = aliasDe(lugar, canal);
    if (mciSendStringW((L"open \"" + caminho + L"\" type mpegvideo alias " + alias).c_str(), nullptr, 0, nullptr) != 0) {
        return false;
    }
    mci(L"setaudio " + alias + L" volume to 0");   // entra do silêncio: quem sobe é o fade
    mci(L"play " + alias + L" repeat");
    return true;
}

void aplicarVolumeCanal(int lugar, int canal, float volume) {
    int& aplicado = a.lugares[static_cast<std::size_t>(lugar)].volumeAplicado[canal];
    const int v = static_cast<int>(volume * 1000.0f + 0.5f);
    if (std::abs(v - aplicado) < 4 && !(v == 0 && aplicado != 0)) return;
    aplicado = v;
    mci(L"setaudio " + aliasDe(lugar, canal) + L" volume to " + std::to_wstring(v));
}

}  // namespace

void iniciarAudio(const std::wstring& pastaAssets, const TabelaSons& tabela, std::string& avisos) {
    a.tabela = tabela;
    a.pasta = pastaAssets;
    InitializeCriticalSection(&a.trava);
    a.travaCriada = true;
    for (int i = 0; i < TOTAL_SONS; ++i) {
        const IdSom id = static_cast<IdSom>(i);
        const DefinicaoSom& d = tabela[static_cast<std::size_t>(i)];
        if (d.arquivo.empty() || ehMusica(id)) continue;
        std::wstring caminho = pastaAssets;
        for (char c : d.arquivo) caminho += (c == '/') ? L'\\' : static_cast<wchar_t>(static_cast<unsigned char>(c));
        std::string conteudo;
        lerArquivoTexto(std::filesystem::path(caminho), conteudo);   // leitura binária
        const std::vector<std::uint8_t> bytes(conteudo.begin(), conteudo.end());
        std::string erro;
        if (bytes.empty() || !lerWav(bytes, a.sons[static_cast<std::size_t>(i)], erro)) {
            avisos += std::string(nomeSom(id)) + ": " + (bytes.empty() ? "arquivo não encontrado" : erro) + "\n";
            a.sons[static_cast<std::size_t>(i)] = SomPcm{};
        } else if (a.sons[static_cast<std::size_t>(i)].taxa != TAXA) {
            avisos += std::string(nomeSom(id)) + ": taxa diferente de 44100 Hz (ignorado)\n";
            a.sons[static_cast<std::size_t>(i)] = SomPcm{};
        }
    }
    if (!abrirDispositivo()) avisos += "dispositivo de áudio indisponível: jogo sem efeitos sonoros\n";
}

void encerrarAudio() {
    fecharLugar(0);   // ao sair, sem fade
    fecharLugar(1);
    if (a.thread) {
        InterlockedExchange(&a.parar, 1);
        SetEvent(a.evento);
        WaitForSingleObject(a.thread, 2000);
        CloseHandle(a.thread);
    }
    if (a.dispositivo) {
        waveOutReset(a.dispositivo);
        for (WAVEHDR& h : a.cabecalhos) waveOutUnprepareHeader(a.dispositivo, &h, sizeof(WAVEHDR));
        waveOutClose(a.dispositivo);
    }
    if (a.evento) CloseHandle(a.evento);
    if (a.travaCriada) DeleteCriticalSection(&a.trava);
    a = EstadoAudio{};
}

void tocarEfeito(IdSom id, float volume) {
    if (!a.dispositivo || ehMusica(id)) return;
    const SomPcm& s = a.sons[static_cast<std::size_t>(id)];
    if (s.amostras.empty()) return;
    EnterCriticalSection(&a.trava);
    // Voz livre; se todas tocam, rouba a que está mais adiantada (a que termina antes).
    Voz* escolhida = nullptr;
    for (Voz& v : a.vozes) {
        if (!v.som) { escolhida = &v; break; }
        if (!escolhida || v.quadro > escolhida->quadro) escolhida = &v;
    }
    *escolhida = {&s, 0, volume * a.tabela[static_cast<std::size_t>(id)].volume, false};
    LeaveCriticalSection(&a.trava);
}

void ligarAmbiente(IdSom id) {
    if (!a.dispositivo) return;
    EnterCriticalSection(&a.trava);
    if (id == IdSom::Total || a.sons[static_cast<std::size_t>(id)].amostras.empty()) {
        a.ambienteDesejado = nullptr;   // desce até sumir (o mixer troca aos poucos)
    } else {
        a.ambienteDesejado = &a.sons[static_cast<std::size_t>(id)];
        a.volumeAmbienteDesejado = a.tabela[static_cast<std::size_t>(id)].volume;
    }
    LeaveCriticalSection(&a.trava);
}

void definirMusica(IdSom musica) {
    if (musica == a.musica) return;
    a.musica = musica;
    EstadoAudio::Lugar& agora = a.lugares[static_cast<std::size_t>(a.atual)];
    if (musica != IdSom::Total && agora.musica == musica) {   // a mesma que estava saindo: só volta a subir
        agora.alvo = 1.0f;
        return;
    }
    agora.alvo = 0.0f;           // a que toca desce aos poucos
    agora.tensao = a.tensao;     // e guarda o equilíbrio jogo/tensão em que estava
    a.tensao = a.alvoTensao = 0.0f;
    if (musica == IdSom::Total) return;
    const int outro = 1 - a.atual;
    EstadoAudio::Lugar& novo = a.lugares[static_cast<std::size_t>(outro)];
    a.atual = outro;
    if (novo.musica == musica) {   // voltando para a que acabou de sair (ainda tocando, descendo): sobe de novo
        novo.alvo = 1.0f;
        a.tensao = novo.tensao;
        return;
    }
    fecharLugar(outro);   // o que sobrava ali (uma 3ª troca em menos de 2 s) sai já
    novo.musica = musica;
    novo.fade = 0.0f;
    novo.alvo = 1.0f;
    abrirCanal(outro, 0, musica);
    if (musica == IdSom::MusicaJogo) abrirCanal(outro, 1, IdSom::MusicaTensao);
}

std::string diagnosticoAudio() {
    int efeitos = 0;
    for (const SomPcm& s : a.sons) efeitos += s.amostras.empty() ? 0 : 1;
    auto estado = [](int canal) {
        wchar_t modo[64] = L"", vol[32] = L"";
        mciSendStringW((L"status " + aliasDe(a.atual, canal) + L" mode").c_str(), modo, 64, nullptr);
        mciSendStringW((L"status " + aliasDe(a.atual, canal) + L" volume").c_str(), vol, 32, nullptr);
        std::string r;
        for (const wchar_t* p = modo; *p; ++p) r += static_cast<char>(*p);
        r += " vol=";
        for (const wchar_t* p = vol; *p; ++p) r += static_cast<char>(*p);
        return r.empty() ? std::string("fechada") : r;
    };
    return std::string("waveOut=") + (a.dispositivo ? "ok" : "indisponivel") + "\nefeitos_carregados=" +
           std::to_string(efeitos) + "\nmusica=" + estado(0) + "\ntensao=" + estado(1) +
           "\nnivel_tensao=" + std::to_string(a.tensao) + "\n";
}

void definirTensao(float alvo) { a.alvoTensao = alvo < 0.0f ? 0.0f : (alvo > 1.0f ? 1.0f : alvo); }

void definirVolumes(float geral, float musica, float efeitos) {
    if (!a.travaCriada) return;
    EnterCriticalSection(&a.trava);
    a.volGeral = geral;
    a.volMusica = musica;
    a.volEfeitos = efeitos;
    LeaveCriticalSection(&a.trava);
}

void atualizarAudio(float dt, bool janelaFocada) {
    if (!a.travaCriada) return;
    EnterCriticalSection(&a.trava);
    a.mudo = !janelaFocada;
    const float mestre = a.mudo ? 0.0f : a.volGeral * a.volMusica;
    LeaveCriticalSection(&a.trava);
    // Crossfade: a tensão anda em direção ao alvo numa velocidade fixa.
    const float passo = VELOCIDADE_CROSSFADE * dt;
    if (a.tensao < a.alvoTensao) a.tensao = std::fmin(a.alvoTensao, a.tensao + passo);
    else a.tensao = std::fmax(a.alvoTensao, a.tensao - passo);
    const float tens = a.tabela[static_cast<std::size_t>(IdSom::MusicaTensao)].volume;
    for (int l = 0; l < 2; ++l) {
        EstadoAudio::Lugar& lugar = a.lugares[static_cast<std::size_t>(l)];
        if (lugar.musica == IdSom::Total) continue;
        if (lugar.fade < lugar.alvo) lugar.fade = std::fmin(lugar.alvo, lugar.fade + dt / DURACAO_ENTRADA);
        else lugar.fade = std::fmax(lugar.alvo, lugar.fade - dt / DURACAO_SAIDA);
        if (lugar.fade <= 0.0f && lugar.alvo <= 0.0f) {   // terminou de sair: fecha
            fecharLugar(l);
            continue;
        }
        // Curvas de potência constante (sen/cos): o volume total não "afunda" no meio de nenhuma das trocas.
        const float ganho = std::sin(lugar.fade * 1.5707963f);
        const float t = (l == a.atual ? a.tensao : lugar.tensao) * 1.5707963f;
        const bool jogo = lugar.musica == IdSom::MusicaJogo;
        const float base = a.tabela[static_cast<std::size_t>(lugar.musica)].volume;
        aplicarVolumeCanal(l, 0, mestre * base * ganho * (jogo ? std::cos(t) : 1.0f));
        if (jogo) aplicarVolumeCanal(l, 1, mestre * tens * ganho * std::sin(t));
    }
}
