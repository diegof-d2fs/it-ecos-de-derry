// Testes dos menus como pendências (PRD §11.2 e §15.5; critério M15 "pausar"): pausa,
// inventário com abas, uso de item pelo menu, abandonar e sair com confirmação. Tudo por comandos.
#include <filesystem>

#include "jogo/partida.h"
#include "teste.h"

namespace {

Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

void apertar(EstadoPartida& p, const Conteudo& c, std::initializer_list<Comando> cmds) {
    EstadoEntrada e;
    for (Comando cmd : cmds) e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) { apertar(p, c, {cmd}); }

EstadoPartida novaTentativa(const Conteudo& c) {
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 21, Dificuldade::Normal, erro);
    return p;
}

}  // namespace

TESTE(menus_esc_pausa_congela_e_esc_de_novo_continua) {
    const Conteudo c = salasReais();
    EstadoPartida p = novaTentativa(c);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
    apertar(p, c, {Comando::Pausa, Comando::Voltar});   // o Esc do teclado manda os dois
    VERIFICAR(p.pendencia == Pendencia::Pausa && p.cursor == 0);
    const float tempo = p.stats.tempo;
    EstadoEntrada andar;
    andar.eixo = {1.0f, 0.0f};
    const Vec2 pes = p.bill.pes;
    for (int q = 0; q < 60; ++q) atualizarPartida(p, c, andar, DT);
    VERIFICAR(p.stats.tempo == tempo && p.bill.pes.x == pes.x);   // nada anda, o relógio não conta
    apertar(p, c, {Comando::Pausa, Comando::Voltar});
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
    // "Continuar" com Enter também fecha.
    apertar(p, c, Comando::Pausa);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
}

TESTE(menus_inventario_usa_item_e_troca_de_aba) {
    const Conteudo c = salasReais();
    EstadoPartida p = novaTentativa(c);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Tonico, 1);
    coletarNoInventario(p.bill, p.inventario, TipoItem::Kit, 1);
    apertar(p, c, Comando::Inventario);
    VERIFICAR(p.pendencia == Pendencia::Inventario && p.aba == AbaInventario::Itens);
    // Kit com vida cheia: não gasta e avisa.
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(quantidadeDe(p.bill, p.inventario, TipoItem::Kit) == 1 && p.tempoAvisoMenu > 0.0f);
    // Tônico (3º da lista): usa, liga o efeito e some do inventário.
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Baixo);
    VERIFICAR(ITENS_USAVEIS[p.cursor] == TipoItem::Tonico);
    apertar(p, c, Comando::Atacar);
    VERIFICAR(p.bill.tonico > 0.0f && quantidadeDe(p.bill, p.inventario, TipoItem::Tonico) == 0);
    // O efeito não corre com o menu aberto (o tempo está parado).
    const float resta = p.bill.tonico;
    for (int q = 0; q < 60; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.bill.tonico == resta);
    // Abas: → e > avançam, ← volta, dando a volta.
    apertar(p, c, Comando::AbaDireita);
    VERIFICAR(p.aba == AbaInventario::Equipamento);
    apertar(p, c, Comando::Direita);
    apertar(p, c, Comando::Direita);
    VERIFICAR(p.aba == AbaInventario::Ecos);   // aba nova da Etapa 8.1
    apertar(p, c, Comando::Direita);
    VERIFICAR(p.aba == AbaInventario::Ficha);
    apertar(p, c, Comando::Direita);
    VERIFICAR(p.aba == AbaInventario::Itens);
    apertar(p, c, Comando::Esquerda);
    VERIFICAR(p.aba == AbaInventario::Ficha);
    // I fecha; aberto da pausa, fecha VOLTANDO à pausa.
    apertar(p, c, Comando::Inventario);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
    apertar(p, c, Comando::Pausa);
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pendencia == Pendencia::Inventario);
    apertar(p, c, Comando::Voltar);
    VERIFICAR(p.pendencia == Pendencia::Pausa && p.cursor == static_cast<int>(OpcaoPausa::Inventario));
}

TESTE(menus_abandonar_pede_confirmacao_e_nao_conta_vida) {
    const Conteudo c = salasReais();
    EstadoPartida p = novaTentativa(c);
    apertar(p, c, Comando::Pausa);
    for (int i = 0; i < static_cast<int>(OpcaoPausa::Abandonar); ++i) apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.confirmando == OpcaoPausa::Abandonar && p.cursor == 1);   // começa no "Não"
    apertar(p, c, Comando::Confirmar);                                    // "Não": volta ao menu
    VERIFICAR(!p.terminou && p.confirmando == OpcaoPausa::Total && p.pendencia == Pendencia::Pausa);
    apertar(p, c, Comando::Confirmar);
    apertar(p, c, Comando::Cima);                                         // "Sim"
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.terminou && p.abandonou && !p.vitoria && p.stats.U == 0 && p.pendencia == Pendencia::Nenhuma);
}

TESTE(menus_sair_do_jogo_so_com_sim) {
    const Conteudo c = salasReais();
    EstadoPartida p = novaTentativa(c);
    apertar(p, c, Comando::Pausa);
    apertar(p, c, Comando::Cima);   // dá a volta: última opção = Sair
    VERIFICAR(p.cursor == static_cast<int>(OpcaoPausa::Sair));
    apertar(p, c, Comando::Confirmar);
    apertar(p, c, Comando::Voltar);   // desiste
    VERIFICAR(!p.pedidoSair && p.confirmando == OpcaoPausa::Total);
    apertar(p, c, Comando::Confirmar);
    apertar(p, c, Comando::Cima);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pedidoSair);
}

TESTE(menus_pausa_pela_plataforma_e_ficha_pela_pausa) {
    const Conteudo c = salasReais();
    EstadoPartida p = novaTentativa(c);
    abrirPausa(p);   // controle desconectado
    VERIFICAR(p.pendencia == Pendencia::Pausa);
    abrirPausa(p);   // já aberta: nada muda
    VERIFICAR(p.pendencia == Pendencia::Pausa);
    p.bill.progresso.pontosPendentes = 1;
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pendencia == Pendencia::SubirNivel);
    p.terminou = true;
    p.pendencia = Pendencia::Nenhuma;
    abrirPausa(p);   // partida acabou: sem pausa
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
}
