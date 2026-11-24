#include "plataforma/janela.h"

#include "core/tipos.h"

namespace {

const wchar_t* CLASSE_JANELA = L"EcosDeDerryJanela";

// O procedimento de janela é chamado pelo Windows; o estado do jogo chega a ele por um
// ponteiro guardado na própria janela (GWLP_USERDATA) — exemplo de ponteiro em T03.
EstadoJanela* estadoDa(HWND hwnd) { return reinterpret_cast<EstadoJanela*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA)); }

void atualizarTamanhoCliente(EstadoJanela& janela) {
    RECT r{};
    GetClientRect(janela.hwnd, &r);
    janela.clienteW = r.right - r.left;
    janela.clienteH = r.bottom - r.top;
    janela.limparBordas = true;
}

LRESULT CALLBACK procedimentoJanela(HWND hwnd, UINT mensagem, WPARAM wParam, LPARAM lParam) {
    if (mensagem == WM_NCCREATE) {
        const auto* criacao = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(criacao->lpCreateParams));
        return DefWindowProcW(hwnd, mensagem, wParam, lParam);
    }
    EstadoJanela* janela = estadoDa(hwnd);
    if (janela == nullptr) {
        return DefWindowProcW(hwnd, mensagem, wParam, lParam);
    }
    switch (mensagem) {
        case WM_CLOSE:
            janela->pedidoFechar = true;
            return 0;
        case WM_SIZE:
            atualizarTamanhoCliente(*janela);
            return 0;
        case WM_ERASEBKGND:
            return 1;   // nunca apagar o fundo: evita piscar (F02)
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            janela->limparBordas = true;   // o próximo quadro repinta tudo
            return 0;
        }
        case WM_SETFOCUS:
            janela->focada = true;
            return 0;
        case WM_KILLFOCUS:
            // Ao perder o foco: soltar todas as teclas para nada ficar "preso".
            janela->focada = false;
            janela->teclas.fill(false);
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256) janela->teclas[wParam] = true;
            if (mensagem == WM_SYSKEYDOWN && wParam == VK_RETURN) {   // Alt+Enter
                alternarTelaCheia(*janela);
                return 0;
            }
            if (wParam == VK_F10) return 0;   // F10 abriria o menu do sistema
            break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256) janela->teclas[wParam] = false;
            if (wParam == VK_F10) return 0;
            break;
        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = 320;
            info->ptMinTrackSize.y = 200;
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, mensagem, wParam, lParam);
}

// Maior escala (até 3×) cuja janela cabe em 90% da área de trabalho.
int escalaInicial() {
    RECT area{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &area, 0);
    const int w = (area.right - area.left) * 9 / 10;
    const int h = (area.bottom - area.top) * 9 / 10;
    int escala = calcularApresentacao(w, h, LARGURA_LOGICA, ALTURA_LOGICA, 3).escala;
    return escala < 1 ? 1 : escala;
}

}  // namespace

bool criarJanela(EstadoJanela& janela, HINSTANCE instancia, const wchar_t* titulo) {
    WNDCLASSEXW classe{};
    classe.cbSize = sizeof(classe);
    classe.style = CS_HREDRAW | CS_VREDRAW;
    classe.lpfnWndProc = procedimentoJanela;
    classe.hInstance = instancia;
    classe.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    classe.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    classe.lpszClassName = CLASSE_JANELA;
    if (!RegisterClassExW(&classe)) return false;

    const int escala = escalaInicial();
    RECT r{0, 0, LARGURA_LOGICA * escala, ALTURA_LOGICA * escala};
    const DWORD estilo = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&r, estilo, FALSE);
    janela.hwnd = CreateWindowExW(0, CLASSE_JANELA, titulo, estilo, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                                  r.bottom - r.top, nullptr, nullptr, instancia, &janela);
    if (!janela.hwnd) return false;
    janela.posicaoJanela.length = sizeof(WINDOWPLACEMENT);
    ShowWindow(janela.hwnd, SW_SHOW);
    UpdateWindow(janela.hwnd);
    atualizarTamanhoCliente(janela);
    return true;
}

void processarMensagens(EstadoJanela& janela) {
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            janela.pedidoFechar = true;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

Apresentacao apresentacaoAtual(const EstadoJanela& janela) {
    return calcularApresentacao(janela.clienteW, janela.clienteH, LARGURA_LOGICA, ALTURA_LOGICA);
}

void apresentarCanvas(EstadoJanela& janela, const Canvas& canvas) {
    HDC dcJanela = GetDC(janela.hwnd);
    const Apresentacao a = apresentacaoAtual(janela);
    if (janela.limparBordas || a.escala == 0) {
        RECT cliente{0, 0, janela.clienteW, janela.clienteH};
        FillRect(dcJanela, &cliente, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        janela.limparBordas = false;
    }
    if (a.escala == 0) {
        // Janela menor que 640 × 360: não corta o jogo; mostra uma orientação legível.
        SetBkMode(dcJanela, TRANSPARENT);
        SetTextColor(dcJanela, RGB(230, 220, 200));
        // DT_VCENTER só centraliza texto de uma linha: medimos a altura e centralizamos à mão.
        const wchar_t* aviso = L"Janela pequena demais.\nAumente para pelo menos 640 × 360.";
        RECT medida{0, 0, janela.clienteW, 0};
        DrawTextW(dcJanela, aviso, -1, &medida, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        const int altura = medida.bottom - medida.top;
        RECT area{0, (janela.clienteH - altura) / 2, janela.clienteW, (janela.clienteH + altura) / 2};
        DrawTextW(dcJanela, aviso, -1, &area, DT_CENTER | DT_WORDBREAK);
        janela.limparBordas = true;
    } else {
        SetStretchBltMode(dcJanela, COLORONCOLOR);   // vizinho mais próximo: pixels nítidos
        StretchBlt(dcJanela, a.x, a.y, a.largura, a.altura, canvas.dc, 0, 0, canvas.w, canvas.h, SRCCOPY);
    }
    ReleaseDC(janela.hwnd, dcJanela);
}

void alternarTelaCheia(EstadoJanela& janela) {
    const DWORD estilo = static_cast<DWORD>(GetWindowLongW(janela.hwnd, GWL_STYLE));
    if (!janela.telaCheia) {
        MONITORINFO monitor{};
        monitor.cbSize = sizeof(monitor);
        if (GetWindowPlacement(janela.hwnd, &janela.posicaoJanela) &&
            GetMonitorInfoW(MonitorFromWindow(janela.hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor)) {
            SetWindowLongW(janela.hwnd, GWL_STYLE, estilo & ~WS_OVERLAPPEDWINDOW);
            const RECT& m = monitor.rcMonitor;
            SetWindowPos(janela.hwnd, HWND_TOP, m.left, m.top, m.right - m.left, m.bottom - m.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            janela.telaCheia = true;
        }
    } else {
        SetWindowLongW(janela.hwnd, GWL_STYLE, estilo | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(janela.hwnd, &janela.posicaoJanela);
        SetWindowPos(janela.hwnd, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        janela.telaCheia = false;
    }
    atualizarTamanhoCliente(janela);
}
