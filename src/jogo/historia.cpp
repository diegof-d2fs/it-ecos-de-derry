#include "jogo/historia.h"

#include <algorithm>
#include <sstream>

#include "core/arquivo.h"
#include "jogo/dialogo.h"   // contarCaracteres

namespace {

std::string aparar(const std::string& s) {
    const auto ini = s.find_first_not_of(" \t\r");
    if (ini == std::string::npos) return "";
    const auto fim = s.find_last_not_of(" \t\r");
    return s.substr(ini, fim - ini + 1);
}

bool lerMovimento(const std::string& nome, MovimentoQuadro& m) {
    static const struct { const char* nome; MovimentoQuadro m; } TABELA[] = {
        {"zoom_entra", MovimentoQuadro::ZoomEntra}, {"zoom_sai", MovimentoQuadro::ZoomSai},
        {"esquerda", MovimentoQuadro::Esquerda},    {"direita", MovimentoQuadro::Direita},
        {"cima", MovimentoQuadro::Cima},            {"baixo", MovimentoQuadro::Baixo},
    };
    for (const auto& t : TABELA) {
        if (nome == t.nome) {
            m = t.m;
            return true;
        }
    }
    return false;
}

}  // namespace

float duracaoQuadro(const QuadroHistoria& q) { return std::max(q.segundos, tempoMinimoDoQuadro(letrasDoQuadro(q))); }

float tempoMinimoDoQuadro(int letras) {
    return HISTORIA_ATRASO_TEXTO + static_cast<float>(letras) / HISTORIA_LETRAS_POR_SEGUNDO + HISTORIA_LEITURA_MINIMA;
}

int letrasDoQuadro(const QuadroHistoria& q) { return contarCaracteres(q.texto); }

bool lerCenasHistoria(const std::string& texto, std::vector<CenaHistoria>& cenas, std::string& erro) {
    cenas.clear();
    std::istringstream entrada(texto);
    std::string linha;
    int numero = 0;
    auto falha = [&](const std::string& motivo) {
        erro = "linha " + std::to_string(numero) + ": " + motivo;
        return false;
    };
    while (std::getline(entrada, linha)) {
        ++numero;
        linha = aparar(linha);
        if (linha.empty() || linha[0] == '#') continue;
        if (linha[0] == '[') {   // [id] Título
            const auto fecha = linha.find(']');
            if (fecha == std::string::npos || fecha == 1) return falha("cabeçalho deve ser [id] Título");
            CenaHistoria c;
            c.id = aparar(linha.substr(1, fecha - 1));
            c.titulo = aparar(linha.substr(fecha + 1));
            if (buscarCenaHistoria(cenas, c.id)) return falha("cena repetida: " + c.id);
            cenas.push_back(c);
            continue;
        }
        if (cenas.empty()) return falha("quadro antes de qualquer [cena]");
        // imagem;segundos;movimento;texto (o texto é o resto da linha)
        std::string campos[3];
        std::size_t inicio = 0;
        for (std::string& campo : campos) {
            const auto pv = linha.find(';', inicio);
            if (pv == std::string::npos) return falha("esperado imagem;segundos;movimento;texto");
            campo = aparar(linha.substr(inicio, pv - inicio));
            inicio = pv + 1;
        }
        QuadroHistoria q;
        q.imagem = campos[0];
        if (q.imagem.empty()) return falha("imagem vazia");
        try {
            q.segundos = std::stof(campos[1]);
        } catch (...) {
            return falha("segundos inválidos: " + campos[1]);
        }
        if (q.segundos <= 0.0f || q.segundos > 60.0f) return falha("segundos fora de 0..60");
        if (!lerMovimento(campos[2], q.movimento)) return falha("movimento desconhecido: " + campos[2]);
        q.texto = aparar(linha.substr(inicio));
        cenas.back().quadros.push_back(q);
    }
    for (const CenaHistoria& c : cenas) {
        if (c.quadros.empty()) {
            erro = "cena sem quadros: " + c.id;
            return false;
        }
    }
    return true;
}

bool carregarCenasHistoria(const std::filesystem::path& caminho, std::vector<CenaHistoria>& cenas, std::string& erro) {
    std::string texto;
    if (!lerArquivoTexto(caminho, texto)) {   // sem o arquivo: o jogo segue sem cenas
        cenas.clear();
        return true;
    }
    if (!lerCenasHistoria(texto, cenas, erro)) {
        erro = "data/cenas.txt: " + erro;
        return false;
    }
    return true;
}

const CenaHistoria* buscarCenaHistoria(const std::vector<CenaHistoria>& cenas, const std::string& id) {
    for (const CenaHistoria& c : cenas) {
        if (c.id == id) return &c;
    }
    return nullptr;
}

std::string cenaDaChegadaAoBioma(int bioma) {
    if (bioma < 2 || bioma > 7) return "";
    return "bioma_" + std::to_string(bioma);
}

float duracaoCena(const CenaHistoria& cena) {
    float total = HISTORIA_SAIDA;
    for (const QuadroHistoria& q : cena.quadros) total += duracaoQuadro(q);
    return total;
}

// ---------- tocador ----------
void iniciarTocador(TocadorHistoria& t, const CenaHistoria* cena) {
    t = TocadorHistoria{};
    t.cena = cena;
    t.terminou = cena == nullptr || cena->quadros.empty();
}

int letrasVisiveis(const TocadorHistoria& t) {
    if (!t.cena || t.terminou) return 0;
    const int total = letrasDoQuadro(t.cena->quadros[static_cast<std::size_t>(t.quadro)]);
    const float digitadas = (t.tempoQuadro - HISTORIA_ATRASO_TEXTO) * HISTORIA_LETRAS_POR_SEGUNDO;
    return std::max(0, std::min(total, static_cast<int>(digitadas)));
}

float progressoDoQuadro(const TocadorHistoria& t) {
    if (!t.cena || t.cena->quadros.empty()) return 0.0f;
    const float d = duracaoQuadro(t.cena->quadros[static_cast<std::size_t>(t.quadro)]);
    return std::min(1.0f, t.tempoQuadro / (d + HISTORIA_TRANSICAO));
}

void atualizarTocador(TocadorHistoria& t, bool avancar, bool segurandoVoltar, float dt) {
    if (t.terminou || !t.cena) return;
    t.tempoCena += dt;
    t.ultimoToque = (avancar || segurandoVoltar) ? 0.0f : t.ultimoToque + dt;
    // Pular: segurar (não tocar) Voltar — um toque sem querer não joga a história fora.
    t.segurandoPular = segurandoVoltar ? t.segurandoPular + dt : 0.0f;
    if (t.segurandoPular >= HISTORIA_SEGURAR_PULAR) {
        t.pulou = true;
        t.terminou = true;
        return;
    }
    const std::vector<QuadroHistoria>& qs = t.cena->quadros;
    const bool ultimo = t.quadro + 1 >= static_cast<int>(qs.size());
    const float duracao = duracaoQuadro(qs[static_cast<std::size_t>(t.quadro)]);
    if (t.saida > 0.0f) {   // a última imagem sumindo no preto
        t.tempoQuadro += dt;
        t.saida += dt;
        if (t.saida >= HISTORIA_SAIDA) t.terminou = true;
        return;
    }
    if (avancar) {
        const int total = letrasDoQuadro(qs[static_cast<std::size_t>(t.quadro)]);
        if (letrasVisiveis(t) < total) {   // 1º toque: o texto aparece inteiro
            t.tempoQuadro = std::max(t.tempoQuadro, HISTORIA_ATRASO_TEXTO + static_cast<float>(total) / HISTORIA_LETRAS_POR_SEGUNDO + 0.01f);
        } else if (t.tempoQuadro >= HISTORIA_TOQUE_MINIMO) {   // 2º toque: próximo quadro (com a dissolução)
            t.tempoQuadro = std::max(t.tempoQuadro, duracao);
        }
    }
    t.tempoQuadro += dt;
    if (t.tempoQuadro < duracao) return;
    if (ultimo) {
        t.saida = std::max(t.tempoQuadro - duracao, 0.0001f);   // começa a sumir no preto (com a sobra do passo)
        return;
    }
    ++t.quadro;
    t.tempoQuadro -= duracao;   // a sobra do passo não se perde: a cena dura o que o arquivo diz
}
