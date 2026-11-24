// Espectro de fuligem (Etapa 13.2, PRD §9 e §14.2): os números do PRD, o teleporte anunciado por
// fumaça (0,5 s) e as brasas que ferem Bill (6/s por 4 s). E a IA que faz força contra a esteira.
#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo salaUnica(const std::string& texto) {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala(texto, s, erro);
    c.salas.push_back(s);
    return c;
}

const char* const SALAO =
    "id=t\ntipo=teste\nmapa:\n"
    "######################\n"
    "#E...................#\n"
    "#....................#\n"
    "#....................#\n"
    "#....................#\n"
    "#....................#\n"
    "######################\n";

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 9, 0, Dificuldade::Normal);
    p.bill.attr.agilidade = 0;
    recalcularJogador(p.bill);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

IdEntidade colocar(EstadoPartida& p, TipoInimigo t, Celula cel) {
    Inimigo i = criarInimigo(t, p.proximoId++, centroCelula(cel));
    i.surgindo = 0.0f;
    p.inimigos.push_back(i);
    return i.id;
}
}  // namespace

TESTE(espectro_tem_os_numeros_do_prd) {
    const ParametrosInimigo& e = parametrosDe(TipoInimigo::Espectro);
    VERIFICAR(e.hp == 40 && e.dano == 11 && e.armadura == 0 && e.agilidade == 3);
    VERIFICAR_PERTO(e.velocidade, 100.0f, 0.01f);
    VERIFICAR(e.xp == 14 && e.voa);
    VERIFICAR_PERTO(e.deteccao, 8.0f, 0.01f);
}

TESTE(espectro_vira_fumaca_e_reaparece_perto_de_bill_so_depois_do_aviso) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({18, 3}));
    const IdEntidade id = colocar(p, TipoInimigo::Espectro, {4, 3});   // 14 células: longe
    float fumaca = -1.0f;
    Vec2 destino;
    bool apareceu = false;
    for (int q = 0; q < 60 * 4 && !apareceu; ++q) {
        p.bill.invulneravel = 1.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        const Inimigo* e = buscarInimigo(p, id);
        if (!e) break;
        if (e->teleporte > 0.0f && fumaca < 0.0f) {
            fumaca = p.stats.tempo;
            destino = e->destinoTeleporte;
            VERIFICAR(!podeSerAtingido(*e));   // fumaça: não é atingido
            VERIFICAR(comprimento(subtrair(destino, p.bill.pes)) < 2.5f * TILE);   // ~2 células de Bill
        }
        if (fumaca >= 0.0f && e->teleporte <= 0.0f) {
            apareceu = true;
            VERIFICAR(p.stats.tempo - fumaca >= AVISO_TELEPORTE - 0.02f);   // só depois do aviso
            VERIFICAR(comprimento(subtrair(e->pes, destino)) < 0.5f);
        }
    }
    VERIFICAR(fumaca >= 0.0f && apareceu);
}

TESTE(espectro_brasas_ferem_bill_6_por_segundo_e_somem_em_4_s) {
    const Conteudo c = salaUnica(SALAO);
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({10, 3}));
    p.brasas.push_back({p.bill.pes, VIDA_BRASA, ID_NULO});
    const int cheio = p.bill.hp;
    for (int q = 0; q < 60 * 2 + 30; ++q) {   // 2,5 s em cima: 3 toques (0, ~1,1 e ~2,2 s: o hitstop congela)
        p.bill.invulneravel = 0.0f;
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    }
    const int um = std::max(1, danoEfetivo(DANO_BRASA, p.bill.stats.armadura));
    VERIFICAR(p.bill.hp == cheio - 3 * um);
    for (int q = 0; q < 60 * 2; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    VERIFICAR(p.brasas.empty());   // 4 s de vida
}

TESTE(esteira_forca_contra_aponta_exatamente_para_o_rumo) {
    // velocidade·u + esteira tem de apontar para onde se quer ir (Bill pelo piloto, inimigos pela IA).
    for (char simbolo : {'>', '<', 'v', 'A'}) {
        const Tile t = tileDoSimbolo(simbolo);
        for (const Vec2 rumo : {Vec2{-1.0f, 0.3f}, Vec2{0.2f, 1.0f}, Vec2{1.0f, -1.0f}, Vec2{0.0f, -1.0f}}) {
            const float v = 140.0f;
            const Vec2 u = contraEsteira(t, rumo, v);
            VERIFICAR_PERTO(comprimento(u), 1.0f, 0.001f);
            const Vec2 resultado = somar(escalar(u, v), escalar(direcaoDaEsteira(t), VELOCIDADE_ESTEIRA));
            const Vec2 r = normalizar(rumo), n = normalizar(resultado);
            VERIFICAR_PERTO(r.x * n.x + r.y * n.y, 1.0f, 0.001f);   // mesma direção
        }
    }
    const Tile chao = tileDoSimbolo('.');
    const Vec2 u = contraEsteira(chao, {3.0f, 4.0f}, 100.0f);   // fora da esteira: só normaliza
    VERIFICAR_PERTO(u.x, 0.6f, 0.001f);
    VERIFICAR_PERTO(u.y, 0.8f, 0.001f);
}

// Regressão (13.2): a contagem do teleporte ficava dentro do decidir, que só roda em alguns estados.
// Perdendo Bill de vista no meio da fumaça, ela CONGELAVA (fantasma invulnerável andando).
TESTE(espectro_fumaca_termina_mesmo_se_perder_bill_de_vista) {
    const Conteudo c = salaUnica(
        "id=t\ntipo=teste\nmapa:\n"
        "##############################\n"
        "#E...........................#\n"
        "#............................#\n"
        "#............................#\n"
        "#............................#\n"
        "##############################\n");
    EstadoPartida p = comecar(c);
    posicionarJogador(p.bill, centroCelula({12, 3}));
    const IdEntidade id = colocar(p, TipoInimigo::Espectro, {4, 3});
    p.bill.invulneravel = 99.0f;
    bool comecou = false;
    for (int q = 0; q < 60 * 4 && !comecou; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        const Inimigo* e = buscarInimigo(p, id);
        comecou = e && e->teleporte > 0.0f;
    }
    VERIFICAR(comecou);
    if (!comecou) return;
    const Vec2 destino = buscarInimigo(p, id)->destinoTeleporte;
    posicionarJogador(p.bill, centroCelula({28, 1}));   // some de vista: longe, fora da detecção
    int quadros = 0;
    for (; quadros < 60 && buscarInimigo(p, id) && buscarInimigo(p, id)->teleporte > 0.0f; ++quadros)
        atualizarPartida(p, c, EstadoEntrada{}, DT);
    const Inimigo* e = buscarInimigo(p, id);
    VERIFICAR(quadros <= static_cast<int>(AVISO_TELEPORTE * 60.0f) + 1);   // não congelou
    VERIFICAR(e && e->submerso <= 0.0f);
    VERIFICAR(e && comprimento(subtrair(e->pes, destino)) < 0.5f);   // apareceu onde a fumaça anunciou
}
