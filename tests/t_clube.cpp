// Clube dos Perdedores (Etapa 7.2, PRD §13): a sala do hub, o Mural comprando pelo MENU (só
// comandos), o alçapão começando a tentativa com os talentos, o piloto descendo sozinho e a
// pausa do Clube voltando ao menu. Tudo sem janela.
#include <filesystem>

#include "jogo/clube.h"
#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo salasReais() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    return c;
}

// Um passo com um comando apertado (como um toque de tecla).
void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

// Bill parado ao lado do primeiro objeto `codigo` (na célula de baixo, que no Clube é chão).
void irAte(EstadoPartida& p, char codigo) {
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo != codigo) continue;
        posicionarJogador(p.bill, centroCelula({o.celula.col, o.celula.lin + 1}));
        return;
    }
}

int contar(const SalaModelo& s, char codigo) {
    int n = 0;
    for (const ObjetoSala& o : s.objetos) n += o.codigo == codigo ? 1 : 0;
    return n;
}
}  // namespace

TESTE(clube_sala_real_carrega_valida_e_fica_depois_dos_biomas) {
    const Conteudo c = salasReais();
    const int i = indiceDoClube(c);
    VERIFICAR(i >= 0);
    if (i < 0) return;
    const SalaModelo& s = c.salas[static_cast<std::size_t>(i)];
    VERIFICAR(validarSala(s).empty());
    VERIFICAR(s.ambiente == "clube");
    VERIFICAR(contar(s, 'M') == 1 && contar(s, 'Q') == 1 && contar(s, 'H') == 1);
    VERIFICAR(contar(s, 'e') >= 8);   // enfeites da folha objetos_clube
    int amigos = 0;
    for (char d = '1'; d <= '9'; ++d) amigos += contar(s, d);
    VERIFICAR(amigos == 6);           // os seis amigos estão no Clube ao entardecer
    VERIFICAR(s.npcs[1] == "ben" && s.npcs[2] == "mike");
    // "hub_" fica depois de "b*_": os índices de --sala N não mudaram.
    for (int k = 0; k < static_cast<int>(c.salas.size()); ++k) {   // todas as "hub_" depois das dos biomas
        const bool hub = c.salas[static_cast<std::size_t>(k)].id.rfind("hub_", 0) == 0;
        VERIFICAR(hub == (k >= i));
    }
    VERIFICAR(c.salas[1].id == "b1_c01");
}

TESTE(clube_formato_npc_e_enfeite_com_erros_claros) {
    const std::string base = "id=x\ntipo=clube\n";
    SalaModelo s;
    std::string erro;
    VERIFICAR(lerSala(base + "npc=1 ben\nenfeite=7 2 1\nmapa:\n#####\n#E1.#\n#..H#\n#####\n", s, erro));
    VERIFICAR(s.npcs[1] == "ben" && contar(s, 'e') == 1 && contar(s, 'H') == 1);
    VERIFICAR(!lerSala(base + "mapa:\n#####\n#E1.#\n#####\n", s, erro));   // NPC sem npc=
    VERIFICAR(erro.find("npc=1") != std::string::npos);
    VERIFICAR(!lerSala(base + "enfeite=7 0 0\nmapa:\n#####\n#E..#\n#####\n", s, erro));   // enfeite na parede
    VERIFICAR(erro.find("enfeite") != std::string::npos);
    VERIFICAR(!lerSala(base + "enfeite=99 1 1\nmapa:\n#####\n#E..#\n#####\n", s, erro));   // quadro inválido
    VERIFICAR(!lerSala(base + "npc=x ben\nmapa:\n#####\n#E..#\n#####\n", s, erro));
}

TESTE(clube_sem_cronometro_sem_inimigos_e_iluminado) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    VERIFICAR(iniciarClube(p, c, 42, Dificuldade::Normal));
    VERIFICAR(p.noClube && !p.emTentativa && p.sala.modelo->tipo == TipoSala::Clube);
    EstadoEntrada e;
    e.eixo = {1.0f, 0.0f};
    for (int q = 0; q < 120; ++q) atualizarPartida(p, c, e, DT);
    VERIFICAR(p.stats.tempo == 0.0f);   // o cronômetro é da tentativa, não do Clube
    VERIFICAR(p.inimigos.empty() && !p.terminou);
    // Toda célula de chão está à vista (lampiões), não só o raio da lanterna.
    int escuras = 0;
    for (int l = 2; l < p.sala.lins - 1; ++l) {
        for (int col = 1; col < p.sala.cols - 1; ++col) {
            if (!tileEm(p.sala, col, l).bloqueiaVisao && visaoEm(p.visao, col, l).luz == 0) ++escuras;
        }
    }
    VERIFICAR(escuras == 0);
}

TESTE(clube_mural_compra_pelo_menu_e_recusa_o_que_nao_pode) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    iniciarClube(p, c, 7, Dificuldade::Normal);
    Perfil perfil;
    perfil.lembrancas = 50;
    irAte(p, 'M');
    VERIFICAR(objetoDoClubeProximo(p) && objetoDoClubeProximo(p)->codigo == 'M');
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Mural && p.cursor == 0);
    apertar(p, c, Comando::Baixo);   // Coragem herdada (10)
    apertar(p, c, Comando::Confirmar);
    RespostaClube r = atenderClube(perfil, p, c, false);
    VERIFICAR(r.perfilMudou && !r.comecouTentativa);
    VERIFICAR(perfil.talentos[static_cast<std::size_t>(Talento::CoragemHerdada)] == 1 && perfil.lembrancas == 40);
    VERIFICAR(p.avisoMenu.find("Comprou") == 0 && p.pendencia == Pendencia::Mural);
    // "Em breve" (Sorte do Richie, 7ª linha): não vende.
    for (int k = 0; k < 5; ++k) apertar(p, c, Comando::Baixo);
    VERIFICAR(static_cast<Talento>(p.cursor) == Talento::SorteRichie);
    apertar(p, c, Comando::Confirmar);
    r = atenderClube(perfil, p, c, false);
    VERIFICAR(!r.perfilMudou && perfil.lembrancas == 40 && p.avisoMenu.find("Em breve") == 0);
    // Sem saldo: Pacto de Sangue custa 40, sobram 40 → compra; o 2º nível (120) não.
    p.cursor = 0;
    apertar(p, c, Comando::Confirmar);
    atenderClube(perfil, p, c, false);
    apertar(p, c, Comando::Confirmar);
    r = atenderClube(perfil, p, c, false);
    VERIFICAR(perfil.talentos[0] == 1 && perfil.lembrancas == 0 && !r.perfilMudou);
    VERIFICAR(p.avisoMenu == "Lembranças insuficientes");
    // O computador não gasta Lembranças de ninguém.
    perfil.lembrancas = 500;
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);
    r = atenderClube(perfil, p, c, true);
    VERIFICAR(!r.perfilMudou && perfil.lembrancas == 500);
    apertar(p, c, Comando::Voltar);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
}

TESTE(clube_alcapao_comeca_a_tentativa_com_talentos_e_dificuldade) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    iniciarClube(p, c, 99, Dificuldade::Normal);
    Perfil perfil;
    perfil.talentos[static_cast<std::size_t>(Talento::PactoDeSangue)] = 1;
    irAte(p, 'H');
    // Bill ao lado do alçapão (que é piso: dá para pisar nele).
    VERIFICAR(objetoDoClubeProximo(p) && objetoDoClubeProximo(p)->codigo == 'H');
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Alcapao && p.cursor == static_cast<int>(Dificuldade::Normal));
    apertar(p, c, Comando::Voltar);   // desistir: continua no Clube
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && !atenderClube(perfil, p, c, false).comecouTentativa && p.noClube);
    apertar(p, c, Comando::Interagir);
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);
    const RespostaClube r = atenderClube(perfil, p, c, false);
    VERIFICAR(r.comecouTentativa && r.perfilMudou);
    VERIFICAR(p.emTentativa && !p.noClube && p.dificuldade == Dificuldade::Dificil && p.seed == 99);
    VERIFICAR(perfil.ultimaDificuldade == Dificuldade::Dificil);
    EstadoPartida limpa;
    std::string erro;
    iniciarTentativa(limpa, c, 99, Dificuldade::Dificil, erro);
    VERIFICAR(p.bill.pactos == limpa.bill.pactos + 1);   // o talento valeu
    VERIFICAR(p.sala.modelo == limpa.sala.modelo);       // mesma seed = mesma tentativa
}

TESTE(clube_piloto_anda_ate_o_alcapao_e_escolhe_a_dificuldade) {
    const Conteudo c = salasReais();
    for (int d = 0; d < TOTAL_DIFICULDADES; ++d) {
        EstadoPartida p;
        iniciarClube(p, c, 1000 + static_cast<std::uint64_t>(d), Dificuldade::Normal);
        MemoriaPiloto m;
        m.dificuldadeDesejada = static_cast<Dificuldade>(d);
        Perfil perfil;
        perfil.lembrancas = 77;
        int quadros = 0;
        for (; quadros < 20 * 60 && !p.emTentativa; ++quadros) {
            atualizarPartida(p, c, pilotar(p, m, DT), DT);
            atenderClube(perfil, p, c, true);
        }
        VERIFICAR(p.emTentativa && p.dificuldade == static_cast<Dificuldade>(d));
        VERIFICAR(quadros < 10 * 60);   // anda direto, sem se perder
        VERIFICAR(perfil.lembrancas == 77 && perfil.ultimaDificuldade == Dificuldade::Normal);   // perfil intacto
        VERIFICAR(m.travamentos == 0);
    }
}

TESTE(clube_pausa_volta_ao_menu_e_amigo_fala) {
    const Conteudo c = salasReais();
    EstadoPartida p;
    iniciarClube(p, c, 5, Dificuldade::Normal);
    VERIFICAR(std::string(nomeOpcaoPausa(OpcaoPausa::Abandonar, true)) == "Voltar ao menu principal");
    apertar(p, c, Comando::Pausa);
    VERIFICAR(p.pendencia == Pendencia::Pausa);
    p.cursor = static_cast<int>(OpcaoPausa::Abandonar);
    apertar(p, c, Comando::Confirmar);
    VERIFICAR(p.pedidoMenu && !p.terminou && !p.abandonou);   // sem "tem certeza": nada se perde
    // Amigo: balão com a fala, e a próxima conversa é outra.
    irAte(p, '1');
    VERIFICAR(objetoDoClubeProximo(p) && objetoDoClubeProximo(p)->codigo == '1');
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.balao.vida > 0.0f && p.balao.texto.find("Ben: ") == 0);
    const std::string primeira = p.balao.texto;
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.balao.texto != primeira);
    VERIFICAR(nomeDoAmigo("beverly") == "Beverly" && nomeDoAmigo("georgie") == "Georgie");
}
