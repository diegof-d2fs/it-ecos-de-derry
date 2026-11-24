#include "jogo/perfil.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "core/arquivo.h"
#include "jogo/partida.h"

namespace {

// PRD §13.1. Mesma ordem de Talento.
const DefinicaoTalento TALENTOS[TOTAL_TALENTOS] = {
    // código          nome                   efeito por nível                          níveis custos                 disponível
    {"pacto",         "Pacto de Sangue",     "+1 vida extra",                          2, {40, 120},              true},
    {"coragem",       "Coragem herdada",     "+5 de vida máxima",                      5, {10, 20, 30, 40, 50},   true},
    {"lanterna",      "Lanterna reforçada",  "+1 carga de clarão (e +1 no limite)",    2, {25, 60},               true},
    {"bolso",         "Bolso fundo",         "Começa com +1 kit de cura",              2, {20, 50},               true},
    {"treino",        "Treino no Barrens",   "+1 ponto de atributo inicial",           3, {30, 60, 90},           true},
    {"mao_leve",      "Mão leve",            "+20% de moedas de prata",                3, {15, 30, 45},           true},
    {"sorte_richie",  "Sorte do Richie",     "+1 re-sorteio de Eco por tentativa",     3, {20, 40, 60},           false},
    {"mapa_ben",      "Mapa do Ben",         "Revela o minimapa do bioma",             1, {50},                   false},
    {"folego",        "Fôlego extra",        "+1 carga de dash",                       1, {100},                  false},
};

constexpr int LEMBRANCAS_POR_SALA = 3, LEMBRANCAS_POR_CHEFE = 10, LEMBRANCAS_POR_RESGATE = 5, LEMBRANCAS_VITORIA = 20;
constexpr int LEMBRANCAS_POR_LEMBRANCINHA = 15;   // achar o colecionável escondido do bioma (Etapa 8.4)
constexpr int VIDA_POR_CORAGEM = 5, MOEDAS_POR_MAO_LEVE = 20;

std::string aparar(const std::string& s) {
    const auto i = s.find_first_not_of(" \t\r");
    if (i == std::string::npos) return "";
    return s.substr(i, s.find_last_not_of(" \t\r") - i + 1);
}

bool numero(const std::string& v, long long& saida) {
    if (v.empty()) return false;
    char* fim = nullptr;
    saida = std::strtoll(v.c_str(), &fim, 10);
    return *fim == '\0';
}

}  // namespace

const DefinicaoTalento& definicaoTalento(Talento t) {
    const int i = static_cast<int>(t);
    return TALENTOS[(i >= 0 && i < TOTAL_TALENTOS) ? i : 0];
}

std::string escreverPerfil(const Perfil& p) {
    std::ostringstream s;
    s << "# Perfil de IT: Ecos de Derry (gerado pelo jogo; pode ser lido e editado)\n";
    s << "versao=" << p.versao << "\n";
    s << "lembrancas=" << p.lembrancas << "\n";
    for (int i = 0; i < TOTAL_TALENTOS; ++i) s << "talento." << TALENTOS[i].codigo << "=" << p.talentos[static_cast<std::size_t>(i)] << "\n";
    s << "tentativas=" << p.tentativas << "\nmortes=" << p.mortes << "\nvitorias=" << p.vitorias << "\n";
    s << "resgates=" << p.resgates << "\nchefes=" << p.chefes << "\n";
    s << "lembrancinhas=" << p.lembrancinhas << "\n";
    s << "melhor_pontuacao=" << p.melhorPontuacao << "\n";
    s << "ultima_dificuldade=" << codigoDificuldade(p.ultimaDificuldade) << "\n";
    s << "prologo_visto=" << (p.prologoVisto ? 1 : 0) << "\n";
    s << "dicas_vistas=" << p.dicasVistas << "\n";
    s << "ultimo_assassino=" << p.ultimoAssassino << "\n";
    s << "falas_vistas=";
    bool primeira = true;
    for (const std::string& f : p.falasVistas) {
        s << (primeira ? "" : ",") << f;
        primeira = false;
    }
    s << "\n";
    s << "cenas_vistas=";
    primeira = true;
    for (const std::string& c : p.cenasVistas) {
        s << (primeira ? "" : ",") << c;
        primeira = false;
    }
    s << "\n";
    s << "opcoes.dicas=" << (p.opcoes.dicas ? 1 : 0) << "\n";
    s << "opcoes.volume_geral=" << p.opcoes.volumeGeral << "\n";
    s << "opcoes.volume_musica=" << p.opcoes.volumeMusica << "\n";
    s << "opcoes.volume_efeitos=" << p.opcoes.volumeEfeitos << "\n";
    s << "opcoes.tela_cheia=" << (p.opcoes.telaCheia ? 1 : 0) << "\n";
    s << "opcoes.cenas_sempre=" << (p.opcoes.cenasSempre ? 1 : 0) << "\n";
    return s.str();
}

int lerPerfil(const std::string& texto, Perfil& p) {
    int ignoradas = 0;
    std::istringstream entrada(texto);
    std::string linha;
    while (std::getline(entrada, linha)) {
        if (linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);
        linha = aparar(linha);
        if (linha.empty() || linha[0] == '#') continue;
        const auto igual = linha.find('=');
        if (igual == std::string::npos) { ++ignoradas; continue; }
        const std::string chave = aparar(linha.substr(0, igual)), valor = aparar(linha.substr(igual + 1));
        long long n = 0;
        const bool ehNumero = numero(valor, n);
        auto inteiro = [&](int& destino, long long minimo, long long maximo) {
            if (ehNumero && n >= minimo && n <= maximo) destino = static_cast<int>(n);
            else ++ignoradas;
        };
        auto booleano = [&](bool& destino) {
            if (ehNumero && (n == 0 || n == 1)) destino = n == 1;
            else ++ignoradas;
        };
        if (chave == "versao") inteiro(p.versao, 1, 1000);
        else if (chave == "lembrancas") inteiro(p.lembrancas, 0, 1000000000);
        else if (chave == "tentativas") inteiro(p.tentativas, 0, 1000000000);
        else if (chave == "mortes") inteiro(p.mortes, 0, 1000000000);
        else if (chave == "vitorias") inteiro(p.vitorias, 0, 1000000000);
        else if (chave == "resgates") inteiro(p.resgates, 0, 1000000000);
        else if (chave == "chefes") inteiro(p.chefes, 0, 1000000000);
        else if (chave == "lembrancinhas") { if (ehNumero && n >= 0 && n < (1LL << 8)) p.lembrancinhas = static_cast<std::uint32_t>(n); else ++ignoradas; }
        else if (chave == "melhor_pontuacao") { if (ehNumero && n >= 0) p.melhorPontuacao = n; else ++ignoradas; }
        else if (chave == "ultima_dificuldade") { if (!dificuldadeDoCodigo(valor, p.ultimaDificuldade)) ++ignoradas; }
        else if (chave == "prologo_visto") booleano(p.prologoVisto);
        else if (chave == "dicas_vistas") { if (ehNumero && n >= 0) p.dicasVistas = static_cast<std::uint32_t>(n); else ++ignoradas; }
        else if (chave == "ultimo_assassino") inteiro(p.ultimoAssassino, -1, 100);
        else if (chave == "falas_vistas") {
            std::istringstream ids(valor);
            std::string id;
            while (std::getline(ids, id, ',')) if (!aparar(id).empty()) p.falasVistas.insert(aparar(id));
        }
        else if (chave == "cenas_vistas") {
            std::istringstream ids(valor);
            std::string id;
            while (std::getline(ids, id, ',')) if (!aparar(id).empty()) p.cenasVistas.insert(aparar(id));
        }
        else if (chave == "opcoes.dicas") booleano(p.opcoes.dicas);
        else if (chave == "opcoes.volume_geral") inteiro(p.opcoes.volumeGeral, 0, 100);
        else if (chave == "opcoes.volume_musica") inteiro(p.opcoes.volumeMusica, 0, 100);
        else if (chave == "opcoes.volume_efeitos") inteiro(p.opcoes.volumeEfeitos, 0, 100);
        else if (chave == "opcoes.tela_cheia") booleano(p.opcoes.telaCheia);
        else if (chave == "opcoes.cenas_sempre") booleano(p.opcoes.cenasSempre);
        else if (chave.rfind("talento.", 0) == 0) {
            bool achou = false;
            for (int i = 0; i < TOTAL_TALENTOS; ++i) {
                if (chave.substr(8) != TALENTOS[i].codigo) continue;
                achou = true;
                inteiro(p.talentos[static_cast<std::size_t>(i)], 0, TALENTOS[i].niveis);
            }
            if (!achou) ++ignoradas;
        } else {
            ++ignoradas;   // chave de outra versão: ignorada, sem derrubar o jogo
        }
    }
    // Versão 1 gravava tela_cheia=0 por padrão (ninguém tinha escolhido janela): migra para tela cheia.
    if (p.versao < 2) {
        p.opcoes.telaCheia = true;
        p.versao = 2;
    }
    return ignoradas;
}

bool perfilExiste(const std::filesystem::path& caminho) {
    std::error_code e;
    return std::filesystem::exists(caminho, e);
}

Perfil carregarPerfil(const std::filesystem::path& caminho) {
    Perfil p;
    std::string texto;
    if (lerArquivoTexto(caminho, texto)) lerPerfil(texto, p);
    return p;
}

bool gravarPerfil(const std::filesystem::path& caminho, const Perfil& p) {
    return gravarArquivoTexto(caminho, escreverPerfil(p));
}

int custoProximoNivel(const Perfil& p, Talento t) {
    const DefinicaoTalento& d = definicaoTalento(t);
    const int nivel = p.talentos[static_cast<std::size_t>(t)];
    if (!d.disponivel || nivel >= d.niveis) return -1;
    return d.custos[nivel];
}

bool comprarTalento(Perfil& p, Talento t) {
    const int custo = custoProximoNivel(p, t);
    if (custo < 0 || p.lembrancas < custo) return false;
    p.lembrancas -= custo;
    ++p.talentos[static_cast<std::size_t>(t)];
    return true;
}

void aplicarTalentos(const Perfil& perfil, EstadoPartida& partida) {
    auto nivel = [&](Talento t) { return perfil.talentos[static_cast<std::size_t>(t)]; };
    Jogador& b = partida.bill;
    b.pactos += nivel(Talento::PactoDeSangue);
    b.equip.bonusHp = VIDA_POR_CORAGEM * nivel(Talento::CoragemHerdada);
    recalcularJogador(b);
    b.hp = b.stats.hpMax;
    b.cargasMaximo += nivel(Talento::LanternaReforcada);
    b.cargasLanterna += nivel(Talento::LanternaReforcada);
    if (nivel(Talento::BolsoFundo) > 0) coletarNoInventario(b, partida.inventario, TipoItem::Kit, nivel(Talento::BolsoFundo));
    b.progresso.pontosPendentes += nivel(Talento::TreinoBarrens);
    partida.bonusMoedasPct = MOEDAS_POR_MAO_LEVE * nivel(Talento::MaoLeve);
}

int lembrancasDaTentativa(const EstadoPartida& p) {
    const Estatisticas& s = p.stats;
    int lembrancinhas = 0;
    for (std::uint32_t b = p.lembrancinhas; b; b &= b - 1) ++lembrancinhas;
    return LEMBRANCAS_POR_SALA * s.C + LEMBRANCAS_POR_CHEFE * s.B + LEMBRANCAS_POR_RESGATE * s.R + LEMBRANCAS_VITORIA * s.V +
           LEMBRANCAS_POR_LEMBRANCINHA * lembrancinhas;
}

int registrarFimDeTentativa(Perfil& perfil, const EstadoPartida& p, bool jogadaPeloPiloto) {
    if (jogadaPeloPiloto) return 0;   // assistir o computador não conta no perfil de quem joga
    ++perfil.tentativas;
    if (p.terminou && !p.fimDoConteudo && !p.abandonou && !p.vitoria) {
        ++perfil.mortes;
        perfil.ultimoAssassino = p.ultimoAgressor;
    }
    if (p.vitoria) ++perfil.vitorias;
    perfil.resgates += p.stats.R;
    perfil.lembrancinhas |= p.lembrancinhas;   // vão para a estante do Clube (persistem)
    perfil.chefes += p.stats.B;
    if (!p.abandonou) perfil.melhorPontuacao = std::max(perfil.melhorPontuacao, pontuacaoAtual(p));
    perfil.ultimaDificuldade = p.dificuldade;
    const int ganhas = lembrancasDaTentativa(p);
    perfil.lembrancas += ganhas;
    return ganhas;
}
