<#
  Teste automatizado da JANELA do jogo (o equivalente ao Playwright para um programa Win32).
  Abre build\ecos_de_derry.exe, envia teclas direto para a janela dele (PostMessage — nunca
  para outros programas), alterna tela cheia, redimensiona, tira o foco e captura cada passo.

  Uso:  powershell -ExecutionPolicy Bypass -File tools\testar_janela.ps1
  Saída: build\teste_janela\*.png + resumo no console (código 0 = tudo ok).
#>
param(
    [string]$Exe = (Join-Path $PSScriptRoot "..\build\ecos_de_derry.exe"),
    [string]$Saida = (Join-Path $PSScriptRoot "..\build\teste_janela")
)
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing, System.Windows.Forms
Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;
public static class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr a, int x, int y, int cx, int cy, uint f);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr hdc, uint f);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    public static Bitmap Capturar(IntPtr h) {
        RECT r; GetWindowRect(h, out r);
        Bitmap b = new Bitmap(Math.Max(1, r.R - r.L), Math.Max(1, r.B - r.T));
        using (Graphics g = Graphics.FromImage(b)) {
            IntPtr dc = g.GetHdc();
            PrintWindow(h, dc, 2);   // PW_RENDERFULLCONTENT: funciona mesmo com a janela coberta
            g.ReleaseHdc(dc);
        }
        return b;
    }
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, IntPtr pid);
    [DllImport("kernel32.dll")] public static extern uint GetCurrentThreadId();
    [DllImport("user32.dll")] public static extern bool AttachThreadInput(uint a, uint b, bool attach);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    public static bool Focar(IntPtr h) {
        // O Windows bloqueia trocas de foco pedidas por outro programa (foreground lock).
        // Tentativas: direto → ligando a fila de entrada à da janela atual → toque de Alt.
        for (int tentativa = 0; tentativa < 6; tentativa++) {
            ShowWindow(h, 9);
            IntPtr atual = GetForegroundWindow();
            uint threadAtual = GetWindowThreadProcessId(atual, IntPtr.Zero);
            uint minha = GetCurrentThreadId();
            bool ligada = threadAtual != 0 && threadAtual != minha && AttachThreadInput(minha, threadAtual, true);
            if (tentativa >= 2) { keybd_event(0x12, 0, 0, UIntPtr.Zero); keybd_event(0x12, 0, 2, UIntPtr.Zero); }
            BringWindowToTop(h);
            SetForegroundWindow(h);
            if (ligada) AttachThreadInput(minha, threadAtual, false);
            System.Threading.Thread.Sleep(150);
            if (GetForegroundWindow() == h) return true;
        }
        return false;
    }
    public static string Cliente(IntPtr h) { RECT r; GetClientRect(h, out r); return (r.R - r.L) + "x" + (r.B - r.T); }
}
"@

$WM_KEYDOWN = 0x0100; $WM_KEYUP = 0x0101
$VK = @{ D = 0x44; S = 0x53; LEFT = 0x25; UP = 0x26; F11 = 0x7A; F1 = 0x70; ESC = 0x1B; ENTER = 0x0D }
$falhas = @()
New-Item -ItemType Directory -Force -Path $Saida | Out-Null

function Tecla([IntPtr]$h, [int]$vk, [int]$ms) {
    [void][Win]::PostMessage($h, $WM_KEYDOWN, [IntPtr]$vk, [IntPtr]::Zero)
    Start-Sleep -Milliseconds $ms
    [void][Win]::PostMessage($h, $WM_KEYUP, [IntPtr]$vk, [IntPtr]::Zero)
    Start-Sleep -Milliseconds 120
}
function Foto([IntPtr]$h, [string]$nome) {
    Start-Sleep -Milliseconds 250
    $b = [Win]::Capturar($h); $arq = Join-Path $Saida "$nome.png"; $b.Save($arq); $b.Dispose()
    Write-Host ("  foto {0,-22} cliente {1}" -f $nome, [Win]::Cliente($h))
}
function Verificar([bool]$ok, [string]$descricao) {
    if ($ok) { Write-Host "  [ ok ] $descricao" } else { Write-Host "  [FALHA] $descricao"; $script:falhas += $descricao }
}

Write-Host "Abrindo $Exe"
# Saves próprios e limpos: o teste nunca mexe no perfil e nos recordes de quem joga.
$saves = Join-Path $Saida "saves"
Remove-Item $saves -Recurse -Force -ErrorAction SilentlyContinue
$p = Start-Process -FilePath $Exe -ArgumentList "--mudo --saves `"$saves`"" -WorkingDirectory (Split-Path $Exe) -PassThru
$h = [IntPtr]::Zero
for ($i = 0; $i -lt 50 -and $h -eq [IntPtr]::Zero; $i++) { Start-Sleep -Milliseconds 100; $p.Refresh(); $h = $p.MainWindowHandle }
Verificar ($h -ne [IntPtr]::Zero) "janela criada"
if ($h -eq [IntPtr]::Zero) { exit 1 }

Verificar ([Win]::Focar($h)) "janela recebeu o foco"
Foto $h "00_menu"
Tecla $h $VK.ENTER 60                    # Novo jogo (perfil limpo) → prólogo na Rua Witcham (Etapa 7.4)
Tecla $h $VK.ENTER 60                    # tela de dificuldade (desde 17/11): Normal
Start-Sleep -Milliseconds 200
Start-Sleep -Milliseconds 300
Foto $h "01_prologo_dica_D01"
Verificar (Test-Path (Join-Path $saves "perfil.txt")) "o menu criou o perfil ao começar"
Tecla $h $VK.ENTER 60                    # completa a fala do Mike (tutorial D01, Etapa 7.3)
Tecla $h $VK.ENTER 60                    # fecha a caixa de diálogo
Start-Sleep -Milliseconds 200
Verificar ((Get-Content (Join-Path $saves "perfil.txt") -Raw) -match "dicas_vistas=1") "a dica D01 ficou gravada como vista"
Tecla $h $VK.F1 80                       # liga o medidor
Tecla $h $VK.D 900;  Foto $h "02_andou_direita_D"
Tecla $h $VK.S 600;  Foto $h "03_andou_baixo_S"
Tecla $h $VK.LEFT 700; Foto $h "04_andou_esquerda_seta"
Tecla $h $VK.UP 700; Foto $h "05_andou_cima_seta"

# O jogo IGNORA teclas sem foco (de propósito). Se outra janela roubou o foco no meio do teste,
# as teclas se perdem; por isso o foco é pedido de novo antes de cada bloco de teclas.
[void][Win]::Focar($h)
# Desde 17/11 o jogo ABRE em tela cheia (em janela, 1280x720 eram 2/3 da tela do Diego).
$tela = [System.Windows.Forms.Screen]::FromHandle($h).Bounds
$cheia = [Win]::Cliente($h)
Verificar ($cheia -eq "$($tela.Width)x$($tela.Height)") "abre em tela cheia, ocupando o monitor inteiro ($cheia)"
Foto $h "06_tela_cheia"
Tecla $h $VK.F11 80; Start-Sleep -Milliseconds 400
$janela = [Win]::Cliente($h)
Verificar ($janela -ne $cheia) "F11 volta para a janela ($janela)"
[void][Win]::Focar($h)
Tecla $h $VK.F11 80; Start-Sleep -Milliseconds 400
Verificar ([Win]::Cliente($h) -eq $cheia) "F11 de novo volta à tela cheia"
[void][Win]::Focar($h)
Tecla $h $VK.F11 80; Start-Sleep -Milliseconds 400   # o resto do teste é em janela

[void][Win]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 760, 470, 0x0016)   # NOMOVE|NOZORDER|NOACTIVATE
Foto $h "07_redimensionada_escala1"
[void][Win]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 1400, 860, 0x0016)
Foto $h "08_redimensionada_escala2"
[void][Win]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 520, 330, 0x0016)
Foto $h "09_pequena_demais"
[void][Win]::SetWindowPos($h, [IntPtr]::Zero, 0, 0, 1296, 759, 0x0016)

# Perda de foco: uma janela nossa (formulário vazio) assume o primeiro plano.
$form = New-Object System.Windows.Forms.Form
$form.Text = "foco"; $form.Width = 200; $form.Height = 120; $form.StartPosition = "Manual"; $form.Left = 0; $form.Top = 0
$form.Show(); $form.Activate(); [System.Windows.Forms.Application]::DoEvents()
[void][Win]::Focar($form.Handle)
Verificar ([Win]::GetForegroundWindow() -ne $h) "jogo perdeu o foco"
Tecla $h $VK.D 600                       # sem foco, a tecla NÃO pode mover Bill
Foto $h "10_sem_foco_pausado"
$form.Close(); $form.Dispose()
Verificar ([Win]::Focar($h)) "jogo recuperou o foco"
Foto $h "11_foco_devolvido"

# Esc abre a PAUSA (Etapa 6.4); o jogo fecha pelo menu: Sair do jogo → Sim.
[void][Win]::Focar($h)
Tecla $h $VK.ESC 80
Foto $h "12_pausa"
Verificar (-not $p.HasExited) "Esc pausa (não fecha o jogo)"
Tecla $h $VK.UP 60                       # dá a volta: última opção = Sair do jogo
Tecla $h $VK.ENTER 60
Foto $h "13_confirmar_sair"
Tecla $h $VK.UP 60                       # "Sim"
Tecla $h $VK.ENTER 60
$saiu = $p.WaitForExit(3000)
Verificar $saiu "Sair do jogo → Sim fecha o jogo"
if (-not $saiu) { $p.Kill() }

Write-Host ""
if ($falhas.Count -eq 0) { Write-Host "TUDO OK — fotos em $Saida"; exit 0 }
Write-Host "FALHAS: $($falhas -join '; ')"; exit 1
