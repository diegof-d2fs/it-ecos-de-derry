#include "mundo/tile.h"

#include <algorithm>
#include <cmath>

namespace {

struct PropriedadesTile {
    TipoTile tipo;
    bool bloqueiaMovimento;
    bool bloqueiaVisao;
    bool bloqueiaVoo;
    float fatorVelocidade;
    MaterialPiso material;
};

// Tabela única (mesma ordem de TipoTile). Alterar uma regra de terreno = alterar uma linha.
const PropriedadesTile TABELA[static_cast<int>(TipoTile::Total)] = {
    //  tipo                      move   visão  voo    veloc. material
    {TipoTile::Vazio,             true,  true,  true,  1.0f, MaterialPiso::Parede},
    {TipoTile::Chao,              false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::ChaoVariante,      false, false, false, 1.0f, MaterialPiso::PisoVariante},
    {TipoTile::Parede,            true,  true,  true,  1.0f, MaterialPiso::Parede},
    {TipoTile::AguaRasa,          false, false, false, 0.7f, MaterialPiso::Liquido},
    {TipoTile::AguaFunda,         true,  false, false, 1.0f, MaterialPiso::LiquidoFundo},
    {TipoTile::Buraco,            true,  false, false, 1.0f, MaterialPiso::Buraco},
    {TipoTile::Lama,              false, false, false, 0.6f, MaterialPiso::PisoVariante},
    {TipoTile::Detrito,           true,  true,  true,  1.0f, MaterialPiso::Piso},
    {TipoTile::PortaSala,         false, false, true,  1.0f, MaterialPiso::Piso},
    {TipoTile::PortaTrancada,     true,  true,  true,  1.0f, MaterialPiso::Piso},
    {TipoTile::Grade,             true,  false, true,  1.0f, MaterialPiso::Piso},
    {TipoTile::Ponte,             false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::Ilusao,            true,  true,  true,  1.0f, MaterialPiso::Parede},
    {TipoTile::Espinhos,          false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::PisoCedendo,       false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::Jato,              false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::Fogo,              false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::Esteira,           false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::PocaSangue,        false, false, false, 1.0f, MaterialPiso::Piso},
    {TipoTile::Refugio,           false, false, false, 1.0f, MaterialPiso::PisoVariante},
    {TipoTile::ParedeRachada,     true,  true,  true,  1.0f, MaterialPiso::Parede},   // parece parede
    {TipoTile::Labareda,          false, false, false, 1.0f, MaterialPiso::Piso},
};

const PropriedadesTile& propriedades(TipoTile tipo) {
    const int i = static_cast<int>(tipo);
    return TABELA[(i >= 0 && i < static_cast<int>(TipoTile::Total)) ? i : 0];
}

}  // namespace

Vec2 direcaoDaEsteira(const Tile& t) {
    static const Vec2 D[4] = {{1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}};
    return D[t.estado & 3];
}

Vec2 contraEsteira(const Tile& t, Vec2 desejada, float velocidade) {
    if (comprimento(desejada) < 0.001f) return {};
    if (t.tipo != TipoTile::Esteira) return normalizar(desejada);   // sempre unitária (quem chama escala)
    const Vec2 d = normalizar(desejada);
    const Vec2 b = escalar(direcaoDaEsteira(t), VELOCIDADE_ESTEIRA);
    const float v = std::max(velocidade, VELOCIDADE_ESTEIRA + 1.0f);
    const float db = d.x * b.x + d.y * b.y;
    const float k = db + std::sqrt(std::max(0.0f, db * db - (b.x * b.x + b.y * b.y) + v * v));   // |k·d − b| = v
    return normalizar(subtrair(escalar(d, k), b));
}

Tile tileDoSimbolo(char s) {
    Tile t = criarTile(tipoDoSimbolo(s));
    if (t.tipo == TipoTile::Esteira) t.estado = s == '>' ? 0 : s == '<' ? 1 : s == 'v' ? 2 : 3;
    return t;
}

Tile criarTile(TipoTile tipo) {
    const PropriedadesTile& p = propriedades(tipo);
    Tile t;
    t.tipo = tipo;
    t.bloqueiaMovimento = p.bloqueiaMovimento;
    t.bloqueiaVisao = p.bloqueiaVisao;
    t.bloqueiaVoo = p.bloqueiaVoo;
    return t;
}

float fatorVelocidade(TipoTile tipo) { return propriedades(tipo).fatorVelocidade; }

MaterialPiso materialDe(TipoTile tipo) { return propriedades(tipo).material; }

TipoTile tipoDoSimbolo(char s) {
    switch (s) {
        case '#': return TipoTile::Parede;
        case '.': return TipoTile::Chao;
        case ',': return TipoTile::ChaoVariante;
        case '~': return TipoTile::AguaRasa;
        case 'w': return TipoTile::AguaFunda;
        case 'o': return TipoTile::Buraco;
        case '=': return TipoTile::Ponte;
        case 'd': return TipoTile::Detrito;
        case 'g': return TipoTile::Grade;
        case 'i': return TipoTile::Ilusao;
        case 'X': return TipoTile::PisoCedendo;
        case 'J': return TipoTile::Jato;
        case '^': return TipoTile::Espinhos;
        case 'm': return TipoTile::Lama;
        case 'f': return TipoTile::Fogo;
        case '>': case '<': case 'v': case 'A': return TipoTile::Esteira;
        case 'b': return TipoTile::PocaSangue;
        case 'R': return TipoTile::Refugio;
        case 'r': return TipoTile::ParedeRachada;
        case 'N': case 'S': case 'L': case 'O': return TipoTile::PortaSala;
        case 'T': return TipoTile::PortaTrancada;
        // Objetos e marcadores: ficam sobre chão comum.
        case 'E': case 's': case 'a': case 'K': case 'k': case 'B': case 'c': case '*':
        case 'l': case '@': case 'M': case 'Q': case 'H': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            return TipoTile::Chao;
        default: return TipoTile::Total;
    }
}
