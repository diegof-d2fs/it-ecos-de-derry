// Tela de teste do controle (Etapa 2, SDD §6). Mostra, ao vivo, cada botão por POSIÇÃO,
// os analógicos com a zona morta, os gatilhos e os COMANDOS que o jogo recebeu. Serve para
// provar que "reconhecido" também é "funcional" (lição nº 1) e, depois, entra no menu Controles.
#include <cstdio>
#include <string>

#include "cenas/cena.h"
#include "jogo/dialogo.h"
#include "render/texto.h"

namespace {

constexpr float TEMPO_PARA_SAIR = 1.0f;   // segurar Esc/Start: B e A precisam ser testáveis

const wchar_t* const NOMES_COMANDOS[] = {
    L"Cima", L"Baixo", L"Esquerda", L"Direita", L"Atacar", L"Especial", L"Clarão", L"Dash", L"Interagir",
    L"Usar kit", L"Inventário", L"Ficha", L"Mapa", L"Pausa", L"Confirmar", L"Voltar", L"Aba ←", L"Aba →",
    L"Tela cheia", L"Debug", L"Teste do controle", L"Piloto automático",
};
static_assert(sizeof(NOMES_COMANDOS) / sizeof(NOMES_COMANDOS[0]) == TOTAL_COMANDOS,
              "um nome para cada Comando, na mesma ordem");

const std::uint32_t COR_TEXTO = rgb(240, 232, 214);
const std::uint32_t COR_SECUNDARIA = rgb(190, 184, 170);
const std::uint32_t COR_APAGADO = rgb(42, 46, 58);
const std::uint32_t COR_BORDA = rgb(120, 118, 128);
const std::uint32_t COR_ACESO = rgb(179, 32, 42);
const std::uint32_t COR_OK = rgb(150, 230, 160);

// Um botão: caixa acesa quando apertado, com o rótulo centralizado.
void desenharBotao(Canvas& canvas, const Fonte& fonte, const InfoControle& c, BotaoControle b, float x, float y,
                   float w, float h, const wchar_t* rotulo) {
    const bool aceso = botao(c.leitura, b);
    retanguloCheio(canvas, x, y, w, h, aceso ? COR_ACESO : COR_APAGADO);
    contornoRetangulo(canvas, x, y, w, h, aceso ? COR_TEXTO : COR_BORDA);
    desenharTextoCentralizado(canvas, fonte, x + w / 2, y + (h - fonte.alturaLogica) / 2 - 1, rotulo, COR_TEXTO);
}

// Cruz de 4 botões ao redor de (cx, cy): cima, baixo, esquerda, direita.
void desenharCruz(Canvas& canvas, const Fonte& fonte, const InfoControle& c, float cx, float cy,
                  const BotaoControle (&b)[4], const wchar_t* const (&rotulos)[4]) {
    constexpr float L = 20.0f;
    desenharBotao(canvas, fonte, c, b[0], cx - L / 2, cy - L * 1.5f, L, L, rotulos[0]);
    desenharBotao(canvas, fonte, c, b[1], cx - L / 2, cy + L * 0.5f, L, L, rotulos[1]);
    desenharBotao(canvas, fonte, c, b[2], cx - L * 1.5f, cy - L / 2, L, L, rotulos[2]);
    desenharBotao(canvas, fonte, c, b[3], cx + L * 0.5f, cy - L / 2, L, L, rotulos[3]);
}

// Analógico: base (acesa com L3/R3), anel da zona morta e o ponto na posição bruta.
void desenharAnalogico(Canvas& canvas, Vec2 centro, Vec2 valor, bool apertado) {
    constexpr float RAIO = 22.0f;
    elipseAlpha(canvas, centro.x, centro.y, RAIO, RAIO, apertado ? COR_ACESO : COR_APAGADO, 255);
    elipseAlpha(canvas, centro.x, centro.y, RAIO * ZONA_MORTA_ANALOGICO, RAIO * ZONA_MORTA_ANALOGICO, rgb(0, 0, 0), 110);
    const bool foraDaZona = comprimento(valor) > ZONA_MORTA_ANALOGICO;
    elipseAlpha(canvas, centro.x + valor.x * RAIO, centro.y + valor.y * RAIO, 5, 5, foraDaZona ? COR_OK : COR_BORDA, 255);
}

// Barra horizontal de gatilho (0..1), com a marca do limiar de "apertado".
void desenharGatilho(Canvas& canvas, const Fonte& fonte, float x, float y, float valor, const wchar_t* rotulo) {
    constexpr float W = 70.0f, H = 9.0f;
    retanguloCheio(canvas, x, y, W, H, COR_APAGADO);
    retanguloCheio(canvas, x, y, W * valor, H, valor >= LIMIAR_GATILHO ? COR_ACESO : COR_BORDA);
    retanguloCheio(canvas, x + W * LIMIAR_GATILHO, y, 1, H, COR_TEXTO);
    contornoRetangulo(canvas, x, y, W, H, COR_BORDA);
    desenharTexto(canvas, fonte, x + W + 4, y - 2, rotulo, COR_SECUNDARIA);
}

// Duas linhas: a origem e um complemento (o nome do controle ou a dica de por que não há).
std::wstring origemTexto(const InfoControle& c) {
    if (c.origem == OrigemControle::Sdl) return L"Origem: SDL2 GameController";
    if (c.origem == OrigemControle::XInput) return L"Origem: XInput";
    return L"Origem: nenhum controle";
}
std::wstring complementoOrigem(const InfoControle& c) {
    if (c.origem != OrigemControle::Nenhuma) return L"Nome: " + paraWide(c.nome);
    if (c.sdlCarregada) return L"SDL2 pronta: conecte e aguarde até 2 s";
    if (c.xinputCarregado) return L"SDL2.dll ausente: só controles XInput";
    return L"Sem SDL2.dll nem XInput: só teclado";
}

}  // namespace

void entrarTesteControle(Jogo& jogo) {
    jogo.testeControle = EstadoTesteControle{};
}

void atualizarTesteControle(Jogo& jogo, const EstadoEntrada& entrada, float dt) {
    EstadoTesteControle& t = jogo.testeControle;
    if (!segurando(entrada, Comando::Pausa)) {
        t.soltouPausa = true;
        t.segurandoSaida = 0.0f;
        return;
    }
    if (!t.soltouPausa) return;
    t.segurandoSaida += dt;
    if (t.segurandoSaida >= TEMPO_PARA_SAIR) desempilharCena(jogo);
}

void desenharTesteControle(const Jogo& jogo, Canvas& canvas) {
    const Recursos& r = *jogo.recursos;
    const InfoControle& c = jogo.controle;
    const EstadoEntrada& e = jogo.entrada;

    retanguloAlpha(canvas, 0, 0, LARGURA_LOGICA, ALTURA_LOGICA, rgb(6, 7, 12), 252);
    desenharTextoCentralizado(canvas, r.fonteGrande, LARGURA_LOGICA / 2.0f, 12, L"Teste do controle", COR_TEXTO);
    desenharTextoCentralizado(canvas, r.fontePequena, LARGURA_LOGICA / 2.0f, 40,
                              L"Botões por POSIÇÃO (letras do Xbox). No 8BitDo vale o botão no mesmo lugar.",
                              COR_SECUNDARIA);

    // ---------- desenho do controle ----------
    const Fonte& fp = r.fontePequena;
    desenharGatilho(canvas, fp, 70, 66, c.leitura.gatilhoEsq, L"LT");
    desenharGatilho(canvas, fp, 290, 66, c.leitura.gatilhoDir, L"RT");
    desenharBotao(canvas, fp, c, BotaoControle::OmbroEsq, 70, 80, 70, 16, L"LB");
    desenharBotao(canvas, fp, c, BotaoControle::OmbroDir, 290, 80, 70, 16, L"RB");
    retanguloAlpha(canvas, 50, 104, 330, 170, rgb(20, 22, 30), 255);
    contornoRetangulo(canvas, 50, 104, 330, 170, COR_BORDA);

    const BotaoControle cruz[4] = {BotaoControle::Cima, BotaoControle::Baixo, BotaoControle::Esquerda,
                                   BotaoControle::Direita};
    const wchar_t* const setas[4] = {L"↑", L"↓", L"←", L"→"};
    desenharCruz(canvas, fp, c, 110, 160, cruz, setas);
    const BotaoControle face[4] = {BotaoControle::Norte, BotaoControle::Sul, BotaoControle::Oeste,
                                   BotaoControle::Leste};
    const wchar_t* const letras[4] = {L"Y", L"A", L"X", L"B"};
    desenharCruz(canvas, fp, c, 320, 160, face, letras);

    desenharBotao(canvas, fp, c, BotaoControle::Selecionar, 172, 136, 30, 14, L"Back");
    desenharBotao(canvas, fp, c, BotaoControle::Guia, 200, 158, 30, 14, L"Guia");
    desenharBotao(canvas, fp, c, BotaoControle::Iniciar, 228, 136, 30, 14, L"Start");

    desenharAnalogico(canvas, {165, 232}, c.leitura.analogicoEsq, botao(c.leitura, BotaoControle::AnalogicoEsq));
    desenharAnalogico(canvas, {265, 232}, c.leitura.analogicoDir, botao(c.leitura, BotaoControle::AnalogicoDir));

    // ---------- painel de diagnóstico ----------
    const float px = 396;
    float y = 66;
    retanguloAlpha(canvas, px - 8, y - 6, 246, 214, rgb(12, 14, 20), 255);
    contornoRetangulo(canvas, px - 8, y - 6, 246, 214, COR_BORDA);
    auto linha = [&](const std::wstring& texto, std::uint32_t cor) {
        desenharTexto(canvas, fp, px, y, texto, cor);
        y += 13;
    };
    linha(origemTexto(c), c.origem != OrigemControle::Nenhuma ? COR_OK : COR_SECUNDARIA);
    linha(complementoOrigem(c), c.origem != OrigemControle::Nenhuma ? COR_TEXTO : COR_SECUNDARIA);
    wchar_t buf[128];
    std::swprintf(buf, 128, L"Analógico esq.: %+.2f  %+.2f", c.leitura.analogicoEsq.x, c.leitura.analogicoEsq.y);
    linha(buf, COR_TEXTO);
    std::swprintf(buf, 128, L"Movimento (zona morta %.0f%%): %+.2f  %+.2f", ZONA_MORTA_ANALOGICO * 100, e.eixo.x,
                  e.eixo.y);
    linha(buf, COR_TEXTO);
    std::swprintf(buf, 128, L"Gatilhos: LT %.2f · RT %.2f", c.leitura.gatilhoEsq, c.leitura.gatilhoDir);
    linha(buf, COR_TEXTO);
    std::swprintf(buf, 128, L"Última busca por controle: %.2f ms", c.custoBuscaMs);
    linha(buf, COR_SECUNDARIA);
    linha(std::wstring(L"Último dispositivo usado: ") +
              (e.ultimoUsado == Dispositivo::Gamepad ? L"controle" : L"teclado"),
          COR_SECUNDARIA);
    y += 6;
    linha(L"Comandos que o jogo recebe agora:", COR_TEXTO);

    // Lista dos comandos segurados, quebrando a linha quando não cabe no painel.
    std::wstring atual;
    bool algum = false;
    for (int i = 0; i < TOTAL_COMANDOS; ++i) {
        if (!e.segurando[static_cast<size_t>(i)]) continue;
        algum = true;
        const std::wstring proximo = atual.empty() ? NOMES_COMANDOS[i] : atual + L" · " + NOMES_COMANDOS[i];
        if (larguraTexto(canvas, fp, proximo) > 228 && !atual.empty()) {
            linha(atual, COR_ACESO);
            atual = NOMES_COMANDOS[i];
        } else {
            atual = proximo;
        }
    }
    if (algum) linha(atual, rgb(240, 90, 90));
    else linha(L"(nenhum)", COR_SECUNDARIA);

    // ---------- rodapé: segurar para sair ----------
    const float progresso = limitar(jogo.testeControle.segurandoSaida / TEMPO_PARA_SAIR, 0.0f, 1.0f);
    retanguloCheio(canvas, 50, 300, 540, 6, COR_APAGADO);
    retanguloCheio(canvas, 50, 300, 540 * progresso, 6, COR_ACESO);
    // As letras do controle conectado (Xbox, PlayStation ou Nintendo; 24/11), não as do Xbox fixas.
    desenharTextoCentralizado(canvas, fp, LARGURA_LOGICA / 2.0f, 310,
                              paraWide(trocarBotoes("Segure Esc ou {PAUSA} por 1 s para sair · F2 também fecha", true)), COR_SECUNDARIA);
    desenharTextoCentralizado(canvas, fp, LARGURA_LOGICA / 2.0f, 323,
                              paraWide(trocarBotoes("{DASH}: dash · {CLARAO}: clarão · {ATACAR}: atacar · {ESPECIAL}: especial · "
                                                    "{CONFIRMAR}: confirmar · {VOLTAR}: voltar", true)),
                              COR_SECUNDARIA);
    desenharTextoCentralizado(canvas, fp, LARGURA_LOGICA / 2.0f, 336,
                              paraWide(trocarBotoes("{INTERAGIR}: interagir · {KIT}: kit · {INVENTARIO}: inventário · "
                                                    "{MAPA}: mapa ({FICHA}: ficha)", true)),
                              COR_SECUNDARIA);
}
