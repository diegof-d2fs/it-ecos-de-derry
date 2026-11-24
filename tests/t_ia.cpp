// Testes da IA e da partida sem janela (SDD §12 e §19.1). Tudo aqui roda a lógica real do
// jogo, quadro a quadro, sem render: é o mesmo caminho que o piloto automático vai usar.
#include <cmath>
#include <filesystem>

#include "jogo/partida.h"
#include "mundo/caminho.h"
#include "mundo/colisao.h"
#include "teste.h"

namespace {

Conteudo conteudoDeTeste(const char* mapa) {
    Conteudo c;
    SalaModelo m;
    std::string erro;
    lerSala(mapa, m, erro);
    c.salas.push_back(m);
    return c;
}

// Arena 14×9 com um pilar no meio; 2 ondas; spawns nos cantos.
const char* const ARENA =
    "id=arena\nbioma=1\ntipo=combate\nondas=2\norcamento=6\nmapa:\n"
    "##############\n"
    "#s..........s#\n"
    "#............#\n"
    "#.....##.....#\n"
    "#.....##.....L\n"
    "#............#\n"
    "#......E.....#\n"
    "#s..........s#\n"
    "##############\n";

// "Bill de teste": vira para o inimigo mais próximo, ataca, e usa o especial quando cercado.
// (Não é o piloto automático da Etapa 5b; só garante que a sala é vencível.)
EstadoEntrada lutador(const EstadoPartida& p, int quadro) {
    EstadoEntrada e;
    const Inimigo* alvo = nullptr;
    float melhor = 1e9f;
    for (const Inimigo& i : p.inimigos) {
        const float d = comprimento(subtrair(i.pes, p.bill.pes));
        if (i.vivo && d < melhor) { melhor = d; alvo = &i; }
    }
    if (!alvo) return e;
    const Vec2 dir = normalizar(subtrair(alvo->pes, p.bill.pes));
    e.eixo = melhor > 34.0f ? dir : Vec2{dir.x * 0.25f, dir.y * 0.25f};
    if (melhor < 50.0f && quadro % 12 == 0) e.pressionou[static_cast<int>(Comando::Atacar)] = true;
    if (melhor < 45.0f && quadro % 90 == 0) e.pressionou[static_cast<int>(Comando::Especial)] = true;
    return e;
}

}  // namespace

TESTE(ia_caminho_contorna_paredes) {
    Conteudo c = conteudoDeTeste(ARENA);
    const SalaViva sala = criarSalaViva(c.salas[0]);
    // Do lado esquerdo do pilar ao direito: precisa dar a volta (mais que a distância reta).
    VERIFICAR(distanciaCaminho(sala, {5, 3}, {8, 3}, false) > 3);
    const Celula passo = proximoPasso(sala, {5, 3}, {8, 3}, false);
    VERIFICAR(!tileEm(sala, passo.col, passo.lin).bloqueiaMovimento);
    VERIFICAR(distanciaCaminho(sala, {1, 1}, {0, 0}, false) == -1);   // dentro da parede: inalcançável
}

TESTE(ia_inimigos_nunca_entram_em_parede_em_10_mil_passos) {
    Conteudo c = conteudoDeTeste(ARENA);
    EstadoPartida p;
    iniciarPartida(p, c, 777, 0);
    p.bill.hp = 100000;   // Bill parado e "imortal": só observamos a IA
    p.bill.stats.hpMax = 100000;
    const EstadoEntrada nada;
    int violacoes = 0;
    for (int passo = 0; passo < 10000; ++passo) {
        atualizarPartida(p, c, nada, DT);
        for (const Inimigo& i : p.inimigos) {
            if (colideComSala(p.sala, caixaNosPes(i.pes, CAIXA_INIMIGO), parametrosDe(i.tipo).voa)) ++violacoes;
        }
        if (p.inimigos.empty() && p.salaConcluida) break;
    }
    VERIFICAR(violacoes == 0);
}

TESTE(ia_persegue_e_perde_de_vista) {
    // Corredor em L: a sombra vê Bill, persegue; Bill some atrás da parede e ela desiste.
    Conteudo c = conteudoDeTeste(
        "id=l\ntipo=teste\nmapa:\n############\n#E.........#\n#########..#\n#########..#\n############\n");
    const SalaViva sala = criarSalaViva(c.salas[0]);
    Rng rng;
    semear(rng, 5);
    std::vector<Aviso> avisos;
    std::vector<Golpe> golpes;
    std::uint32_t prox = 1;
    Inimigo s = criarInimigo(TipoInimigo::Sombra, 2, centroCelula({9, 1}));
    s.surgindo = 0.0f;
    ContextoIA ctx{sala, centroCelula({2, 1}), true, rng, avisos, golpes, prox};
    atualizarInimigo(s, ctx, DT);
    VERIFICAR(s.estado == EstadoIA::Perseguindo);
    const float antes = s.pes.x;
    for (int i = 0; i < 30; ++i) atualizarInimigo(s, ctx, DT);
    VERIFICAR(s.pes.x < antes);   // aproximou-se
    // Bill "some" (fora da linha de visão: atrás da parede no fim do corredor de baixo).
    ContextoIA longe{sala, centroCelula({10, 3}), true, rng, avisos, golpes, prox};
    s.pes = centroCelula({2, 1});
    atualizarInimigo(s, longe, DT);
    VERIFICAR(s.estado == EstadoIA::Buscando);
    for (int i = 0; i < 60 * 4; ++i) atualizarInimigo(s, longe, DT);
    VERIFICAR(s.estado == EstadoIA::Vagando || s.estado == EstadoIA::Perseguindo);
}

TESTE(partida_mesma_seed_mesma_partida) {
    Conteudo c = conteudoDeTeste(ARENA);
    EstadoPartida a, b;
    iniciarPartida(a, c, 4242, 0);
    iniciarPartida(b, c, 4242, 0);
    for (int q = 0; q < 1800; ++q) {
        atualizarPartida(a, c, lutador(a, q), DT);
        atualizarPartida(b, c, lutador(b, q), DT);
    }
    VERIFICAR(a.stats.K == b.stats.K && a.bill.hp == b.bill.hp && a.inimigos.size() == b.inimigos.size());
    VERIFICAR(std::fabs(a.bill.pes.x - b.bill.pes.x) < 1e-3f && std::fabs(a.bill.pes.y - b.bill.pes.y) < 1e-3f);
}

TESTE(partida_sala_de_duas_ondas_e_vencivel_e_abre_as_portas) {
    // Critério de saída da Etapa 5: limpar uma sala de 2 ondas. Várias seeds, sem janela.
    Conteudo c = conteudoDeTeste(ARENA);
    int vencidas = 0;
    for (std::uint64_t seed = 1; seed <= 10; ++seed) {
        EstadoPartida p;
        iniciarPartida(p, c, seed, 0);
        VERIFICAR(!p.salaConcluida && p.sala.portas[0].aberta == false);   // portas fecham no combate
        for (int q = 0; q < 60 * 90 && !p.salaConcluida && !p.terminou; ++q) atualizarPartida(p, c, lutador(p, q), DT);
        if (p.salaConcluida) {
            ++vencidas;
            VERIFICAR(p.ondaAtual == 2 && p.stats.K >= 2 && p.stats.C == 1);
            VERIFICAR(p.sala.portas[0].aberta && !tileEm(p.sala, 13, 4).bloqueiaMovimento);
            VERIFICAR(p.bill.progresso.xp > 0);
        }
    }
    std::printf("    salas vencidas pelo Bill de teste: %d de 10\n", vencidas);
    VERIFICAR(vencidas >= 8);
}

TESTE(partida_pacto_revive_e_sem_pacto_termina) {
    Conteudo c = conteudoDeTeste(ARENA);
    EstadoPartida p;
    iniciarPartida(p, c, 1, 0);
    p.bill.hp = 0;
    const EstadoEntrada nada;
    atualizarPartida(p, c, nada, DT);
    VERIFICAR(!p.terminou && p.bill.pactos == 0 && p.bill.hp == p.bill.stats.hpMax / 2 && p.stats.U == 1);
    p.bill.hp = 0;
    p.bill.invulneravel = 0.0f;
    atualizarPartida(p, c, nada, DT);
    VERIFICAR(p.terminou && !p.vitoria && p.stats.U == 2);
}

TESTE(partida_salas_reais_rodam_sem_janela) {
    // Todas as salas de data/salas: 60 s de jogo com o Bill de teste, sem travar nem quebrar regras.
    Conteudo c;
    std::string erro;
    VERIFICAR(carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro));
    for (int s = 0; s < static_cast<int>(c.salas.size()); ++s) {
        EstadoPartida p;
        iniciarPartida(p, c, 99, s);
        for (int q = 0; q < 3600 && !p.terminou; ++q) {
            atualizarPartida(p, c, lutador(p, q), DT);
            VERIFICAR(!colideComSala(p.sala, caixaNosPes(p.bill.pes, CAIXA_BILL), false));
        }
        VERIFICAR(p.bill.hp <= p.bill.stats.hpMax);
    }
}
