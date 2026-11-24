// Testes da Etapa 1: tipos, RNG, entrada, manifesto, medidor e escala.
#include <filesystem>
#include <set>
#include <string>

#include "core/arquivo.h"
#include "core/entrada.h"
#include "core/escala.h"
#include "core/manifesto.h"
#include "core/medidor.h"
#include "core/rng.h"
#include "core/tipos.h"
#include "teste.h"

TESTE(rng_mesma_seed_mesma_sequencia) {
    Rng a, b;
    semear(a, 12345);
    semear(b, 12345);
    bool iguais = true;
    for (int i = 0; i < 1000; ++i) iguais = iguais && proximo(a) == proximo(b);
    VERIFICAR(iguais);
    Rng c;
    semear(c, 12346);
    semear(a, 12345);
    VERIFICAR(proximo(a) != proximo(c));
}

TESTE(rng_intervalo_respeita_limites_e_cobre_todos) {
    Rng r;
    semear(r, 7);
    std::set<int> vistos;
    bool dentro = true;
    for (int i = 0; i < 5000; ++i) {
        const int v = intervalo(r, -3, 3);
        dentro = dentro && v >= -3 && v <= 3;
        vistos.insert(v);
    }
    VERIFICAR(dentro);
    VERIFICAR(vistos.size() == 7u);
    VERIFICAR(intervalo(r, 5, 5) == 5);
    VERIFICAR(intervalo(r, 9, 2) == 9);   // intervalo invertido não trava
    bool real = true;
    for (int i = 0; i < 5000; ++i) {
        const float f = real01(r);
        real = real && f >= 0.0f && f < 1.0f;
    }
    VERIFICAR(real);
}

TESTE(direcao_dominante_e_celula) {
    VERIFICAR(direcaoDominante({1, 0.2f}, Direcao::Cima) == Direcao::Direita);
    VERIFICAR(direcaoDominante({-1, 0.5f}, Direcao::Cima) == Direcao::Esquerda);
    VERIFICAR(direcaoDominante({0.1f, -1}, Direcao::Baixo) == Direcao::Cima);
    VERIFICAR(direcaoDominante({0, 0}, Direcao::Esquerda) == Direcao::Esquerda);   // parado mantém
    VERIFICAR(celulaDe({31.9f, 32.0f}).col == 0);
    VERIFICAR(celulaDe({31.9f, 32.0f}).lin == 1);
    VERIFICAR(celulaDe({-0.5f, 0}).col == -1);   // fora do mapa à esquerda é -1, não 0
}

TESTE(entrada_bordas_e_diagonal_normalizada) {
    EstadoEntrada e;
    std::array<bool, TOTAL_COMANDOS> anterior{};
    e.segurando[static_cast<int>(Comando::Atacar)] = true;
    calcularBordas(e, anterior);
    VERIFICAR(pressionou(e, Comando::Atacar));
    limparBordas(e);
    anterior = e.segurando;
    calcularBordas(e, anterior);   // continua segurando: não há nova borda
    VERIFICAR(!pressionou(e, Comando::Atacar));

    EstadoEntrada d;
    d.segurando[static_cast<int>(Comando::Cima)] = true;
    d.segurando[static_cast<int>(Comando::Direita)] = true;
    const Vec2 eixo = eixoDosDirecionais(d);
    VERIFICAR_PERTO(comprimento(eixo), 1.0f, 1e-4f);   // diagonal não é mais rápida
    limparEntrada(d);
    VERIFICAR(!segurando(d, Comando::Cima));
}

TESTE(manifesto_le_valida_e_busca) {
    const std::string texto =
        "# comentario\n"
        "bill_walk.png 128 192 32 48 4 4 pes\r\n"
        "\n"
        "galeria.png 128 128 32 32 4 4 -\n";
    std::vector<EntradaManifesto> m;
    std::string erro;
    VERIFICAR(lerManifesto(texto, m, erro));
    VERIFICAR(m.size() == 2u);
    const EntradaManifesto* bill = buscarManifesto(m, "bill_walk");
    VERIFICAR(bill != nullptr);
    VERIFICAR(bill != nullptr && bill->frameAltura == 48 && bill->pivo == "pes");
    VERIFICAR(buscarManifesto(m, "nao_existe") == nullptr);

    VERIFICAR(!lerManifesto("x.png 100 100 32 32 4 4 pes\n", m, erro));   // 32×4 ≠ 100
    VERIFICAR(erro.find("linha 1") != std::string::npos);
    VERIFICAR(!lerManifesto("x.png 10 10\n", m, erro));
}

TESTE(manifesto_real_dos_assets_e_valido) {
    std::string texto;
    const std::filesystem::path caminho = std::filesystem::path(ECOS_PASTA_DADOS) / "assets/jogo/manifesto.txt";
    VERIFICAR(lerArquivoTexto(caminho, texto));
    std::vector<EntradaManifesto> m;
    std::string erro;
    VERIFICAR(lerManifesto(texto, m, erro));
    if (!erro.empty()) std::printf("    %s\n", erro.c_str());
    for (const char* nome : {"bill_walk", "galeria", "pennywise_4_actions", "capa_menu"}) {
        VERIFICAR(buscarManifesto(m, nome) != nullptr);
    }
    const EntradaManifesto* capa = buscarManifesto(m, "capa_menu");
    VERIFICAR(capa != nullptr && capa->largura == LARGURA_LOGICA && capa->altura == ALTURA_LOGICA);
}

TESTE(medidor_media_e_p95) {
    MedidorQuadros m;
    VERIFICAR(mediaMs(m) == 0.0f);
    for (int i = 1; i <= 100; ++i) registrarQuadro(m, static_cast<float>(i));
    VERIFICAR_PERTO(mediaMs(m), 50.5f, 1e-3f);
    VERIFICAR_PERTO(percentil95Ms(m), 96.0f, 1.0f);
    for (int i = 0; i < 1000; ++i) registrarQuadro(m, 2.0f);   // anel sobrescreve antigos
    VERIFICAR_PERTO(percentil95Ms(m), 2.0f, 1e-3f);
}

TESTE(escala_inteira_centralizada) {
    Apresentacao a = calcularApresentacao(1920, 1080, 640, 360);
    VERIFICAR(a.escala == 3 && a.x == 0 && a.y == 0);
    a = calcularApresentacao(1366, 768, 640, 360);
    VERIFICAR(a.escala == 2 && a.x == 43 && a.y == 24);
    a = calcularApresentacao(2560, 1440, 640, 360, 3);   // limite máximo respeitado
    VERIFICAR(a.escala == 3 && a.largura == 1920);
    a = calcularApresentacao(600, 400, 640, 360);
    VERIFICAR(a.escala == 0);   // menor que o mínimo: não corta o mapa
}

TESTE(arquivo_gravacao_atomica) {
    const std::filesystem::path caminho = std::filesystem::temp_directory_path() / "ecos_teste" / "perfil.txt";
    VERIFICAR(gravarArquivoTexto(caminho, "versao=1\nlembrancas=42\n"));
    std::string lido;
    VERIFICAR(lerArquivoTexto(caminho, lido));
    VERIFICAR(lido == "versao=1\nlembrancas=42\n");
    VERIFICAR(gravarArquivoTexto(caminho, "versao=1\n"));   // sobrescreve
    VERIFICAR(lerArquivoTexto(caminho, lido) && lido == "versao=1\n");
    std::filesystem::remove_all(caminho.parent_path());
}
