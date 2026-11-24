#include "mundo/sala.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <sstream>

#include "core/arquivo.h"

namespace {

const char* const NOMES_TIPO_SALA[static_cast<int>(TipoSala::Total)] = {
    "entrada", "combate", "eco", "tesouro", "loja", "refugio", "resgate", "chefe", "prologo", "clube", "covil", "teste",
    "bonus",
};

const char* const AMBIENTES_BIOMA[] = {"rua_derry", "galeria", "barrens", "neibolt",
                                       "poroes", "siderurgica", "cisterna", "covil"};

std::string aparar(const std::string& s) {
    const auto inicio = s.find_first_not_of(" \t\r");
    if (inicio == std::string::npos) return "";
    const auto fim = s.find_last_not_of(" \t\r");
    return s.substr(inicio, fim - inicio + 1);
}

bool lerInteiro(const std::string& valor, int& destino) {
    char* fim = nullptr;
    const long v = std::strtol(valor.c_str(), &fim, 10);
    if (fim == valor.c_str() || *fim != '\0') return false;
    destino = static_cast<int>(v);
    return true;
}

bool ehObjeto(char c) {
    return c == 'K' || c == 'k' || c == 'B' || c == 'c' || c == '*' || c == 'M' || c == 'Q' || c == 'H' || c == 'l' ||
           (c >= '1' && c <= '9');
}

Direcao ladoDaPorta(char c) {
    switch (c) {
        case 'N': return Direcao::Cima;
        case 'L': return Direcao::Direita;
        case 'O': return Direcao::Esquerda;
        default: return Direcao::Baixo;
    }
}

// Cabeçalho "chave=valor". Devolve false com o motivo se a chave for desconhecida ou inválida.
bool aplicarChave(SalaModelo& sala, const std::string& chave, const std::string& valor, std::string& motivo) {
    if (chave == "id") sala.id = valor;
    else if (chave == "nome") sala.nome = valor;
    else if (chave == "ambiente") sala.ambiente = valor;
    else if (chave == "bioma") {
        if (!lerInteiro(valor, sala.bioma) || sala.bioma < 0 || sala.bioma > 7) {
            motivo = "bioma deve ser 0..7";
            return false;
        }
    } else if (chave == "tipo") {
        for (int i = 0; i < static_cast<int>(TipoSala::Total); ++i) {
            if (valor == NOMES_TIPO_SALA[i]) {
                sala.tipo = static_cast<TipoSala>(i);
                return true;
            }
        }
        motivo = "tipo desconhecido '" + valor + "'";
        return false;
    } else if (chave == "ondas") {
        if (!lerInteiro(valor, sala.ondas)) { motivo = "ondas inválido"; return false; }
    } else if (chave == "orcamento") {
        if (!lerInteiro(valor, sala.orcamentoOnda)) { motivo = "orcamento inválido"; return false; }
    } else if (chave == "entrada") {
        if (valor != "fixa" && valor != "livre") { motivo = "entrada deve ser fixa ou livre"; return false; }
        sala.entradaFixa = valor == "fixa";
    } else if (chave == "musica") {
        if (valor != "normal" && valor != "tensao") { motivo = "musica deve ser normal ou tensao"; return false; }
        sala.musicaTensao = valor == "tensao";
    } else if (chave == "npc") {
        // "npc=1 ben": o NPC '1' do mapa é o Ben (sprite ben_idle, falas do Ben).
        std::istringstream v(valor);
        char digito = 0;
        std::string nome;
        if (!(v >> digito >> nome) || digito < '1' || digito > '9') { motivo = "npc deve ser '<1..9> <nome>'"; return false; }
        sala.npcs[static_cast<std::size_t>(digito - '0')] = nome;
    } else if (chave == "ao_resgatar") {
        // "ao_resgatar=<col> <lin> <símbolo>": ex. água funda que vira ponte quando o Ben é resgatado.
        std::istringstream v(valor);
        SalaModelo::Alteracao a;
        if (!(v >> a.celula.col >> a.celula.lin >> a.simbolo) || tipoDoSimbolo(a.simbolo) == TipoTile::Total) {
            motivo = "ao_resgatar deve ser '<col> <lin> <símbolo do mapa>'";
            return false;
        }
        sala.aoResgatar.push_back(a);
    } else if (chave == "opcional") {
        std::istringstream v(valor);
        Celula c;
        if (!(v >> c.col >> c.lin)) {
            motivo = "opcional deve ser '<col> <lin>' (uma armadilha que pode ficar desligada)";
            return false;
        }
        sala.armadilhasOpcionais.push_back(c);
    } else if (chave == "ao_resgatar_enfeite") {
        std::istringstream v(valor);
        SalaModelo::TrocaEnfeite t;
        std::string passa;
        if (!(v >> t.celula.col >> t.celula.lin >> t.quadro) || t.quadro < 0 || t.quadro > 15 || ((v >> passa) && passa != "passa")) {
            motivo = "ao_resgatar_enfeite deve ser '<col> <lin> <quadro 0..15> [passa]'";
            return false;
        }
        t.passa = passa == "passa";
        sala.enfeitesAoResgatar.push_back(t);
    } else if (chave == "enfeite") {
        // "enfeite=<quadro> <col> <lin>": objeto decorativo (ocupa a célula) da folha objetos_<ambiente>.
        std::istringstream v(valor);
        ObjetoSala o;
        o.codigo = 'e';
        if (!(v >> o.quadro >> o.celula.col >> o.celula.lin) || o.quadro < 0 || o.quadro > 15) {
            motivo = "enfeite deve ser '<quadro 0..15> <col> <lin>'";
            return false;
        }
        sala.objetos.push_back(o);
    } else {
        motivo = "chave desconhecida '" + chave + "'";
        return false;
    }
    return true;
}

// Passável no "estado final" da sala (grade aberta, porta destrancada, detrito quebrado,
// ilusão desfeita, parede rachada derrubada): é o que a BFS da validação usa para garantir que
// nada fica inalcançável.
bool passavelFinal(const Tile& t, bool trancadaBloqueia, bool rachadaBloqueia = false) {
    switch (t.tipo) {
        case TipoTile::PisoCedendo: return false;   // o pior caso: todos os pisos já caíram (viraram buraco)
        case TipoTile::PortaTrancada: return !trancadaBloqueia;
        case TipoTile::ParedeRachada: return !rachadaBloqueia;
        case TipoTile::Grade: case TipoTile::Detrito: case TipoTile::Ilusao: return true;
        default: return !t.bloqueiaMovimento;
    }
}

// Enfeite bloqueia a célula, a não ser que o resgate o torne passável ("passa": estado final).
bool enfeiteBloqueia(const SalaModelo& s, int col, int lin) {
    for (const ObjetoSala& o : s.objetos) {
        if (o.codigo != 'e' || o.celula.col != col || o.celula.lin != lin) continue;
        for (const SalaModelo::TrocaEnfeite& t : s.enfeitesAoResgatar) {
            if (t.passa && t.celula.col == col && t.celula.lin == lin) return false;
        }
        return true;
    }
    return false;
}

std::vector<bool> alcancaveis(const SalaModelo& s, bool trancadaBloqueia, bool rachadaBloqueia = false) {
    std::vector<bool> visto(static_cast<size_t>(s.cols * s.lins), false);
    const Celula e = s.entradaJogador;
    if (e.col < 0) return visto;
    std::deque<Celula> fila{e};
    visto[static_cast<size_t>(e.lin * s.cols + e.col)] = true;
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    while (!fila.empty()) {
        const Celula c = fila.front();
        fila.pop_front();
        for (int k = 0; k < 4; ++k) {
            const int nc = c.col + dc[k], nl = c.lin + dl[k];
            if (nc < 0 || nl < 0 || nc >= s.cols || nl >= s.lins) continue;
            const size_t i = static_cast<size_t>(nl * s.cols + nc);
            Tile t = tileEm(s, nc, nl);
            for (const SalaModelo::Alteracao& a : s.aoResgatar) {   // o mapa depois do resgate
                if (a.celula.col == nc && a.celula.lin == nl) t = tileDoSimbolo(a.simbolo);
            }
            if (visto[i] || !passavelFinal(t, trancadaBloqueia, rachadaBloqueia)) continue;
            if (enfeiteBloqueia(s, nc, nl)) continue;   // enfeite ocupa a célula (como na partida)
            visto[i] = true;
            fila.push_back({nc, nl});
        }
    }
    return visto;
}

std::string onde(Celula c) {
    return "(col " + std::to_string(c.col) + ", lin " + std::to_string(c.lin) + ")";
}

}  // namespace

const char* ambientePadrao(int bioma) {
    return AMBIENTES_BIOMA[(bioma >= 0 && bioma < 8) ? bioma : 0];
}

bool lerSala(const std::string& texto, SalaModelo& sala, std::string& erro) {
    sala = SalaModelo{};
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    enum class Parte { Cabecalho, Mapa, Rodape } parte = Parte::Cabecalho;
    std::vector<std::string> mapa;
    auto falhar = [&](const std::string& motivo) {
        erro = "linha " + std::to_string(numero) + ": " + motivo;
        return false;
    };

    while (std::getline(entrada, linha)) {
        ++numero;
        if (!linha.empty() && linha.back() == '\r') linha.pop_back();
        if (numero == 1 && linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);   // BOM do Bloco de Notas
        if (parte == Parte::Mapa) {
            if (aparar(linha).empty()) {   // linha em branco encerra o mapa
                parte = Parte::Rodape;
                continue;
            }
            mapa.push_back(linha);
            continue;
        }
        const std::string limpa = aparar(linha);
        // No cabeçalho '#' é comentário; no mapa '#' é parede (por isso o mapa vem marcado).
        if (limpa.empty() || limpa[0] == '#') continue;
        if (limpa == "mapa:") {
            if (parte == Parte::Rodape) return falhar("mais de um 'mapa:'");
            parte = Parte::Mapa;
            continue;
        }
        const auto igual = limpa.find('=');
        if (igual == std::string::npos) return falhar("esperado 'chave=valor' ou 'mapa:'");
        std::string motivo;
        if (!aplicarChave(sala, aparar(limpa.substr(0, igual)), aparar(limpa.substr(igual + 1)), motivo)) {
            return falhar(motivo);
        }
    }

    numero = 0;
    if (mapa.empty()) return falhar("sala sem 'mapa:'");
    sala.lins = static_cast<int>(mapa.size());
    sala.cols = static_cast<int>(mapa[0].size());
    if (sala.cols > SALA_MAX_COLS || sala.lins > SALA_MAX_LINS) {
        return falhar("mapa maior que " + std::to_string(SALA_MAX_COLS) + "×" + std::to_string(SALA_MAX_LINS));
    }
    for (int l = 0; l < sala.lins; ++l) {
        const std::string& m = mapa[static_cast<size_t>(l)];
        if (static_cast<int>(m.size()) != sala.cols) {
            erro = "mapa, linha " + std::to_string(l + 1) + ": largura " + std::to_string(m.size()) +
                   " (esperado " + std::to_string(sala.cols) + ")";
            return false;
        }
        for (int c = 0; c < sala.cols; ++c) {
            const char s = m[static_cast<size_t>(c)];
            const TipoTile tipo = tipoDoSimbolo(s);
            if (tipo == TipoTile::Total) {
                erro = "mapa, linha " + std::to_string(l + 1) + ", coluna " + std::to_string(c + 1) +
                       ": símbolo desconhecido '" + std::string(1, s) + "'";
                return false;
            }
            sala.tiles[static_cast<size_t>(l * sala.cols + c)] = tileDoSimbolo(s);
            const Celula cel{c, l};
            if (s == 'E') {
                if (sala.entradaJogador.col >= 0) {
                    erro = "mais de uma entrada 'E' (a segunda em " + onde(cel) + ")";
                    return false;
                }
                sala.entradaJogador = cel;
            } else if (s == '@') {
                sala.posChefe = cel;
            } else if (s == 's' || s == 'a') {
                sala.spawns.push_back({cel, s == 'a'});
            } else if (s == 'N' || s == 'S' || s == 'L' || s == 'O') {
                sala.portas.push_back({cel, ladoDaPorta(s), true});
            } else if (ehObjeto(s)) {
                if (s >= '1' && s <= '9' && sala.npcs[static_cast<std::size_t>(s - '0')].empty()) {
                    erro = "NPC '" + std::string(1, s) + "' em " + onde(cel) + " sem 'npc=" + std::string(1, s) + " <nome>'";
                    return false;
                }
                ObjetoSala o;
                o.codigo = s;
                o.celula = cel;
                sala.objetos.push_back(o);
            }
        }
    }
    for (const SalaModelo::Alteracao& a : sala.aoResgatar) {
        if (a.celula.col < 0 || a.celula.lin < 0 || a.celula.col >= sala.cols || a.celula.lin >= sala.lins) {
            erro = "ao_resgatar fora do mapa em " + onde(a.celula);
            return false;
        }
    }
    for (const ObjetoSala& o : sala.objetos) {   // enfeites (vieram do cabeçalho): dentro e no chão
        if (o.codigo != 'e') continue;
        const Celula c = o.celula;
        if (c.col < 0 || c.lin < 0 || c.col >= sala.cols || c.lin >= sala.lins ||
            tileEm(sala, c.col, c.lin).bloqueiaMovimento) {
            erro = "enfeite fora do chão em " + onde(c);
            return false;
        }
    }
    if (sala.ambiente.empty()) sala.ambiente = ambientePadrao(sala.bioma);
    if (sala.id.empty()) return falhar("falta 'id='");
    return true;
}

bool carregarSala(const std::filesystem::path& caminho, SalaModelo& sala, std::string& erro) {
    std::string texto;
    if (!lerArquivoTexto(caminho, texto)) {
        erro = caminho.filename().u8string() + ": não foi possível abrir";
        return false;
    }
    if (!lerSala(texto, sala, erro)) {
        erro = caminho.filename().u8string() + ", " + erro;
        return false;
    }
    return true;
}

bool carregarSalas(const std::filesystem::path& pasta, std::vector<SalaModelo>& salas, std::string& erro) {
    std::error_code ec;
    std::vector<std::filesystem::path> arquivos;
    for (const auto& item : std::filesystem::directory_iterator(pasta, ec)) {
        if (item.is_regular_file() && item.path().extension() == ".txt") arquivos.push_back(item.path());
    }
    if (ec) {
        erro = "pasta de salas não encontrada: " + pasta.u8string();
        return false;
    }
    std::sort(arquivos.begin(), arquivos.end());
    salas.clear();
    salas.reserve(arquivos.size());   // depois disto o vetor não cresce mais (ponteiros estáveis)
    for (const auto& caminho : arquivos) {
        SalaModelo sala;
        if (!carregarSala(caminho, sala, erro)) return false;
        salas.push_back(std::move(sala));
    }
    if (salas.empty()) {
        erro = "nenhuma sala em " + pasta.u8string();
        return false;
    }
    return true;
}

int minimoDePortas(TipoSala tipo) {
    switch (tipo) {
        case TipoSala::Entrada: return 2;   // nenhuma é de entrada (Bill chega pela descida): 2 opções
        case TipoSala::Resgate: return 2;   // mão única (entrada=fixa): entra e sai
        case TipoSala::Chefe: return 2;     // entra e desce
        case TipoSala::Clube: case TipoSala::Prologo: case TipoSala::Teste: return 0;   // fora do gerador
        default: return 3;                  // entra por 1 e escolhe entre 2
    }
}

std::vector<std::string> validarSala(const SalaModelo& s) {
    std::vector<std::string> problemas;
    if (s.cols < 3 || s.lins < 3 || s.cols > SALA_MAX_COLS || s.lins > SALA_MAX_LINS) {
        problemas.push_back("dimensões fora do permitido");
        return problemas;
    }
    // 1. Borda fechada por parede ou porta.
    for (int l = 0; l < s.lins; ++l) {
        for (int c = 0; c < s.cols; ++c) {
            if (l != 0 && c != 0 && l != s.lins - 1 && c != s.cols - 1) continue;
            const TipoTile t = tileEm(s, c, l).tipo;
            if (t != TipoTile::Parede && t != TipoTile::PortaSala && t != TipoTile::PortaTrancada) {
                problemas.push_back("borda aberta em " + onde({c, l}));
            }
        }
    }
    // 2. Exatamente uma entrada; portas com vizinho caminhável.
    if (s.entradaJogador.col < 0) problemas.push_back("sem entrada 'E'");
    const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
    for (const PortaSala& p : s.portas) {
        bool temVizinho = false;
        for (int k = 0; k < 4; ++k) {
            const int nc = p.celula.col + dc[k], nl = p.celula.lin + dl[k];
            if (nc >= 0 && nl >= 0 && nc < s.cols && nl < s.lins && !tileEm(s, nc, nl).bloqueiaMovimento &&
                tileEm(s, nc, nl).tipo != TipoTile::PortaSala) {
                temVizinho = true;
            }
        }
        if (!temVizinho) problemas.push_back("porta sem vizinho caminhável em " + onde(p.celula));
    }
    if (s.entradaJogador.col < 0) return problemas;

    // 3. BFS a partir de E alcança portas e objetos.
    const std::vector<bool> alcance = alcancaveis(s, false);
    auto alcancou = [&](Celula c) { return alcance[static_cast<size_t>(c.lin * s.cols + c.col)]; };
    for (const PortaSala& p : s.portas) {
        if (!alcancou(p.celula)) problemas.push_back("porta inalcançável em " + onde(p.celula));
    }
    for (const ObjetoSala& o : s.objetos) {
        if (o.codigo == 'e') continue;   // enfeite é decoração: não precisa ser alcançado (só não pode isolar nada)
        if (!alcancou(o.celula)) {
            problemas.push_back(std::string("objeto '") + o.codigo + "' inalcançável em " + onde(o.celula));
        }
    }
    // 4. Nenhum spawn a menos de 4 células da entrada (distância de Chebyshev).
    for (const PontoSpawn& sp : s.spawns) {
        const int d = std::max(std::abs(sp.celula.col - s.entradaJogador.col),
                               std::abs(sp.celula.lin - s.entradaJogador.lin));
        if (d < 4) problemas.push_back("spawn perto demais da entrada em " + onde(sp.celula));
    }
    // 5. Regras por tipo.
    if (s.tipo == TipoSala::Combate && s.spawns.size() < 4) problemas.push_back("sala de combate com menos de 4 spawns");
    // "opcional=" só faz sentido numa célula de armadilha.
    for (const Celula& c : s.armadilhasOpcionais) {
        const TipoTile t = tileEm(s, c.col, c.lin).tipo;
        if (t != TipoTile::Espinhos && t != TipoTile::PisoCedendo && t != TipoTile::Jato && t != TipoTile::Fogo && t != TipoTile::PocaSangue)
            problemas.push_back("opcional sem armadilha em " + onde(c));
    }
    // A troca de quadro no resgate precisa de um enfeite naquela célula (a comporta do Stan).
    for (const SalaModelo::TrocaEnfeite& t : s.enfeitesAoResgatar) {
        const bool existe = std::any_of(s.objetos.begin(), s.objetos.end(), [&](const ObjetoSala& o) {
            return o.codigo == 'e' && o.celula.col == t.celula.col && o.celula.lin == t.celula.lin;
        });
        if (!existe) problemas.push_back("ao_resgatar_enfeite sem enfeite em " + onde(t.celula));
    }
    // Portas mínimas (Etapa 10.1): Bill entra por uma (que fica selada) e a etapa seguinte oferece
    // até 2 opções. Uma sala de teste com 1 porta só virou beco sem saída dentro da tentativa.
    const int minimo = minimoDePortas(s.tipo);
    if (static_cast<int>(s.portas.size()) < minimo) {
        problemas.push_back("sala com " + std::to_string(s.portas.size()) + " porta(s); o tipo pede pelo menos " +
                            std::to_string(minimo));
    }
    if (s.tipo == TipoSala::Resgate) {
        bool temTrancada = false;
        for (int i = 0; i < s.cols * s.lins; ++i) temTrancada |= s.tiles[static_cast<size_t>(i)].tipo == TipoTile::PortaTrancada;
        if (!temTrancada) problemas.push_back("sala de resgate sem porta trancada 'T'");
    }
    // 6. Segredo (SDD §24.5): a Lembrancinha só se alcança DERRUBANDO a parede rachada, e cada
    //    parede rachada tem os dois lados caminháveis (senão ela não levaria a lugar nenhum).
    const std::vector<bool> semRachada = alcancaveis(s, false, true);
    for (const ObjetoSala& o : s.objetos) {
        if (o.codigo == 'l' && semRachada[static_cast<size_t>(o.celula.lin * s.cols + o.celula.col)]) {
            problemas.push_back("Lembrancinha alcançável sem derrubar a parede rachada em " + onde(o.celula));
        }
    }
    for (int l = 0; l < s.lins; ++l) {
        for (int c = 0; c < s.cols; ++c) {
            if (tileEm(s, c, l).tipo != TipoTile::ParedeRachada) continue;
            auto livre = [&](int cc, int ll) { return !tileEm(s, cc, ll).bloqueiaMovimento; };
            if (!(livre(c - 1, l) && livre(c + 1, l)) && !(livre(c, l - 1) && livre(c, l + 1))) {
                problemas.push_back("parede rachada sem os dois lados caminháveis em " + onde({c, l}));
            }
        }
    }
    const std::vector<bool> semPassarT = alcancaveis(s, true);
    for (const ObjetoSala& o : s.objetos) {
        if (o.codigo == 'K' && !semPassarT[static_cast<size_t>(o.celula.lin * s.cols + o.celula.col)]) {
            problemas.push_back("Chave do Medo só alcançável passando pela porta trancada");
        }
    }
    return problemas;
}

SalaViva criarSalaViva(const SalaModelo& modelo) {
    SalaViva v;
    v.modelo = &modelo;
    v.cols = modelo.cols;
    v.lins = modelo.lins;
    v.tiles = modelo.tiles;
    v.portas = modelo.portas;
    v.objetos = modelo.objetos;
    return v;
}

bool dentroDaSala(const SalaViva& sala, int col, int lin) {
    return col >= 0 && lin >= 0 && col < sala.cols && lin < sala.lins;
}

namespace {
const Tile PAREDE_EXTERNA = criarTile(TipoTile::Parede);
}

const Tile& tileEm(const SalaViva& sala, int col, int lin) {
    if (!dentroDaSala(sala, col, lin)) return PAREDE_EXTERNA;
    return sala.tiles[static_cast<size_t>(lin * sala.cols + col)];
}

const Tile& tileEm(const SalaModelo& sala, int col, int lin) {
    if (col < 0 || lin < 0 || col >= sala.cols || lin >= sala.lins) return PAREDE_EXTERNA;
    return sala.tiles[static_cast<size_t>(lin * sala.cols + col)];
}

Vec2 centroCelula(Celula c) {
    return {c.col * static_cast<float>(TILE) + TILE / 2.0f, c.lin * static_cast<float>(TILE) + TILE / 2.0f};
}
