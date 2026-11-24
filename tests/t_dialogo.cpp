// Diálogos e tutorial (Etapa 7.3; SDD §13.2–13.3, PRD §4.6, §15.3, §15.4): formato do arquivo,
// condições, o arquivo real (40 falas no Clube, 20 dicas, retratos existentes), a caixa na
// partida (digitação, avanço, escolhas, mundo congelado), "1 fala nova por amigo por visita"
// e as dicas (uma vez, desligáveis, nunca para o piloto).
#include <filesystem>
#include <map>
#include <sstream>

#include "core/arquivo.h"

#include "jogo/clube.h"
#include "jogo/piloto.h"
#include "teste.h"

namespace {
Conteudo conteudoReal() {
    Conteudo c;
    std::string erro;
    carregarSalas(std::filesystem::path(ECOS_PASTA_DADOS) / "data/salas", c.salas, erro);
    carregarDialogos(std::filesystem::path(ECOS_PASTA_DADOS) / "data/dialogos.txt", c.dialogos, erro);
    return c;
}

void apertar(EstadoPartida& p, const Conteudo& c, Comando cmd) {
    EstadoEntrada e;
    e.pressionou[static_cast<int>(cmd)] = true;
    atualizarPartida(p, c, e, DT);
}

void esperar(EstadoPartida& p, const Conteudo& c, int quadros) {
    for (int q = 0; q < quadros; ++q) atualizarPartida(p, c, EstadoEntrada{}, DT);
}

// Lê a conversa inteira só com Confirmar (escolhas: a primeira). Devolve quantos toques usou.
int lerAteOFim(EstadoPartida& p, const Conteudo& c) {
    int toques = 0;
    while (p.pendencia == Pendencia::Dialogo && toques < 200) {
        apertar(p, c, Comando::Confirmar);
        ++toques;
    }
    return toques;
}

void irAte(EstadoPartida& p, char codigo) {
    for (const ObjetoSala& o : p.sala.objetos) {
        if (o.codigo == codigo) posicionarJogador(p.bill, centroCelula({o.celula.col, o.celula.lin + 1}));
    }
}
}  // namespace

TESTE(dialogo_formato_condicoes_escolhas_e_erros) {
    const std::string texto =
        "# comentário\n"
        "@a\n"
        "quem=ben\n"
        "quando=hub; mortes>=2; morreu_para=rato\n"
        "Ben|timido: Olá: tudo bem?\n"
        "Bill: Tudo.\n"
        "escolha=Sim|b\n"
        "escolha=Não|b\n"
        "efeito=flag:Teste\n"
        "\n"
        "@b\n"
        "Mike: Fim.\n";
    std::vector<Dialogo> ds;
    std::string erro;
    VERIFICAR(lerDialogos(texto, ds, erro));
    VERIFICAR(ds.size() == 2);
    const Dialogo& a = ds[0];
    VERIFICAR(a.quem == "ben" && a.condicoes.size() == 3 && a.linhas.size() == 2 && a.escolhas.size() == 2);
    VERIFICAR(a.linhas[0].falante == "Ben" && a.linhas[0].expressao == "timido" && a.linhas[0].texto == "Olá: tudo bem?");
    VERIFICAR(a.efeitos.size() == 1 && a.efeitos[0] == "flag:Teste");
    VERIFICAR(a.condicoes[2].tipo == TipoCondicao::MorreuPara && a.condicoes[2].valor == static_cast<int>(TipoInimigo::Rato));
    // Erros claros, com a linha.
    VERIFICAR(!lerDialogos("@x\nquando=voando\nBen: oi\n", ds, erro) && erro.find("linha 2") == 0);
    VERIFICAR(!lerDialogos("Ben: solto\n", ds, erro));
    VERIFICAR(!lerDialogos("@x\nBen Hanscom: oi\n", ds, erro));
    VERIFICAR(!lerDialogos("@x\nBen: oi\nescolha=sem destino\n", ds, erro));
    VERIFICAR(!lerDialogos("@x\nquando=morreu_para=palhaco\nBen: oi\n", ds, erro));
    // Validação: escolha para id inexistente e escolha única.
    VERIFICAR(lerDialogos("@x\nBen: oi\nescolha=A|y\nescolha=B|nada\n\n@y\nBen: ok\n", ds, erro));
    bool achou = false;
    for (const std::string& pr : validarDialogos(ds)) achou |= pr.find("nada") != std::string::npos;
    VERIFICAR(achou);
}

TESTE(dialogo_condicoes_e_fala_nova_do_amigo) {
    std::vector<Dialogo> ds;
    std::string erro;
    lerDialogos("@especial\nquem=ben\nquando=hub; mortes>=3\nBen: 3 mortes\n\n"
                "@comum\nquem=ben\nquando=hub\nBen: comum\n\n"
                "@outra\nquem=ben\nquando=hub; vitorias>=1\nBen: venceu\n",
                ds, erro);
    std::set<std::string> vistas;
    ContextoFala ctx;
    ctx.noClube = true;
    ctx.vistas = &vistas;
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx)->id == "comum");
    ctx.mortes = 3;
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx)->id == "especial");   // a especial vem antes no arquivo
    vistas.insert("especial");
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx)->id == "comum");
    vistas.insert("comum");
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx) == nullptr);           // vista não se repete
    ctx.vitorias = 1;
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx)->id == "outra");
    ctx.noClube = false;
    VERIFICAR(falaNovaDoAmigo(ds, "ben", ctx) == nullptr);           // "hub" só vale no Clube
    VERIFICAR(falaNovaDoAmigo(ds, "stan", ctx) == nullptr);
}

TESTE(dialogo_arquivo_real_completo_e_com_retratos) {
    std::vector<Dialogo> ds;
    std::string erro;
    VERIFICAR(carregarDialogos(std::filesystem::path(ECOS_PASTA_DADOS) / "data/dialogos.txt", ds, erro));
    const std::vector<std::string> problemas = validarDialogos(ds);
    for (const std::string& pr : problemas) std::printf("    %s\n", pr.c_str());
    VERIFICAR(problemas.empty());
    std::map<std::string, int> porAmigo;
    std::set<std::string> falantes;
    for (const Dialogo& d : ds) {
        if (!d.quem.empty()) ++porAmigo[d.quem];
        for (const LinhaDialogo& l : d.linhas) falantes.insert(l.falante);
    }
    // Meta P0 do PRD §4.6: pelo menos 6 falas por amigo e 10 para o Mike (40 no total).
    for (const char* amigo : {"ben", "beverly", "eddie", "richie", "stan"}) VERIFICAR(porAmigo[amigo] >= 6);
    VERIFICAR(porAmigo["mike"] >= 10);
    // Todo falante tem retrato (retrato_<falante>.png).
    for (std::string f : falantes) {
        for (char& ch : f) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        const bool existe = std::filesystem::exists(std::filesystem::path(ECOS_PASTA_DADOS) / "assets/jogo" / ("retrato_" + f + ".png"));
        if (!existe) std::printf("    sem retrato: %s\n", f.c_str());
        VERIFICAR(existe);
    }
}

TESTE(dialogo_botoes_e_letras) {
    VERIFICAR(trocarBotoes("Use {ATACAR} e {DASH}.", false) == "Use J e Espaço.");
    VERIFICAR(trocarBotoes("Use {ATACAR} e {DASH}.", true) == "Use X e A.");
    VERIFICAR(trocarBotoes("{KIT}{KIT}", false) == "QQ");
    VERIFICAR(trocarBotoes("{CONFIRMAR} ok · {VOLTAR} sair", true) == "A ok · B sair");
    VERIFICAR(trocarBotoes("{CONFIRMAR} ok · {VOLTAR} sair", false) == "Enter ok · Esc sair");
    VERIFICAR(contarCaracteres("ação") == 4 && contarCaracteres("abc") == 3);
    VERIFICAR(idDaDica(1) == "dica_D01" && idDaDica(20) == "dica_D20");
    VERIFICAR(dicaVista(0x5u, 1) && !dicaVista(0x5u, 2) && dicaVista(0x5u, 3));
}

TESTE(dialogo_na_partida_digita_congela_avanca_e_escolhe) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    VERIFICAR(iniciarClube(p, c, 3, Dificuldade::Normal));
    Perfil perfil;
    perfil.opcoes.dicas = false;   // aqui só a conversa
    prepararVisitaAoClube(p, c, perfil, false);
    VERIFICAR(p.falaNova[2] && p.falaNova[2]->id == "mike_clube_boas_vindas");   // '2' = Mike
    irAte(p, '2');
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Dialogo && p.dialogo.atual->id == "mike_clube_boas_vindas");
    VERIFICAR(p.falaNova[2] == nullptr);   // a fala nova da visita foi gasta
    // Digitação: 30 quadros = 0,5 s ≈ 22 letras. O mundo congela (Bill não anda).
    const Vec2 antes = p.bill.pes;
    EstadoEntrada andar;
    andar.eixo = {1.0f, 0.0f};
    for (int q = 0; q < 30; ++q) atualizarPartida(p, c, andar, DT);
    VERIFICAR(p.bill.pes.x == antes.x && p.bill.pes.y == antes.y);
    VERIFICAR(p.dialogo.letras > 20.0f && p.dialogo.letras < 25.0f && p.dialogo.linha == 0);
    apertar(p, c, Comando::Confirmar);   // completa a linha
    VERIFICAR(p.dialogo.linha == 0 && p.dialogo.letras == static_cast<float>(contarCaracteres(p.dialogo.atual->linhas[0].texto)));
    apertar(p, c, Comando::Confirmar);   // avança
    VERIFICAR(p.dialogo.linha == 1 && p.dialogo.letras < 1.0f);
    apertar(p, c, Comando::Voltar);      // pula para o fim: as escolhas
    VERIFICAR(p.dialogo.escolhendo && p.cursor == 0);
    apertar(p, c, Comando::Baixo);
    apertar(p, c, Comando::Confirmar);   // "Pra que serve aquele mural?"
    VERIFICAR(p.pendencia == Pendencia::Dialogo && p.dialogo.atual->id == "mike_clube_mural");
    lerAteOFim(p, c);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma);
    // Lidas: a fala e o ramo escolhido vão para o perfil.
    VERIFICAR(sincronizarPerfil(perfil, p, false));
    VERIFICAR(perfil.falasVistas.count("mike_clube_boas_vindas") && perfil.falasVistas.count("mike_clube_mural"));
    VERIFICAR(!perfil.falasVistas.count("mike_clube_galeria"));
    // Falar de novo nesta visita: balão (a fala nova é uma por visita).
    apertar(p, c, Comando::Interagir);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && p.balao.vida > 0.0f);
    // Próxima visita, já com uma morte: o Mike tem fala nova e ela é a do ciclo.
    perfil.mortes = 1;
    perfil.tentativas = 1;
    EstadoPartida q;
    iniciarClube(q, c, 4, Dificuldade::Normal);
    prepararVisitaAoClube(q, c, perfil, false);
    VERIFICAR(q.falaNova[2] && q.falaNova[2]->id == "mike_clube_ciclo");
    // O piloto: lê e não grava nada.
    q.falasVistasAgora.push_back("mike_clube_ciclo");
    VERIFICAR(!sincronizarPerfil(perfil, q, true) && !perfil.falasVistas.count("mike_clube_ciclo"));
}

TESTE(dicas_aparecem_uma_vez_congelam_o_tempo_e_respeitam_opcoes) {
    const Conteudo c = conteudoReal();
    Perfil perfil;   // dicas ligadas (padrão)
    // Clube, 1ª visita: D01.
    EstadoPartida clube;
    iniciarClube(clube, c, 1, Dificuldade::Normal);
    prepararVisitaAoClube(clube, c, perfil, false);
    esperar(clube, c, 1);
    VERIFICAR(clube.pendencia == Pendencia::Dialogo && clube.dialogo.atual->id == "dica_D01");
    lerAteOFim(clube, c);
    sincronizarPerfil(perfil, clube, false);
    VERIFICAR(dicaVista(perfil.dicasVistas, 1));
    // Tentativa: D03 e D06 na 1ª sala, uma de cada vez; o cronômetro não anda enquanto se lê.
    EstadoPartida p;
    std::string erro;
    iniciarTentativa(p, c, 77, Dificuldade::Normal, erro);
    configurarDicas(p, perfil, false);
    esperar(p, c, 2);
    VERIFICAR(p.pendencia == Pendencia::Dialogo && p.dialogo.atual->id == "dica_D03");
    const float tempo = p.stats.tempo;
    esperar(p, c, 60);
    VERIFICAR(p.stats.tempo == tempo);
    lerAteOFim(p, c);
    esperar(p, c, 1);
    VERIFICAR(p.pendencia == Pendencia::Dialogo && p.dialogo.atual->id == "dica_D06");
    lerAteOFim(p, c);
    esperar(p, c, 30);
    VERIFICAR(p.pendencia != Pendencia::Dialogo || p.dialogo.atual->id != "dica_D03");   // não repete
    sincronizarPerfil(perfil, p, false);
    VERIFICAR(dicaVista(perfil.dicasVistas, 3) && dicaVista(perfil.dicasVistas, 6));
    // Volta ao Clube depois de uma tentativa: D19.
    perfil.tentativas = 1;
    EstadoPartida volta;
    iniciarClube(volta, c, 2, Dificuldade::Normal);
    prepararVisitaAoClube(volta, c, perfil, false);
    esperar(volta, c, 1);
    VERIFICAR(volta.pendencia == Pendencia::Dialogo && volta.dialogo.atual->id == "dica_D19");
    // Dicas desligadas nas Opções, ou o piloto jogando: nenhuma.
    Perfil semDicas;
    semDicas.opcoes.dicas = false;
    EstadoPartida a, b;
    iniciarTentativa(a, c, 77, Dificuldade::Normal, erro);
    configurarDicas(a, semDicas, false);
    iniciarTentativa(b, c, 77, Dificuldade::Normal, erro);
    configurarDicas(b, Perfil{}, true);
    esperar(a, c, 5);
    esperar(b, c, 5);
    VERIFICAR(a.pendencia == Pendencia::Nenhuma && b.pendencia == Pendencia::Nenhuma);
}

TESTE(dialogo_piloto_le_e_fecha_sozinho) {
    const Conteudo c = conteudoReal();
    EstadoPartida p;
    iniciarClube(p, c, 9, Dificuldade::Normal);
    VERIFICAR(abrirDialogo(p, buscarDialogo(c.dialogos, "mike_clube_boas_vindas")));
    MemoriaPiloto m;
    int quadros = 0;
    for (; quadros < 600 && p.pendencia == Pendencia::Dialogo; ++quadros) atualizarPartida(p, c, pilotar(p, m, DT), DT);
    VERIFICAR(p.pendencia == Pendencia::Nenhuma && quadros < 120);
}

// Teclado E joystick (PRD §15.4): nenhum texto de ajuda das cenas pode fixar uma tecla. Eles
// passam por ajuda("{INTERAGIR}: …"), que troca o marcador pelo botão do dispositivo em uso.
// (A página Controles lista as duas colunas de propósito e não usa estes formatos.)
TESTE(ajuda_da_interface_nao_fixa_tecla_de_teclado) {
    const char* proibidos[] = {"L\"E: ", "L\"Enter", "Enter ou J", "L\"WASD/", "L\"Esc voltar", "· Esc ", "L\"J atacar"};
    for (const char* arquivo : {"src/cenas/cena_sala.cpp", "src/cenas/cena_menu.cpp", "src/cenas/cena_historia.cpp", "src/cenas/cena_cartao.cpp"}) {
        std::string texto;
        VERIFICAR(lerArquivoTexto(std::filesystem::path(ECOS_PASTA_DADOS) / arquivo, texto));
        std::istringstream linhas(texto);
        std::string linha;
        int n = 0;
        while (std::getline(linhas, linha)) {
            ++n;
            if (linha.find("ajuda(") != std::string::npos) continue;   // já adaptável
            for (const char* p : proibidos) {
                if (linha.find(p) == std::string::npos) continue;
                std::printf("    %s:%d fixa tecla: %s\n", arquivo, n, p);
                VERIFICAR(false);
            }
        }
    }
}

// Diego, 24/11: as dicas mostram as letras do controle em uso. Os nomes são os que o SDL (no .exe) e o
// navegador (Gamepad.id, com o fabricante: 054c Sony, 057e Nintendo) informam.
TESTE(dialogo_letras_de_xbox_playstation_e_nintendo) {
    VERIFICAR(estiloDoControle("Xbox Series X Controller") == EstiloControle::Xbox);
    VERIFICAR(estiloDoControle("Xbox 360 Controller (XInput STANDARD GAMEPAD)") == EstiloControle::Xbox);
    VERIFICAR(estiloDoControle("PS5 Controller") == EstiloControle::PlayStation);
    VERIFICAR(estiloDoControle("DualSense Wireless Controller (STANDARD GAMEPAD Vendor: 054c Product: 0ce6)") == EstiloControle::PlayStation);
    VERIFICAR(estiloDoControle("Wireless Controller (STANDARD GAMEPAD Vendor: 054c Product: 09cc)") == EstiloControle::PlayStation);
    VERIFICAR(estiloDoControle("054c-05c4-Wireless Controller") == EstiloControle::PlayStation);
    VERIFICAR(estiloDoControle("Nintendo Switch Pro Controller") == EstiloControle::Nintendo);
    VERIFICAR(estiloDoControle("Pro Controller (STANDARD GAMEPAD Vendor: 057e Product: 2009)") == EstiloControle::Nintendo);
    VERIFICAR(estiloDoControle("8BitDo Pro 2") == EstiloControle::Xbox);   // modo X-input: imita o Xbox

    definirEstiloControle(EstiloControle::PlayStation);
    VERIFICAR(trocarBotoes("{CONFIRMAR} ok · {VOLTAR} sair · {ATACAR} · {PAUSA}", true) == "X ok · Bola sair · Quadrado · Options");
    definirEstiloControle(EstiloControle::Nintendo);
    VERIFICAR(trocarBotoes("{CONFIRMAR} ok · {VOLTAR} sair · {ATACAR} · {PAUSA}", true) == "A ok · B sair · Y · +");
    definirEstiloControle(EstiloControle::Xbox);
    VERIFICAR(trocarBotoes("{CONFIRMAR} ok · {VOLTAR} sair · {ATACAR} · {PAUSA}", true) == "A ok · B sair · X · Start");
    VERIFICAR(trocarBotoes("{CONFIRMAR}", false) == "Enter");   // teclado não depende do controle
}
