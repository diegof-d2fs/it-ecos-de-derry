#include "jogo/dialogo.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "core/arquivo.h"
#include "entidades/inimigos.h"

namespace {

std::string aparar(const std::string& s) {
    const auto inicio = s.find_first_not_of(" \t\r");
    if (inicio == std::string::npos) return "";
    const auto fim = s.find_last_not_of(" \t\r");
    return s.substr(inicio, fim - inicio + 1);
}

bool inteiro(const std::string& t, int& v) {
    char* fim = nullptr;
    const long n = std::strtol(t.c_str(), &fim, 10);
    if (t.empty() || *fim != '\0' || n < 0) return false;
    v = static_cast<int>(n);
    return true;
}

// "mortes>=3", "hub", "morreu_para=rato"… → Condicao. false = desconhecida.
bool lerCondicao(const std::string& t, Condicao& c) {
    struct Comparacao { const char* prefixo; TipoCondicao tipo; };
    static const Comparacao COMPARACOES[] = {
        {"mortes>=", TipoCondicao::MortesMin},         {"mortes<=", TipoCondicao::MortesMax},
        {"tentativas>=", TipoCondicao::TentativasMin}, {"tentativas<=", TipoCondicao::TentativasMax},
        {"vitorias>=", TipoCondicao::VitoriasMin},     {"resgates>=", TipoCondicao::ResgatesMin},
        {"chefes>=", TipoCondicao::ChefesMin},
    };
    if (t == "sempre") { c.tipo = TipoCondicao::Sempre; return true; }
    if (t == "hub") { c.tipo = TipoCondicao::Hub; return true; }
    if (t == "nao_visto") { c.tipo = TipoCondicao::NaoVisto; return true; }
    for (const Comparacao& k : COMPARACOES) {
        const std::string p = k.prefixo;
        if (t.rfind(p, 0) == 0) {
            c.tipo = k.tipo;
            return inteiro(t.substr(p.size()), c.valor);
        }
    }
    const std::string morreu = "morreu_para=";
    if (t.rfind(morreu, 0) == 0) {   // pelo nome do sprite do inimigo (rato, sombra, balao…)
        const std::string nome = t.substr(morreu.size());
        for (int i = 0; i < TOTAL_TIPOS_INIMIGO; ++i) {
            if (nome == parametrosDe(static_cast<TipoInimigo>(i)).atlas) {
                c.tipo = TipoCondicao::MorreuPara;
                c.valor = i;
                return true;
            }
        }
    }
    return false;
}

bool falanteValido(const std::string& f) {
    if (f.empty()) return false;
    return std::all_of(f.begin(), f.end(), [](char ch) {
        return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
    });
}

}  // namespace

bool lerDialogos(const std::string& texto, std::vector<Dialogo>& dialogos, std::string& erro) {
    dialogos.clear();
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    bool aberto = false;   // há um bloco recebendo linhas
    auto falhar = [&](const std::string& motivo) {
        erro = "linha " + std::to_string(numero) + ": " + motivo;
        return false;
    };
    while (std::getline(entrada, linha)) {
        ++numero;
        if (numero == 1 && linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);
        const std::string limpa = aparar(linha);
        if (limpa.empty()) { aberto = false; continue; }
        if (limpa[0] == '#') continue;
        if (limpa[0] == '@') {
            Dialogo d;
            d.id = aparar(limpa.substr(1));
            if (d.id.empty()) return falhar("'@' sem id");
            dialogos.push_back(d);
            aberto = true;
            continue;
        }
        if (!aberto) return falhar("texto fora de um bloco (falta '@id' antes?)");
        Dialogo& d = dialogos.back();
        if (limpa.rfind("quem=", 0) == 0) {
            d.quem = aparar(limpa.substr(5));
        } else if (limpa.rfind("quando=", 0) == 0) {
            std::istringstream conds(limpa.substr(7));
            std::string c;
            while (std::getline(conds, c, ';')) {
                c = aparar(c);
                if (c.empty()) continue;
                Condicao cond;
                if (!lerCondicao(c, cond)) return falhar("condição desconhecida '" + c + "'");
                d.condicoes.push_back(cond);
            }
        } else if (limpa.rfind("escolha=", 0) == 0) {
            const std::string v = limpa.substr(8);
            const auto barra = v.rfind('|');
            if (barra == std::string::npos) return falhar("escolha deve ser 'texto|id'");
            d.escolhas.push_back({aparar(v.substr(0, barra)), aparar(v.substr(barra + 1))});
        } else if (limpa.rfind("efeito=", 0) == 0) {
            d.efeitos.push_back(aparar(limpa.substr(7)));
        } else {
            const auto doisPontos = limpa.find(':');
            if (doisPontos == std::string::npos) return falhar("esperado 'Falante: texto'");
            LinhaDialogo l;
            std::string quem = aparar(limpa.substr(0, doisPontos));
            const auto barra = quem.find('|');
            if (barra != std::string::npos) {
                l.expressao = aparar(quem.substr(barra + 1));
                quem = aparar(quem.substr(0, barra));
            }
            if (!falanteValido(quem)) return falhar("falante inválido '" + quem + "'");
            l.falante = quem;
            l.texto = aparar(limpa.substr(doisPontos + 1));
            if (l.texto.empty()) return falhar("fala vazia");
            d.linhas.push_back(l);
        }
    }
    return true;
}

bool carregarDialogos(const std::filesystem::path& caminho, std::vector<Dialogo>& dialogos, std::string& erro) {
    std::string texto;
    if (!lerArquivoTexto(caminho, texto)) {
        erro = caminho.filename().u8string() + ": não foi possível abrir";
        return false;
    }
    if (!lerDialogos(texto, dialogos, erro)) {
        erro = caminho.filename().u8string() + ", " + erro;
        return false;
    }
    return true;
}

std::vector<std::string> validarDialogos(const std::vector<Dialogo>& dialogos) {
    std::vector<std::string> problemas;
    std::set<std::string> ids;
    for (const Dialogo& d : dialogos) {
        if (!ids.insert(d.id).second) problemas.push_back("id repetido: " + d.id);
        if (d.linhas.empty()) problemas.push_back("bloco sem fala: " + d.id);
        if (d.escolhas.size() == 1 || d.escolhas.size() > 3) problemas.push_back("escolhas devem ser 2 ou 3: " + d.id);
    }
    for (const Dialogo& d : dialogos) {
        for (const EscolhaDialogo& e : d.escolhas) {
            if (!ids.count(e.destino)) problemas.push_back("escolha de " + d.id + " leva a id inexistente: " + e.destino);
        }
    }
    for (int n = 1; n <= TOTAL_DICAS; ++n) {
        if (!ids.count(idDaDica(n))) problemas.push_back("falta a dica " + idDaDica(n));
    }
    return problemas;
}

const Dialogo* buscarDialogo(const std::vector<Dialogo>& dialogos, const std::string& id) {
    for (const Dialogo& d : dialogos) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

bool condicoesSatisfeitas(const Dialogo& d, const ContextoFala& ctx) {
    for (const Condicao& c : d.condicoes) {
        bool ok = true;
        switch (c.tipo) {
            case TipoCondicao::Sempre: break;
            case TipoCondicao::Hub: ok = ctx.noClube; break;
            case TipoCondicao::NaoVisto: ok = !ctx.vistas || !ctx.vistas->count(d.id); break;
            case TipoCondicao::MortesMin: ok = ctx.mortes >= c.valor; break;
            case TipoCondicao::MortesMax: ok = ctx.mortes <= c.valor; break;
            case TipoCondicao::TentativasMin: ok = ctx.tentativas >= c.valor; break;
            case TipoCondicao::TentativasMax: ok = ctx.tentativas <= c.valor; break;
            case TipoCondicao::VitoriasMin: ok = ctx.vitorias >= c.valor; break;
            case TipoCondicao::ResgatesMin: ok = ctx.resgates >= c.valor; break;
            case TipoCondicao::ChefesMin: ok = ctx.chefes >= c.valor; break;
            case TipoCondicao::MorreuPara: ok = ctx.ultimoAssassino == c.valor; break;
        }
        if (!ok) return false;
    }
    return true;
}

const Dialogo* falaNovaDoAmigo(const std::vector<Dialogo>& dialogos, const std::string& quem, const ContextoFala& ctx) {
    for (const Dialogo& d : dialogos) {
        if (d.quem != quem || (ctx.vistas && ctx.vistas->count(d.id))) continue;
        if (condicoesSatisfeitas(d, ctx)) return &d;
    }
    return nullptr;
}

int contarCaracteres(const std::string& utf8) {
    int n = 0;
    for (unsigned char c : utf8) n += (c & 0xC0) != 0x80 ? 1 : 0;
    return n;
}

namespace {
bool rotulosNintendo = false;
}

void definirRotulosNintendo(bool nintendo) { rotulosNintendo = nintendo; }

std::string trocarBotoes(const std::string& texto, bool controle) {
    // Os botões são lidos por POSIÇÃO (o de baixo é sempre dash/confirmar). Num controle Nintendo (o
    // 8BitDo do Diego no modo Switch), as letras impressas nas posições são outras: baixo = B, direita
    // = A, esquerda = Y, cima = X; ombros L/R, gatilhos ZL/ZR (Diego, 17/11: "X atacar" no botão Y).
    struct Botao { const char* marcador; const char* teclado; const char* gamepad; const char* nintendo; };
    static const Botao BOTOES[] = {
        {"{MOVER}", "WASD ou as setas", "o analógico", "o analógico"}, {"{ATACAR}", "J", "X", "Y"},
        {"{ESPECIAL}", "K", "Y", "X"},                       {"{CLARAO}", "L", "B", "A"},
        {"{DASH}", "Espaço", "A", "B"},                      {"{INTERAGIR}", "E", "RB", "R"},
        {"{KIT}", "Q", "LB", "L"},                           {"{INVENTARIO}", "I", "Back", "−"},
        {"{FICHA}", "C", "LT + ↑", "ZL + ↑"},                {"{PAUSA}", "Esc", "Start", "+"},
        {"{VOLTAR}", "Esc", "B", "A"},
        {"{CONFIRMAR}", "Enter", "A", "B"},                  {"{MAPA}", "M", "LT", "ZL"},
    };
    std::string s = texto;
    for (const Botao& b : BOTOES) {
        const std::string m = b.marcador, nome = controle ? (rotulosNintendo ? b.nintendo : b.gamepad) : b.teclado;
        for (std::size_t pos = s.find(m); pos != std::string::npos; pos = s.find(m, pos + nome.size())) {
            s.replace(pos, m.size(), nome);
        }
    }
    return s;
}

std::string idDaDica(int numero) {
    char t[16];
    std::snprintf(t, sizeof t, "dica_D%02d", numero);
    return t;
}

const char* tituloDaDica(int numero) {
    static const char* const TITULOS[TOTAL_DICAS] = {
        "Mover",                    "Interagir e conversar",     "Escuridão e lanterna",   "Atacar e combo",
        "Dash e avisos de ataque",  "Clarão e cargas",           "Inventário",             "Kit de cura",
        "Portas com recompensa",    "Ecos",                      "Níveis e atributos",     "Armadilhas",
        "Portas e baús trancados",  "Moedas e a loja do Elias",  "Refúgio",                "Chefes",
        "Resgates",                 "Pactos de Sangue",          "O ciclo e o Mural",      "Mapa e diário",
    };
    return (numero >= 1 && numero <= TOTAL_DICAS) ? TITULOS[numero - 1] : "?";
}
