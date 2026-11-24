#include "jogo/simulacao.h"

#include <algorithm>
#include <cstdio>

#include "jogo/piloto.h"

const char* nomeFim(FimSimulacao f) {
    switch (f) {
        case FimSimulacao::Completou: return "completou";
        case FimSimulacao::Morreu: return "morreu";
        case FimSimulacao::Venceu: return "venceu";
        default: return "tempo_esgotado";
    }
}

ResultadoSimulacao simularTentativa(const Conteudo& c, std::uint64_t seed, Dificuldade d, float limiteSegundos, float reacao, int distracao) {
    ResultadoSimulacao r;
    r.seed = seed;
    r.dificuldade = d;
    EstadoPartida p;
    std::string erro;
    if (!iniciarTentativa(p, c, seed, d, erro)) return r;
    MemoriaPiloto m;
    m.atrasoReacao = reacao;
    m.distracaoPct = distracao;
    auto abrirSala = [&p]() {
        SalaSimulada s{p.etapa, p.sala.modelo->id, 0.0f, 0};
        s.moedasAoEntrar = p.inventario.quantidades[static_cast<std::size_t>(TipoItem::Moeda)];
        s.hpAoEntrar = p.bill.hp;
        s.hpMaximo = p.bill.stats.hpMax;
        s.abates = p.stats.K;   // guardado como referência; vira a diferença ao fechar
        return s;
    };
    SalaSimulada atual = abrirSala();
    int hpAntes = p.bill.hp;
    // Limite em QUADROS (passos de 1/60 s), não no cronômetro: menus congelam o cronômetro.
    const int quadros = static_cast<int>(limiteSegundos * 60.0f) * 2;
    for (int q = 0; q < quadros && !p.terminou && p.stats.tempo < limiteSegundos; ++q) {
        const float antes = p.stats.tempo;
        const EstadoEntrada e = pilotar(p, m, DT);
        atualizarPartida(p, c, e, DT);
        p.sons.clear();
        atual.segundos += p.stats.tempo - antes;
        if (p.bill.hp < hpAntes) atual.danoRecebido += hpAntes - p.bill.hp;
        hpAntes = p.bill.hp;
        if (p.trocouDeSala) {   // fecha a sala anterior e abre a nova
            atual.abates = p.stats.K - atual.abates;
            r.salas.push_back(atual);
            atual = abrirSala();
        }
    }
    atual.abates = p.stats.K - atual.abates;
    r.salas.push_back(atual);
    r.fim = p.vitoria ? FimSimulacao::Venceu
          : p.fimDoConteudo ? FimSimulacao::Completou : (p.terminou ? FimSimulacao::Morreu : FimSimulacao::TempoEsgotado);
    r.bioma = p.plano.bioma;
    r.etapa = p.etapa;
    r.sala = p.sala.modelo->id;
    r.tempo = p.stats.tempo;
    r.pontos = pontuacaoAtual(p);
    r.hpFinal = p.bill.hp;
    r.kitsUsados = p.kitsUsados;
    r.abates = p.stats.K;
    r.vidasUsadas = p.stats.U;
    r.resgates = p.stats.R;
    r.companheiro = p.companheiro.ativo ? codigoAmigo(p.companheiro.quem) : "-";
    r.manivela = p.manivelaEntregue;
    r.compras = p.compras;
    r.nivel = p.bill.progresso.nivel;
    r.causa = p.ultimoAgressor;
    return r;
}

std::string csvTentativas(const std::vector<ResultadoSimulacao>& lista) {
    std::string s = "seed;dificuldade;fim;bioma;etapa;sala;tempo_s;pontos;hp_final;nivel;abates;kits;vidas_usadas;causa;resgates;companheiro;manivela;compras\n";
    char linha[256];
    for (const ResultadoSimulacao& r : lista) {
        std::snprintf(linha, sizeof linha, "%llu;%s;%s;%d;%d;%s;%.1f;%lld;%d;%d;%d;%d;%d;%s;%d;%s;%d;%d\n",
                      static_cast<unsigned long long>(r.seed), codigoDificuldade(r.dificuldade), nomeFim(r.fim), r.bioma,
                      r.etapa, r.sala.c_str(), static_cast<double>(r.tempo), r.pontos, r.hpFinal, r.nivel, r.abates,
                      r.kitsUsados, r.vidasUsadas,
                      r.causa >= 0 ? parametrosDe(static_cast<TipoInimigo>(r.causa)).atlas : "-", r.resgates, r.companheiro.c_str(), r.manivela ? 1 : 0, r.compras);
        s += linha;
    }
    return s;
}

std::string csvSalas(const std::vector<ResultadoSimulacao>& lista) {
    std::string s = "seed;dificuldade;etapa;sala;segundos;dano_recebido;moedas_entrada;hp_entrada;hp_max;abates\n";
    char linha[224];
    for (const ResultadoSimulacao& r : lista) {
        for (const SalaSimulada& sala : r.salas) {
            std::snprintf(linha, sizeof linha, "%llu;%s;%d;%s;%.1f;%d;%d;%d;%d;%d\n", static_cast<unsigned long long>(r.seed),
                          codigoDificuldade(r.dificuldade), sala.etapa, sala.sala.c_str(), static_cast<double>(sala.segundos),
                          sala.danoRecebido, sala.moedasAoEntrar, sala.hpAoEntrar, sala.hpMaximo, sala.abates);
            s += linha;
        }
    }
    return s;
}

std::string resumoSimulacao(const std::vector<ResultadoSimulacao>& lista) {
    if (lista.empty()) return "Nenhuma tentativa simulada.\n";
    int completas = 0, mortes = 0, esgotadas = 0, vitorias = 0;
    float soma = 0.0f, menor = 1e9f, maior = 0.0f;
    long long pontos = 0;
    for (const ResultadoSimulacao& r : lista) {
        const bool chegou = r.fim == FimSimulacao::Completou || r.fim == FimSimulacao::Venceu;
        completas += chegou ? 1 : 0;
        vitorias += r.fim == FimSimulacao::Venceu ? 1 : 0;
        mortes += r.fim == FimSimulacao::Morreu ? 1 : 0;
        esgotadas += r.fim == FimSimulacao::TempoEsgotado ? 1 : 0;
        if (chegou) {
            soma += r.tempo;
            menor = std::min(menor, r.tempo);
            maior = std::max(maior, r.tempo);
        }
        pontos += r.pontos;
    }
    char t[512];
    const int n = static_cast<int>(lista.size());
    std::snprintf(t, sizeof t,
                  "Simulação: %d tentativas (%s)\n"
                  "  completaram o conteúdo: %d (%.0f%%) · VENCERAM o Pennywise IV: %d\n"
                  "  morreram: %d · tempo esgotado: %d\n"
                  "  tempo das completas: média %.0f s · mín. %.0f s · máx. %.0f s\n"
                  "  pontuação média: %lld\n",
                  n, codigoDificuldade(lista[0].dificuldade), completas, 100.0 * completas / n, vitorias, mortes, esgotadas,
                  completas ? static_cast<double>(soma / static_cast<float>(completas)) : 0.0,
                  completas ? static_cast<double>(menor) : 0.0, completas ? static_cast<double>(maior) : 0.0, pontos / n);
    return t;
}
