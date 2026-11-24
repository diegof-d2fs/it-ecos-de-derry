#ifdef __EMSCRIPTEN__
// Áudio na versão web (Etapa 18). Mesmo contrato do plataforma/audio.cpp:
//   Efeitos e ambiente: os WAV são lidos e decodificados pelo MESMO lerWav e viram AudioBuffers da Web Audio
//                       (um nó por toque); o ambiente é um laço que troca aos poucos.
//   Músicas: elementos <audio> em streaming (MP3), dois "lugares" A/B com base + tensão, a mesma troca cruzada
//            (a que sai desce em 1,6 s, a que entra sobe em 2 s) e a mesma curva cos/sen da tensão.
// O navegador só libera o som depois de um clique ou tecla: o index.html chama ecosAudio.ctx.resume().
#include "plataforma/audio.h"

#include <emscripten.h>

#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "core/arquivo.h"

EM_JS(void, js_audio_iniciar, (), {
    if (window.ecosAudio) return;
    const ctx = new (window.AudioContext || window.webkitAudioContext)();
    const mestre = ctx.createGain();
    mestre.connect(ctx.destination);
    window.ecosAudio = {ctx : ctx, mestre : mestre, buffers : [], ambiente : null, musicas : {}};
    const liberar = () => { if (ctx.state !== 'running') ctx.resume(); };
    window.addEventListener('pointerdown', liberar);
    window.addEventListener('keydown', liberar);
    window.addEventListener('gamepadconnected', liberar);
});

EM_JS(void, js_audio_carregar, (int id, const short* amostras, int quadros, int canais, int taxa), {
    const A = window.ecosAudio;
    const buf = A.ctx.createBuffer(canais, quadros, taxa);
    for (let c = 0; c < canais; ++c) {
        const dados = buf.getChannelData(c);
        for (let i = 0; i < quadros; ++i) dados[i] = HEAP16[(amostras >> 1) + i * canais + c] / 32768;
    }
    A.buffers[id] = buf;
});

EM_JS(void, js_audio_tocar, (int id, float volume), {
    const A = window.ecosAudio;
    if (!A || !A.buffers[id] || A.ctx.state !== 'running') return;
    const s = A.ctx.createBufferSource();
    s.buffer = A.buffers[id];
    const g = A.ctx.createGain();
    g.gain.value = volume;
    s.connect(g).connect(A.mestre);
    s.start();
});

EM_JS(void, js_audio_ambiente, (int id, float volume), {
    const A = window.ecosAudio;
    if (!A) return;
    const agora = A.ctx.currentTime;
    if (A.ambiente && A.ambiente.id === id) return;
    if (A.ambiente) {   // o que toca desce em 1 s e para
        const velho = A.ambiente;
        velho.ganho.gain.setTargetAtTime(0, agora, 0.3);
        setTimeout(() => { try { velho.fonte.stop(); } catch (e) {} }, 1500);
        A.ambiente = null;
    }
    if (id < 0 || !A.buffers[id]) return;
    const s = A.ctx.createBufferSource();
    s.buffer = A.buffers[id];
    s.loop = true;
    const g = A.ctx.createGain();
    g.gain.value = 0;
    g.gain.setTargetAtTime(volume, agora, 0.3);
    s.connect(g).connect(A.mestre);
    s.start();
    A.ambiente = {id : id, fonte : s, ganho : g};
});

EM_JS(void, js_audio_mestre, (float volume), {
    if (window.ecosAudio) window.ecosAudio.mestre.gain.value = volume;
});

EM_JS(void, js_musica_abrir, (int canal, const char* url), {
    const A = window.ecosAudio;
    if (!A) return;
    const el = new Audio(UTF8ToString(url));
    el.loop = true;
    el.volume = 0;
    el.play().catch(() => { const tentar = () => { el.play().catch(() => {}); };
                            window.addEventListener('pointerdown', tentar, {once : true});
                            window.addEventListener('keydown', tentar, {once : true}); });
    A.musicas[canal] = el;
});

EM_JS(void, js_musica_fechar, (int canal), {
    const A = window.ecosAudio;
    if (!A || !A.musicas[canal]) return;
    A.musicas[canal].pause();
    A.musicas[canal].src = '';
    delete A.musicas[canal];
});

EM_JS(void, js_musica_volume, (int canal, float volume), {
    const A = window.ecosAudio;
    if (A && A.musicas[canal]) A.musicas[canal].volume = Math.max(0, Math.min(1, volume));
});

namespace {

constexpr float VELOCIDADE_CROSSFADE = 1.0f / 1.5f;
constexpr float DURACAO_ENTRADA = 2.0f;
constexpr float DURACAO_SAIDA = 1.6f;

struct Lugar {
    IdSom musica = IdSom::Total;
    float fade = 0.0f, alvo = 0.0f, tensao = 0.0f;
};

struct EstadoAudio {
    TabelaSons tabela;
    std::string pasta;
    bool iniciado = false;
    float volGeral = 1.0f, volMusica = 1.0f, volEfeitos = 1.0f;
    std::array<Lugar, 2> lugares;
    int atual = 0;
    IdSom musica = IdSom::Total;
    float tensao = 0.0f, alvoTensao = 0.0f;
    IdSom ambiente = IdSom::Total;
};
EstadoAudio a;

int canal(int lugar, int c) { return lugar * 2 + c; }

std::string caminhoDe(IdSom id) {
    std::string s = a.pasta + a.tabela[static_cast<std::size_t>(id)].arquivo;
    for (char& c : s) if (c == '\\') c = '/';
    return s;
}

void fecharLugar(int l) {
    js_musica_fechar(canal(l, 0));
    js_musica_fechar(canal(l, 1));
    a.lugares[static_cast<std::size_t>(l)] = Lugar{};
}

}  // namespace

void iniciarAudio(const std::wstring& pastaAssets, const TabelaSons& tabela, std::string& avisos) {
    a.tabela = tabela;
    a.pasta.clear();
    for (wchar_t c : pastaAssets) a.pasta.push_back(c == L'\\' ? '/' : static_cast<char>(c));
    js_audio_iniciar();
    for (int i = 0; i < TOTAL_SONS; ++i) {
        const IdSom id = static_cast<IdSom>(i);
        const DefinicaoSom& d = tabela[static_cast<std::size_t>(i)];
        if (d.arquivo.empty() || ehMusica(id)) continue;
        std::string conteudo;
        lerArquivoTexto(std::filesystem::path(caminhoDe(id)), conteudo);
        const std::vector<std::uint8_t> bytes(conteudo.begin(), conteudo.end());
        SomPcm som;
        std::string erro;
        if (bytes.empty() || !lerWav(bytes, som, erro)) {
            avisos += std::string(nomeSom(id)) + ": " + (bytes.empty() ? "arquivo não encontrado" : erro) + "\n";
            continue;
        }
        const int quadros = static_cast<int>(som.amostras.size()) / std::max(1, som.canais);
        js_audio_carregar(i, som.amostras.data(), quadros, som.canais, som.taxa);
    }
    a.iniciado = true;
}

void encerrarAudio() {
    fecharLugar(0);
    fecharLugar(1);
}

void tocarEfeito(IdSom id, float volume) {
    if (!a.iniciado || ehMusica(id)) return;
    js_audio_tocar(static_cast<int>(id), volume * a.tabela[static_cast<std::size_t>(id)].volume);
}

void ligarAmbiente(IdSom id) {
    if (!a.iniciado || id == a.ambiente) return;
    a.ambiente = id;
    js_audio_ambiente(id == IdSom::Total ? -1 : static_cast<int>(id),
                      id == IdSom::Total ? 0.0f : a.tabela[static_cast<std::size_t>(id)].volume);
}

void definirMusica(IdSom musica) {
    if (!a.iniciado || musica == a.musica) return;
    a.musica = musica;
    Lugar& agora = a.lugares[static_cast<std::size_t>(a.atual)];
    if (musica != IdSom::Total && agora.musica == musica) {
        agora.alvo = 1.0f;
        return;
    }
    agora.alvo = 0.0f;
    agora.tensao = a.tensao;
    a.tensao = a.alvoTensao = 0.0f;
    if (musica == IdSom::Total) return;
    const int outro = 1 - a.atual;
    Lugar& novo = a.lugares[static_cast<std::size_t>(outro)];
    a.atual = outro;
    if (novo.musica == musica) {
        novo.alvo = 1.0f;
        a.tensao = novo.tensao;
        return;
    }
    fecharLugar(outro);
    novo.musica = musica;
    novo.alvo = 1.0f;
    js_musica_abrir(canal(outro, 0), caminhoDe(musica).c_str());
    if (musica == IdSom::MusicaJogo) js_musica_abrir(canal(outro, 1), caminhoDe(IdSom::MusicaTensao).c_str());
}

void definirTensao(float alvo) { a.alvoTensao = alvo < 0.0f ? 0.0f : (alvo > 1.0f ? 1.0f : alvo); }

void definirVolumes(float geral, float musica, float efeitos) {
    a.volGeral = geral;
    a.volMusica = musica;
    a.volEfeitos = efeitos;
    if (a.iniciado) js_audio_mestre(geral * efeitos);
}

void atualizarAudio(float dt, bool janelaFocada) {
    if (!a.iniciado) return;
    const float mestre = janelaFocada ? a.volGeral * a.volMusica : 0.0f;
    js_audio_mestre(janelaFocada ? a.volGeral * a.volEfeitos : 0.0f);
    const float passo = VELOCIDADE_CROSSFADE * dt;
    if (a.tensao < a.alvoTensao) a.tensao = std::fmin(a.alvoTensao, a.tensao + passo);
    else a.tensao = std::fmax(a.alvoTensao, a.tensao - passo);
    const float tens = a.tabela[static_cast<std::size_t>(IdSom::MusicaTensao)].volume;
    for (int l = 0; l < 2; ++l) {
        Lugar& lugar = a.lugares[static_cast<std::size_t>(l)];
        if (lugar.musica == IdSom::Total) continue;
        if (lugar.fade < lugar.alvo) lugar.fade = std::fmin(lugar.alvo, lugar.fade + dt / DURACAO_ENTRADA);
        else lugar.fade = std::fmax(lugar.alvo, lugar.fade - dt / DURACAO_SAIDA);
        if (lugar.fade <= 0.0f && lugar.alvo <= 0.0f) {
            fecharLugar(l);
            continue;
        }
        const float ganho = std::sin(lugar.fade * 1.5707963f);
        const float t = (l == a.atual ? a.tensao : lugar.tensao) * 1.5707963f;
        const bool jogo = lugar.musica == IdSom::MusicaJogo;
        const float base = a.tabela[static_cast<std::size_t>(lugar.musica)].volume;
        js_musica_volume(canal(l, 0), mestre * base * ganho * (jogo ? std::cos(t) : 1.0f));
        if (jogo) js_musica_volume(canal(l, 1), mestre * tens * ganho * std::sin(t));
    }
}

std::string diagnosticoAudio() { return "web\n"; }
#endif
