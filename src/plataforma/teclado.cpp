#include "plataforma/teclado.h"

#include <windows.h>

namespace {

// Tabela única de teclas: alterar um controle = alterar uma linha aqui.
const MapeamentoTecla TABELA_TECLAS[] = {
    {'W', Comando::Cima},          {VK_UP, Comando::Cima},
    {'S', Comando::Baixo},         {VK_DOWN, Comando::Baixo},
    {'A', Comando::Esquerda},      {VK_LEFT, Comando::Esquerda},
    {'D', Comando::Direita},       {VK_RIGHT, Comando::Direita},
    {'J', Comando::Atacar},        {'K', Comando::Especial},
    {'L', Comando::Clarao},        {VK_SPACE, Comando::Dash},
    {'E', Comando::Interagir},     {'Q', Comando::UsarKit},
    {'I', Comando::Inventario},    {VK_TAB, Comando::Inventario},
    {'C', Comando::Ficha},         {'M', Comando::Mapa},
    {VK_ESCAPE, Comando::Pausa},   {'P', Comando::Pausa},
    {VK_RETURN, Comando::Confirmar},
    {VK_ESCAPE, Comando::Voltar},  {VK_BACK, Comando::Voltar},
    {VK_PRIOR, Comando::AbaEsquerda}, {VK_NEXT, Comando::AbaDireita},
    // "<" e ">" (vírgula e ponto: mesma posição no ABNT2 e no americano). Hoje trocam de sala
    // (atalho de teste); nos menus, trocam de aba.
    {VK_OEM_COMMA, Comando::AbaEsquerda}, {VK_OEM_PERIOD, Comando::AbaDireita},
    {VK_F11, Comando::TelaCheia},  {VK_F1, Comando::Debug},
    {VK_F2, Comando::TesteControle}, {VK_F10, Comando::Piloto},
};

}  // namespace

void aplicarTeclado(const std::array<bool, 256>& teclas, EstadoEntrada& entrada) {
    bool algumaTecla = false;
    for (const MapeamentoTecla& m : TABELA_TECLAS) {
        if (teclas[static_cast<size_t>(m.teclaVirtual)]) {
            entrada.segurando[static_cast<int>(m.comando)] = true;
            algumaTecla = true;
        }
    }
    if (algumaTecla) {
        entrada.ultimoUsado = Dispositivo::Teclado;
    }
}
