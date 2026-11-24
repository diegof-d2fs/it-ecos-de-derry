#include "jogo/clube.h"

#include <cctype>

namespace {

struct FalasAmigo {
    const char* codigo;
    const char* nome;
    const char* falas[3];
};

// Três falas por amigo, em rodízio. Curtas: cabem no balão sobre a cabeça.
const FalasAmigo FALAS[] = {
    {"ben", "Ben", {"Desenhei o mapa dos esgotos de novo. Não sei por quê.",
                    "Se a galeria alagar, segue a corrente. Ela sempre sai em algum lugar.",
                    "Você tá com cara de quem não dormiu, Bill."}},
    {"beverly", "Beverly", {"Minha mira tá ótima hoje. Só pra você saber.",
                            "Ouvi uma voz no ralo do banheiro. Deve ser bobagem.",
                            "Volta inteiro, tá? A gente precisa de você."}},
    {"eddie", "Eddie", {"Você sabe quantos germes tem num esgoto? Eu sei.",
                        "Leva um curativo. Leva dois. Leva a caixa.",
                        "Minha mãe diz que eu sou frágil. Minha mãe não sabe de nada."}},
    {"richie", "Richie", {"Bill, se você encontrar um palhaço, conta uma piada pra ele. Funciona comigo.",
                          "Eu ia junto, mas meus óculos ficam embaçados no escuro.",
                          "Por que essa cara? Parece que já ouviu essa piada mil vezes."}},
    {"stan", "Stan", {"Tudo tem uma ordem. Até lá embaixo.",
                      "Anotei os pássaros que vi hoje. Nenhum voltou pro ninho.",
                      "Eu sinto que a gente já teve essa conversa."}},
    {"mike", "Mike", {"Derry tem uma história antiga. E ela se repete a cada vinte e sete anos.",
                      "O alçapão leva até a galeria. Daqui pra frente, é com você.",
                      "Eu conto todo mundo antes de dormir. Hoje a conta não fecha."}},
};

const FalasAmigo* buscar(const std::string& codigo) {
    for (const FalasAmigo& f : FALAS) {
        if (codigo == f.codigo) return &f;
    }
    return nullptr;
}

const char* mensagemDaCompra(const Perfil& perfil, Talento t) {
    const DefinicaoTalento& d = definicaoTalento(t);
    if (!d.disponivel) return "Em breve: este talento ainda não está no jogo";
    if (perfil.talentos[static_cast<std::size_t>(t)] >= d.niveis) return "Este talento já está no nível máximo";
    return "Lembranças insuficientes";
}

}  // namespace

std::string nomeDoAmigo(const std::string& codigo) {
    if (const FalasAmigo* f = buscar(codigo)) return f->nome;
    std::string nome = codigo;
    if (!nome.empty()) nome[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(nome[0])));
    return nome;
}

std::string falaRapida(const std::string& amigo, int vez) {
    const FalasAmigo* f = buscar(amigo);
    if (!f) return "...";
    return std::string(f->nome) + ": " + f->falas[((vez % 3) + 3) % 3];
}

ContextoFala contextoDoPerfil(const Perfil& perfil, bool noClube) {
    ContextoFala ctx;
    ctx.mortes = perfil.mortes;
    ctx.tentativas = perfil.tentativas;
    ctx.vitorias = perfil.vitorias;
    ctx.resgates = perfil.resgates;
    ctx.chefes = perfil.chefes;
    ctx.ultimoAssassino = perfil.ultimoAssassino;
    ctx.noClube = noClube;
    ctx.vistas = &perfil.falasVistas;
    return ctx;
}

void configurarDicas(EstadoPartida& p, const Perfil& perfil, bool piloto) {
    p.dicasLigadas = perfil.opcoes.dicas && !piloto;
    p.dicasVistas = perfil.dicasVistas;
    p.dicasNaFila.clear();
}

void prepararVisitaAoClube(EstadoPartida& p, const Conteudo& c, const Perfil& perfil, bool piloto) {
    const ContextoFala ctx = contextoDoPerfil(perfil, true);
    p.falaNova.fill(nullptr);
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo < '1' || o.codigo > '9') continue;
        const std::size_t d = static_cast<std::size_t>(o.codigo - '0');
        p.falaNova[d] = falaNovaDoAmigo(c.dialogos, p.sala.modelo->npcs[d], ctx);
    }
    configurarDicas(p, perfil, piloto);
    pedirDica(p, perfil.tentativas >= 1 ? 19 : 1);
}

void prepararPrologo(EstadoPartida& p, const Perfil& perfil, bool piloto) {
    configurarDicas(p, perfil, piloto);
    pedirDica(p, 1);
}

bool sincronizarPerfil(Perfil& perfil, EstadoPartida& p, bool piloto) {
    if (piloto) {
        p.falasVistasAgora.clear();
        return false;
    }
    bool mudou = false;
    for (const std::string& id : p.falasVistasAgora) mudou |= perfil.falasVistas.insert(id).second;
    p.falasVistasAgora.clear();
    if ((perfil.dicasVistas | p.dicasVistas) != perfil.dicasVistas) {
        perfil.dicasVistas |= p.dicasVistas;
        mudou = true;
    }
    return mudou;
}

RespostaClube atenderClube(Perfil& perfil, EstadoPartida& p, const Conteudo& c, bool piloto) {
    RespostaClube r;
    if (!p.noClube && !p.noPrologo) return r;
    const bool vemDoPrologo = p.noPrologo;
    r.perfilMudou = sincronizarPerfil(perfil, p, piloto);   // antes: começar a tentativa zera a partida
    if (p.pedidoCompra >= 0) {
        const Talento t = static_cast<Talento>(p.pedidoCompra);
        p.pedidoCompra = -1;
        if (piloto) {
            avisarNoMenu(p, "O computador não gasta as suas Lembranças");
        } else if (comprarTalento(perfil, t)) {
            const DefinicaoTalento& d = definicaoTalento(t);
            avisarNoMenu(p, std::string("Comprou: ") + d.nome + " (nível " +
                                std::to_string(perfil.talentos[static_cast<std::size_t>(t)]) + ")");
            p.sons.push_back(IdSom::Moeda);
            r.perfilMudou = true;
        } else {
            avisarNoMenu(p, mensagemDaCompra(perfil, t));
        }
    }
    if (p.pedidoTentativa) {
        p.pedidoTentativa = false;
        const Dificuldade d = p.dificuldade;
        const std::uint64_t seed = p.seed;   // a seed do Clube vira a da tentativa (registrada no recorde)
        std::string erro;
        if (!iniciarTentativa(p, c, seed, d, erro)) {
            // Sem bioma jogável: volta ao Clube com o aviso (nunca deixa a partida pela metade).
            iniciarClube(p, c, seed, d);
            prepararVisitaAoClube(p, c, perfil, piloto);
            avisarNoMenu(p, "Não há bioma jogável: " + erro);
            return r;
        }
        configurarDicas(p, perfil, piloto);
        p.assassinoAnterior = perfil.ultimoAssassino;   // o Elias comenta a última morte
        if (!piloto) {
            aplicarTalentos(perfil, p);   // o Mural vale para quem joga
            if (vemDoPrologo) perfil.prologoVisto = true;
            perfil.ultimaDificuldade = d;
            r.perfilMudou = true;
        }
        // O ponto de salvamento da 1ª sala inclui os talentos do Mural e a fala do Elias (aplicados agora).
        p.pontoDaSala = capturarPonto(p, p.indiceSala);
        p.sons.push_back(IdSom::TrocaSala);
        r.comecouTentativa = true;
    }
    return r;
}
