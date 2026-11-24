// Cenas da história (cut scenes, pedido do Diego em 23/11/2026): formato de data/cenas.txt, o arquivo
// real (abertura ~1 min 30 s, uma cena por chegada de bioma com 5 a 8 quadros, final de 1 a 2 min,
// nenhuma imagem repetida, tempo para ler cada texto) e o tocador (digitação, avançar, segurar para pular).
#include <filesystem>
#include <set>

#include "core/arquivo.h"
#include "jogo/historia.h"
#include "jogo/perfil.h"
#include "teste.h"

namespace {
constexpr float PASSO = 1.0f / 60.0f;

std::vector<CenaHistoria> cenasReais() {
    std::vector<CenaHistoria> cenas;
    std::string erro;
    VERIFICAR(carregarCenasHistoria(std::filesystem::path(ECOS_PASTA_DADOS) / "data/cenas.txt", cenas, erro));
    if (!erro.empty()) std::printf("    %s\n", erro.c_str());
    return cenas;
}

// Toca a cena sem ninguém apertar nada; devolve os segundos até terminar.
float tocarSozinha(const CenaHistoria& c) {
    TocadorHistoria t;
    iniciarTocador(t, &c);
    float tempo = 0.0f;
    while (!t.terminou && tempo < 600.0f) {
        atualizarTocador(t, false, false, PASSO);
        tempo += PASSO;
    }
    return tempo;
}
}  // namespace

TESTE(historia_formato_le_cenas_e_quadros) {
    std::vector<CenaHistoria> cenas;
    std::string erro;
    VERIFICAR(lerCenasHistoria("# comentário\n[abertura] Prólogo\nimg_a;12;zoom_entra;Olá; com ponto e vírgula\n"
                               "-;5;esquerda;FIM\n\n[final] Fim\nimg_b;9.5;cima;Tchau\n",
                               cenas, erro));
    VERIFICAR(cenas.size() == 2);
    VERIFICAR(cenas[0].id == "abertura" && cenas[0].titulo == "Prólogo" && cenas[0].quadros.size() == 2);
    VERIFICAR(cenas[0].quadros[0].texto == "Olá; com ponto e vírgula");
    VERIFICAR(cenas[0].quadros[1].movimento == MovimentoQuadro::Esquerda);
    VERIFICAR_PERTO(cenas[1].quadros[0].segundos, 9.5f, 0.001f);
    VERIFICAR(buscarCenaHistoria(cenas, "final") == &cenas[1]);
    VERIFICAR(buscarCenaHistoria(cenas, "bioma_9") == nullptr);
}

TESTE(historia_formato_rejeita_erros_com_a_linha) {
    std::vector<CenaHistoria> cenas;
    std::string erro;
    VERIFICAR(!lerCenasHistoria("img;10;zoom_entra;sem cena\n", cenas, erro));
    VERIFICAR(!lerCenasHistoria("[a] A\nimg;10;rodopio;x\n", cenas, erro));
    VERIFICAR(erro.find("linha 2") != std::string::npos);
    VERIFICAR(!lerCenasHistoria("[a] A\nimg;dez;cima;x\n", cenas, erro));
    VERIFICAR(!lerCenasHistoria("[a] A\nimg;10\n", cenas, erro));
    VERIFICAR(!lerCenasHistoria("[a] A\nimg;10;cima;x\n[a] B\nimg;10;cima;y\n", cenas, erro));   // repetida
    VERIFICAR(!lerCenasHistoria("[a] A\n", cenas, erro));                                          // sem quadros
}

TESTE(historia_cena_de_cada_bioma) {
    VERIFICAR(cenaDaChegadaAoBioma(1).empty());   // o bioma 1 já vem da abertura e do prólogo
    VERIFICAR(cenaDaChegadaAoBioma(2) == "bioma_2");
    VERIFICAR(cenaDaChegadaAoBioma(7) == "bioma_7");
    VERIFICAR(cenaDaChegadaAoBioma(8).empty());
}

// O pedido do Diego, número a número.
TESTE(historia_arquivo_real_tem_todas_as_cenas_no_tamanho_pedido) {
    const std::vector<CenaHistoria> cenas = cenasReais();
    const CenaHistoria* abertura = buscarCenaHistoria(cenas, "abertura");
    VERIFICAR(abertura != nullptr);
    if (abertura) {
        VERIFICAR(abertura->quadros.size() >= 5 && abertura->quadros.size() <= 8);
        VERIFICAR(duracaoCena(*abertura) >= 80.0f && duracaoCena(*abertura) <= 100.0f);   // ~1 min 30 s
    }
    for (int b = 2; b <= 7; ++b) {
        const CenaHistoria* c = buscarCenaHistoria(cenas, cenaDaChegadaAoBioma(b));
        VERIFICAR(c != nullptr);
        if (!c) continue;
        VERIFICAR(c->quadros.size() >= 5 && c->quadros.size() <= 8);
        VERIFICAR(duracaoCena(*c) >= 50.0f && duracaoCena(*c) <= 90.0f);
    }
    const CenaHistoria* final_ = buscarCenaHistoria(cenas, "final");
    VERIFICAR(final_ != nullptr);
    if (final_) VERIFICAR(duracaoCena(*final_) >= 60.0f && duracaoCena(*final_) <= 120.0f);   // 1 a 2 min
}

TESTE(historia_nenhuma_imagem_se_repete_e_todas_existem_ou_estao_encomendadas) {
    std::string manifesto, prompts;
    lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / "assets/jogo_hd/manifesto.txt", manifesto);
    lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / "instructions/Prompts-Assets-IT-v6-cenas.md", prompts);
    std::set<std::string> vistas;
    int faltando = 0;
    for (const CenaHistoria& c : cenasReais()) {
        for (const QuadroHistoria& q : c.quadros) {
            if (q.imagem == "-") continue;
            if (!vistas.insert(q.imagem).second) {
                std::printf("    imagem repetida: %s (%s)\n", q.imagem.c_str(), c.id.c_str());
                VERIFICAR(false);
            }
            const bool exportada = manifesto.find(q.imagem + ".png ") != std::string::npos;
            const bool encomendada = prompts.find("**Arquivo:** " + q.imagem + ".png") != std::string::npos;
            if (!exportada) ++faltando;
            if (!exportada && !encomendada) std::printf("    imagem sem asset nem prompt: %s\n", q.imagem.c_str());
            VERIFICAR(exportada || encomendada);   // erro de digitação no nome não passa
        }
    }
    if (faltando > 0) std::printf("    (aviso) %d quadros ainda sem imagem exportada: aparecem em preto\n", faltando);
}

// "Tem que dar tempo de apreciar a imagem e ler o texto": nenhum quadro do arquivo depende do mínimo.
TESTE(historia_todo_quadro_da_tempo_de_ler) {
    for (const CenaHistoria& c : cenasReais()) {
        for (const QuadroHistoria& q : c.quadros) {
            if (q.segundos + 0.001f < tempoMinimoDoQuadro(letrasDoQuadro(q))) {
                std::printf("    %s / %s: %.1f s < %.1f s\n", c.id.c_str(), q.imagem.c_str(), q.segundos,
                            tempoMinimoDoQuadro(letrasDoQuadro(q)));
                VERIFICAR(false);
            }
        }
    }
}

TESTE(historia_tocador_sozinho_dura_o_tempo_da_cena) {
    for (const CenaHistoria& c : cenasReais()) VERIFICAR_PERTO(tocarSozinha(c), duracaoCena(c), 0.1f);
}

TESTE(historia_tocador_digita_e_confirmar_completa_depois_avanca) {
    CenaHistoria c;
    c.id = "x";
    c.quadros = {{"a", 10.0f, MovimentoQuadro::ZoomEntra, "Uma frase com algumas letras."},
                 {"b", 10.0f, MovimentoQuadro::Cima, "Outra."}};
    TocadorHistoria t;
    iniciarTocador(t, &c);
    VERIFICAR(letrasVisiveis(t) == 0);
    for (int i = 0; i < 80; ++i) atualizarTocador(t, false, false, PASSO);   // 1,33 s: começou a digitar
    VERIFICAR(letrasVisiveis(t) > 0 && letrasVisiveis(t) < letrasDoQuadro(c.quadros[0]));
    atualizarTocador(t, true, false, PASSO);   // 1º toque: o texto inteiro
    VERIFICAR(t.quadro == 0 && letrasVisiveis(t) == letrasDoQuadro(c.quadros[0]));
    atualizarTocador(t, true, false, PASSO);   // 2º toque: próximo quadro
    atualizarTocador(t, false, false, PASSO);
    VERIFICAR(t.quadro == 1 && letrasVisiveis(t) == 0);
    VERIFICAR(!t.terminou);
}

TESTE(historia_tocador_toque_cedo_nao_pula_quadro) {
    CenaHistoria c;
    c.quadros = {{"a", 10.0f, MovimentoQuadro::ZoomEntra, ""}, {"b", 10.0f, MovimentoQuadro::ZoomEntra, ""}};
    TocadorHistoria t;
    iniciarTocador(t, &c);
    atualizarTocador(t, true, false, PASSO);   // logo no começo: ainda nem deu para ver a imagem
    atualizarTocador(t, false, false, PASSO);
    VERIFICAR(t.quadro == 0);
}

TESTE(historia_tocador_segurar_voltar_pula_e_tocar_nao) {
    CenaHistoria c;
    c.quadros = {{"a", 30.0f, MovimentoQuadro::ZoomEntra, "Texto."}};
    TocadorHistoria t;
    iniciarTocador(t, &c);
    for (int i = 0; i < 20; ++i) {   // toques curtos e soltos: não pula
        atualizarTocador(t, false, true, PASSO);
        atualizarTocador(t, false, false, PASSO);
    }
    VERIFICAR(!t.terminou);
    for (int i = 0; i < 70 && !t.terminou; ++i) atualizarTocador(t, false, true, PASSO);   // segurou ~1 s
    VERIFICAR(t.terminou && t.pulou);
}

TESTE(historia_perfil_guarda_cenas_vistas_e_a_opcao) {
    Perfil p;
    p.cenasVistas = {"abertura", "bioma_2"};
    p.opcoes.cenasSempre = true;
    Perfil lido;
    lerPerfil(escreverPerfil(p), lido);
    VERIFICAR(lido.cenasVistas == p.cenasVistas);
    VERIFICAR(lido.opcoes.cenasSempre);
    Perfil novo;
    VERIFICAR(!novo.opcoes.cenasSempre && novo.cenasVistas.empty());   // padrão: cada cena só na 1ª vez
}

// Regra do Diego (23/11/2026): nunca travessão (U+2014) em título ou texto que aparece no jogo. Vale para
// as cenas, as falas, os nomes de sala e as strings das telas (comentários com '#' e '//' não contam).
TESTE(textos_do_jogo_nao_usam_travessao) {
    const std::filesystem::path raiz(ECOS_PASTA_DADOS);
    std::vector<std::filesystem::path> arquivos = {raiz / "data/cenas.txt", raiz / "data/dialogos.txt"};
    for (const auto& e : std::filesystem::directory_iterator(raiz / "data/salas")) arquivos.push_back(e.path());
    for (const auto& e : std::filesystem::recursive_directory_iterator(raiz / "src")) {
        if (e.path().extension() == ".cpp" || e.path().extension() == ".h") arquivos.push_back(e.path());
    }
    const std::string travessao = "\xE2\x80\x94";   // U+2014 em UTF-8
    for (const auto& arquivo : arquivos) {
        std::string texto;
        lerArquivoTexto(arquivo, texto);
        const bool codigo = arquivo.extension() != ".txt";
        std::size_t inicio = 0;
        int n = 0;
        while (inicio < texto.size()) {
            std::size_t fim = texto.find('\n', inicio);
            if (fim == std::string::npos) fim = texto.size();
            std::string linha = texto.substr(inicio, fim - inicio);
            inicio = fim + 1;
            ++n;
            const std::size_t primeiro = linha.find_first_not_of(" \t");
            if (!codigo && primeiro != std::string::npos && linha[primeiro] == '#') continue;
            if (codigo) {   // só o que está entre aspas, antes de um comentário de linha
                const std::size_t barra = linha.find("//");
                if (barra != std::string::npos) linha = linha.substr(0, barra);
                const std::size_t aspas = linha.find('"');
                if (aspas == std::string::npos) continue;
                linha = linha.substr(aspas);
            }
            if (linha.find(travessao) == std::string::npos) continue;
            std::printf("    travessão em %s:%d\n", arquivo.filename().string().c_str(), n);
            VERIFICAR(false);
        }
    }
}
