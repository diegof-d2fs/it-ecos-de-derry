#include "jogo/salvamento.h"

#include <algorithm>
#include <cstdio>
#include <map>
#include <sstream>

#include "core/arquivo.h"
#include "jogo/partida.h"

namespace {

constexpr int VERSAO_BLOCO = 1;

std::string aparar(const std::string& s) {
    const auto ini = s.find_first_not_of(" \t\r");
    if (ini == std::string::npos) return "";
    const auto fim = s.find_last_not_of(" \t\r");
    return s.substr(ini, fim - ini + 1);
}

std::vector<std::string> partir(const std::string& s, char sep) {
    std::vector<std::string> v;
    std::string parte;
    std::istringstream entrada(s);
    while (std::getline(entrada, parte, sep)) v.push_back(parte);
    return v;
}

std::vector<long long> numeros(const std::string& s) {
    std::vector<long long> v;
    for (const std::string& p : partir(s, ',')) {
        if (aparar(p).empty()) continue;
        try {
            v.push_back(std::stoll(aparar(p)));
        } catch (...) {
            v.push_back(0);
        }
    }
    return v;
}

template <typename T>
std::string juntar(const std::vector<T>& v) {
    std::ostringstream s;
    for (std::size_t i = 0; i < v.size(); ++i) s << (i ? "," : "") << v[i];
    return s.str();
}

std::string rngTexto(const Rng& r) {
    return std::to_string(r.s[0]) + "," + std::to_string(r.s[1]) + "," + std::to_string(r.s[2]) + "," + std::to_string(r.s[3]);
}

bool rngDoTexto(const std::string& t, Rng& r) {
    const std::vector<long long> n = numeros(t);
    if (n.size() != 4) return false;
    for (int i = 0; i < 4; ++i) r.s[i] = static_cast<std::uint32_t>(n[static_cast<std::size_t>(i)]);
    return true;
}

int indicePorId(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

std::string idDaSala(const Conteudo& c, int indice) {
    return (indice >= 0 && indice < static_cast<int>(c.salas.size())) ? c.salas[static_cast<std::size_t>(indice)].id : "?";
}

// Uma opção de porta: "<id da sala>:<tipo>:<recompensa>".
std::string opcaoTexto(const Conteudo& c, const OpcaoPorta& o) {
    return idDaSala(c, o.sala) + ":" + std::to_string(static_cast<int>(o.tipo)) + ":" + std::to_string(static_cast<int>(o.recompensa));
}

bool opcaoDoTexto(const Conteudo& c, const std::string& t, OpcaoPorta& o) {
    const std::vector<std::string> p = partir(t, ':');
    if (p.size() != 3) return false;
    o.sala = indicePorId(c, p[0]);
    try {
        o.tipo = static_cast<TipoSala>(std::stoi(p[1]));
        o.recompensa = static_cast<Recompensa>(std::stoi(p[2]));
    } catch (...) {
        return false;
    }
    return o.sala >= 0;
}

std::string equipTexto(const Equipamento& e) {
    return juntar(std::vector<int>{e.bonusDanoArma, e.melhorias, e.armaduraEscudo, e.bonusDanoPct, e.bonusVelocidadePct, e.bonusHp,
                                   e.ecoArmadura, e.ecoHp, e.ecoVelocidadePct, e.ecoKitPct});
}

}  // namespace

// ---------- captura e restauração ----------
PontoSalvamento capturarPonto(const EstadoPartida& p, int indiceSala) {
    PontoSalvamento s;
    s.valido = true;
    s.seed = p.seed;
    s.dificuldade = p.dificuldade;
    s.rng = p.rng;
    s.rngGerador = p.rngGerador;
    s.plano = p.plano;
    s.etapa = p.etapa;
    s.salaPrometida = p.salaPrometida;
    s.indiceSala = indiceSala;
    const Jogador& b = p.bill;
    s.attr = b.attr;
    s.equip = b.equip;
    s.hp = b.hp;
    s.progresso = b.progresso;
    s.pactos = b.pactos;
    s.pactoUsado = b.pactoUsado;
    s.cargasLanterna = b.cargasLanterna;
    s.cargasMaximo = b.cargasMaximo;
    s.dashExtras = b.dashExtras;
    s.coragem = b.coragem;
    s.tonico = b.tonico;
    s.oculos = b.oculos;
    s.inventario = p.inventario;
    s.ecos = p.ecos;
    s.companheiroAtivo = p.companheiro.ativo;
    s.companheiro = p.companheiro.quem;
    s.resgatados = p.resgatados;
    s.stats = p.stats;
    s.kitsUsados = p.kitsUsados;
    s.bonusMoedasPct = p.bonusMoedasPct;
    s.assassinoAnterior = p.assassinoAnterior;
    s.conversasElias = p.conversasElias;
    s.compras = p.compras;
    s.ultimoAgressor = p.ultimoAgressor;
    s.manivelaEntregue = p.manivelaEntregue;
    s.poemaUsado = p.poemaUsado;
    s.segredoAtivo = p.segredoAtivo;
    s.segredoUsado = p.segredoUsado;
    s.lembrancinhas = p.lembrancinhas;
    s.pontos = pontuacaoAtual(p);
    return s;
}

bool restaurarPonto(EstadoPartida& p, const Conteudo& c, const PontoSalvamento& s, std::string& erro) {
    if (!s.valido || s.indiceSala < 0 || s.indiceSala >= static_cast<int>(c.salas.size())) {
        erro = "ponto de salvamento sem sala válida";
        return false;
    }
    for (const EtapaTentativa& e : s.plano.etapas) {
        for (const OpcaoPorta& o : e.opcoes) {
            if (o.sala < 0 || o.sala >= static_cast<int>(c.salas.size())) {
                erro = "plano do bioma com sala inexistente";
                return false;
            }
        }
    }
    if (!iniciarTentativa(p, c, s.seed, s.dificuldade, erro)) return false;
    p.rng = s.rng;
    p.rngGerador = s.rngGerador;
    p.plano = s.plano;
    p.etapa = s.etapa;
    p.salaPrometida = s.salaPrometida;
    Jogador& b = p.bill;
    b.attr = s.attr;
    b.equip = s.equip;
    b.progresso = s.progresso;
    b.pactos = s.pactos;
    b.pactoUsado = s.pactoUsado;
    b.cargasLanterna = s.cargasLanterna;
    b.cargasMaximo = s.cargasMaximo;
    b.dashExtras = s.dashExtras;
    b.dashExtrasDisponiveis = s.dashExtras;
    b.coragem = s.coragem;
    b.tonico = s.tonico;
    b.oculos = s.oculos;
    recalcularJogador(b);
    b.hp = std::max(1, std::min(s.hp, b.stats.hpMax));
    p.inventario = s.inventario;
    p.ecos = s.ecos;
    p.companheiro = Companheiro{};
    p.companheiro.ativo = s.companheiroAtivo;
    p.companheiro.quem = s.companheiro;
    p.resgatados = s.resgatados;
    p.stats = s.stats;
    p.kitsUsados = s.kitsUsados;
    p.bonusMoedasPct = s.bonusMoedasPct;
    p.assassinoAnterior = s.assassinoAnterior;
    p.conversasElias = s.conversasElias;
    p.compras = s.compras;
    p.ultimoAgressor = s.ultimoAgressor;
    p.manivelaEntregue = s.manivelaEntregue;
    p.poemaUsado = s.poemaUsado;
    p.segredoAtivo = s.segredoAtivo;
    p.segredoUsado = s.segredoUsado;
    p.lembrancinhas = s.lembrancinhas;
    entrarNaSala(p, c, s.indiceSala, nullptr);
    return true;
}

// ---------- arquivo ----------
std::string escreverPonto(const PontoSalvamento& s, const Conteudo& c, const std::string& data) {
    std::ostringstream o;
    o << "# IT: Ecos de Derry · bloco do cartão de memória (ponto de entrada da sala)\n";
    o << "versao=" << VERSAO_BLOCO << "\n";
    o << "data=" << data << "\n";
    const SalaModelo* sala = (s.indiceSala >= 0 && s.indiceSala < static_cast<int>(c.salas.size()))
                                 ? &c.salas[static_cast<std::size_t>(s.indiceSala)] : nullptr;
    o << "bioma=" << s.plano.bioma << "\n";
    o << "nome_sala=" << (sala ? (sala->nome.empty() ? sala->id : sala->nome) : "?") << "\n";
    o << "pontos=" << s.pontos << "\n";
    o << "seed=" << s.seed << "\n";
    o << "dificuldade=" << codigoDificuldade(s.dificuldade) << "\n";
    o << "rng=" << rngTexto(s.rng) << "\n";
    o << "rng_gerador=" << rngTexto(s.rngGerador) << "\n";
    o << "sala=" << idDaSala(c, s.indiceSala) << "\n";
    o << "etapa=" << s.etapa << "\n";
    o << "prometida=" << opcaoTexto(c, s.salaPrometida) << "\n";
    o << "plano=";
    for (std::size_t e = 0; e < s.plano.etapas.size(); ++e) {
        o << (e ? ";" : "");
        const std::vector<OpcaoPorta>& ops = s.plano.etapas[e].opcoes;
        for (std::size_t k = 0; k < ops.size(); ++k) o << (k ? "|" : "") << opcaoTexto(c, ops[k]);
    }
    o << "\n";
    o << "atributos=" << s.attr.forca << "," << s.attr.foco << "," << s.attr.agilidade << "," << s.attr.coragem << "\n";
    o << "equipamento=" << equipTexto(s.equip) << "\n";
    o << "vida=" << s.hp << "\n";
    o << "progresso=" << s.progresso.xp << "," << s.progresso.nivel << "," << s.progresso.pontosPendentes << "\n";
    o << "pactos=" << s.pactos << "," << (s.pactoUsado ? 1 : 0) << "\n";
    o << "lanterna=" << s.cargasLanterna << "," << s.cargasMaximo << "\n";
    o << "dash_extras=" << s.dashExtras << "\n";
    o << "efeitos_ms=" << static_cast<int>(s.coragem * 1000) << "," << static_cast<int>(s.tonico * 1000) << ","
      << static_cast<int>(s.oculos * 1000) << "\n";
    o << "inventario=" << juntar(std::vector<int>(s.inventario.quantidades.begin(), s.inventario.quantidades.end())) << "\n";
    o << "ecos=";
    for (std::size_t i = 0; i < s.ecos.size(); ++i)
        o << (i ? "," : "") << definicaoEco(s.ecos[i].definicao).nome << ":" << static_cast<int>(s.ecos[i].raridade);
    o << "\n";
    o << "companheiro=" << (s.companheiroAtivo ? static_cast<int>(s.companheiro) : -1) << "\n";
    std::vector<int> resg;
    for (Amigo a : s.resgatados) resg.push_back(static_cast<int>(a));
    o << "resgatados=" << juntar(resg) << "\n";
    const Estatisticas& t = s.stats;
    o << "estatisticas=" << t.K << "," << t.E << "," << t.I << "," << t.R << "," << t.B << "," << t.C << "," << t.V << "," << t.M
      << "," << t.U << "," << static_cast<long long>(t.tempo * 1000) << "\n";
    o << "contadores=" << s.kitsUsados << "," << s.bonusMoedasPct << "," << s.assassinoAnterior << "," << s.conversasElias << ","
      << s.compras << "," << s.ultimoAgressor << "\n";
    o << "marcas=" << (s.manivelaEntregue ? 1 : 0) << "," << (s.poemaUsado ? 1 : 0) << "," << (s.segredoAtivo ? 1 : 0) << ","
      << (s.segredoUsado ? 1 : 0) << "," << s.lembrancinhas << "\n";
    return o.str();
}

bool lerPonto(const std::string& texto, const Conteudo& c, PontoSalvamento& s, ResumoBloco& r, std::string& erro) {
    std::map<std::string, std::string> v;
    std::istringstream entrada(texto);
    std::string linha;
    while (std::getline(entrada, linha)) {
        linha = aparar(linha);
        if (linha.empty() || linha[0] == '#') continue;
        const auto igual = linha.find('=');
        if (igual != std::string::npos) v[aparar(linha.substr(0, igual))] = aparar(linha.substr(igual + 1));
    }
    s = PontoSalvamento{};
    r = ResumoBloco{};
    auto falha = [&](const char* m) {
        erro = m;
        return false;
    };
    if (v["versao"] != std::to_string(VERSAO_BLOCO)) return falha("versão do bloco desconhecida");
    try {
        s.seed = std::stoull(v["seed"]);
    } catch (...) {
        return falha("seed inválida");
    }
    if (!dificuldadeDoCodigo(v["dificuldade"], s.dificuldade)) return falha("dificuldade inválida");
    if (!rngDoTexto(v["rng"], s.rng) || !rngDoTexto(v["rng_gerador"], s.rngGerador)) return falha("gerador aleatório inválido");
    s.indiceSala = indicePorId(c, v["sala"]);
    if (s.indiceSala < 0) return falha("a sala salva não existe mais");
    if (!opcaoDoTexto(c, v["prometida"], s.salaPrometida)) return falha("sala prometida inválida");
    const std::vector<long long> bioma = numeros(v["bioma"]);
    s.plano.bioma = bioma.empty() ? 1 : static_cast<int>(bioma[0]);
    for (const std::string& etapa : partir(v["plano"], ';')) {
        EtapaTentativa e;
        for (const std::string& op : partir(etapa, '|')) {
            OpcaoPorta o;
            if (!opcaoDoTexto(c, op, o)) return falha("plano do bioma inválido");
            e.opcoes.push_back(o);
        }
        s.plano.etapas.push_back(e);
    }
    const std::vector<long long> et = numeros(v["etapa"]);
    s.etapa = et.empty() ? 0 : static_cast<int>(et[0]);
    const std::vector<long long> a = numeros(v["atributos"]);
    if (a.size() != 4) return falha("atributos inválidos");
    s.attr = {static_cast<int>(a[0]), static_cast<int>(a[1]), static_cast<int>(a[2]), static_cast<int>(a[3])};
    const std::vector<long long> eq = numeros(v["equipamento"]);
    if (eq.size() != 10) return falha("equipamento inválido");
    int* campos[] = {&s.equip.bonusDanoArma, &s.equip.melhorias, &s.equip.armaduraEscudo, &s.equip.bonusDanoPct, &s.equip.bonusVelocidadePct,
                     &s.equip.bonusHp, &s.equip.ecoArmadura, &s.equip.ecoHp, &s.equip.ecoVelocidadePct, &s.equip.ecoKitPct};
    for (int i = 0; i < 10; ++i) *campos[i] = static_cast<int>(eq[static_cast<std::size_t>(i)]);
    const std::vector<long long> vida = numeros(v["vida"]);
    s.hp = vida.empty() ? 1 : static_cast<int>(vida[0]);
    const std::vector<long long> pr = numeros(v["progresso"]);
    if (pr.size() != 3) return falha("progresso inválido");
    s.progresso = {static_cast<int>(pr[0]), static_cast<int>(pr[1]), static_cast<int>(pr[2])};
    const std::vector<long long> pa = numeros(v["pactos"]), la = numeros(v["lanterna"]), ef = numeros(v["efeitos_ms"]);
    if (pa.size() != 2 || la.size() != 2 || ef.size() != 3) return falha("estado de Bill inválido");
    s.pactos = static_cast<int>(pa[0]);
    s.pactoUsado = pa[1] != 0;
    s.cargasLanterna = static_cast<int>(la[0]);
    s.cargasMaximo = static_cast<int>(la[1]);
    s.coragem = static_cast<float>(ef[0]) / 1000.0f;
    s.tonico = static_cast<float>(ef[1]) / 1000.0f;
    s.oculos = static_cast<float>(ef[2]) / 1000.0f;
    const std::vector<long long> de = numeros(v["dash_extras"]);
    s.dashExtras = de.empty() ? 0 : static_cast<int>(de[0]);
    const std::vector<long long> inv = numeros(v["inventario"]);
    for (std::size_t i = 0; i < inv.size() && i < s.inventario.quantidades.size(); ++i) s.inventario.quantidades[i] = static_cast<int>(inv[i]);
    for (const std::string& e : partir(v["ecos"], ',')) {
        const std::vector<std::string> pe = partir(e, ':');
        if (pe.size() != 2) continue;
        for (int i = 0; i < totalEcos(); ++i) {
            if (pe[0] != definicaoEco(i).nome) continue;
            try {
                s.ecos.push_back({i, static_cast<Raridade>(std::stoi(pe[1]))});
            } catch (...) {
            }
        }
    }
    const std::vector<long long> comp = numeros(v["companheiro"]);
    s.companheiroAtivo = !comp.empty() && comp[0] >= 0 && comp[0] < static_cast<long long>(Amigo::Total);
    s.companheiro = s.companheiroAtivo ? static_cast<Amigo>(comp[0]) : Amigo::Total;
    for (long long a2 : numeros(v["resgatados"]))
        if (a2 >= 0 && a2 < static_cast<long long>(Amigo::Total)) s.resgatados.push_back(static_cast<Amigo>(a2));
    const std::vector<long long> t = numeros(v["estatisticas"]);
    if (t.size() != 10) return falha("estatísticas inválidas");
    s.stats.K = static_cast<int>(t[0]); s.stats.E = static_cast<int>(t[1]); s.stats.I = static_cast<int>(t[2]);
    s.stats.R = static_cast<int>(t[3]); s.stats.B = static_cast<int>(t[4]); s.stats.C = static_cast<int>(t[5]);
    s.stats.V = static_cast<int>(t[6]); s.stats.M = static_cast<int>(t[7]); s.stats.U = static_cast<int>(t[8]);
    s.stats.tempo = static_cast<float>(t[9]) / 1000.0f;
    const std::vector<long long> ct = numeros(v["contadores"]), mc = numeros(v["marcas"]);
    if (ct.size() == 6) {
        s.kitsUsados = static_cast<int>(ct[0]); s.bonusMoedasPct = static_cast<int>(ct[1]); s.assassinoAnterior = static_cast<int>(ct[2]);
        s.conversasElias = static_cast<int>(ct[3]); s.compras = static_cast<int>(ct[4]); s.ultimoAgressor = static_cast<int>(ct[5]);
    }
    if (mc.size() == 5) {
        s.manivelaEntregue = mc[0] != 0; s.poemaUsado = mc[1] != 0; s.segredoAtivo = mc[2] != 0; s.segredoUsado = mc[3] != 0;
        s.lembrancinhas = static_cast<std::uint32_t>(mc[4]);
    }
    const std::vector<long long> pts = numeros(v["pontos"]);
    s.pontos = pts.empty() ? 0 : pts[0];
    s.valido = true;
    r.ocupado = true;
    r.bioma = s.plano.bioma;
    r.nomeSala = v["nome_sala"];
    r.nivel = s.progresso.nivel;
    r.segundos = static_cast<int>(s.stats.tempo);
    r.pontos = s.pontos;
    r.dificuldade = s.dificuldade;
    r.data = v["data"];
    return true;
}

std::filesystem::path arquivoDoBloco(const std::filesystem::path& pasta, int bloco) {
    char nome[32];
    std::snprintf(nome, sizeof nome, "bloco_%02d.txt", bloco);
    return pasta / nome;
}

std::array<ResumoBloco, TOTAL_BLOCOS> listarBlocos(const std::filesystem::path& pasta, const Conteudo& c) {
    std::array<ResumoBloco, TOTAL_BLOCOS> blocos{};
    for (int b = 1; b <= TOTAL_BLOCOS; ++b) {
        std::string texto, erro;
        PontoSalvamento ponto;
        if (lerArquivoTexto(arquivoDoBloco(pasta, b), texto)) lerPonto(texto, c, ponto, blocos[static_cast<std::size_t>(b - 1)], erro);
    }
    return blocos;
}

bool gravarBloco(const std::filesystem::path& pasta, int bloco, const PontoSalvamento& ponto, const Conteudo& c, const std::string& data) {
    if (bloco < 1 || bloco > TOTAL_BLOCOS || !ponto.valido) return false;
    std::error_code ec;
    std::filesystem::create_directories(pasta, ec);
    return gravarArquivoTexto(arquivoDoBloco(pasta, bloco), escreverPonto(ponto, c, data));
}

bool carregarBloco(const std::filesystem::path& pasta, int bloco, const Conteudo& c, PontoSalvamento& ponto, std::string& erro) {
    std::string texto;
    if (bloco < 1 || bloco > TOTAL_BLOCOS || !lerArquivoTexto(arquivoDoBloco(pasta, bloco), texto)) {
        erro = "bloco vazio";
        return false;
    }
    ResumoBloco r;
    return lerPonto(texto, c, ponto, r, erro);
}

bool apagarBloco(const std::filesystem::path& pasta, int bloco) {
    if (bloco < 1 || bloco > TOTAL_BLOCOS) return false;
    std::error_code ec;
    return std::filesystem::remove(arquivoDoBloco(pasta, bloco), ec);
}
