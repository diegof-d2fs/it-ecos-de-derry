#pragma once
// Pontuação (PRD §16 + SDD §24.3; critérios F04, F13, M03). A fórmula existe numa ÚNICA
// função, usada pelo HUD, pela tela de resultado, pelo menu de explicação e pelos testes.
//
//   bruto = 100K + 300E + 50I + 500R + 1000B + 250C + 5000V
//         + V·máx(0, 9000 − 2T)          ← bônus de vitória rápida (T em segundos)
//         − ⌊M/2⌋ − 1000U − ⌊T/5⌋        ← movimento, vidas e tempo
//   S = máx(0, ⌊bruto × D⌋)              ← D = multiplicador da dificuldade (em %)

struct Estatisticas {
    int K = 0;          // inimigos comuns derrotados (sem chefes nem invocações)
    int E = 0;          // elites derrotados
    int I = 0;          // itens coletados do chão ou de baús (compras não contam)
    int R = 0;          // amigos resgatados
    int B = 0;          // chefes-medo dispersados
    int C = 0;          // salas concluídas
    int V = 0;          // 1 se derrotou o Pennywise no Covil
    int M = 0;          // movimentos: células novas pisadas pelo Bill
    int U = 0;          // vidas utilizadas: pactos consumidos + 1 se terminou em morte
    float tempo = 0.0f; // T: segundos de JOGO (só passos de lógica; pausa e menus não contam)
};

constexpr int BONUS_VITORIA_RAPIDA = 9000;   // pontos se vencer em T = 0
constexpr int SEGUNDOS_POR_PONTO = 5;        // −1 ponto a cada 5 s

// Segundos inteiros de T (o que entra na fórmula e o que o cronômetro mostra).
int segundosDeJogo(const Estatisticas& s);

// `multiplicadorPct` = D em porcentagem (75, 100, 150).
long long calcularPontuacao(const Estatisticas& s, int multiplicadorPct);

// Parcelas da fórmula, para a tela de resultado mostrar "de onde veio cada ponto".
struct ParcelasPontuacao {
    long long inimigos, elites, itens, resgates, chefes, salas, vitoria, vitoriaRapida;
    long long movimento, vidas, tempo;   // valores positivos que são SUBTRAÍDOS
    long long bruto;                     // soma com sinais
    int multiplicadorPct;
    long long total;                     // = calcularPontuacao
};
ParcelasPontuacao detalharPontuacao(const Estatisticas& s, int multiplicadorPct);
