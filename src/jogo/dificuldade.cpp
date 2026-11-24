#include "jogo/dificuldade.h"

#include <sstream>

namespace {

// Uma linha do arquivo ↔ um campo da struct. Tabela de ponteiros para membro: acrescentar um
// parâmetro novo = uma linha aqui + uma linha no arquivo.
struct CampoBalanceamento {
    const char* chave;
    int ParametrosDificuldade::*campo;
};

const CampoBalanceamento CAMPOS[] = {
    {"orcamento_onda_pct", &ParametrosDificuldade::orcamentoOnda},
    {"peso_inimigos_fortes_pct", &ParametrosDificuldade::pesoFortes},
    {"chance_elite_pct", &ParametrosDificuldade::chanceElite},
    {"chance_drop_pct", &ParametrosDificuldade::chanceDrop},
    {"peso_kit_bateria_pct", &ParametrosDificuldade::pesoKit},
    {"chance_roubo_rato_pct", &ParametrosDificuldade::chanceRoubo},
    {"armadilhas_opcionais_pct", &ParametrosDificuldade::armadilhasOpcionais},
    {"critico_inimigo_pct", &ParametrosDificuldade::criticoInimigo},
    {"dano_recebido_pct", &ParametrosDificuldade::danoRecebido},
    {"chance_sala_secreta_pct", &ParametrosDificuldade::chanceSecreta},
    {"pactos_extras", &ParametrosDificuldade::pactosExtras},
    {"multiplicador_pontos_pct", &ParametrosDificuldade::multiplicadorPontos},
    {"chance_moeda_abate_pct", &ParametrosDificuldade::chanceMoedaAbate},
    {"forca_inimigos_pct", &ParametrosDificuldade::forcaInimigos},
    {"velocidade_inimigos_pct", &ParametrosDificuldade::velocidadeInimigos},
    {"forca_chefes_pct", &ParametrosDificuldade::forcaChefes},
    {"forca_covil_pct", &ParametrosDificuldade::forcaCovil},
};

const char* const NOMES[TOTAL_DIFICULDADES] = {"Fácil", "Normal", "Difícil"};
const char* const CODIGOS[TOTAL_DIFICULDADES] = {"facil", "normal", "dificil"};

std::string aparar(const std::string& s) {
    const auto inicio = s.find_first_not_of(" \t\r");
    if (inicio == std::string::npos) return "";
    const auto fim = s.find_last_not_of(" \t\r");
    return s.substr(inicio, fim - inicio + 1);
}

}  // namespace

TabelaDificuldade tabelaDificuldadePadrao() {
    TabelaDificuldade t;
    //                orc  fortes elite drop kit roubo armad crit dano secreta pactos D  moeda forca veloc chefes covil
    constexpr int N = static_cast<int>(sizeof CAMPOS / sizeof CAMPOS[0]);
    const int v[TOTAL_DIFICULDADES][N] = {
        {75, 50, 0, 22, 150, 25, 30, 0, 80, 80, 1, 75, 65, 90, 95, 88, 100},          // Fácil
        {100, 100, 10, 15, 100, 50, 60, 3, 100, 60, 0, 100, 55, 100, 100, 100, 100},  // Normal
        {130, 160, 25, 10, 70, 80, 100, 6, 125, 45, 0, 150, 45, 100, 100, 100, 85},  // Difícil
    };
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        for (int k = 0; k < N; ++k) t.niveis[static_cast<std::size_t>(d)].*CAMPOS[k].campo = v[d][k];
    }
    return t;
}

bool lerBalanceamento(const std::string& texto, TabelaDificuldade& tabela, std::string& erro) {
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    while (std::getline(entrada, linha)) {
        ++numero;
        if (numero == 1 && linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);   // BOM do Bloco de Notas
        const auto comentario = linha.find('#');
        if (comentario != std::string::npos) linha.erase(comentario);
        linha = aparar(linha);
        if (linha.empty()) continue;
        const auto igual = linha.find('=');
        if (igual == std::string::npos) {
            erro = "linha " + std::to_string(numero) + ": falta '='";
            return false;
        }
        const std::string chave = aparar(linha.substr(0, igual));
        const CampoBalanceamento* campo = nullptr;
        for (const CampoBalanceamento& c : CAMPOS) {
            if (chave == c.chave) campo = &c;
        }
        if (!campo) {
            erro = "linha " + std::to_string(numero) + ": chave desconhecida '" + chave + "'";
            return false;
        }
        std::istringstream valores(linha.substr(igual + 1));
        int v[TOTAL_DIFICULDADES];
        for (int& x : v) {
            if (!(valores >> x)) {
                erro = "linha " + std::to_string(numero) + ": '" + chave + "' precisa de 3 números (fácil normal difícil)";
                return false;
            }
        }
        std::string sobra;
        if (valores >> sobra) {
            erro = "linha " + std::to_string(numero) + ": valor a mais em '" + chave + "'";
            return false;
        }
        for (int d = 0; d < TOTAL_DIFICULDADES; ++d) tabela.niveis[static_cast<std::size_t>(d)].*campo->campo = v[d];
    }
    return true;
}

const ParametrosDificuldade& parametros(const TabelaDificuldade& tabela, Dificuldade d) {
    const int i = static_cast<int>(d);
    return tabela.niveis[static_cast<std::size_t>(i >= 0 && i < TOTAL_DIFICULDADES ? i : 1)];
}

const char* nomeDificuldade(Dificuldade d) {
    const int i = static_cast<int>(d);
    return (i >= 0 && i < TOTAL_DIFICULDADES) ? NOMES[i] : "?";
}

const char* codigoDificuldade(Dificuldade d) {
    const int i = static_cast<int>(d);
    return (i >= 0 && i < TOTAL_DIFICULDADES) ? CODIGOS[i] : "normal";
}

bool dificuldadeDoCodigo(const std::string& codigo, Dificuldade& d) {
    for (int i = 0; i < TOTAL_DIFICULDADES; ++i) {
        if (codigo == CODIGOS[i]) {
            d = static_cast<Dificuldade>(i);
            return true;
        }
    }
    return false;
}

int aplicarPorcentagem(int valor, int porcento) {
    const long long p = static_cast<long long>(valor) * porcento;
    return static_cast<int>(p >= 0 ? (p + 50) / 100 : -((-p + 50) / 100));
}

bool sortearPorcentagem(Rng& rng, int porcento) {
    if (porcento <= 0) return false;      // 0% nunca acontece e NÃO gasta número do Rng…
    if (porcento >= 100) return true;     // …nem 100%: o Fácil sem crítico não muda a sequência
    return intervalo(rng, 1, 100) <= porcento;
}
