// IT: Ecos de Derry — ponto de entrada (Win32).
// Responsável por criar e liberar tudo (posse de memória: SDD §18) e rodar o loop
// de passo fixo (SDD §4): entrada → N passos lógicos de 1/60 s → desenho → apresentação.
#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <filesystem>
#include <string>

#include "cenas/cena.h"
#include "core/arquivo.h"
#include "core/sons.h"
#include "jogo/simulacao.h"
#include "plataforma/audio.h"
#include "plataforma/gamepad.h"
#include "plataforma/janela.h"
#include "plataforma/teclado.h"
#include "plataforma/tempo.h"
#include "render/canvas.h"
#include "render/recursos.h"
#include "jogo/dialogo.h"

namespace {

std::wstring pastaDoExecutavel() {
    wchar_t caminho[MAX_PATH]{};
    GetModuleFileNameW(nullptr, caminho, MAX_PATH);
    std::wstring s = caminho;
    return s.substr(0, s.find_last_of(L"\\/") + 1);
}

// Opções de linha de comando (uso em testes, documentação e defesa).
//   --captura arquivo.bmp   roda --quadros passos lógicos, salva o quadro e as métricas e fecha
//   --captura-evento atlas  captura assim que o ataque desse inimigo aparece; acerto_chefe captura
//                           o impacto e o tremor de um golpe de Bill no chefe (até --quadros)
//   --quadros N             (padrão 180)
//   --andar                 Bill anda sozinho num percurso fixo (testa animação e câmera)
//   --bioma N               ambiente inicial da vitrine (0..8)
//   --vitrine               abre a vitrine de assets (Etapa 1) em vez da sala
//   --sala N | ID           sala AVULSA: índice em data/salas (ordem alfabética) ou o id do arquivo
//                           (ex.: --sala b5_chefe); sem ela, começa a tentativa
//   --inimigo atlas         em --sala de combate, força só esse inimigo perto de Bill (revisão de efeitos)
//   --amigo codigo          em --sala, o amigo (mike, ben, stan, eddie, beverly, richie) começa ao lado do Bill
//   --bioma-inicial N       a tentativa começa no bioma N (1..7), com Bill chegando como chegaria ali (apresentação)
//   --captura-limpa         com --captura: sem medidor, caixas de colisão e avisos (imagens para o README)
//   --fase-covil N          Modo Defesa: em --sala b7_covil, o Pennywise IV começa na fase 2 (Georgie) ou 3 (Luzes Mortas)
//   --forte                 Modo Defesa: na sala avulsa, Bill chega como na tentativa daquele bioma
//                           (nível, atributos, kit e vida). Ex.: --sala b5_chefe --forte --piloto
//   --pennywise             Modo Defesa: na arena (--sala bN_chefe), o medo surge e já se rasga no
//                           Pennywise daquele estágio (fase 2, metade da vida)
//   --teste-controle        abre direto a tela de teste do controle
//   --simular-controle      leituras do joystick vêm de um roteiro fixo (captura sem controle físico)
//   --seed N                seed da partida (mesma seed = mesma partida); sem ela, sorteia
//   --piloto                o computador começa jogando (F10 alterna)
//   --dificuldade D         facil | normal | dificil (padrão normal; o menu da Etapa 7 vai escolher)
//   --simular N             SEM JANELA: o piloto joga N tentativas (seeds a partir de --seed, padrão 1) na
//                           --dificuldade escolhida e grava saves\simulacao*.csv e o resumo (balanceamento)
//   --saves PASTA           grava recordes em outra pasta (testes e capturas não sujam os reais)
//   --jogar                 pula o menu e começa direto uma tentativa (--sala e --piloto também pulam)
//   --itens                 Modo Defesa: começa com escudo, 2 melhorias, kit, Ficha, Tônico e Óculos
//   --pontos N              Modo Defesa: começa com N pontos de atributo para distribuir (C abre a ficha)
//   --roteiro "cmd:s,..."   comandos com duração em segundos, em sequência (testes e capturas sem
//                           depender do foco da janela). Ex.: "direita:0.4,cima:1.3,interagir:0.05,espera:1"
//                           Nomes: cima baixo esquerda direita atacar especial clarao dash interagir kit
//                           inventario ficha pausa confirmar voltar espera (vários juntos: "direita+cima:1")
//   --mudo                  sem áudio (testes automáticos; o modo --captura já é mudo)
//   --diag-audio arq.txt    roda --quadros passos COM áudio, grava o estado do áudio e fecha
//   --cena ID               abre a cena da história ID (data/cenas.txt: abertura, bioma_2..bioma_7, final)
//                           por cima do menu; ao terminar, volta ao menu (demonstração e capturas)
struct OpcoesExecucao {
    std::wstring arquivoCaptura;
    std::string eventoCaptura;
    int quadros = 180;
    bool andarSozinho = false;
    int bioma = 0;
    bool testeControle = false;
    bool simularControle = false;
    bool vitrine = false;
    int sala = -1;               // −1 = tentativa (o jogo de verdade); N = sala avulsa
    bool mudo = false;
    bool piloto = false;
    Dificuldade dificuldade = Dificuldade::Normal;
    std::uint64_t seed = 0;
    bool temSeed = false;
    std::wstring arquivoDiagAudio;
    std::wstring roteiro;
    int pontosIniciais = 0;
    bool forte = false;
    bool capturaLimpa = false;   // --captura-limpa: sem medidor, caixas e avisos (imagens do README)
    bool pennywise = false;
    int faseCovil = 1;
    std::string inimigoDefesa;
    std::string amigoDefesa;
    int biomaInicial = 1;
    std::wstring idSala;      // --sala com um id (resolvido depois de carregar as salas)
    std::wstring pastaSaves;
    int simular = 0;
    float reacao = ATRASO_REACAO;   // --reacao S: proxy de jogador mais lento na simulação
    int distracao = 0;              // --distracao P: % dos avisos que o piloto não vê (proxy de pessoa)
    bool itensDefesa = false;
    std::vector<std::string> ecos;   // --eco "Nome" (repetível): Ecos já ativos ao começar
    bool jogar = false;
    bool telaCheiaCaptura = false;   // --tela-cheia: a captura em tela cheia (mede o que se vê no monitor)
    std::string cena;                // --cena ID: abre uma cena da história sobre o menu
};

// ---------- --roteiro: comandos programados (ferramenta de teste) ----------
struct PassoRoteiro {
    std::vector<Comando> comandos;   // vazio = esperar
    float inicio = 0.0f, fim = 0.0f; // segundos de lógica
};

bool comandoDoNome(const std::wstring& nome, Comando& c) {
    static const struct { const wchar_t* nome; Comando comando; } NOMES[] = {
        {L"cima", Comando::Cima}, {L"baixo", Comando::Baixo}, {L"esquerda", Comando::Esquerda},
        {L"direita", Comando::Direita}, {L"atacar", Comando::Atacar}, {L"especial", Comando::Especial},
        {L"clarao", Comando::Clarao}, {L"dash", Comando::Dash}, {L"interagir", Comando::Interagir},
        {L"kit", Comando::UsarKit}, {L"inventario", Comando::Inventario}, {L"ficha", Comando::Ficha},
        {L"pausa", Comando::Pausa},
        {L"confirmar", Comando::Confirmar}, {L"voltar", Comando::Voltar},
    };
    for (const auto& n : NOMES) {
        if (nome == n.nome) { c = n.comando; return true; }
    }
    return false;
}

std::vector<PassoRoteiro> lerRoteiro(const std::wstring& texto) {
    std::vector<PassoRoteiro> passos;
    float t = 0.0f;
    std::size_t i = 0;
    while (i < texto.size()) {
        std::size_t fim = texto.find(L',', i);
        if (fim == std::wstring::npos) fim = texto.size();
        const std::wstring item = texto.substr(i, fim - i);
        i = fim + 1;
        const std::size_t dois = item.find(L':');
        if (dois == std::wstring::npos) continue;
        PassoRoteiro p;
        std::wstring nomes = item.substr(0, dois);
        std::size_t j = 0;
        while (j <= nomes.size()) {
            std::size_t mais = nomes.find(L'+', j);
            if (mais == std::wstring::npos) mais = nomes.size();
            Comando c;
            if (comandoDoNome(nomes.substr(j, mais - j), c)) p.comandos.push_back(c);
            j = mais + 1;
        }
        p.inicio = t;
        t += static_cast<float>(_wtof(item.substr(dois + 1).c_str()));
        p.fim = t;
        passos.push_back(p);
    }
    return passos;
}

// Substitui a entrada do quadro pelos comandos do roteiro no instante `tempo` (bordas incluídas).
void aplicarRoteiro(const std::vector<PassoRoteiro>& roteiro, float tempo, EstadoEntrada& e,
                    std::array<bool, TOTAL_COMANDOS>& anterior) {
    std::array<bool, TOTAL_COMANDOS> agora{};
    for (const PassoRoteiro& p : roteiro) {
        if (tempo >= p.inicio && tempo < p.fim) {
            for (Comando c : p.comandos) agora[static_cast<int>(c)] = true;
        }
    }
    for (int c = 0; c < TOTAL_COMANDOS; ++c) {
        e.segurando[static_cast<std::size_t>(c)] = agora[static_cast<std::size_t>(c)];
        e.pressionou[static_cast<std::size_t>(c)] = e.pressionou[static_cast<std::size_t>(c)] ||
                                                     (agora[static_cast<std::size_t>(c)] && !anterior[static_cast<std::size_t>(c)]);
    }
    e.eixo = eixoDosDirecionais(e);
    anterior = agora;
}

OpcoesExecucao lerOpcoes() {
    OpcoesExecucao opcoes;
    int total = 0;
    LPWSTR* args = CommandLineToArgvW(GetCommandLineW(), &total);
    for (int i = 1; args && i < total; ++i) {
        const std::wstring a = args[i];
        if (a == L"--captura" && i + 1 < total) opcoes.arquivoCaptura = args[++i];
        else if (a == L"--captura-evento" && i + 1 < total) {
            for (wchar_t c : std::wstring(args[++i])) if (c < 128) opcoes.eventoCaptura += static_cast<char>(c);
        }
        else if (a == L"--quadros" && i + 1 < total) opcoes.quadros = _wtoi(args[++i]);
        else if (a == L"--andar") opcoes.andarSozinho = true;
        else if (a == L"--bioma" && i + 1 < total) opcoes.bioma = _wtoi(args[++i]);
        else if (a == L"--teste-controle") opcoes.testeControle = true;
        else if (a == L"--simular-controle") opcoes.simularControle = true;
        else if (a == L"--vitrine") opcoes.vitrine = true;
        else if (a == L"--mudo") opcoes.mudo = true;
        else if (a == L"--piloto") opcoes.piloto = true;
        else if (a == L"--seed" && i + 1 < total) { opcoes.seed = _wcstoui64(args[++i], nullptr, 10); opcoes.temSeed = true; }
        else if (a == L"--diag-audio" && i + 1 < total) opcoes.arquivoDiagAudio = args[++i];
        else if (a == L"--sala" && i + 1 < total) {
            const std::wstring v = args[++i];
            if (!v.empty() && iswdigit(v[0])) opcoes.sala = _wtoi(v.c_str());
            else opcoes.idSala = v;   // ex.: b5_chefe
        }
        else if (a == L"--forte") opcoes.forte = true;
        else if (a == L"--captura-limpa") opcoes.capturaLimpa = true;
        else if (a == L"--tela-cheia") opcoes.telaCheiaCaptura = true;
        else if (a == L"--cena" && i + 1 < total) {
            for (wchar_t c : std::wstring(args[++i])) if (c < 128) opcoes.cena += static_cast<char>(c);
        }
        else if (a == L"--pennywise") opcoes.pennywise = true;
        else if (a == L"--fase-covil" && i + 1 < total) opcoes.faseCovil = _wtoi(args[++i]);
        else if (a == L"--bioma-inicial" && i + 1 < total) { opcoes.biomaInicial = _wtoi(args[++i]); opcoes.jogar = true; }
        else if (a == L"--amigo" && i + 1 < total) {
            for (wchar_t c : std::wstring(args[++i])) if (c < 128) opcoes.amigoDefesa += static_cast<char>(c);
        }
        else if (a == L"--inimigo" && i + 1 < total) {
            for (wchar_t c : std::wstring(args[++i])) if (c < 128) opcoes.inimigoDefesa += static_cast<char>(c);
        }
        else if (a == L"--roteiro" && i + 1 < total) opcoes.roteiro = args[++i];
        else if (a == L"--pontos" && i + 1 < total) opcoes.pontosIniciais = _wtoi(args[++i]);
        else if (a == L"--saves" && i + 1 < total) opcoes.pastaSaves = args[++i];
        else if (a == L"--simular" && i + 1 < total) opcoes.simular = _wtoi(args[++i]);
        else if (a == L"--reacao" && i + 1 < total) opcoes.reacao = static_cast<float>(_wtof(args[++i]));
        else if (a == L"--distracao" && i + 1 < total) opcoes.distracao = _wtoi(args[++i]);
        else if (a == L"--itens") opcoes.itensDefesa = true;
        else if (a == L"--eco" && i + 1 < total) {   // UTF-8 do nome (os Ecos têm acento: "Remédio")
            std::string nome;
            for (wchar_t w : std::wstring(args[++i])) {
                const unsigned u = static_cast<unsigned>(w);
                if (u < 0x80) nome += static_cast<char>(u);
                else if (u < 0x800) { nome += static_cast<char>(0xC0 | (u >> 6)); nome += static_cast<char>(0x80 | (u & 0x3F)); }
                else { nome += static_cast<char>(0xE0 | (u >> 12)); nome += static_cast<char>(0x80 | ((u >> 6) & 0x3F)); nome += static_cast<char>(0x80 | (u & 0x3F)); }
            }
            opcoes.ecos.push_back(nome);
        }
        else if (a == L"--jogar") opcoes.jogar = true;
        else if (a == L"--dificuldade" && i + 1 < total) {
            const std::wstring v = args[++i];
            dificuldadeDoCodigo(std::string(v.begin(), v.end()), opcoes.dificuldade);   // inválido: fica Normal
        }
    }
    LocalFree(args);
    return opcoes;
}

// Percurso fixo do modo --andar: direita, baixo, esquerda, cima (60 passos cada).
Vec2 eixoAutomatico(int passo) {
    const Vec2 percurso[] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    return percurso[(passo / 60) % 4];
}

void gravarMetricas(const std::wstring& arquivoBmp, const Jogo& jogo) {
    FILE* f = _wfopen((arquivoBmp + L".txt").c_str(), L"w");
    if (!f) return;
    std::fprintf(f, "quadros_medidos=%d\nmedia_ms=%.3f\np95_ms=%.3f\nescala=%d\n", jogo.medidor.total,
                 mediaMs(jogo.medidor), percentil95Ms(jogo.medidor), jogo.escalaAtual);
    std::fclose(f);
}

// Maior escala de render: 6× = 3840 × 2160 (4K). Acima disso só sobraria margem.
constexpr int ESCALA_MAXIMA = 6;

// O canvas tem 640×360 × escala pixels de tela. Quando a janela muda de escala, recria o
// canvas e prepara as imagens nessa escala (uma vez; nunca a cada quadro). Janela pequena
// demais (escala 0) mantém o canvas atual: a apresentação mostra só o aviso.
bool ajustarEscala(const EstadoJanela& janela, Canvas& canvas, Recursos& recursos) {
    int escala = calcularApresentacao(janela.clienteW, janela.clienteH, LARGURA_LOGICA, ALTURA_LOGICA, ESCALA_MAXIMA).escala;
    if (escala == 0) {
        if (canvas.dc) return true;
        escala = 1;
    }
    if (escala == canvas.escala && canvas.dc) return true;
    liberarCanvas(canvas);
    return criarCanvas(canvas, LARGURA_LOGICA, ALTURA_LOGICA, escala) && prepararEscala(recursos, escala);
}

void erroFatal(const std::wstring& mensagem) {
    MessageBoxW(nullptr, mensagem.c_str(), L"IT: Ecos de Derry", MB_ICONERROR | MB_OK);
}

// Lê teclado e joystick, soma os dois (funcionam juntos) e calcula bordas e eixo.
// Sem foco, nada entra (o joystick continua sendo lido só para perceber conexão/desconexão).
// `usarControle` = false nas capturas e no diagnóstico: o controle físico é global no Windows, e quem estivesse
// jogando em outra janela apertava botões dentro da captura (24/11).
void lerEntrada(const EstadoJanela& janela, Jogo& jogo, bool aceitarSemFoco, bool usarControle) {
    const auto anterior = jogo.entrada.segurando;
    jogo.entrada.segurando.fill(false);

    const bool tinhaControle = jogo.controle.origem != OrigemControle::Nenhuma;
    atualizarGamepad(jogo.controle, agoraSegundos());
    const bool temControle = jogo.controle.origem != OrigemControle::Nenhuma;
    jogo.entrada.gamepadConectado = temControle;
    {   // Letras do controle conectado: Xbox, PlayStation ou Nintendo (Switch Pro, 8BitDo no modo Switch)
        const std::string& n = jogo.controle.nome;
        definirEstiloControle(temControle ? estiloDoControle(n) : EstiloControle::Xbox);
        definirConfirmarNintendo(temControle && estiloDoControle(n) == EstiloControle::Nintendo);
    }
    if (temControle && !tinhaControle && !jogo.semAvisos) {
        mostrarAviso(jogo, L"Controle conectado: " + paraWide(jogo.controle.nome), 3.0f);
    } else if (!temControle && tinhaControle) {
        mostrarAviso(jogo, L"Controle desconectado", 4.0f);
        // Controle caiu no meio da partida: pausa (PRD §17), para ninguém morrer sem poder jogar.
        if (cenaDoTopo(jogo) == IdCena::Sala) abrirPausa(jogo.salaAtual.partida);
    }

    Vec2 analogico;
    if (janela.focada || aceitarSemFoco) {
        if (janela.focada) aplicarTeclado(janela.teclas, jogo.entrada);
        if (temControle && usarControle) analogico = aplicarControle(jogo.controle.leitura, jogo.entrada);
    } else {
        jogo.controle.leitura = LeituraControle{};
    }
    combinarEixo(jogo.entrada, analogico);
    calcularBordas(jogo.entrada, anterior);
}

// Comandos globais que valem em qualquer cena.
void tratarComandosGlobais(EstadoJanela& janela, Jogo& jogo) {
    if (pressionou(jogo.entrada, Comando::TelaCheia)) alternarTelaCheia(janela);
    if (pressionou(jogo.entrada, Comando::Debug)) jogo.mostrarMedidor = !jogo.mostrarMedidor;
    if (pressionou(jogo.entrada, Comando::Piloto)) jogo.pilotoAtivo = !jogo.pilotoAtivo;
    const IdCena topo = cenaDoTopo(jogo);
    if (pressionou(jogo.entrada, Comando::TesteControle)) {
        if (topo == IdCena::TesteControle) desempilharCena(jogo);
        else empilharCena(jogo, IdCena::TesteControle);
    } else if (topo == IdCena::Vitrine && pressionou(jogo.entrada, Comando::Pausa)) {
        jogo.rodando = false;   // vitrine (ferramenta): Esc fecha. Na sala, Esc/Start abrem a PAUSA (regras).
    }
    // Consome as bordas: com monitor acima de 60 Hz há quadros de desenho sem passo lógico,
    // e o mesmo toque não pode alternar duas vezes.
    for (Comando c : {Comando::TelaCheia, Comando::Debug, Comando::TesteControle, Comando::Piloto}) {
        jogo.entrada.pressionou[static_cast<int>(c)] = false;
    }
}

// Salas + balanceamento. Um erro de digitação num arquivo volta com linha e coluna.
bool carregarConteudo(const std::wstring& pasta, Conteudo& conteudo, std::string& erro) {
    if (!carregarSalas(std::filesystem::path(pasta) / L"data" / L"salas", conteudo.salas, erro)) {
        erro = "salas: " + erro;
        return false;
    }
    // Diálogos e tutorial (Etapa 7.3). Erro de formato é fatal (dado nosso: melhor ver já).
    if (!carregarDialogos(std::filesystem::path(pasta) / L"data" / L"dialogos.txt", conteudo.dialogos, erro)) {
        erro = "diálogos: " + erro;
        return false;
    }
    // Cenas da história (cut scenes, 23/11). Sem o arquivo, o jogo segue sem elas.
    if (!carregarCenasHistoria(std::filesystem::path(pasta) / L"data" / L"cenas.txt", conteudo.cenas, erro)) return false;
    // Dificuldade (SDD §24.2). Sem o arquivo, vale a tabela padrão.
    std::string texto;
    if (lerArquivoTexto(std::filesystem::path(pasta) / L"data" / L"balanceamento.txt", texto) &&
        !lerBalanceamento(texto, conteudo.dificuldades, erro)) {
        erro = "data/balanceamento.txt: " + erro;
        return false;
    }
    return true;
}

// --simular N: sem janela. Grava os CSVs e mostra o resumo no terminal que chamou (se houver).
int executarSimulacao(const OpcoesExecucao& opcoes, const std::wstring& pasta) {
    static Conteudo conteudo;
    std::string erro;
    if (!carregarConteudo(pasta, conteudo, erro)) {
        erroFatal(L"Falha ao carregar o conteúdo:\n" + paraWide(erro));
        return 1;
    }
    const std::uint64_t base = opcoes.temSeed ? opcoes.seed : 1;
    std::vector<ResultadoSimulacao> resultados;
    for (int i = 0; i < opcoes.simular; ++i) {
        resultados.push_back(simularTentativa(conteudo, base + static_cast<std::uint64_t>(i), opcoes.dificuldade, LIMITE_SIMULACAO_S, opcoes.reacao, opcoes.distracao));
    }
    const std::filesystem::path saves = opcoes.pastaSaves.empty() ? std::filesystem::path(pasta) / L"saves"
                                                                  : std::filesystem::path(opcoes.pastaSaves);
    const std::string resumo = resumoSimulacao(resultados);
    const bool ok = gravarArquivoTexto(saves / L"simulacao.csv", csvTentativas(resultados)) &&
                    gravarArquivoTexto(saves / L"simulacao_salas.csv", csvSalas(resultados)) &&
                    gravarArquivoTexto(saves / L"simulacao_resumo.txt", resumo);
    // Programa de janela não tem console: empresta o do terminal que chamou, se houver.
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        const std::wstring texto = L"\n" + paraWide(resumo) + L"  arquivos em " + saves.wstring() + L"\n";
        DWORD escritos = 0;
        WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), texto.c_str(), static_cast<DWORD>(texto.size()), &escritos, nullptr);
        FreeConsole();
    }
    return ok ? 0 : 1;
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instancia, HINSTANCE, PWSTR, int) {
    SetProcessDPIAware();   // sem isso o Windows amplia a janela e borra os pixels
    iniciarRelogio();
    const OpcoesExecucao opcoes = lerOpcoes();
    if (opcoes.simular > 0) return executarSimulacao(opcoes, pastaDoExecutavel());   // sem janela
    if (!iniciarDecodificadorPng()) {
        erroFatal(L"Não foi possível iniciar o GDI+.");
        return 1;
    }

    static Recursos recursos;   // estático: vários MB de imagens fora da pilha
    std::string erro;
    const std::wstring pasta = pastaDoExecutavel();
    if (!carregarRecursos(recursos, pasta + L"assets\\jogo_hd\\", pasta + L"assets\\jogo\\", erro)) {
        erroFatal(L"Falha ao carregar os recursos:\n" + paraWide(erro));
        encerrarDecodificadorPng();
        return 1;
    }

    EstadoJanela janela;
    Canvas canvas;
    if (!criarJanela(janela, instancia, L"IT: Ecos de Derry") || !ajustarEscala(janela, canvas, recursos)) {
        erroFatal(L"Não foi possível criar a janela.");
        liberarRecursos(recursos);
        encerrarDecodificadorPng();
        return 1;
    }

    const bool modoCaptura = !opcoes.arquivoCaptura.empty();
    // Testes automáticos rodam sem ninguém clicar na janela: a lógica não pausa por falta de foco.
    const bool ignorarFoco = modoCaptura || !opcoes.arquivoDiagAudio.empty();
    int passosExecutados = 0;
    const std::vector<PassoRoteiro> roteiro = lerRoteiro(opcoes.roteiro);
    std::array<bool, TOTAL_COMANDOS> roteiroAnterior{};

    // Conteúdo de jogo (salas e balanceamento).
    static Conteudo conteudo;
    if (!carregarConteudo(pasta, conteudo, erro)) {
        erroFatal(L"Falha ao carregar o conteúdo:\n" + paraWide(erro));
        liberarCanvas(canvas);
        DestroyWindow(janela.hwnd);
        liberarRecursos(recursos);
        encerrarDecodificadorPng();
        return 1;
    }

    static Jogo jogo;
    jogo.recursos = &recursos;
    jogo.conteudo = &conteudo;
    jogo.mostrarMedidor = modoCaptura && !opcoes.capturaLimpa;
    jogo.semAvisos = opcoes.capturaLimpa;
    jogo.ambienteInicial = opcoes.bioma;
    jogo.salaInicial = opcoes.sala;
    if (!opcoes.idSala.empty()) {   // --sala b5_chefe: o índice do arquivo com esse id
        for (int i = 0; i < static_cast<int>(conteudo.salas.size()); ++i) {
            if (paraWide(conteudo.salas[static_cast<std::size_t>(i)].id) == opcoes.idSala) jogo.salaInicial = i;
        }
        if (jogo.salaInicial < 0) erroFatal(L"--sala: não existe sala com o id " + opcoes.idSala);
    }
    jogo.forte = opcoes.forte;
    jogo.pennywise = opcoes.pennywise;
    jogo.faseCovil = opcoes.faseCovil;
    jogo.inimigoDefesa = opcoes.inimigoDefesa;
    jogo.amigoDefesa = opcoes.amigoDefesa;
    jogo.biomaInicial = opcoes.biomaInicial;
    jogo.pilotoAtivo = opcoes.piloto;
    jogo.dificuldade = opcoes.dificuldade;
    jogo.pontosIniciais = opcoes.pontosIniciais;
    jogo.itensDefesa = opcoes.itensDefesa;
    jogo.ecosDefesa = opcoes.ecos;
    jogo.pastaSaves = opcoes.pastaSaves.empty() ? pasta + L"saves" : opcoes.pastaSaves;
    jogo.perfil = carregarPerfil(std::filesystem::path(jogo.pastaSaves) / L"perfil.txt");
    // Seed: fixa nos testes por captura (reprodutível); senão, o relógio escolhe uma nova a cada execução.
    jogo.seed = opcoes.temSeed ? opcoes.seed : (modoCaptura ? 12345u : static_cast<std::uint64_t>(GetTickCount64()));
    iniciarGamepad(pasta);
    simularGamepad(opcoes.simularControle);
    // Áudio: opcional. Arquivos ausentes só ficam mudos (a lista vai para o depurador).
    if (!modoCaptura && !opcoes.mudo) {
        std::string textoSons, avisosAudio;
        TabelaSons tabelaSons;
        if (lerArquivoTexto(std::filesystem::path(pasta) / L"data" / L"sons.txt", textoSons) &&
            lerTabelaSons(textoSons, tabelaSons, erro)) {
            iniciarAudio(pasta + L"assets\\", tabelaSons, avisosAudio);
        } else {
            avisosAudio = "data/sons.txt: " + erro;
        }
        if (!avisosAudio.empty()) OutputDebugStringW(paraWide("[audio] " + avisosAudio).c_str());
    }

    // As cenas entram DEPOIS do áudio: a sala pede a música ao entrar.
    // Abre no MENU; --sala, --jogar e --piloto pulam direto para a partida (testes e capturas).
    const bool direto = jogo.salaInicial >= 0 || opcoes.jogar || opcoes.piloto;
    if ((!modoCaptura && jogo.perfil.opcoes.telaCheia) || opcoes.telaCheiaCaptura) alternarTelaCheia(janela);
    {
        const OpcoesJogo& o = jogo.perfil.opcoes;
        definirVolumes(o.volumeGeral / 100.0f, o.volumeMusica / 100.0f, o.volumeEfeitos / 100.0f);
    }
    trocarCenaBase(jogo, opcoes.vitrine ? IdCena::Vitrine : (direto ? IdCena::Sala : IdCena::Menu));
    if (opcoes.testeControle) empilharCena(jogo, IdCena::TesteControle);
    if (!opcoes.cena.empty() && !abrirCenaHistoria(jogo, opcoes.cena)) erroFatal(L"--cena: não existe a cena " + paraWide(opcoes.cena));

    double anterior = agoraSegundos();
    double acumulador = 0.0;
    while (jogo.rodando) {
        const double inicioQuadro = agoraSegundos();
        processarMensagens(janela);
        if (janela.pedidoFechar) break;

        lerEntrada(janela, jogo, ignorarFoco, !ignorarFoco || opcoes.simularControle);
        tratarComandosGlobais(janela, jogo);
        if (jogo.pedidoTelaCheia) {   // pedido pelas Opções do menu
            jogo.pedidoTelaCheia = false;
            if (janela.telaCheia != jogo.perfil.opcoes.telaCheia) alternarTelaCheia(janela);
        }
        if (opcoes.andarSozinho) jogo.entrada.eixo = eixoAutomatico(passosExecutados);
        if (!roteiro.empty()) aplicarRoteiro(roteiro, static_cast<float>(passosExecutados) * DT, jogo.entrada, roteiroAnterior);

        const Apresentacao apresentacao = apresentacaoAtual(janela);
        if (!ajustarEscala(janela, canvas, recursos)) {
            erroFatal(L"Memória insuficiente para a nova escala da janela.");
            break;
        }
        jogo.escalaAtual = apresentacao.escala;
        // Sem foco ou janela pequena demais: a lógica para (inimigos não agem sozinhos).
        // No modo captura o foco é ignorado, para rodar sem ninguém clicar.
        jogo.pausadoPorFoco = !janela.focada && !ignorarFoco;
        const bool logicaParada = jogo.pausadoPorFoco || apresentacao.escala == 0;

        const double agora = agoraSegundos();
        double decorrido = agora - anterior;
        anterior = agora;
        if (decorrido > 0.25) decorrido = 0.25;   // após travadas longas, não "corre atrás"
        if (logicaParada) {
            acumulador = 0.0;
            limparEntrada(jogo.entrada);
        } else {
            acumulador += decorrido;
            int passos = 0;
            while (acumulador >= DT && passos < 5) {
                atualizarCenas(jogo, DT);
                limparBordas(jogo.entrada);   // cada toque vale para um único passo lógico
                jogo.tempoTotal += DT;
                acumulador -= DT;
                ++passos;
                ++passosExecutados;
            }
        }

        desenharCenas(jogo, canvas);
        GdiFlush();
        registrarQuadro(jogo.medidor, static_cast<float>((agoraSegundos() - inicioQuadro) * 1000.0));
        apresentarCanvas(janela, canvas);
        bool eventoEncontrado = false;
        if (modoCaptura && !opcoes.eventoCaptura.empty()) {
            const EstadoPartida& p = jogo.salaAtual.partida;
            if (opcoes.eventoCaptura == "acerto_chefe") {
                bool chefeEmImpacto = false, impactoVisivel = false;
                for (const Inimigo& i : p.inimigos)
                    if (parametrosDe(i.tipo).chefe && i.flash > 0.0f) chefeEmImpacto = true;
                for (const EfeitoAtaque& e : p.efeitosAtaque)
                    if (e.atacante == TipoInimigo::Total && e.vida > 0.0f) impactoVisivel = true;
                eventoEncontrado = chefeEmImpacto && impactoVisivel;
            } else {
                for (const EfeitoAtaque& e : p.efeitosAtaque) {
                    if (e.atacante != TipoInimigo::Total && e.vida > 0.0f &&
                        opcoes.eventoCaptura == parametrosDe(e.atacante).atlas) eventoEncontrado = true;
                }
            }
        }
        if (modoCaptura && (eventoEncontrado || passosExecutados >= opcoes.quadros)) {
            salvarBmp(canvas, opcoes.arquivoCaptura.c_str());
            gravarMetricas(opcoes.arquivoCaptura, jogo);
            if (!opcoes.eventoCaptura.empty()) {
                if (FILE* f = _wfopen((opcoes.arquivoCaptura + L".txt").c_str(), L"a")) {
                    std::fprintf(f, "evento=%s\nencontrado=%d\n", opcoes.eventoCaptura.c_str(), eventoEncontrado ? 1 : 0);
                    std::fclose(f);
                }
            }
            jogo.rodando = false;
        }
        atualizarAudio(static_cast<float>(decorrido), janela.focada || ignorarFoco);
        if (!opcoes.arquivoDiagAudio.empty() && passosExecutados >= opcoes.quadros) {
            if (FILE* f = _wfopen(opcoes.arquivoDiagAudio.c_str(), L"w")) {
                std::fputs(diagnosticoAudio().c_str(), f);
                std::fclose(f);
            }
            jogo.rodando = false;
        }
        esperarProximoQuadro(inicioQuadro, DT);
    }

    encerrarGamepad();
    encerrarAudio();
    liberarCacheTerreno(jogo.salaAtual.cache);
    liberarCanvas(canvas);
    DestroyWindow(janela.hwnd);
    liberarRecursos(recursos);
    encerrarDecodificadorPng();
    encerrarRelogio();
    return 0;
}
