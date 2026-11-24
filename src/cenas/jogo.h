#pragma once
// Raiz do estado do programa. Vive em main.cpp (dono de tudo) e é passada por referência
// às cenas. Cresce a cada etapa: perfil, partida e conteúdo entram nas próximas.
#include <array>
#include <string>
#include <vector>

#include "core/controle.h"
#include "core/entrada.h"
#include "core/medidor.h"
#include "core/tipos.h"
#include "jogo/partida.h"
#include "jogo/perfil.h"
#include "jogo/piloto.h"
#include "jogo/telemetria.h"
#include "jogo/recordes.h"
#include "render/recursos.h"
#include "render/terreno.h"

enum class IdCena : std::uint8_t { Vitrine, TesteControle, Sala, Menu, Historia, Cartao, Total };

// Menu principal (cenas/cena_menu.cpp). Mesma ordem da tabela de desenho de lá.
enum class TelaMenu : std::uint8_t {
    Principal, ConfirmarNovo, Dificuldade, ComoJogar, Itens, Pontuacao, Controles, Opcoes, Recordes, Creditos
};
struct EstadoMenu {
    TelaMenu tela = TelaMenu::Principal;
    int cursor = 0;
    int pagina = 0;
    bool assistir = false;         // "Assistir o computador jogar"
    int velocidade = 1;            // ×1, ×2 ou ×4 (só assistindo)
    FiltroRecordes filtro = FiltroRecordes::Todas;
    std::vector<Recorde> recordes;
    bool temPerfil = false;
    bool novoJogo = false;         // veio de "Novo jogo": a abertura da história passa antes do prólogo
    bool creditosDaVitoria = false; // após o resultado, os créditos voltam ao Clube
};

// Cena de sala: só DESENHA a partida e repassa os comandos. Todo o jogo está em EstadoPartida
// (regras), que também roda sem janela no piloto automático e nos testes (SDD §24.7.1).
struct EstadoSala {
    EstadoPartida partida;
    MemoriaPiloto piloto;            // estado do piloto automático (F10)
    Vec2 camera;
    float tempoNome = 0.0f;          // mostra o nome da sala por alguns segundos ao entrar
    mutable CacheTerreno cache;      // só o render mexe (desenhar recebe const Jogo&)
    // Resultado da tentativa (SDD §24.3): gravado UMA vez quando a partida termina.
    Telemetria telemetria;           // tempo por sala (PRD §18) → saves/telemetria.csv no fim
    bool pilotoJogou = false;        // o computador jogou algum trecho: o recorde é "computador"
    bool resultadoPronto = false;
    int colocacao = 0;               // 1..10 na categoria; 0 = não entrou (ou abandonou)
    std::vector<Recorde> recordes;
    FiltroRecordes filtro = FiltroRecordes::Todas;
    int lembrancasGanhas = 0;        // nesta tentativa (resultado)
    int lembrancasTotal = 0;
    float telaCiclo = 0.0f;          // > 0: "Bill acorda no Clube" (1ª morte) na frente de tudo
    float cartaoBioma = 0.0f;        // > 0: o cartão do bioma (ilustração + nome) ao chegar nele (Etapa 10.5)
    int biomaDoCartao = -1;          // o último bioma anunciado nesta tentativa (−1 = nenhum)
};

// Cena da história (cut scene, cenas/cena_historia.cpp) aberta por cima da cena atual.
struct EstadoHistoria {
    TocadorHistoria tocador;                    // regras (jogo/historia): quadro, tempo, avançar, pular
    // Só o desenho mexe (desenhar recebe const Jogo&): cada ilustração já reduzida para a tela, com a
    // folga do zoom; refeito se a escala da janela mudar. Liberado quando a cena termina.
    mutable std::vector<Imagem> preparadas;     // uma por quadro (vazia = sem imagem: tela preta)
    mutable int escalaPreparada = 0;
    mutable Fonte fonteTexto, fonteTitulo;      // Georgia, criadas na escala da tela
};

// Cartão de memória (cenas/cena_cartao.cpp, 23/11): os 15 blocos de salvamento.
enum class AcaoCartao : std::uint8_t { Nenhuma, Salvar, Apagar, Carregar };
struct EstadoCartao {
    bool salvar = true;            // aberto pela pausa (salvar) ou pelo menu (carregar)
    int cursor = 0;                // bloco 0..14
    AcaoCartao confirmando = AcaoCartao::Nenhuma;
    int simNao = 1;                // 0 = Sim, 1 = Não
    std::array<ResumoBloco, TOTAL_BLOCOS> blocos{};
    std::wstring mensagem;
    float tempoMensagem = 0.0f;
};

struct PilhaCenas {
    std::array<IdCena, 8> itens{};
    int quantidade = 0;
};

// Estado da cena de vitrine da Etapa 1 (será substituída pela cena de sala na Etapa 3).
struct EstadoVitrine {
    Vec2 posBill{320.0f, 260.0f};
    Direcao dirBill = Direcao::Baixo;
    bool andando = false;
    float tempoAnimacao = 0.0f;
    Vec2 camera;
    int ambiente = 0;   // índice em AMBIENTES_VITRINE (PageDown/PageUp troca)
};

// Tela de teste de botões (Etapa 2; depois vira parte do menu Controles, PRD v2 §12).
struct EstadoTesteControle {
    float segurandoSaida = 0.0f;   // segundos com Pausa (Esc/Start) segurado; 1 s fecha
    bool soltouPausa = false;      // o Start que ABRIU a tela não conta para fechá-la
};

// Aviso curto no topo da tela (controle conectado/desconectado).
struct AvisoGlobal {
    std::wstring texto;
    float restante = 0.0f;   // segundos
};

struct Jogo {
    const Recursos* recursos = nullptr;   // pertence a main.cpp; só leitura nas cenas
    EstadoEntrada entrada;
    InfoControle controle;                // preenchido pela plataforma a cada quadro
    AvisoGlobal aviso;
    EstadoTesteControle testeControle;
    PilhaCenas cenas;
    MedidorQuadros medidor;
    float tempoTotal = 0.0f;
    bool rodando = true;
    bool mostrarMedidor = false;
    bool pilotoAtivo = false;  // F10: o computador joga (SDD §24.7)
    bool pausadoPorFoco = false;
    int escalaAtual = 1;
    int ambienteInicial = 0;   // --bioma N (modo captura)
    int salaInicial = -1;      // --sala N (sala avulsa); −1 = tentativa gerada
    std::uint64_t seed = 0;    // seed da partida (main sorteia; --seed N fixa)
    Dificuldade dificuldade = Dificuldade::Normal;   // --dificuldade (o menu da Etapa 7 escolhe)
    int pontosIniciais = 0;    // --pontos N (Modo Defesa)
    bool forte = false;        // --forte (Modo Defesa): na sala avulsa, Bill chega como na tentativa
    bool pennywise = false;    // --pennywise (Modo Defesa): o chefe da arena já começa na fase 2
    int faseCovil = 1;         // --fase-covil N: o Covil começa na fase do Georgie (2) ou das Luzes Mortas (3)
    std::string inimigoDefesa;  // --inimigo atlas: encontro isolado para capturar o ataque real
    bool semAvisos = false;      // --captura-limpa: sem o aviso de controle conectado
    std::string amigoDefesa;    // --amigo stan: a sala avulsa começa com esse amigo ao lado do Bill
    int biomaInicial = 1;       // --bioma-inicial N: a tentativa começa no bioma N (apresentação)
    bool itensDefesa = false;  // --itens (Modo Defesa: mostrar os itens no personagem)
    std::vector<std::string> ecosDefesa;   // --eco "Nome": Ecos ativos desde o começo (Modo Defesa, capturas)
    Perfil perfil;             // saves/perfil.txt (carregado no main; gravado pelo menu e pelo resultado)
    EstadoMenu menu;
    int velocidade = 1;        // passos de lógica por passo (assistir o computador ×1/×2/×4)
    bool fluxoClube = false;   // veio do menu: começa no Clube e a morte volta a ele (7.2);
                               // false = --jogar/--piloto (tentativa direta, testes e capturas)
    bool pedidoTelaCheia = false;   // Opções → o main alterna a janela
    std::wstring pastaSaves;   // onde ficam recordes.txt (padrão: saves\ ao lado do exe; --saves muda)
    EstadoVitrine vitrine;
    const Conteudo* conteudo = nullptr;   // pertence a main.cpp
    EstadoSala salaAtual;
    EstadoHistoria historia;
    EstadoCartao cartao;
    PontoSalvamento pontoParaCarregar;   // "Carregar jogo": a sala entra a partir deste ponto
    bool carregarPonto = false;
};
