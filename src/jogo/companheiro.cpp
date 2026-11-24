#include "jogo/companheiro.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "combate/area.h"
#include "combate/atributos.h"
#include "jogo/partida.h"
#include "mundo/colisao.h"
#include "mundo/sala.h"
#include "mundo/visao.h"

namespace {

constexpr float INTERVALO_REPLANEJAR = 0.25f;
constexpr float RAIO_PROVOCAR = 6.0f * TILE;       // Richie provoca quem está perto de Bill
constexpr int CURA_EDDIE_PCT = 10;
constexpr int LIMIAR_EDDIE_PCT = 30;
constexpr float DURACAO_BARREIRA = 2.0f;
constexpr float DURACAO_PROVOCACAO = 2.0f;

const char* const CODIGOS[TOTAL_AMIGOS] = {"mike", "ben", "stan", "eddie", "beverly", "richie"};
const char* const NOMES[TOTAL_AMIGOS] = {"Mike", "Ben", "Stan", "Eddie", "Beverly", "Richie"};

void falar(EstadoPartida& p, Vec2 pos, const std::string& t, float vida = 1.2f) {
    TextoFlutuante tf;
    tf.pos = {pos.x, pos.y - 50.0f};
    tf.texto = t;
    tf.tipo = TipoTexto::Info;
    tf.vida = vida;
    p.textos.push_back(tf);
}

bool livre(const EstadoPartida& p, Vec2 pes) {
    return !colideComSala(p.sala, caixaNosPes(pes, CAIXA_COMPANHEIRO), false);
}

// Célula livre mais perto do ponto atrás de Bill, num anel de até 3 células em volta dele.
Vec2 lugarPertoDoBill(const EstadoPartida& p) {
    const Vec2 atras = somar(p.bill.pes, escalar(vetorDaDirecao(p.bill.direcao), -DISTANCIA_SEGUIR * TILE));
    if (livre(p, atras)) return atras;
    const Celula b = celulaDe(p.bill.pes);
    Vec2 melhor = p.bill.pes;
    float menor = 1e9f;
    for (int raio = 1; raio <= 3 && menor > 1e8f; ++raio) {
        for (int l = b.lin - raio; l <= b.lin + raio; ++l) {
            for (int c = b.col - raio; c <= b.col + raio; ++c) {
                if (!dentroDaSala(p.sala, c, l)) continue;
                const Vec2 centro = centroCelula({c, l});
                if (!livre(p, centro)) continue;
                const float d = comprimento(subtrair(centro, atras));
                if (d < menor) { menor = d; melhor = centro; }
            }
        }
    }
    return melhor;
}

// BFS pelos tiles caminháveis (o companheiro conhece a sala: ele não explora, acompanha).
std::vector<Celula> caminhoAte(const EstadoPartida& p, Celula de, Celula para) {
    const int cols = p.sala.cols, lins = p.sala.lins;
    if (!dentroDaSala(p.sala, de.col, de.lin) || !dentroDaSala(p.sala, para.col, para.lin)) return {};
    std::array<short, SALA_MAX_CELULAS> veio;
    veio.fill(-2);
    std::array<short, SALA_MAX_CELULAS> fila{};
    int ini = 0, fim = 0;
    const int o = de.lin * cols + de.col, alvo = para.lin * cols + para.col;
    veio[static_cast<std::size_t>(o)] = -1;
    fila[static_cast<std::size_t>(fim++)] = static_cast<short>(o);
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    while (ini < fim && veio[static_cast<std::size_t>(alvo)] == -2) {
        const int atual = fila[static_cast<std::size_t>(ini++)];
        for (int k = 0; k < 4; ++k) {
            const int nc = atual % cols + dc[k], nl = atual / cols + dl[k];
            if (nc < 0 || nl < 0 || nc >= cols || nl >= lins || tileEm(p.sala, nc, nl).bloqueiaMovimento) continue;
            const int i = nl * cols + nc;
            if (veio[static_cast<std::size_t>(i)] != -2) continue;
            veio[static_cast<std::size_t>(i)] = static_cast<short>(atual);
            fila[static_cast<std::size_t>(fim++)] = static_cast<short>(i);
        }
    }
    if (veio[static_cast<std::size_t>(alvo)] == -2) return {};
    std::vector<Celula> inverso;
    for (int i = alvo; i != o; i = veio[static_cast<std::size_t>(i)]) inverso.push_back({i % cols, i / cols});
    return std::vector<Celula>(inverso.rbegin(), inverso.rend());
}

void teleportar(EstadoPartida& p) {
    Companheiro& c = p.companheiro;
    c.pes = lugarPertoDoBill(p);
    c.caminho.clear();
    c.tempoTravado = 0.0f;
    c.andando = false;
    ++c.teleportes;
}

void seguirBill(EstadoPartida& p, float dt) {
    Companheiro& c = p.companheiro;
    const float distBill = comprimento(subtrair(p.bill.pes, c.pes));
    if (distBill > DISTANCIA_TELEPORTE * TILE || c.tempoTravado > TEMPO_TRAVADO_TELEPORTE) {
        teleportar(p);
        return;
    }
    Vec2 alvo = somar(p.bill.pes, escalar(vetorDaDirecao(p.bill.direcao), -DISTANCIA_SEGUIR * TILE));
    if (!livre(p, alvo)) alvo = p.bill.pes;
    // Em luta: o flanco do Bill (do lado em que o amigo já está), um pouco atrás, com um balanço lento de passinhos,
    // e o rosto para o inimigo. Antes ele só seguia o ponto ATRÁS do Bill: com o Bill parado golpeando, ficava
    // imóvel e de costas para a briga, e parecia travado.
    Vec2 frente{};
    c.emLuta = false;
    for (const Inimigo& i : p.inimigos) {
        if (!i.vivo || i.surgindo > 0.0f || i.aliado > 0.0f || !estaVisivel(p.visao, i.pes)) continue;
        const Vec2 d = subtrair(i.pes, p.bill.pes);
        if (comprimento(d) > 6.0f * TILE) continue;
        if (!c.emLuta || comprimento(d) < comprimento(frente)) frente = d;
        c.emLuta = true;
    }
    if (c.emLuta) {
        const Vec2 f = normalizar(frente);
        Vec2 lado{-f.y, f.x};
        const Vec2 deBill = subtrair(c.pes, p.bill.pes);
        if (lado.x * deBill.x + lado.y * deBill.y < 0.0f) lado = escalar(lado, -1.0f);
        const float balanco = 0.35f * std::sin(c.relogio * 1.7f);   // ±11 px: passinhos de um lado para o outro
        for (int tentativa = 0; tentativa < 2; ++tentativa) {
            const Vec2 flanco = somar(p.bill.pes, somar(escalar(lado, 1.3f * TILE), escalar(f, (balanco - 0.25f) * TILE)));
            if (livre(p, flanco)) {
                alvo = flanco;
                break;
            }
            lado = escalar(lado, -1.0f);   // o flanco dá na parede: o outro lado
        }
    }
    const float dAlvo = comprimento(subtrair(alvo, c.pes));
    if (dAlvo < 6.0f || (!c.emLuta && distBill < 0.9f * TILE)) {   // perto o bastante: espera (não empurra Bill)
        c.andando = false;
        c.tempoTravado = 0.0f;
        if (c.emLuta) c.direcao = direcaoDominante(frente, c.direcao);   // de frente para a briga
        return;
    }
    c.replanejar -= dt;
    if (c.replanejar <= 0.0f || c.caminho.empty()) {
        c.caminho = caminhoAte(p, celulaDe(c.pes), celulaDe(alvo));
        c.replanejar = INTERVALO_REPLANEJAR;
    }
    while (c.caminho.size() > 1) {   // célula alcançada: próxima
        const Celula aqui = celulaDe(c.pes);
        if (aqui.col != c.caminho.front().col || aqui.lin != c.caminho.front().lin) break;
        c.caminho.erase(c.caminho.begin());
    }
    const Vec2 ponto = c.caminho.size() > 1 ? centroCelula(c.caminho.front()) : alvo;
    const Vec2 dir = normalizar(subtrair(ponto, c.pes));
    // Um pouco mais rápido que Bill (e bem mais quando está longe), para nunca ficar para trás.
    const float velocidade = std::max(120.0f, p.bill.stats.velocidade) * (dAlvo > 3.0f * TILE ? 1.4f : 1.05f);
    const Vec2 antes = c.pes;
    c.pes = moverComColisao(p.sala, c.pes, CAIXA_COMPANHEIRO, escalar(dir, velocidade * dt), false);
    const float andou = comprimento(subtrair(c.pes, antes));
    c.tempoTravado = andou < 0.2f * velocidade * dt ? c.tempoTravado + dt : 0.0f;
    c.andando = andou > 0.01f;
    c.direcao = direcaoDominante(dir, c.direcao);
    c.tempoAnimacao += dt;
}

const Inimigo* inimigoVisivelMaisPerto(const EstadoPartida& p, Vec2 de, float raio) {
    const Inimigo* melhor = nullptr;
    float menor = raio;
    for (const Inimigo& i : p.inimigos) {
        if (!i.vivo || i.surgindo > 0.0f || !estaVisivel(p.visao, i.pes)) continue;
        const float d = comprimento(subtrair(i.pes, de));
        if (d <= menor) { menor = d; melhor = &i; }
    }
    return melhor;
}

// ---------- habilidades (uma função por amigo; a tabela abaixo liga nome, recarga e função) ----------
void agirMike(EstadoPartida&, float) {}    // passiva: bonusDeLuz e itemMarcado
// Passiva (revelaArmadilhas): o passarinho pousa nas armadilhas na luz. Na primeira que aparece em cada sala o Stan
// avisa em voz alta, para o jogador aprender o que o passarinho quer dizer (24/11, Diego não entendia o brilho).
void agirStan(EstadoPartida& p, float) {
    Companheiro& c = p.companheiro;
    if (c.avisouArmadilha) return;
    for (int l = 0; l < p.sala.lins; ++l)
        for (int col = 0; col < p.sala.cols; ++col)
            if (stanApontaArmadilha(p, col, l)) {
                c.avisouArmadilha = true;
                falar(p, c.pes, "Stan: Bill, cuidado ali!", 1.8f);
                return;
            }
}

void agirBen(EstadoPartida& p, float) {   // barreira de tijolos quando um golpe anunciado vai pegar Bill
    Companheiro& c = p.companheiro;
    if (c.recarga > 0.0f || c.efeito > 0.0f) return;
    const Retangulo caixa = caixaNosPes(p.bill.pes, CAIXA_BILL);
    for (const Aviso& a : p.avisos) {
        if (a.golpe.equipe == Equipe::Jogador || a.tempoRestante > 0.25f || !areaToca(a.golpe.area, caixa)) continue;
        c.efeito = DURACAO_BARREIRA;
        c.recarga = habilidadeDe(Amigo::Ben).recarga;
        ++c.usos;
        falar(p, c.pes, "Ben: Atrás de mim!");
        return;
    }
}

void agirEddie(EstadoPartida& p, float) {   // cura 10% quando Bill passa de 30% (1× por sala)
    Companheiro& c = p.companheiro;
    Jogador& b = p.bill;
    if (c.usouNaSala || !estaVivo(b) || b.hp * 100 >= LIMIAR_EDDIE_PCT * b.stats.hpMax) return;
    const int cura = std::max(1, b.stats.hpMax * CURA_EDDIE_PCT / 100);
    b.hp = std::min(b.stats.hpMax, b.hp + cura);
    b.cura = 0.4f;
    c.usouNaSala = true;
    ++c.usos;
    falar(p, b.pes, "Eddie: +" + std::to_string(cura) + " de vida");
    p.sons.push_back(IdSom::Kit);
}

// Beverly: o estilingue virou a ARMA dela (24/11); o apoio passa a ser a Pontaria (passiva: a bala de prata
// e a labareda atravessam a armadura, em resolverAtaque).
void agirBeverly(EstadoPartida&, float) {}

void agirRichie(EstadoPartida& p, float) {   // provoca o inimigo mais perto de Bill: ele mira no Richie
    Companheiro& c = p.companheiro;
    if (c.recarga > 0.0f) return;
    const Inimigo* alvo = inimigoVisivelMaisPerto(p, p.bill.pes, RAIO_PROVOCAR);
    if (!alvo) return;
    c.alvo = alvo->id;
    c.efeito = DURACAO_PROVOCACAO;
    c.recarga = habilidadeDe(Amigo::Richie).recarga;
    ++c.usos;
    falar(p, c.pes, "Richie: Ei, Coisa! Aqui!");
}

// SDD §24.6 (mesma ordem do enum Amigo).
const HabilidadeCompanheiro HABILIDADES_COMPANHEIRO[TOTAL_AMIGOS] = {
    {"Lanterna extra", "+1 célula de luz e marca o item mais próximo", 0.0f, agirMike},
    {"Barreira de tijolos", "Bloqueia por 2 s os golpes que iam pegar Bill", 12.0f, agirBen},
    {"Olho atento", "Aponta as armadilhas na luz", 0.0f, agirStan},
    {"Primeiros socorros", "Cura 10% da vida abaixo de 30% (1× por sala)", 0.0f, agirEddie},
    {"Pontaria", "A bala de prata e a labareda atravessam a armadura", 0.0f, agirBeverly},
    {"Provocação", "O inimigo mais perto mira no Richie por 2 s", 10.0f, agirRichie},
};

// Arma e poder de cada amigo (24/11, mesma ordem do enum Amigo). A lore: o Mike é da fazenda e do açougue da
// família; o Ben construiu a represa e o Clube; o Stan é escoteiro e observa pássaros; o Eddie acredita que o
// inalador é ácido de bateria (e no livro isso fere a Coisa); a Beverly é a melhor no estilingue e a mais
// destemida; o Richie vive no fliperama e faz as Vozes.
const CombateAmigo COMBATE_AMIGOS[TOTAL_AMIGOS] = {
    {"Cutelo do açougue", "Pistola de abate", "O cutelo do pai, arremessado girando",
     "O estampido da pistola do açougue atordoa", 1.0f, 0.0f, 10.0f,
     "Mike: É ele, Bill. Eu... eu não consigo!", "Mike: Desta vez eu fico. Até o fim!"},
    {"Martelo de carpinteiro", "Desabamento da ponte", "O martelo da represa, arremessado",
     "As tábuas da ponte desabam no alvo e o deixam lento", 0.0f, 2.0f, 16.0f,
     "Ben: Bill... eu tô com medo. Muito medo.", "Ben: Construímos tudo juntos. Vamos terminar juntos!"},
    {"Machadinha de escoteiro", "Revoada", "A machadinha do escoteiro, arremessada",
     "Os pássaros do livro do Stan dão um rasante e desnorteiam", 0.7f, 0.0f, 8.0f,
     "Stan: Isso não é real. Isso não é real...", "Stan: Eu não vou fugir de novo."},
    {"Estaca de cerca", "Ácido do inalador", "A estaca da cerca da Rua Neibolt",
     "A névoa do inalador queima como ácido de bateria e deixa lento", 0.0f, 2.5f, 8.0f,
     "Eddie: Cadê o meu inalador? Eu tô com medo!", "Eddie: Isto é ácido de bateria, palhaço!"},
    {"Estilingue de prata", "Labareda", "A bala de prata do estilingue",
     "Uma labareda sobe do chão e empurra o alvo", 0.4f, 0.0f, 22.0f,
     "Beverly: Bill... eu não consigo me mexer.", "Beverly: Eu não tenho mais medo de você."},
    {"Fichas do fliperama", "As Vozes", "Fichas douradas do fliperama, atiradas em leque",
     "As Vozes do Richie estouram os ouvidos e empurram para longe", 0.4f, 0.0f, 40.0f,
     "Richie: Nem piada eu tenho para isso. Tô com medo!", "Richie: Bip bip, Pennywise. Bip bip!"},
};

// Hostil que conta para o amigo ajudar: vivo, já surgido, à vista e atingível agora.
bool hostilNaTela(const EstadoPartida& p, const Inimigo& i) {
    return i.vivo && i.surgindo <= 0.0f && i.submerso <= 0.0f && i.teleporte <= 0.0f && i.aliado <= 0.0f &&
           i.transicao <= 0.0f && estaVisivel(p.visao, i.pes);
}

Vec2 corpoDe(const Inimigo& i) { return {i.pes.x, i.pes.y - parametrosDe(i.tipo).alturaCorpo / 2.0f}; }

const Inimigo* chefeDaSala(const EstadoPartida& p) {
    for (const Inimigo& i : p.inimigos) {
        if (i.vivo && parametrosDe(i.tipo).chefe) return &i;
    }
    return nullptr;
}

// Quem o amigo ataca agora (ou nullptr): com 2+ hostis na tela, o mais perto dele ao alcance. Contra o chefe ainda
// na forma do medo (ou o Pennywise IV), o chefe conta sozinho: na arena ele é a briga inteira.
const Inimigo* alvoDoAmigo(const EstadoPartida& p) {
    const Companheiro& c = p.companheiro;
    const Inimigo* chefe = chefeDaSala(p);
    const bool chefeLutavel = chefe && hostilNaTela(p, *chefe) && (chefe->tipo == TipoInimigo::PennywiseIV || chefe->fase == 1);
    if (inimigosNaTela(p) < MINIMO_INIMIGOS_AMIGO && !chefeLutavel) return nullptr;
    const Inimigo* melhor = nullptr;
    float menor = ALCANCE_ATAQUE_AMIGO;
    for (const Inimigo& i : p.inimigos) {
        if (!hostilNaTela(p, i)) continue;
        const float d = comprimento(subtrair(i.pes, c.pes));
        if (d <= menor) { menor = d; melhor = &i; }
    }
    return melhor;
}

void lancarAtaque(EstadoPartida& p, const Inimigo& alvo) {
    Companheiro& c = p.companheiro;
    AtaqueDoAmigo a;
    a.ativo = true;
    a.poder = c.proximoPoder;
    a.alvo = alvo.id;
    a.de = {c.pes.x, c.pes.y - 22.0f};
    a.para = corpoDe(alvo);
    a.duracao = a.poder ? PREPARO_PODER_AMIGO : VOO_ARMA_AMIGO;
    c.ataque = a;
    c.proximoPoder = !c.proximoPoder;
    c.recargaAtaque = INTERVALO_ATAQUE_AMIGO;
    if (a.poder) ++c.ataquesPoder; else ++c.ataquesArma;
    c.direcao = direcaoDominante(subtrair(alvo.pes, c.pes), c.direcao);   // vira para quem ataca
    p.sons.push_back(IdSom::Golpe);
}

// O golpe sai quando a arma chega (ou o poder termina de surgir). O alvo pode ter morrido no caminho.
void resolverAtaque(EstadoPartida& p) {
    Companheiro& c = p.companheiro;
    c.ataque.resolvido = true;
    const Inimigo* alvo = nullptr;
    for (const Inimigo& i : p.inimigos) {
        if (i.id == c.ataque.alvo) alvo = &i;
    }
    if (!alvo || !alvo->vivo) return;
    const CombateAmigo& def = combateDe(c.quem);
    Golpe g;
    g.id = p.proximoGolpe++;
    g.equipe = Equipe::Jogador;
    g.area = areaCirculo(corpoDe(*alvo), 10.0f);
    g.dano = danoDoAmigo();
    if (c.quem == Amigo::Beverly) g.dano += parametrosDe(alvo->tipo).armadura;   // Pontaria: atravessa a armadura
    g.rolaAcerto = false;   // arremesso e poder não erram (PRD §8.1); a esquiva do alvo continua valendo
    g.duracao = 0.05f;
    g.doAmigo = true;
    g.recuo = c.ataque.poder ? def.recuo : 6.0f;
    if (c.ataque.poder) {
        g.atordoa = def.atordoa;
        g.lentidao = def.lento;
    }
    p.golpes.push_back(g);
}

void lutar(EstadoPartida& p, float dt) {
    Companheiro& c = p.companheiro;
    if (c.ataque.ativo) {
        c.ataque.tempo += dt;
        if (!c.ataque.resolvido) {   // a arma segue o alvo enquanto voa
            for (const Inimigo& i : p.inimigos) {
                if (i.id == c.ataque.alvo && i.vivo) c.ataque.para = corpoDe(i);
            }
            if (c.ataque.tempo >= c.ataque.duracao) resolverAtaque(p);
        }
        if (c.ataque.tempo >= c.ataque.duracao + EFEITO_ATAQUE_AMIGO) c.ataque.ativo = false;
    }
    if (c.recargaAtaque > 0.0f) c.recargaAtaque -= dt;
    if (c.comMedo || c.recargaAtaque > 0.0f || (c.ataque.ativo && !c.ataque.resolvido)) return;
    if (const Inimigo* alvo = alvoDoAmigo(p)) lancarAtaque(p, *alvo);
}

// O chefe virou Pennywise: o amigo trava de medo (fora do Covil). No Covil, ele fala e fica.
void verMedo(EstadoPartida& p) {
    Companheiro& c = p.companheiro;
    const Inimigo* chefe = chefeDaSala(p);
    if (!chefe) return;
    const CombateAmigo& def = combateDe(c.quem);
    if (chefe->tipo == TipoInimigo::PennywiseIV) {
        if (!c.falouNoCovil && estaVisivel(p.visao, chefe->pes)) {
            c.falouNoCovil = true;
            falar(p, c.pes, def.falaCovil, 2.6f);
        }
        return;
    }
    if (!c.comMedo && (chefe->fase >= 2 || chefe->transicao > 0.0f)) {
        c.comMedo = true;
        c.ataque = AtaqueDoAmigo{};
        c.efeito = 0.0f;   // a barreira do Ben e a provocação do Richie acabam
        c.alvo = ID_NULO;
        falar(p, c.pes, def.falaMedo, 2.6f);
    }
}

}  // namespace

const CombateAmigo& combateDe(Amigo a) {
    const int i = static_cast<int>(a);
    return COMBATE_AMIGOS[(i >= 0 && i < TOTAL_AMIGOS) ? i : 0];
}

int danoDoAmigo() { return calcularStats(Atributos{}, Equipamento{}).danoBase; }

int inimigosNaTela(const EstadoPartida& p) {
    int n = 0;
    for (const Inimigo& i : p.inimigos) n += hostilNaTela(p, i) ? 1 : 0;
    return n;
}

bool amigoPodeDerrubar(const EstadoPartida& p, const Inimigo& alvo) {
    if (parametrosDe(alvo.tipo).chefe) return false;
    for (const Inimigo& i : p.inimigos) {
        if (i.id != alvo.id && i.vivo && i.aliado <= 0.0f) return true;
    }
    return false;
}

const char* codigoAmigo(Amigo a) {
    const int i = static_cast<int>(a);
    return (i >= 0 && i < TOTAL_AMIGOS) ? CODIGOS[i] : "";
}

const char* nomeAmigo(Amigo a) {
    const int i = static_cast<int>(a);
    return (i >= 0 && i < TOTAL_AMIGOS) ? NOMES[i] : "?";
}

Amigo amigoDoCodigo(const std::string& codigo) {
    for (int i = 0; i < TOTAL_AMIGOS; ++i) {
        if (codigo == CODIGOS[i]) return static_cast<Amigo>(i);
    }
    return Amigo::Total;
}

const HabilidadeCompanheiro& habilidadeDe(Amigo a) {
    const int i = static_cast<int>(a);
    return HABILIDADES_COMPANHEIRO[(i >= 0 && i < TOTAL_AMIGOS) ? i : 0];
}

void entrarCompanheiro(EstadoPartida& p, Amigo quem, Vec2 origem) {
    if (quem == Amigo::Total) {
        p.companheiro = Companheiro{};
        return;
    }
    Companheiro c;
    c.ativo = true;
    c.quem = quem;
    c.pes = (origem.x != 0.0f || origem.y != 0.0f) ? origem : p.bill.pes;
    p.companheiro = c;
    if (origem.x == 0.0f && origem.y == 0.0f) p.companheiro.pes = lugarPertoDoBill(p);
}

void companheiroEntrouNaSala(EstadoPartida& p) {
    if (!p.companheiro.ativo) return;
    Companheiro& c = p.companheiro;
    c.pes = lugarPertoDoBill(p);
    c.caminho.clear();
    c.tempoTravado = 0.0f;
    c.usouNaSala = false;
    c.efeito = 0.0f;
    c.alvo = ID_NULO;
    c.recargaAtaque = PRIMEIRO_ATAQUE_AMIGO;
    c.ataque = AtaqueDoAmigo{};
    c.comMedo = false;
    c.falouNoCovil = false;
    c.avisouArmadilha = false;
}

void atualizarCompanheiro(EstadoPartida& p, float dt) {
    Companheiro& c = p.companheiro;
    if (!c.ativo) return;
    if (c.recarga > 0.0f) c.recarga -= dt;
    if (c.efeito > 0.0f) c.efeito -= dt;
    if (c.tiro > 0.0f) c.tiro -= dt;
    if (c.efeito <= 0.0f) c.alvo = ID_NULO;
    c.relogio += dt;
    seguirBill(p, dt);
    verMedo(p);
    if (!c.comMedo) habilidadeDe(c.quem).agir(p, dt);   // com medo, ele não ajuda (Diego, 24/11)
    lutar(p, dt);
}

int bonusDeLuz(const EstadoPartida& p) { return p.companheiro.ativo && p.companheiro.quem == Amigo::Mike ? 1 : 0; }

bool barreiraAtiva(const EstadoPartida& p) {
    return p.companheiro.ativo && p.companheiro.quem == Amigo::Ben && p.companheiro.efeito > 0.0f;
}

bool revelaArmadilhas(const EstadoPartida& p) { return p.companheiro.ativo && p.companheiro.quem == Amigo::Stan; }

bool stanApontaArmadilha(const EstadoPartida& p, int col, int lin) {
    if (!revelaArmadilhas(p) || p.companheiro.comMedo) return false;
    const TipoTile t = tileEm(p.sala, col, lin).tipo;
    const bool armadilha = t == TipoTile::Espinhos || t == TipoTile::Jato || t == TipoTile::PisoCedendo || t == TipoTile::Fogo;
    return armadilha && visaoEm(p.visao, col, lin).luz > 0;
}

IdEntidade itemMarcado(const EstadoPartida& p) {
    if (!p.companheiro.ativo || p.companheiro.quem != Amigo::Mike) return ID_NULO;
    IdEntidade melhor = ID_NULO;
    float menor = 1e9f;
    for (const ItemNoChao& it : p.itensChao) {
        if (!estaVisivel(p.visao, it.pos)) continue;
        const float d = comprimento(subtrair(it.pos, p.bill.pes));
        if (d < menor) { menor = d; melhor = it.id; }
    }
    return melhor;
}

bool provocado(const EstadoPartida& p, IdEntidade inimigo) {
    const Companheiro& c = p.companheiro;
    return c.ativo && c.quem == Amigo::Richie && c.efeito > 0.0f && c.alvo == inimigo && inimigo != ID_NULO;
}
