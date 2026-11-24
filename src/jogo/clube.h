#pragma once
// Clube dos Perdedores (hub, PRD §13 e SDD §11): a ponte entre a partida e o perfil.
//
// A partida (jogo/partida) cuida do Clube como de qualquer sala: Bill anda, interage, e o
// Mural/alçapão viram PENDÊNCIAS que só registram o pedido (`pedidoCompra`,
// `pedidoTentativa`). Quem tem o perfil atende aqui. Assim as regras continuam sem saber
// de arquivo, e o mesmo fluxo roda na janela, no piloto e nos testes.
#include <string>

#include "jogo/partida.h"
#include "jogo/perfil.h"

struct RespostaClube {
    bool perfilMudou = false;        // comprou um talento ou escolheu a dificuldade: gravar
    bool comecouTentativa = false;   // a partida agora é a tentativa (a cena reposiciona a câmera)
};

// Atende os pedidos pendentes do Clube. `piloto` = o computador está jogando: ele começa a
// tentativa "limpo" (sem talentos) e não mexe no perfil de quem joga.
RespostaClube atenderClube(Perfil& perfil, EstadoPartida& p, const Conteudo& c, bool piloto);

// O que as falas consultam, tirado do perfil.
ContextoFala contextoDoPerfil(const Perfil& perfil, bool noClube);
// Dicas do tutorial na partida: ligadas pelas Opções (nunca para o piloto) e as já vistas.
void configurarDicas(EstadoPartida& p, const Perfil& perfil, bool piloto);
// Começo de cada visita ao Clube: a fala nova de cada amigo ("!" sobre a cabeça; no máximo
// uma por amigo por visita, PRD §4.6) e as dicas D01 (1ª visita) ou D19 (volta ao Clube).
void prepararVisitaAoClube(EstadoPartida& p, const Conteudo& c, const Perfil& perfil, bool piloto);
// Prólogo (1ª vez): dicas configuradas e D01 (andar) na fila.
void prepararPrologo(EstadoPartida& p, const Perfil& perfil, bool piloto);
// Recolhe para o perfil as falas lidas e as dicas vistas. true = o perfil mudou (gravar).
// O piloto não mexe no perfil de quem joga.
bool sincronizarPerfil(Perfil& perfil, EstadoPartida& p, bool piloto);

// Nome de exibição de um amigo ("ben" → "Ben", "beverly" → "Beverly").
std::string nomeDoAmigo(const std::string& codigo);
// Fala curta (balão) de um amigo no Clube, a `vez`-ésima conversa da visita: é o que ele diz
// quando já não tem fala nova nesta visita (as falas novas vêm de data/dialogos.txt).
std::string falaRapida(const std::string& amigo, int vez);
