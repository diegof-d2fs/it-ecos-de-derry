// Richie (Etapa 14.3, PRD §4.4, §12 e §14.3): o resgate em que ele grita e DESFAZ AS PAREDES ILUSÓRIAS,
// os Ecos Voz do locutor, Piada ruim e Bip-bip, e o lendário Imitação (um inimigo luta do lado de Bill).
// O Eco "Óculos" é só desenho (contorno de inimigos fora da luz): conferido em captura.
#include <filesystem>

#include "entidades/jogador.h"
#include "jogo/ecos.h"
#include "jogo/partida.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

int indicePorId(const Conteudo& c, const std::string& id) {
    for (int i = 0; i < static_cast<int>(c.salas.size()); ++i) {
        if (c.salas[static_cast<std::size_t>(i)].id == id) return i;
    }
    return -1;
}

int ecoPorNome(const std::string& nome) {
    for (int i = 0; i < totalEcos(); ++i) {
        if (nome == definicaoEco(i).nome) return i;
    }
    return -1;
}

Conteudo salao() {
    Conteudo c;
    SalaModelo s;
    std::string erro;
    lerSala("id=t\ntipo=teste\nmapa:\n"
            "####################\n"
            "#E.................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "#..................#\n"
            "####################\n",
            s, erro);
    c.salas.push_back(s);
    return c;
}

EstadoPartida comecar(const Conteudo& c) {
    EstadoPartida p;
    iniciarPartida(p, c, 7, 0, Dificuldade::Normal);
    p.bill.hp = p.bill.stats.hpMax;
    return p;
}

IdEntidade colocar(EstadoPartida& p, TipoInimigo tipo, Vec2 pes, int hp) {
    Inimigo i = criarInimigo(tipo, p.proximoId++, pes);
    i.surgindo = 0.0f;
    i.hp = hp;
    p.inimigos.push_back(i);
    return i.id;
}

Inimigo* achar(EstadoPartida& p, IdEntidade id) {
    for (Inimigo& i : p.inimigos) {
        if (i.id == id) return &i;
    }
    return nullptr;
}

// O giro do Especial de Bill em volta dele (como em jogador.cpp), sem depender da recarga.
void especial(EstadoPartida& p, const Conteudo& c) {
    Golpe g = golpeDoBill(p.bill, p.proximoGolpe, areaCirculo(corpoJogador(p.bill), 52.0f), p.bill.stats.danoBase);
    g.especial = true;
    g.rolaAcerto = false;
    g.duracao = 0.05f;
    p.golpes.push_back(g);
    atualizarPartida(p, c, EstadoEntrada{}, DT);
}
}  // namespace

TESTE(richie_resgate_desfaz_as_paredes_ilusorias_e_da_imitacao) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    std::string erro;
    VERIFICAR(iniciarTentativa(p, c, 41, Dificuldade::Normal, erro));
    entrarNaSala(p, c, indicePorId(c, "b6_resgate"), nullptr);
    // A coluna 20 é de paredes ILUSÓRIAS: bloqueiam como parede até o Richie gritar.
    VERIFICAR(tileEm(p.sala, 20, 7).tipo == TipoTile::Ilusao && tileEm(p.sala, 20, 7).bloqueiaMovimento);
    p.inventario.quantidades[static_cast<std::size_t>(TipoItem::ChaveDoMedo)] = 1;
    posicionarJogador(p.bill, centroCelula({12, 7}));
    EstadoEntrada e;
    e.pressionou[static_cast<int>(Comando::Interagir)] = true;
    atualizarPartida(p, c, e, DT);   // abre a porta do Medo
    const ObjetoSala* richie = nullptr;
    for (const ObjetoSala& o : p.sala.objetos) richie = o.codigo == '1' ? &o : richie;
    VERIFICAR(richie != nullptr);
    if (!richie) return;
    posicionarJogador(p.bill, centroCelula({richie->celula.col, richie->celula.lin + 1}));
    atualizarPartida(p, c, e, DT);
    VERIFICAR(p.companheiro.ativo && p.companheiro.quem == Amigo::Richie);
    for (int lin = 2; lin <= 12; ++lin) {   // "Ei, Coisa! Isso aqui é só papelão!": a coluna inteira some
        VERIFICAR(!tileEm(p.sala, 20, lin).bloqueiaMovimento);
    }
    VERIFICAR(valorEco(p, EfeitoEco::Imitacao) == 6);   // o lendário vem com o resgate
}

TESTE(richie_ecos_existem_e_o_lendario_nao_sai_no_altar) {
    for (const char* nome : {"Voz do locutor", "Óculos", "Piada ruim", "Bip-bip", "Imitação"}) {
        const int i = ecoPorNome(nome);
        VERIFICAR(i >= 0);
        if (i >= 0) VERIFICAR(definicaoEco(i).amigo == Amigo::Richie);
    }
    VERIFICAR(ecoLendarioDe(Amigo::Richie) == ecoPorNome("Imitação"));
    Rng rng;
    for (int k = 0; k < 50; ++k) {
        for (const OfertaEco& o : sortearEcos({}, Amigo::Richie, rng)) VERIFICAR(!definicaoEco(o.definicao).lendario);
    }
}

TESTE(richie_voz_do_locutor_o_especial_atordoa) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoPorNome("Voz do locutor"), Raridade::Raro);   // 900 ms
    const IdEntidade a = colocar(p, TipoInimigo::Sombra, somar(p.bill.pes, {30.0f, 0.0f}), 500);
    especial(p, c);
    const Inimigo* ia = achar(p, a);
    VERIFICAR(ia && ia->atordoado > 0.8f && ia->atordoado <= 0.9f);
}

TESTE(richie_imitacao_um_inimigo_por_sala_luta_do_lado_de_bill) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoPorNome("Imitação"), Raridade::Lendario);
    const IdEntidade a = colocar(p, TipoInimigo::Sombra, somar(p.bill.pes, {30.0f, 0.0f}), 500);
    especial(p, c);
    VERIFICAR(achar(p, a) && achar(p, a)->aliado > 5.0f);
    // 1× por sala: um segundo inimigo atingido pelo especial continua hostil.
    const IdEntidade b = colocar(p, TipoInimigo::Sombra, somar(p.bill.pes, {-30.0f, 0.0f}), 500);
    p.bill.recargaEspecial = 0.0f;
    especial(p, c);
    VERIFICAR(achar(p, b) && achar(p, b)->aliado <= 0.0f);
    // O aliado não apanha dos golpes de Bill.
    const int antes = achar(p, a)->hp;
    especial(p, c);
    VERIFICAR(achar(p, a) && achar(p, a)->hp == antes);
    // O que o aliado anuncia é do lado de Bill (fere inimigos, não Bill).
    for (int q = 0; q < 240; ++q) {
        atualizarPartida(p, c, EstadoEntrada{}, DT);
        for (const Aviso& av : p.avisos) {
            if (av.golpe.origem == a) VERIFICAR(av.golpe.equipe == Equipe::Jogador);
        }
    }
}

TESTE(richie_bip_bip_o_dash_fere_quem_bill_atravessa_uma_vez) {
    const Conteudo c = salao();
    EstadoPartida p = comecar(c);
    aplicarEco(p, ecoPorNome("Bip-bip"), Raridade::Epico);   // 16
    const IdEntidade a = colocar(p, TipoInimigo::Balao, somar(p.bill.pes, {8.0f, 0.0f}), 500);
    if (Inimigo* ia = achar(p, a)) ia->atordoado = 5.0f;   // parado: só o dash o atinge
    p.bill.dashRestante = 0.3f;
    for (int q = 0; q < 12; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
    const Inimigo* ia = achar(p, a);
    VERIFICAR(ia && ia->hp == 500 - 16);   // balão: armadura 0; uma vez só no mesmo dash
}

TESTE(richie_piada_ruim_os_inimigos_erram_mais) {
    const Conteudo c = salao();
    auto acertos = [&](bool comEco) {
        EstadoPartida p = comecar(c);
        if (comEco) aplicarEco(p, ecoPorNome("Piada ruim"), Raridade::Epico);   // −12
        int n = 0;
        for (int k = 0; k < 400; ++k) {
            p.bill.hp = p.bill.stats.hpMax;
            p.bill.invulneravel = 0.0f;
            p.hitStop = 0.0f;   // o congelamento do acerto anterior pularia o passo
            Golpe g;
            g.id = p.proximoGolpe++;
            g.equipe = Equipe::Hostil;
            g.origem = 999;
            g.area = areaCirculo(corpoJogador(p.bill), 20.0f);
            g.dano = 20;   // maior que a armadura (todo acerto aparece na vida) e, mesmo crítico, não mata
            g.focoAtacante = 1;
            g.duracao = 0.02f;
            p.golpes.push_back(g);
            const int antes = p.bill.hp;
            atualizarPartida(p, c, EstadoEntrada{}, DT);
            n += p.bill.hp < antes ? 1 : 0;
            p.golpes.clear();
        }
        return n;
    };
    const int sem = acertos(false), com = acertos(true);
    std::printf("    acertos em 400: sem Eco %d, com Piada ruim %d\n", sem, com);
    VERIFICAR(sem - com >= 20);   // −12 pontos de acerto ≈ 48 acertos a menos em 400
}
