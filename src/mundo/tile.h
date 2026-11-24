#pragma once
// Tiles do mundo (SDD §3.1). Cada símbolo do arquivo de sala vira um Tile com as suas
// propriedades de colisão e visão. Regras puras: sem Windows.
#include <cstdint>

#include "core/tipos.h"

enum class TipoTile : std::uint8_t {
    Vazio, Chao, ChaoVariante, Parede, AguaRasa, AguaFunda, Buraco, Lama, Detrito, PortaSala, PortaTrancada,
    Grade, Ponte, Ilusao, Espinhos, PisoCedendo, Jato, Fogo, Esteira, PocaSangue, Refugio,
    ParedeRachada,   // 'r' (Etapa 8.4): parede que o Especial ou o Clarão derrubam (segredo, SDD §24.5)
    Labareda,        // sem símbolo (Etapa 13.3): fogo TEMPORÁRIO no piso, aceso pela chuva de brasas do Pássaro
    Total
};

struct Tile {
    TipoTile tipo = TipoTile::Vazio;
    std::uint8_t estado = 0;          // porta aberta, ponte baixada, armadilha disparada…
    bool bloqueiaMovimento = true;
    bool bloqueiaVisao = true;
    bool bloqueiaVoo = true;          // só parede/porta/grade: a sanguessuga sobrevoa água e buraco
};

// Tile "padrão" de cada tipo (as propriedades vêm de uma tabela única em tile.cpp).
Tile criarTile(TipoTile tipo);

// Tipo do terreno de um símbolo do mapa. Símbolos de OBJETO (E, s, B, K, @…) ficam sobre o
// chão e devolvem Chao; símbolo desconhecido devolve Total (erro de leitura).
TipoTile tipoDoSimbolo(char simbolo);
// O Tile de um símbolo do mapa, com o que o símbolo diz além do tipo: a direção da esteira
// ('>' 0 direita, '<' 1 esquerda, 'v' 2 baixo, 'A' 3 cima) fica em Tile::estado.
Tile tileDoSimbolo(char simbolo);
// Esteira: quanto e para onde ela leva quem está em cima (Etapa 13.1).
constexpr float VELOCIDADE_ESTEIRA = 60.0f;
Vec2 direcaoDaEsteira(const Tile& t);
// Quem anda em cima da esteira faz força contra ela: a direção unitária u tal que velocidade·u +
// esteira aponte para `desejada` (Bill pelo piloto, os inimigos pela IA; 13.1–13.2).
Vec2 contraEsteira(const Tile& t, Vec2 desejada, float velocidade);

// Multiplicador de velocidade de quem anda por cima (água rasa −30%, lama −40%: PRD §14.2).
float fatorVelocidade(TipoTile tipo);

// Material de arte do piso: decide qual textura cobre a célula (cache de terreno).
enum class MaterialPiso : std::uint8_t { Piso, PisoVariante, Liquido, LiquidoFundo, Buraco, Parede };
MaterialPiso materialDe(TipoTile tipo);
