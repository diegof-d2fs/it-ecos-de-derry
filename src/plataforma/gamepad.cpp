#include "plataforma/gamepad.h"

#include <windows.h>
#include <xinput.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "plataforma/tempo.h"

namespace {

constexpr double INTERVALO_BUSCA = 2.0;   // segundos entre buscas sem controle (lição nº 2)

// ---------- SDL2, carregada em tempo de execução ----------
// Só os tipos que usamos: o controle é um ponteiro opaco e SDL_bool é um int.
using SdlControle = void*;
struct BibliotecaSdl {
    HMODULE dll = nullptr;
    int(__cdecl* iniciar)(unsigned) = nullptr;
    void(__cdecl* encerrar)(unsigned) = nullptr;
    int(__cdecl* definirHint)(const char*, const char*) = nullptr;
    int(__cdecl* eventosControle)(int) = nullptr;
    int(__cdecl* eventosJoystick)(int) = nullptr;
    int(__cdecl* quantidade)() = nullptr;
    int(__cdecl* ehControle)(int) = nullptr;
    SdlControle(__cdecl* abrir)(int) = nullptr;
    void(__cdecl* fechar)(SdlControle) = nullptr;
    int(__cdecl* conectado)(SdlControle) = nullptr;
    const char*(__cdecl* nome)(SdlControle) = nullptr;
    void(__cdecl* atualizar)() = nullptr;
    short(__cdecl* eixo)(SdlControle, int) = nullptr;
    unsigned char(__cdecl* botao)(SdlControle, int) = nullptr;
    bool iniciada = false;
    SdlControle controle = nullptr;
};
constexpr unsigned SDL_INIT_GAMECONTROLLER = 0x2000;
// Índices de SDL_GameControllerAxis.
constexpr int SDL_EIXO_ESQ_X = 0, SDL_EIXO_ESQ_Y = 1, SDL_EIXO_DIR_X = 2, SDL_EIXO_DIR_Y = 3;
constexpr int SDL_GATILHO_ESQ = 4, SDL_GATILHO_DIR = 5;

// ---------- XInput ----------
using FuncaoXInputGetState = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);

struct EstadoGamepad {
    BibliotecaSdl sdl;
    HMODULE xinputDll = nullptr;
    FuncaoXInputGetState xinputLer = nullptr;
    int portaXInput = -1;       // -1 = sem controle XInput
    double proximaBusca = 0.0;
    bool simulado = false;
    float custoUltimaBuscaMs = 0.0f;
};
EstadoGamepad g;

template <class F>
bool obter(HMODULE dll, const char* nome, F& funcao) {
    funcao = reinterpret_cast<F>(reinterpret_cast<void*>(GetProcAddress(dll, nome)));
    return funcao != nullptr;
}

bool carregarSdl(const std::wstring& pasta) {
    BibliotecaSdl& s = g.sdl;
    const std::wstring caminho = pasta + L"SDL2.dll";
    // Só a DLL da pasta do jogo (lição nº 5: não usar uma SDL2 qualquer instalada na máquina).
    s.dll = LoadLibraryExW(caminho.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!s.dll) return false;
    const bool ok = obter(s.dll, "SDL_InitSubSystem", s.iniciar) && obter(s.dll, "SDL_QuitSubSystem", s.encerrar) &&
                    obter(s.dll, "SDL_SetHint", s.definirHint) &&
                    obter(s.dll, "SDL_GameControllerEventState", s.eventosControle) &&
                    obter(s.dll, "SDL_JoystickEventState", s.eventosJoystick) &&
                    obter(s.dll, "SDL_NumJoysticks", s.quantidade) && obter(s.dll, "SDL_IsGameController", s.ehControle) &&
                    obter(s.dll, "SDL_GameControllerOpen", s.abrir) && obter(s.dll, "SDL_GameControllerClose", s.fechar) &&
                    obter(s.dll, "SDL_GameControllerGetAttached", s.conectado) &&
                    obter(s.dll, "SDL_GameControllerName", s.nome) &&
                    obter(s.dll, "SDL_GameControllerUpdate", s.atualizar) &&
                    obter(s.dll, "SDL_GameControllerGetAxis", s.eixo) &&
                    obter(s.dll, "SDL_GameControllerGetButton", s.botao);
    if (ok) {
        // O jogo não tem janela SDL: para a SDL ele estaria sempre "sem foco". Lemos sempre e
        // o foco é filtrado em main.cpp (sem foco, a leitura é ignorada). Igual ao Bomberman.
        s.definirHint("SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS", "1");
        // Botões por POSIÇÃO: sem isto, num controle Nintendo/8BitDo o "A" seria o botão leste.
        s.definirHint("SDL_GAMECONTROLLER_USE_BUTTON_LABELS", "0");
        if (s.iniciar(SDL_INIT_GAMECONTROLLER) == 0) {
            // Lemos o estado direto; com eventos ligados a fila da SDL só cresceria.
            s.eventosControle(0);
            s.eventosJoystick(0);
            s.iniciada = true;
            return true;
        }
    }
    FreeLibrary(s.dll);
    s = BibliotecaSdl{};
    return false;
}

void carregarXInput() {
    for (const wchar_t* nome : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"}) {
        HMODULE dll = LoadLibraryW(nome);
        if (!dll) continue;
        FuncaoXInputGetState f = nullptr;
        if (obter(dll, "XInputGetState", f)) {
            g.xinputDll = dll;
            g.xinputLer = f;
            return;
        }
        FreeLibrary(dll);
    }
}

void copiarNome(InfoControle& info, const char* nome) {
    std::snprintf(info.nome, sizeof(info.nome), "%s", (nome && nome[0]) ? nome : "Controle");
}

// ---------- leitura SDL ----------
bool lerSdl(InfoControle& info, double agora) {
    BibliotecaSdl& s = g.sdl;
    if (s.controle) {
        s.atualizar();
        if (!s.conectado(s.controle)) {
            s.fechar(s.controle);
            s.controle = nullptr;
            g.proximaBusca = agora;   // procura de novo já (pode ter sido só troca de porta)
            return false;
        }
    } else {
        if (agora < g.proximaBusca) return false;
        const double t0 = agoraSegundos();
        g.proximaBusca = agora + INTERVALO_BUSCA;
        s.atualizar();   // é aqui que a SDL percebe controles recém-conectados
        const int total = s.quantidade();
        for (int i = 0; i < total && !s.controle; ++i) {
            if (s.ehControle(i)) s.controle = s.abrir(i);
        }
        g.custoUltimaBuscaMs = static_cast<float>((agoraSegundos() - t0) * 1000.0);
        if (!s.controle) return false;
        copiarNome(info, s.nome(s.controle));
    }
    LeituraControle& l = info.leitura;
    for (int b = 0; b < TOTAL_BOTOES; ++b) l.botoes[static_cast<size_t>(b)] = s.botao(s.controle, b) != 0;
    l.analogicoEsq = {normalizarEixo(s.eixo(s.controle, SDL_EIXO_ESQ_X)), normalizarEixo(s.eixo(s.controle, SDL_EIXO_ESQ_Y))};
    l.analogicoDir = {normalizarEixo(s.eixo(s.controle, SDL_EIXO_DIR_X)), normalizarEixo(s.eixo(s.controle, SDL_EIXO_DIR_Y))};
    l.gatilhoEsq = normalizarGatilho(s.eixo(s.controle, SDL_GATILHO_ESQ), 32767);
    l.gatilhoDir = normalizarGatilho(s.eixo(s.controle, SDL_GATILHO_DIR), 32767);
    info.origem = OrigemControle::Sdl;
    return true;
}

// ---------- leitura XInput ----------
void traduzirXInput(const XINPUT_GAMEPAD& x, LeituraControle& l) {
    const WORD b = x.wButtons;
    auto marcar = [&](BotaoControle botao, WORD mascara) { l.botoes[static_cast<int>(botao)] = (b & mascara) != 0; };
    marcar(BotaoControle::Sul, XINPUT_GAMEPAD_A);
    marcar(BotaoControle::Leste, XINPUT_GAMEPAD_B);
    marcar(BotaoControle::Oeste, XINPUT_GAMEPAD_X);
    marcar(BotaoControle::Norte, XINPUT_GAMEPAD_Y);
    marcar(BotaoControle::Selecionar, XINPUT_GAMEPAD_BACK);
    marcar(BotaoControle::Iniciar, XINPUT_GAMEPAD_START);
    marcar(BotaoControle::AnalogicoEsq, XINPUT_GAMEPAD_LEFT_THUMB);
    marcar(BotaoControle::AnalogicoDir, XINPUT_GAMEPAD_RIGHT_THUMB);
    marcar(BotaoControle::OmbroEsq, XINPUT_GAMEPAD_LEFT_SHOULDER);
    marcar(BotaoControle::OmbroDir, XINPUT_GAMEPAD_RIGHT_SHOULDER);
    marcar(BotaoControle::Cima, XINPUT_GAMEPAD_DPAD_UP);
    marcar(BotaoControle::Baixo, XINPUT_GAMEPAD_DPAD_DOWN);
    marcar(BotaoControle::Esquerda, XINPUT_GAMEPAD_DPAD_LEFT);
    marcar(BotaoControle::Direita, XINPUT_GAMEPAD_DPAD_RIGHT);
    // No XInput o y do analógico cresce para CIMA; no jogo, para baixo.
    l.analogicoEsq = {normalizarEixo(x.sThumbLX), normalizarEixo(-static_cast<int>(x.sThumbLY))};
    l.analogicoDir = {normalizarEixo(x.sThumbRX), normalizarEixo(-static_cast<int>(x.sThumbRY))};
    l.gatilhoEsq = normalizarGatilho(x.bLeftTrigger, 255);
    l.gatilhoDir = normalizarGatilho(x.bRightTrigger, 255);
}

bool lerXInput(InfoControle& info, double agora) {
    if (!g.xinputLer) return false;
    XINPUT_STATE estado{};
    if (g.portaXInput >= 0) {
        if (g.xinputLer(static_cast<DWORD>(g.portaXInput), &estado) != ERROR_SUCCESS) {
            g.portaXInput = -1;
            g.proximaBusca = agora;
            return false;
        }
    } else {
        // Consultar portas vazias é lento no XInput: só a cada INTERVALO_BUSCA.
        if (agora < g.proximaBusca) return false;
        const double t0 = agoraSegundos();
        g.proximaBusca = agora + INTERVALO_BUSCA;
        for (DWORD p = 0; p < XUSER_MAX_COUNT && g.portaXInput < 0; ++p) {
            if (g.xinputLer(p, &estado) == ERROR_SUCCESS) g.portaXInput = static_cast<int>(p);
        }
        g.custoUltimaBuscaMs = static_cast<float>((agoraSegundos() - t0) * 1000.0);
        if (g.portaXInput < 0) return false;
        char nome[32];
        std::snprintf(nome, sizeof(nome), "Controle XInput %d", g.portaXInput + 1);
        copiarNome(info, nome);
    }
    traduzirXInput(estado.Gamepad, info.leitura);
    info.origem = OrigemControle::XInput;
    return true;
}

// ---------- roteiro do modo simulado (determinístico: depende só do tempo) ----------
void lerSimulado(InfoControle& info, double agora) {
    LeituraControle& l = info.leitura;
    // Um botão por vez, 0,4 s cada; o analógico esquerdo gira; os gatilhos sobem e descem.
    const int atual = static_cast<int>(agora / 0.4) % TOTAL_BOTOES;
    l.botoes[static_cast<size_t>(atual)] = true;
    const float angulo = static_cast<float>(agora * 1.5);
    l.analogicoEsq = {0.9f * std::cos(angulo), 0.9f * std::sin(angulo)};
    l.analogicoDir = {0.6f * std::cos(-angulo), 0.0f};
    l.gatilhoEsq = 0.5f + 0.5f * std::sin(static_cast<float>(agora * 2.0));
    l.gatilhoDir = 1.0f - l.gatilhoEsq;
    info.origem = OrigemControle::Sdl;
    copiarNome(info, "Controle simulado");
}

}  // namespace

void iniciarGamepad(const std::wstring& pastaExecutavel) {
    if (!carregarSdl(pastaExecutavel)) carregarXInput();
}

void encerrarGamepad() {
    if (g.sdl.iniciada) {
        if (g.sdl.controle) g.sdl.fechar(g.sdl.controle);
        g.sdl.encerrar(SDL_INIT_GAMECONTROLLER);
    }
    if (g.sdl.dll) FreeLibrary(g.sdl.dll);
    if (g.xinputDll) FreeLibrary(g.xinputDll);
    g = EstadoGamepad{};
}

void simularGamepad(bool ativo) { g.simulado = ativo; }

void atualizarGamepad(InfoControle& info, double agoraSegundos) {
    info.leitura = LeituraControle{};
    info.origem = OrigemControle::Nenhuma;
    info.sdlCarregada = g.sdl.iniciada;
    info.xinputCarregado = g.xinputLer != nullptr;
    info.custoBuscaMs = g.custoUltimaBuscaMs;
    if (g.simulado) {
        lerSimulado(info, agoraSegundos);
        return;
    }
    // Com a SDL carregada ela também cobre os controles XInput; o XInput puro é só o reserva.
    if (g.sdl.iniciada) {
        lerSdl(info, agoraSegundos);
    } else {
        lerXInput(info, agoraSegundos);
    }
    if (info.origem == OrigemControle::Nenhuma) {
        info.nome[0] = '\0';
        info.leitura = LeituraControle{};
    }
}
