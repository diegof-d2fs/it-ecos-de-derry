#include "entidades/inimigos.h"

#include <algorithm>
#include <cmath>

#include "mundo/caminho.h"
#include "mundo/colisao.h"
#include "mundo/visao.h"

namespace {

// Tabela de dados (PRD §9). Mesma ordem de TipoInimigo.
const ParametrosInimigo TABELA[TOTAL_TIPOS_INIMIGO] = {
    //  nome                atlas     hp  dano arm agi  vel    xp  det  custo voa    corpo
    {"Balão perverso",    "balao",  20, 10,  0,  1,  70.0f,  6, 6.0f, 2,  true,  26.0f},
    {"Rato sombrio",      "rato",   26,  6,  0,  3, 140.0f,  8, 5.0f, 2,  false, 16.0f},
    {"Sombra encharcada", "sombra", 60, 12,  2,  1,  80.0f, 15, 7.0f, 4,  false, 26.0f},
    // Etapa 13.2: os números do PRD §9 (o protótipo da 6b era 34/9/2/75/12/7).
    {"Espectro de fuligem","espectro_fuligem", 40, 11, 0, 3, 100.0f, 14, 8.0f, 3,  true,  28.0f},
    // Bioma 2 (Etapa 10.2, PRD §9). A sanguessuga é frágil e voa; o afogado é o "forte" do Barrens.
    {"Sanguessuga alada", "sanguessuga", 18,  5,  0,  4, 110.0f,  7, 7.0f, 2,  true,  20.0f},
    {"Afogado",           "afogado",     70, 10,  1,  1,  60.0f, 16, 6.0f, 4,  false, 28.0f},
    // Bioma 3 (Etapa 11.3, PRD §9): o primeiro inimigo de DISTÂNCIA.
    {"Boneco palhaço",    "boneco_palhaco", 34, 7, 0, 2,  90.0f, 12, 8.0f, 3,  false, 28.0f},
    // Bioma 4 (Etapa 12.2, PRD §9): torreta presa a um ralo — CONTROLE DE ÁREA (velocidade 0).
    {"Cabelo do ralo",    "cabelo_ralo",    45, 9, 1, 0,   0.0f, 12, 4.0f, 3,  false, 20.0f},
    // Chefe 1 (PRD §10.1, rebaseado na 8.5): HP 650 (as duas fases), dano 9, armadura 2, esquiva 0%,
    // XP 60. Com 300/1 a telemetria mostrou 18 golpes e 26 s de luta: nenhuma luta de 2 fases cabe aí.
    {"A Múmia",           "medo_mumia", 650, 9, 2,  0,  45.0f, 60, 30.0f, 0, false, 54.0f, true},
    // Chefe 2 (PRD §10.1, escalado a partir da Múmia MEDINDO; Etapa 10.3): Bill chega mais forte
    // (nível ~5, mais Força e melhorias), e os mergulhos tiram tempo de ataque.
    {"O Afogado",         "medo_afogado", 900, 10, 2, 0,  50.0f, 80, 30.0f, 0, false, 54.0f, true},
    // Chefe 3 (PRD §10.1, escalado a partir do Afogado MEDINDO; Etapa 11.4). Esquiva ~5% (AGI 1).
    {"O Leproso",         "medo_leproso", 850, 12, 3, 1,  55.0f, 100, 30.0f, 0, false, 54.0f, true},
    {"O Ralo de Sangue",  "medo_ralo",    1200, 13, 3, 1,  55.0f, 120, 30.0f, 0, false, 50.0f, true},
    // Chefe 5 (Etapa 13.3, PRD §10.1): VOA (imune a terreno: sobrevoa o metal e as grelhas). A vida do
    // PRD (720) é o ponto de partida; o valor vem da medição, como nos chefes 1–4.
    {"O Pássaro das Cinzas", "medo_passaro", 1200, 14, 3, 1, 60.0f, 140, 30.0f, 0, true, 54.0f, true},
    // Chefe 6 (Etapa 14.2, PRD §10.1): o PRD dá 860/16/3/8%; a vida vem da medição, como nos chefes 1–5
    // (o Pássaro precisou de 1200). AGI 2 ≈ 8% de esquiva.
    {"O Lobisomem",       "medo_lobisomem", 2400, 16, 3, 2, 70.0f, 160, 30.0f, 0, false, 56.0f, true},
    // O Pennywise IV (Etapa 15, PRD §4.5 e §10.1): o encontro definitivo, 2×2 células (folha 64×64), três fases
    // (Palhaço 100–60%, Georgie 60–25%, Luzes Mortas 25–0%). O PRD dá 1300/18/3/10%; a vida vem da medição.
    {"Pennywise IV",      "pennywise_4_actions", 3200, 18, 3, 3, 80.0f, 200, 30.0f, 0, false, 58.0f, true},
};

// Padrões da Múmia (fase 1) e do Pennywise I (fase 2), PRD §10.1. Todos anunciados no chão.
// REGRA: a distância máxima de escolha nunca passa do alcance do golpe (comprimento + 4 px), e
// depois de cada ataque vem a janela de punição (recuperação ≥ 0,9 s) e um respiro (recarga).
// Fora disso o chefe ANDA até Bill: um chefe que ataca de onde não alcança vira um chefe que
// nunca chega perto (e uma luta que não termina), como mostrou o diagnóstico da 8.3.
// Etapa 8.5 (telemetria: luta de 30 s e 6 de dano em 46): o PRD §10.2 pede mais LEITURA E
// DESLOCAMENTO antes de mais HP. Entraram o anel em volta do chefe (ficar colado batendo deixa de
// ser seguro), os balões vermelhos do Pennywise (explodem e ferem TODOS, até ele) e o combo
// Aproximação → Mão estendida (a janela de punição vem só depois do 2º golpe).
const PadraoChefe PADROES_MUMIA[] = {
    // nome                    forma                 fase dist. mín/máx   preparo rec.  comp.  larg. prende inv. peso recarga invocado            seguinte
    {"Faixa",                  FormaPadrao::Faixa,    1,  0.0f, 100.0f,   0.75f, 1.0f,  96.0f, 22.0f, 1.0f, 0,  4,  0.6f},
    {"Leque de bandagens",     FormaPadrao::Leque,    1,  0.0f, 100.0f,   0.85f, 1.1f,  96.0f, 16.0f, 0.0f, 0,  3,  0.6f},
    {"Ratos da galeria",       FormaPadrao::Invocar,  1, 64.0f, 9999.0f,  0.9f,  0.7f,   0.0f,  0.0f, 0.0f, 2,  2,  4.0f, TipoInimigo::Rato},
    {"Redemoinho de bandagens",FormaPadrao::Circulo,  1,  0.0f, 60.0f,    0.7f,  0.9f,  64.0f,  0.0f, 0.0f, 0,  3,  0.6f},
    {"Mão estendida",          FormaPadrao::Linha,    2,  0.0f, 68.0f,    0.5f,  0.7f,  64.0f, 20.0f, 0.0f, 0,  4,  0.4f},
    {"Aproximação",            FormaPadrao::Investida,2, 60.0f, 100.0f,   0.6f,  0.6f,  96.0f, 24.0f, 0.0f, 0,  3,  0.4f, TipoInimigo::Rato, 4},
    {"Gargalhada",             FormaPadrao::Circulo,  2,  0.0f, 50.0f,    0.6f,  0.7f,  60.0f,  0.0f, 0.0f, 0,  3,  0.4f},
    {"Balões vermelhos",       FormaPadrao::Invocar,  2, 80.0f, 9999.0f,  0.8f,  0.6f,   0.0f,  0.0f, 0.0f, 2,  2,  5.0f, TipoInimigo::Balao},
};
// O Afogado da Caixa d'Água (fase 1) e o Pennywise I+ (fase 2), PRD §10.1. No Mergulho, o preparo é
// o afundar; o círculo de AVISO_EMERGIR vem depois, com Bill registrado. Nas Ondas, o 1º anel vem
// com o preparo e o 2º (mais largo) 0,5 s depois: o seguro é colado nele ou longe.
const PadraoChefe PADROES_AFOGADO[] = {
    // nome                      forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado                 seguinte
    {"Mergulho",                 FormaPadrao::Mergulho,  1,  0.0f, 9999.0f,   0.6f,  1.0f, 0.0f,   0.0f, 0.0f, 0,  3,  0.8f},
    {"Ondas em anel",            FormaPadrao::Ondas,     1,  0.0f, 148.0f,    0.8f,  1.0f, 144.0f, 46.0f, 0.0f, 0,  3,  0.8f},
    {"Respingo",                 FormaPadrao::Circulo,   1,  0.0f, 52.0f,     0.6f,  0.8f, 56.0f,  0.0f, 0.0f, 0,  2,  0.6f},
    {"Sanguessugas da caixa",    FormaPadrao::Invocar,   1, 64.0f, 9999.0f,   0.9f,  0.7f, 0.0f,   0.0f, 0.0f, 2,  1,  5.0f, TipoInimigo::Sanguessuga},
    {"Linha de três",            FormaPadrao::Linha,     2,  0.0f, 100.0f,    0.5f,  0.7f, 96.0f,  20.0f, 0.0f, 0,  4,  0.4f},
    {"Ondas",                    FormaPadrao::Ondas,     2,  0.0f, 148.0f,    0.7f,  0.8f, 144.0f, 46.0f, 0.0f, 0,  3,  0.4f},
    {"Aproximação",              FormaPadrao::Investida, 2, 60.0f, 100.0f,    0.6f,  0.6f, 96.0f,  24.0f, 0.0f, 0,  3,  0.4f, TipoInimigo::Rato, 4},
    {"Gargalhada",               FormaPadrao::Circulo,   2,  0.0f, 50.0f,     0.6f,  0.7f, 60.0f,  0.0f, 0.0f, 0,  2,  0.4f},
};
// O buraco das ondas tem de caber o CORPO de Bill (a caixa de dano tem ~40 px de altura): com 30 px,
// "colado nele é seguro" era mentira e o piloto apanhava das ondas (diagnóstico da 10.3).
constexpr float RAIO_SEGURO_ONDAS = 52.0f;
// O Leproso (fase 1) e o Pennywise II (fase 2), PRD §10.1: investida LONGA anunciada, nuvem pútrida
// (lento por 5 s), golpe podre contra quem cola; na fase 2 ele some e volta por uma porta da sala.
// Combos (11.5, telemetria: 89 s e só 8 de dano): investida → golpe podre; porta → gargalhada.
const PadraoChefe PADROES_LEPROSO[] = {
    // nome                   forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado               seguinte
    {"Investida longa",       FormaPadrao::Investida, 1, 70.0f, 196.0f,   0.8f,  1.1f, 192.0f, 26.0f, 0.0f, 0,  4,  0.8f, TipoInimigo::Rato, 2},   // → golpe podre
    {"Nuvem pútrida",         FormaPadrao::Nuvem,     1,  0.0f, 9999.0f,  0.9f,  0.8f,  52.0f,  0.0f, 0.0f, 0,  3,  1.2f},
    {"Golpe podre",           FormaPadrao::Circulo,   1,  0.0f, 52.0f,    0.6f,  0.8f,  56.0f,  0.0f, 0.0f, 0,  3,  0.6f},
    {"Bonecos do sótão",      FormaPadrao::Invocar,   1, 64.0f, 9999.0f,  0.9f,  0.7f,   0.0f,  0.0f, 0.0f, 2,  1,  6.0f, TipoInimigo::Boneco},
    {"Pela porta",            FormaPadrao::Porta,     2,  0.0f, 9999.0f,  0.5f,  1.2f, 176.0f, 26.0f, 0.0f, 0,  3,  0.8f, TipoInimigo::Rato, 7},   // → gargalhada
    {"Linha de três",         FormaPadrao::Linha,     2,  0.0f, 100.0f,   0.5f,  0.7f,  96.0f, 20.0f, 0.0f, 0,  3,  0.4f},
    {"Nuvem",                 FormaPadrao::Nuvem,     2,  0.0f, 9999.0f,  0.7f,  0.6f,  48.0f,  0.0f, 0.0f, 0,  1,  1.5f},
    {"Gargalhada",            FormaPadrao::Circulo,   2,  0.0f, 50.0f,    0.6f,  0.7f,  60.0f,  0.0f, 0.0f, 0,  2,  0.4f},
};
// O Ralo de Sangue (Etapa 12.3, PRD §10.1): fixo no ralo, no centro. A arena é dele: a cruz e o xis
// alternam (o lugar seguro muda), e a água sobe em quadrantes opostos alternados. A ≤ 50% o Pennywise
// II+ sai do ralo e persegue — e o ralo continua transbordando.
const PadraoChefe PADROES_RALO[] = {
    // nome                   forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado               seguinte
    {"Cabelos em cruz",       FormaPadrao::Cruz,      1,  0.0f, 228.0f,   0.9f,  1.0f, 224.0f, 22.0f, 0.0f, 0,  4,  0.6f},
    {"Cabelos em xis",        FormaPadrao::Xis,       1,  0.0f, 228.0f,   0.9f,  1.0f, 224.0f, 22.0f, 0.0f, 0,  3,  0.6f},
    {"O ralo transborda",     FormaPadrao::Inundar,   1,  0.0f, 9999.0f,  1.2f,  1.0f, 300.0f,  0.0f, 0.0f, 0,  3,  1.0f},
    {"Golfada",               FormaPadrao::Golfada,   1, 64.0f, 9999.0f,  0.9f,  0.7f,  40.0f,  0.0f, 0.0f, 0,  2,  0.8f},
    {"Cabelos do ralo",       FormaPadrao::Invocar,   1, 96.0f, 9999.0f,  0.9f,  0.8f,   0.0f,  0.0f, 0.0f, 2,  1,  6.0f, TipoInimigo::Cabelo},
    {"Açoite",                FormaPadrao::Circulo,   1,  0.0f, 60.0f,    0.6f,  0.8f,  64.0f,  0.0f, 0.0f, 0,  3,  0.4f},
    {"Linha de três",         FormaPadrao::Linha,     2,  0.0f, 100.0f,   0.5f,  0.7f,  96.0f, 20.0f, 0.0f, 0,  3,  0.4f},
    {"Bote",                  FormaPadrao::Investida, 2, 70.0f, 196.0f,   0.7f,  1.0f, 192.0f, 26.0f, 0.0f, 0,  3,  0.8f, TipoInimigo::Rato, 8},   // → gargalhada
    {"Gargalhada",            FormaPadrao::Circulo,   2,  0.0f, 50.0f,    0.6f,  0.7f,  60.0f,  0.0f, 0.0f, 0,  2,  0.4f},
    // Fase 2 PERSEGUE: de longe só a inundação vale (e não repete), então ele anda. Com a golfada a
    // qualquer distância, as duas se alternavam e o Pennywise nunca saía do ralo (o "Ondas" da 10.3).
    {"Golfada",               FormaPadrao::Golfada,   2, 64.0f, 160.0f,   0.7f,  0.6f,  40.0f,  0.0f, 0.0f, 0,  2,  1.0f},
    // Na fase 2 o ralo transborda só em volta dele (5 células): o centro fica perigoso e a perseguição
    // vai para as bordas secas. Com a arena inteira (300 px), a luta toda era dentro da poça (12.3).
    {"O ralo transborda",     FormaPadrao::Inundar,   2,  0.0f, 9999.0f,  1.2f,  0.8f, 160.0f,  0.0f, 0.0f, 0,  2,  1.0f},
};
// O Pássaro das Cinzas (fase 1) e o Pennywise III (fase 2), Etapa 13.3, PRD §10.1. Ele VOA: o
// mergulho cruza a arena numa linha longa anunciada (por cima do metal e das grelhas), e a chuva de
// brasas cai em círculos, um depois do outro, que deixam FOGO no piso por 4 s — a arena encolhe e
// o lugar seguro muda. Colado nele, o bater de asas. Fase 2: linha de 3 e, depois do rasante, a cruz.
const PadraoChefe PADROES_PASSARO[] = {
    // nome                   forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado               seguinte
    {"Mergulho",              FormaPadrao::Investida, 1, 70.0f, 228.0f,   0.8f,  1.1f, 224.0f, 28.0f, 0.0f, 0,  4,  0.8f},
    {"Chuva de brasas",       FormaPadrao::Chuva,     1,  0.0f, 9999.0f,  0.9f,  0.8f,  72.0f, 26.0f, 0.0f, 5,  3,  1.2f},
    {"Bater de asas",         FormaPadrao::Circulo,   1,  0.0f, 60.0f,    0.6f,  0.9f,  64.0f,  0.0f, 0.0f, 0,  3,  0.6f},
    {"Linha de três",         FormaPadrao::Linha,     2,  0.0f, 100.0f,   0.5f,  0.7f,  96.0f, 20.0f, 0.0f, 0,  3,  0.4f},
    {"Rasante",               FormaPadrao::Investida, 2, 70.0f, 196.0f,   0.7f,  0.6f, 192.0f, 26.0f, 0.0f, 0,  3,  0.6f, TipoInimigo::Rato, 5},   // → cruz
    {"Cruz de fogo",          FormaPadrao::Cruz,      2,  0.0f, 132.0f,   0.6f,  1.0f, 128.0f, 22.0f, 0.0f, 0,  2,  0.6f},
    {"Gargalhada",            FormaPadrao::Circulo,   2,  0.0f, 50.0f,    0.6f,  0.7f,  60.0f,  0.0f, 0.0f, 0,  2,  0.4f},
    {"Brasas",                FormaPadrao::Chuva,     2,  0.0f, 9999.0f,  0.8f,  0.6f,  56.0f, 24.0f, 0.0f, 3,  1,  1.5f},
};
// O Lobisomem (fase 1) e o Pennywise III+ (fase 2), Etapa 14.2, PRD §10.1. Colado nele, o COMBO DE TRÊS
// GARRAS com aviso curto: cada garra re-mira em Bill (dá para sair entre uma e outra; parado, apanha as
// três), e a última é um giro em volta dele. De meia distância, o SALTO: agacha, some no ar e cai onde
// Bill estava (o círculo avisa). De longe, o UIVO chama dois ratos. Fase 2: linha de 3, salto → gargalhada.
// Peso 0 = só aparece como continuação de um combo (nunca sorteado sozinho).
const PadraoChefe PADROES_LOBISOMEM[] = {
    // nome                   forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado               seguinte
    {"Garras",                FormaPadrao::Leque,     1,  0.0f, 72.0f,    0.45f, 0.25f, 68.0f, 16.0f, 0.0f, 0,  4,  0.6f, TipoInimigo::Rato, 1},   // → 2ª garra
    {"Segunda garra",         FormaPadrao::Leque,     1,  0.0f, 9999.0f,  0.35f, 0.25f, 68.0f, 16.0f, 0.0f, 0,  0,  0.6f, TipoInimigo::Rato, 2},   // → giro
    {"Garra final",           FormaPadrao::Circulo,   1,  0.0f, 9999.0f,  0.45f, 1.0f,  64.0f,  0.0f, 0.0f, 0,  0,  0.6f},
    {"Salto",                 FormaPadrao::Salto,     1, 80.0f, 230.0f,   0.6f,  1.0f,   0.0f, 48.0f, 0.0f, 0,  3,  0.8f},
    {"Uivo",                  FormaPadrao::Invocar,   1, 64.0f, 9999.0f,  1.0f,  0.7f,   0.0f,  0.0f, 0.0f, 2,  2,  6.0f, TipoInimigo::Rato},
    {"Linha de três",         FormaPadrao::Linha,     2,  0.0f, 100.0f,   0.5f,  0.7f,  96.0f, 20.0f, 0.0f, 0,  3,  0.4f},
    {"Salto",                 FormaPadrao::Salto,     2, 80.0f, 230.0f,   0.5f,  0.4f,   0.0f, 44.0f, 0.0f, 0,  3,  0.6f, TipoInimigo::Rato, 7},   // → gargalhada
    {"Gargalhada",            FormaPadrao::Circulo,   2,  0.0f, 50.0f,    0.6f,  0.7f,  60.0f,  0.0f, 0.0f, 0,  2,  0.4f},
    {"Uivo",                  FormaPadrao::Invocar,   2, 64.0f, 9999.0f,  0.8f,  0.6f,   0.0f,  0.0f, 0.0f, 2,  1,  7.0f, TipoInimigo::Rato},
};
// O Pennywise IV (Etapa 15, PRD §4.5; refeito em 24/11 como o PALHAÇO-ARANHA, pedido do Diego: "corpo de Pennywise e
// [o resto] com pernas de aranha, algo grande que se move 360 graus e tem poderes"). As três viradas continuam: fase 1,
// o Palhaço-aranha; fase 2, Georgie (a sala escurece e a ilusão chama Bill: atualizarCovil); fase 3, Luzes Mortas (só
// fora dos seis círculos de luz Bill se fere; o IV só é atingido na recuperação: podeSerAtingido, por isso todos os
// golpes da fase 3 terminam PERTO de Bill e com janela longa). Os poderes de aranha:
//   * Investida das patas → Patada: corre pela linha anunciada e, no fim, crava as patas da frente em leque;
//   * Teia: cospe bolas de teia em círculos (a 1ª onde Bill está); quem é pego fica PRESO (sem andar nem dash);
//   * Salto de aranha: agacha, some no ar e cai onde Bill estava (o círculo avisa);
//   * Balões e Gargalhada (do palhaço), Pulso no escuro (fase 2), Luzes Mortas em cruz (fase 3).
const PadraoChefe PADROES_PENNYWISE_IV[] = {
    // nome                   forma                  fase dist. mín/máx   preparo rec.  comp.   larg. prende inv. peso recarga invocado               seguinte
    {"Investida das patas",   FormaPadrao::Investida, 1, 60.0f, 196.0f,   0.7f,  0.6f, 192.0f, 34.0f, 0.0f, 0,  3,  0.6f, TipoInimigo::Rato, 1},   // → patada
    {"Patada",                FormaPadrao::Leque,     1,  0.0f, 9999.0f,  0.45f, 0.8f,  80.0f, 18.0f, 0.0f, 0,  0,  0.5f},
    {"Teia",                  FormaPadrao::Chuva,     1,  0.0f, 9999.0f,  0.9f,  0.7f,  72.0f, 22.0f, 0.5f, 3,  3,  1.0f},
    {"Salto de aranha",       FormaPadrao::Salto,     1, 90.0f, 260.0f,   0.7f,  1.0f,   0.0f, 50.0f, 0.0f, 0,  3,  0.8f},
    {"Balões",                FormaPadrao::Invocar,   1, 80.0f, 9999.0f,  0.8f,  0.6f,   0.0f,  0.0f, 0.0f, 2,  2,  6.0f, TipoInimigo::Balao},
    {"Gargalhada",            FormaPadrao::Circulo,   1,  0.0f, 64.0f,    0.6f,  0.8f,  70.0f,  0.0f, 0.0f, 0,  2,  0.5f},
    {"Pulso",                 FormaPadrao::Ondas,     2,  0.0f, 170.0f,   0.8f,  0.9f, 160.0f, 46.0f, 0.0f, 0,  3,  0.8f},
    {"Teia",                  FormaPadrao::Chuva,     2,  0.0f, 9999.0f,  0.8f,  0.6f,  80.0f, 22.0f, 0.5f, 3,  3,  1.0f},
    {"Salto de aranha",       FormaPadrao::Salto,     2, 90.0f, 260.0f,   0.6f,  0.4f,   0.0f, 50.0f, 0.0f, 0,  3,  0.7f, TipoInimigo::Rato, 9},   // → patada
    {"Patada",                FormaPadrao::Leque,     2,  0.0f, 9999.0f,  0.4f,  0.8f,  80.0f, 18.0f, 0.0f, 0,  0,  0.5f},
    {"Balões",                FormaPadrao::Invocar,   2, 80.0f, 9999.0f,  0.8f,  0.6f,   0.0f,  0.0f, 0.0f, 2,  1,  8.0f, TipoInimigo::Balao},
    {"Bote no escuro",        FormaPadrao::Investida, 3, 50.0f, 230.0f,   0.85f, 1.3f, 224.0f, 34.0f, 0.0f, 0,  3,  0.5f},
    {"Patas sombrias",        FormaPadrao::Circulo,   3,  0.0f, 72.0f,    0.6f,  1.3f,  72.0f,  0.0f, 0.0f, 0,  3,  0.5f},
    {"Salto de aranha",       FormaPadrao::Salto,     3, 90.0f, 260.0f,   0.7f,  1.3f,   0.0f, 50.0f, 0.0f, 0,  2,  0.6f},
    {"Luzes Mortas",          FormaPadrao::CruzAlvo,  3,  0.0f, 9999.0f,  0.8f,  1.0f,  64.0f, 26.0f, 0.0f, 0,  1,  1.0f},
};
constexpr float VELOCIDADE_FASE_2 = 2.4f;   // o Pennywise é menor e bem mais rápido que o medo
constexpr int LIMITE_HOSTIS_INVOCAR = 4;

constexpr float TEMPO_BUSCA = 3.0f;        // perde Bill de vista: procura por 3 s e volta a vagar
constexpr float INTERVALO_CAMINHO = 0.25f;

Vec2 corpo(const Inimigo& i) { return {i.pes.x, i.pes.y - parametrosDe(i.tipo).alturaCorpo / 2}; }

std::uint32_t novoGolpe(ContextoIA& ctx) { return ctx.proximoGolpe++; }

// ---------- movimento comum ----------
// O corpo gira para `para` a no máximo VELOCIDADE_GIRO rad/s (sem estalar de um rumo para outro).
constexpr float VELOCIDADE_GIRO = 9.0f;
void girarRumo(Inimigo& ini, Vec2 para, float dt) {
    if (comprimento(para) < 0.001f) return;
    const float alvo = std::atan2(para.y, para.x);
    float delta = std::remainder(alvo - ini.rumo, 6.2831853f);
    const float passo = VELOCIDADE_GIRO * dt;
    delta = std::clamp(delta, -passo, passo);
    ini.rumo = std::remainder(ini.rumo + delta, 6.2831853f);
}

// A caixa de colisão inteira passa em linha reta de `de` até `para` (os quatro cantos dos pés)?
bool retaLivre(const SalaViva& sala, Vec2 de, Vec2 para, bool voa) {
    const float w = CAIXA_INIMIGO.x / 2.0f + 1.0f, h = CAIXA_INIMIGO.y;
    const Vec2 cantos[4] = {{-w, 0.0f}, {w, 0.0f}, {-w, -h}, {w, -h}};
    const Vec2 d = subtrair(para, de);
    const int passos = std::max(1, static_cast<int>(comprimento(d) / 6.0f));
    for (const Vec2& k : cantos) {
        for (int i = 0; i <= passos; ++i) {
            const Vec2 q = somar(somar(de, k), escalar(d, static_cast<float>(i) / static_cast<float>(passos)));
            const Celula c = celulaDe(q);
            const Tile& tile = tileEm(sala, c.col, c.lin);
            if (voa ? tile.bloqueiaVoo : tile.bloqueiaMovimento) return false;
        }
    }
    return true;
}

void andar(Inimigo& ini, ContextoIA& ctx, Vec2 direcao, float velocidade, float dt) {
    if (comprimento(direcao) < 0.001f) return;
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    const Celula c = celulaDe(ini.pes);
    const float fator = (p.voa ? 1.0f : fatorVelocidade(tileEm(ctx.sala, c.col, c.lin).tipo)) * (ini.lento > 0.0f ? 0.5f : 1.0f) *
                        (ini.velocidadePct == 100 ? 1.0f : static_cast<float>(ini.velocidadePct) / 100.0f);   // 100% exato: o Normal não muda
    // Em cima da esteira, o terrestre faz força contra ela (13.2): sem isso, uma Sombra na ponta da
    // esteira ficava empurrada contra a quina da cuba para sempre.
    const Vec2 d = p.voa ? normalizar(direcao) : contraEsteira(tileEm(ctx.sala, c.col, c.lin), direcao, velocidade * fator);
    ini.pes = moverComColisao(ctx.sala, ini.pes, CAIXA_INIMIGO, escalar(d, velocidade * fator * dt), p.voa);
    ini.direcao = direcaoComHisterese(d, ini.direcao, 1.35f);   // perseguindo na diagonal, sem piscar (24/11)
    girarRumo(ini, d, dt);
    ini.andando = true;
}

// Vai até `destino` (pixels) contornando paredes: BFS célula a célula; na mesma célula, direto.
// Devolve false se NÃO há caminho (destino fechado, como o nicho atrás de uma parede rachada).
bool irPara(Inimigo& ini, ContextoIA& ctx, Vec2 destino, float velocidade, float dt) {
    const Celula aqui = celulaDe(ini.pes), la = celulaDe(destino);
    if (aqui.col == la.col && aqui.lin == la.lin) {
        andar(ini, ctx, subtrair(destino, ini.pes), velocidade, dt);
        return true;
    }
    // O Pennywise (fase 2 e o IV) (24/11, Diego: "movimentação fluida, incluindo diagonais, análogo ao Bill"): com a reta livre, vai
    // direto, em qualquer ângulo; a escada célula a célula do BFS (4 vizinhos) só para contornar o que bloqueia.
    // O RITMO não muda: na diagonal ele anda mais devagar na mesma proporção em que a reta é mais curta que a escada
    // (chega no mesmo tempo de antes). Andar em reta na velocidade cheia deixou b5/b6_chefe em 9/10 pelo piloto.
    if (parametrosDe(ini.tipo).chefe && (ini.fase >= 2 || ini.tipo == TipoInimigo::PennywiseIV) && retaLivre(ctx.sala, ini.pes, destino, parametrosDe(ini.tipo).voa)) {
        ini.proximo = {-1, -1};
        const Vec2 d = subtrair(destino, ini.pes);
        const float escada = std::fabs(d.x) + std::fabs(d.y);
        const float ritmo = escada > 0.001f ? comprimento(d) / escada : 1.0f;
        andar(ini, ctx, d, velocidade * ritmo, dt);
        return true;
    }
    ini.recalcularCaminho -= dt;
    const bool chegouNoPasso = ini.proximo.col >= 0 && comprimento(subtrair(centroCelula(ini.proximo), ini.pes)) < 4.0f;
    if (ini.recalcularCaminho <= 0.0f || chegouNoPasso || ini.proximo.col < 0) {
        ini.proximo = proximoPasso(ctx.sala, aqui, la, parametrosDe(ini.tipo).voa);
        ini.recalcularCaminho = INTERVALO_CAMINHO;
    }
    if (ini.proximo.col < 0) return false;   // sem caminho: fica parado (nunca atravessa parede)
    andar(ini, ctx, subtrair(centroCelula(ini.proximo), ini.pes), velocidade, dt);
    return true;
}

void afastar(Inimigo& ini, ContextoIA& ctx, float velocidade, float dt) {
    andar(ini, ctx, subtrair(ini.pes, ctx.alvo), velocidade, dt);
}

void vagar(Inimigo& ini, ContextoIA& ctx, float dt) {
    if (ini.timer > 0.0f) { ini.timer -= dt; return; }   // pausa entre passeios
    const bool semDestino = ini.destinoVagar.col < 0;
    const bool chegou = !semDestino && comprimento(subtrair(centroCelula(ini.destinoVagar), ini.pes)) < 6.0f;
    if (semDestino || chegou) {
        if (chegou) {
            ini.destinoVagar = {-1, -1};
            ini.timer = 0.5f + real01(ctx.rng);
            return;
        }
        // Célula livre aleatória a até 4 células (até 8 tentativas; determinístico pela seed).
        const Celula c = celulaDe(ini.pes);
        for (int t = 0; t < 8; ++t) {
            const Celula alvo{c.col + intervalo(ctx.rng, -4, 4), c.lin + intervalo(ctx.rng, -4, 4)};
            const Tile& tile = tileEm(ctx.sala, alvo.col, alvo.lin);
            if (parametrosDe(ini.tipo).voa ? !tile.bloqueiaVoo : !tile.bloqueiaMovimento) {
                ini.destinoVagar = alvo;
                break;
            }
        }
        if (ini.destinoVagar.col < 0) { ini.timer = 0.5f; return; }
    }
    // Destino sem caminho (célula livre mas fechada, como o nicho secreto): sorteia outro. Antes
    // o inimigo ficava parado ali para sempre, longe da luz, e a sala nunca concluía (Etapa 8.5).
    if (!irPara(ini, ctx, centroCelula(ini.destinoVagar), parametrosDe(ini.tipo).velocidade * 0.5f, dt)) {
        ini.destinoVagar = {-1, -1};
        ini.timer = 0.3f;
    }
}

// ---------- ataques ----------
Golpe golpeBase(const Inimigo& ini, ContextoIA& ctx, const Area& area, int dano) {
    Golpe g;
    g.id = novoGolpe(ctx);
    g.equipe = Equipe::Hostil;
    g.origem = ini.id;
    g.area = area;
    g.dano = dano;
    g.focoAtacante = 1;
    g.padraoChefe = ini.padrao;
    return g;
}

// ---------- comportamentos (um por tipo) ----------
using Comportamento = void (*)(Inimigo&, ContextoIA&, float dt, float distancia);

// Balão: flutua até perto, infla anunciando um círculo e estoura (fere Bill E inimigos).
void decidirBalao(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    if (ini.estado == EstadoIA::Perseguindo) {
        if (distancia < 36.0f) {
            ini.estado = EstadoIA::Preparando;
            Aviso a;
            a.tempoTotal = a.tempoRestante = 0.8f;
            a.golpe = golpeBase(ini, ctx, areaCirculo(corpo(ini), 48.0f), p.dano);
            a.golpe.equipe = Equipe::Todos;
            a.golpe.rolaAcerto = false;
            a.golpe.recuo = 60.0f;
            a.consomeOrigem = true;
            ctx.avisos.push_back(a);
        } else {
            irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
        }
    }
    // Preparando: parado, inflando. Quem dispara (e estoura o balão) é o aviso.
}

// Rato: rápido e em zigue-zague; morde e recua; com pouca vida foge por 2 s (uma vez).
void decidirRato(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    if (!ini.jaFugiu && ini.hp < p.hp * 3 / 10 && ini.estado != EstadoIA::Fugindo) {   // 30% da vida da TABELA (como sempre foi)
        ini.jaFugiu = true;
        ini.estado = EstadoIA::Fugindo;
        ini.timer = 2.0f;
    }
    switch (ini.estado) {
        case EstadoIA::Perseguindo:
            if (distancia < 28.0f && ini.recarga <= 0.0f) {
                ini.estado = EstadoIA::Atacando;
                ini.timer = 0.18f;   // dano baixo (< 8): bote curto, sem aviso no chão (PRD §8.3)
                ini.alvoRegistrado = ctx.alvo;
            } else {
                // Zigue-zague: desvio lateral que oscila com o tempo.
                const Vec2 d = normalizar(subtrair(ctx.alvo, ini.pes));
                const Vec2 lado{-d.y, d.x};
                const float zig = std::sin(ini.animTempo * 7.0f) * 0.6f;
                if (distancia > 60.0f) andar(ini, ctx, somar(d, escalar(lado, zig)), p.velocidade, dt);
                else irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
            }
            break;
        case EstadoIA::Atacando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                const Vec2 dir = subtrair(ini.alvoRegistrado, ini.pes);
                Golpe g = golpeBase(ini, ctx, areaArco(corpo(ini), dir, 30.0f, 100.0f), p.dano);
                g.duracao = 0.08f;
                g.recuo = 30.0f;
                ctx.golpes.push_back(g);
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.35f;
                ini.recarga = 0.9f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            afastar(ini, ctx, p.velocidade * 0.8f, dt);
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        case EstadoIA::Fugindo:
            if (ini.itemCarregado >= 0) {   // ladrão: foge com o item para o canto mais longe, sem parar
                irPara(ini, ctx, ini.destinoFuga, p.velocidade * 1.05f, dt);
                break;
            }
            ini.timer -= dt;
            afastar(ini, ctx, p.velocidade, dt);
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Sombra: lenta e resistente; golpe largo anunciado (0,6 s) no lugar registrado.
void decidirSombra(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    switch (ini.estado) {
        case EstadoIA::Perseguindo:
            if (distancia < 42.0f && ini.recarga <= 0.0f) {
                ini.estado = EstadoIA::Preparando;
                ini.timer = 0.6f;
                ini.alvoRegistrado = ctx.alvo;
                ini.direcao = direcaoDominante(subtrair(ctx.alvo, ini.pes), ini.direcao);
                Aviso a;
                a.tempoTotal = a.tempoRestante = 0.6f;
                a.golpe = golpeBase(ini, ctx, areaArco(corpo(ini), subtrair(ctx.alvo, ini.pes), 50.0f, 120.0f), p.dano);
                a.golpe.rolaAcerto = false;
                a.golpe.recuo = 70.0f;
                ctx.avisos.push_back(a);
            } else {
                irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
            }
            break;
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Atacando;
                ini.timer = 0.15f;
            }
            break;
        case EstadoIA::Atacando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.6f;   // janela de punição: parada e vulnerável
                ini.recarga = 1.0f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Espectro de fuligem: flutua devagar; sopra uma faixa de fuligem anunciada (0,6 s). O rastro de
// brasas que queima itens no chão é da partida (ela conhece os itens): SDD §24.5.
// Espectro (PRD §9): longe de Bill, vira FUMAÇA (0,5 s; a fumaça aparece também no destino, a 2
// células de Bill) e reaparece lá — o único que se teleporta, sempre anunciado. Perto, a linha.
bool espectroTeleporta(Inimigo& ini, ContextoIA& ctx, float distancia) {
    if (ini.recargaTeleporte > 0.0f || distancia < 4.0f * TILE) return false;
    const Vec2 deBill = normalizar(subtrair(ini.pes, ctx.alvo));
    for (int k = 0; k < 8; ++k) {   // o lado de onde ele vem; senão, gira 45° até achar onde caber
        const float a = 0.785f * static_cast<float>((k + 1) / 2) * (k % 2 == 0 ? 1.0f : -1.0f);
        const Vec2 d{deBill.x * std::cos(a) - deBill.y * std::sin(a), deBill.x * std::sin(a) + deBill.y * std::cos(a)};
        const Vec2 alvo = somar(ctx.alvo, escalar(d, 2.0f * TILE));
        const Celula c = celulaDe(alvo);
        if (!dentroDaSala(ctx.sala, c.col, c.lin) || tileEm(ctx.sala, c.col, c.lin).bloqueiaVoo) continue;
        if (colideComSala(ctx.sala, caixaNosPes(alvo, CAIXA_INIMIGO), true)) continue;
        ini.destinoTeleporte = alvo;
        ini.teleporte = AVISO_TELEPORTE;
        ini.submerso = AVISO_TELEPORTE;   // fumaça: não é atingido
        ini.recargaTeleporte = RECARGA_TELEPORTE;
        return true;
    }
    return false;
}

void decidirEspectro(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    switch (ini.estado) {
        case EstadoIA::Perseguindo:
            if (espectroTeleporta(ini, ctx, distancia)) break;
            if (distancia < 80.0f && ini.recarga <= 0.0f) {
                ini.estado = EstadoIA::Preparando;
                ini.timer = 0.6f;
                const Vec2 dir = subtrair(ctx.alvo, ini.pes);
                ini.direcao = direcaoDominante(dir, ini.direcao);
                Aviso a;
                a.tempoTotal = a.tempoRestante = 0.6f;
                a.golpe = golpeBase(ini, ctx, areaLinha(corpo(ini), normalizar(dir), 96.0f, 20.0f), p.dano);
                a.golpe.rolaAcerto = false;
                a.golpe.recuo = 40.0f;
                ctx.avisos.push_back(a);
            } else {
                irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
            }
            break;
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.7f;
                ini.recarga = 1.8f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Sanguessuga alada: voa e mira; ANUNCIA uma linha (0,5 s) e, no fim do aviso, dá o bote por ela
// (golpe instantâneo na linha; o corpo cruza num instante). Se acertar, se cura (Golpe::drena).
// Depois da investida fica parada 0,6 s: a janela para derrubá-la antes que se cure de novo.
constexpr float VELOCIDADE_INVESTIDA_SANGUESSUGA = 700.0f;   // um bote: o golpe é o da linha anunciada
constexpr float PREPARO_SANGUESSUGA = 0.5f;
constexpr float MINIMO_BOTE_SANGUESSUGA = 28.0f;   // px: mais perto, ela recua (ou desliza) para mirar

void decidirSanguessuga(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    switch (ini.estado) {
        case EstadoIA::Perseguindo: {
            const bool alinhada = distancia >= MINIMO_BOTE_SANGUESSUGA && distancia <= 150.0f && segmentoLivre(ctx.sala, corpo(ini), ctx.alvo, false);
            if (ini.recarga <= 0.0f && alinhada) {
                const Vec2 dir = normalizar(subtrair(ctx.alvo, ini.pes));
                float alcance = std::min(distancia + 36.0f, 180.0f);   // passa um pouco de Bill
                // ...mas POUSA onde Bill alcança: terminando sobre a água funda, ela ficava parada e
                // intocável na janela de punição e drenava de novo (a represa, telemetria da 10.2).
                while (alcance > distancia) {
                    const Celula fim = celulaDe(somar(ini.pes, escalar(dir, alcance)));
                    if (!tileEm(ctx.sala, fim.col, fim.lin).bloqueiaMovimento) break;
                    alcance -= 8.0f;
                }
                ini.estado = EstadoIA::Preparando;
                ini.timer = PREPARO_SANGUESSUGA;
                ini.alvoRegistrado = somar(ini.pes, escalar(dir, alcance));   // o fim da investida
                ini.direcao = direcaoDominante(dir, ini.direcao);
                Aviso a;
                a.tempoTotal = a.tempoRestante = PREPARO_SANGUESSUGA;
                a.golpe = golpeBase(ini, ctx, areaLinha(corpo(ini), dir, alcance, 18.0f), p.dano);
                a.golpe.rolaAcerto = false;
                a.golpe.recuo = 30.0f;
                a.golpe.drena = true;
                // Golpe INSTANTÂNEO no fim do aviso, como todos os outros: uma linha que ainda fere
                // depois que o aviso some é perigo invisível (achado medindo na 10.2).
                ctx.avisos.push_back(a);
            } else if (distancia < MINIMO_BOTE_SANGUESSUGA) {
                // Perto demais: ganha distância para mirar. Encurralada (parede atrás), desliza de lado:
                // antes ela ficava parada sobre uma moita contra a parede, sem atacar nem ser alcançada (11.1).
                const Vec2 antes = ini.pes;
                afastar(ini, ctx, p.velocidade, dt);
                if (comprimento(subtrair(ini.pes, antes)) < 0.2f) {
                    const Vec2 d = normalizar(subtrair(ini.pes, ctx.alvo));
                    const float lado = ini.id % 2 == 0 ? 1.0f : -1.0f;
                    andar(ini, ctx, Vec2{-d.y * lado, d.x * lado}, p.velocidade, dt);
                    if (comprimento(subtrair(ini.pes, antes)) < 0.2f) andar(ini, ctx, Vec2{d.y * lado, -d.x * lado}, p.velocidade, dt);
                }
            } else {
                irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
            }
            break;
        }
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Atacando;
                ini.timer = comprimento(subtrair(ini.alvoRegistrado, ini.pes)) / VELOCIDADE_INVESTIDA_SANGUESSUGA;
            }
            break;
        case EstadoIA::Atacando: {
            ini.timer -= dt;
            const Vec2 falta = subtrair(ini.alvoRegistrado, ini.pes);
            const Vec2 antes = ini.pes;
            if (comprimento(falta) > 2.0f) andar(ini, ctx, falta, VELOCIDADE_INVESTIDA_SANGUESSUGA, dt);
            const bool parou = comprimento(subtrair(ini.pes, antes)) < 0.5f;   // bateu na parede
            if (ini.timer <= 0.0f || comprimento(falta) <= 2.0f || parou) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.6f;
                ini.recarga = 1.6f;
            }
            break;
        }
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Afogado: lento e resistente; agarrão anunciado (0,65 s) que deixa Bill 50% mais lento por 1,5 s.
// As poças que ele deixa e o surgir da água são da partida (ela conhece o mapa e Bill).
constexpr float DURACAO_AGARRADO = 1.5f;

void decidirAfogado(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    switch (ini.estado) {
        case EstadoIA::Perseguindo:
            if (distancia < 40.0f && ini.recarga <= 0.0f) {
                ini.estado = EstadoIA::Preparando;
                ini.timer = 0.65f;
                ini.alvoRegistrado = ctx.alvo;
                ini.direcao = direcaoDominante(subtrair(ctx.alvo, ini.pes), ini.direcao);
                Aviso a;
                a.tempoTotal = a.tempoRestante = 0.65f;
                a.golpe = golpeBase(ini, ctx, areaArco(corpo(ini), subtrair(ctx.alvo, ini.pes), 46.0f, 110.0f), p.dano);
                a.golpe.rolaAcerto = false;
                a.golpe.recuo = 20.0f;
                a.golpe.lentidao = DURACAO_AGARRADO;
                ctx.avisos.push_back(a);
            } else {
                irPara(ini, ctx, ctx.alvo, p.velocidade, dt);
            }
            break;
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Atacando;
                ini.timer = 0.15f;
            }
            break;
        case EstadoIA::Atacando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.8f;   // janela de punição: o agarrão erra e ele demora a se recompor
                ini.recarga = 1.4f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Boneco palhaço: mantém 4–6 células de Bill e arremessa um pino em arco. O aviso é um círculo
// onde Bill ESTAVA (0,7 s: o tempo do pino no ar; o pino voa desenhado). Perto demais, foge; se
// encurralado, desliza de lado. Só arremessa com o caminho reto livre (nada atravessa parede).
constexpr float DISTANCIA_MIN_BONECO = 2.0f * TILE;    // mais perto: foge (um arranque)
constexpr float DISTANCIA_IDEAL_BONECO = 6.0f * TILE;  // mais longe: se aproxima
constexpr float VOO_PINO = 0.7f;
constexpr float RAIO_PINO = 22.0f;
constexpr float DURACAO_FUGA_BONECO = 0.8f;
constexpr float RECARGA_FUGA_BONECO = 4.0f;

void decidirBoneco(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    if (ini.recargaFuga > 0.0f) ini.recargaFuga -= dt;
    switch (ini.estado) {
        case EstadoIA::Perseguindo: {
            // Perto demais: um ARRANQUE de fuga (1 s) e depois recarga de 3 s. Fugindo sempre, um
            // palhaço mais lento que Bill ainda dava voltas na sala por minutos (medido na 11.3).
            if (distancia < DISTANCIA_MIN_BONECO && ini.recargaFuga <= 0.0f) {
                ini.estado = EstadoIA::Fugindo;
                ini.timer = DURACAO_FUGA_BONECO;
                ini.recargaFuga = RECARGA_FUGA_BONECO;
                break;
            }
            const bool livre = segmentoLivre(ctx.sala, Vec2{ini.pes.x, ini.pes.y - 8.0f}, ctx.alvo, false);
            if (ini.recarga <= 0.0f && livre && distancia <= DISTANCIA_IDEAL_BONECO + TILE && distancia >= 40.0f) {
                ini.estado = EstadoIA::Preparando;
                ini.timer = 0.3f;   // a pose de arremesso (coluna 3 do sprite)
                ini.direcao = direcaoDominante(subtrair(ctx.alvo, ini.pes), ini.direcao);
                Aviso a;
                a.tempoTotal = a.tempoRestante = VOO_PINO + 0.3f;
                a.golpe = golpeBase(ini, ctx, areaCirculo(ctx.alvo, RAIO_PINO), p.dano);
                a.golpe.rolaAcerto = false;
                a.golpe.recuo = 30.0f;
                a.projetil = true;
                a.lancadoDe = corpo(ini);
                ctx.avisos.push_back(a);
                break;
            }
            if (distancia > DISTANCIA_IDEAL_BONECO || !livre) irPara(ini, ctx, ctx.alvo, p.velocidade, dt);   // longe ou sem linha
            break;
        }
        case EstadoIA::Fugindo: {
            ini.timer -= dt;
            const Vec2 antes = ini.pes;
            afastar(ini, ctx, p.velocidade, dt);
            if (comprimento(subtrair(ini.pes, antes)) < 0.2f) {   // parede atrás: desliza de lado
                const Vec2 d = normalizar(subtrair(ini.pes, ctx.alvo));
                const float lado = ini.id % 2 == 0 ? 1.0f : -1.0f;
                andar(ini, ctx, Vec2{-d.y * lado, d.x * lado}, p.velocidade, dt);
                if (comprimento(subtrair(ini.pes, antes)) < 0.2f) andar(ini, ctx, Vec2{d.y * lado, -d.x * lado}, p.velocidade, dt);
            }
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        }
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = 0.6f;   // o pino está no ar; ele fica parado: a janela para alcançá-lo
                ini.recarga = 2.0f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// Cabelo do ralo: fixo; com Bill a até 4 células e à vista, anuncia um chicote em CRUZ (3 células
// para cada lado, 0,7 s) e chicoteia; depois se RECOLHE no ralo por 1,2 s (não é atingido). O lugar
// seguro para bater é a diagonal; e a cruz empurra Bill para cima dos outros.
constexpr float PREPARO_CABELO = 0.7f;
constexpr float RECOLHIDO_CABELO = 1.2f;
constexpr float BRACO_CABELO = 3.0f * TILE;

void decidirCabelo(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    switch (ini.estado) {
        case EstadoIA::Perseguindo:
            if (ini.recarga <= 0.0f && distancia <= BRACO_CABELO + TILE) {
                ini.estado = EstadoIA::Preparando;
                ini.timer = PREPARO_CABELO;
                const Vec2 h{1.0f, 0.0f}, v{0.0f, 1.0f};
                const Vec2 c{ini.pes.x, ini.pes.y - 8.0f};
                for (const Vec2& d : {h, v}) {
                    Aviso a;
                    a.tempoTotal = a.tempoRestante = PREPARO_CABELO;
                    a.golpe = golpeBase(ini, ctx, areaLinha(subtrair(c, escalar(d, BRACO_CABELO)), d, 2.0f * BRACO_CABELO, 20.0f), p.dano);
                    a.golpe.rolaAcerto = false;
                    a.golpe.recuo = 40.0f;
                    ctx.avisos.push_back(a);
                }
            }
            break;   // nunca anda
        case EstadoIA::Preparando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Atacando;
                ini.timer = 0.25f;
            }
            break;
        case EstadoIA::Atacando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) {
                ini.estado = EstadoIA::Recuperando;
                ini.timer = RECOLHIDO_CABELO;
                ini.submerso = RECOLHIDO_CABELO;   // recolhido no ralo: não é atingido
                ini.recarga = RECOLHIDO_CABELO + 1.0f;
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.submerso > 0.0f) ini.submerso -= dt;
            if (ini.timer <= 0.0f) {
                ini.submerso = 0.0f;
                if (ini.raloDestino.col >= 0) {   // viajou pelos canos: sobe no outro ralo
                    ini.pes = centroCelula(ini.raloDestino);
                    ini.raloDestino = {-1, -1};
                }
                ini.estado = EstadoIA::Perseguindo;   // de pé de novo: a janela para bater
            }
            break;
        default: break;
    }
}

// Último(s) da sala e Bill longe: o Cabelo se recolhe e SOBE POR OUTRO RALO — o spawn mais perto de
// Bill, a 3+ células dele. Fixo, ele ficava esquecido no canto escuro e a sala não terminava (12.2).
bool cabeloTrocaDeRalo(Inimigo& ini, ContextoIA& ctx) {
    if (!ctx.sala.modelo || comprimento(subtrair(ctx.alvo, ini.pes)) <= 6.0f * TILE) return false;
    float menor = 1e9f;
    Celula melhor{-1, -1};
    for (const PontoSpawn& sp : ctx.sala.modelo->spawns) {
        const float d = comprimento(subtrair(centroCelula(sp.celula), ctx.alvo));
        if (d >= 3.0f * TILE && d < menor) { menor = d; melhor = sp.celula; }
    }
    if (melhor.col < 0) return false;
    ini.raloDestino = melhor;
    ini.estado = EstadoIA::Recuperando;
    ini.timer = 1.0f;
    ini.submerso = 1.0f;
    return true;
}

// ---------- chefe: UMA máquina de estados, padrões em tabela ----------
// Perseguindo → (escolhe um padrão válido pela distância e pela fase) → Preparando (aviso no
// chão com o alvo REGISTRADO) → executa → Recuperando (janela para bater) → Perseguindo.
void anunciar(Inimigo& ini, ContextoIA& ctx, const Area& area, int dano, float preparo, float prende) {
    Aviso a;
    a.tempoTotal = a.tempoRestante = preparo;
    a.golpe = golpeBase(ini, ctx, area, dano);
    a.golpe.rolaAcerto = false;   // ataques em área não erram (PRD §8.1)
    a.golpe.recuo = 60.0f;
    a.golpe.prende = prende;
    ctx.avisos.push_back(a);
}

int escolherPadrao(const Inimigo& ini, ContextoIA& ctx, float distancia) {
    int quantos = 0;
    const PadraoChefe* padroes = padroesDoChefe(ini.tipo, quantos);
    int total = 0;
    // Ataque só com o caminho reto livre até Bill: os pilares cortam as linhas, e um chefe que
    // ataca através da quina do pilar repete o aviso sem andar (impasse achado na 8.5). Sem
    // caminho reto, ele contorna o pilar. Invocar não precisa.
    // A partir de perto dos PÉS (sempre no chão): o "corpo" do medo, 27 px acima, ficava dentro do
    // pilar quando ele encostava nele por baixo, e nenhum ataque valia (impasse da 11.1).
    const bool ve = segmentoLivre(ctx.sala, Vec2{ini.pes.x, ini.pes.y - 8.0f}, ctx.alvo, false);
    // Não repete o mesmo padrão seguido (Etapa 10.3): com Bill sempre na mesma distância, o Pennywise
    // I+ soltou "Ondas" 120 vezes seguidas e ninguém se aproximava. Se só o último vale, ele ANDA.
    auto valido = [&](const PadraoChefe& pc) {
        if (&pc - padroes == ini.ultimoPadrao) return false;
        if (pc.fase != ini.fase || distancia < pc.distanciaMin || distancia > pc.distanciaMax) return false;
        if (pc.forma == FormaPadrao::Invocar) return ctx.hostis < LIMITE_HOSTIS_INVOCAR;
        if (pc.forma == FormaPadrao::Mergulho || pc.forma == FormaPadrao::Ondas || pc.forma == FormaPadrao::Porta) return true;
        if (pc.forma == FormaPadrao::Salto) return true;   // pula por cima das colunas e da água
        if (pc.forma == FormaPadrao::CruzAlvo) return true;   // a cruz nasce onde Bill está
        // Do ralo para a arena (não precisam de caminho reto até Bill: os cabelos e a água vêm pelo chão).
        if (pc.forma == FormaPadrao::Cruz || pc.forma == FormaPadrao::Xis || pc.forma == FormaPadrao::Inundar ||
            pc.forma == FormaPadrao::Golfada) return true;
        if (pc.forma == FormaPadrao::Chuva) return true;   // as brasas caem do alto
        if (pc.forma == FormaPadrao::Nuvem) return ve || distancia < 200.0f;   // arremessa a nuvem por cima
        return ve;
    };
    for (int i = 0; i < quantos; ++i) total += valido(padroes[i]) ? padroes[i].peso : 0;
    if (total == 0) return -1;
    int sorteio = intervalo(ctx.rng, 1, total);
    for (int i = 0; i < quantos; ++i) {
        if (!valido(padroes[i])) continue;
        sorteio -= padroes[i].peso;
        if (sorteio <= 0) return i;
    }
    return -1;
}

// "Pela porta": a célula de dentro de uma porta da sala (a vizinha que se pisa), sorteada entre as
// que ficam a 3+ células de Bill (o bote vem de longe, com tempo de ler a linha).
Vec2 pontoDaPorta(Inimigo& ini, ContextoIA& ctx) {
    std::vector<Vec2> opcoes, todas;
    for (const PortaSala& porta : ctx.sala.portas) {
        const int dc[4] = {1, -1, 0, 0}, dl[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
            const Celula v{porta.celula.col + dc[k], porta.celula.lin + dl[k]};
            const Tile& t = tileEm(ctx.sala, v.col, v.lin);
            if (t.bloqueiaMovimento || t.tipo == TipoTile::PortaSala) continue;
            const Vec2 c = centroCelula(v);
            todas.push_back(c);
            if (comprimento(subtrair(c, ctx.alvo)) >= 3.0f * TILE) opcoes.push_back(c);
            break;
        }
    }
    const std::vector<Vec2>& lista = opcoes.empty() ? todas : opcoes;
    const Vec2 saida = lista.empty() ? ini.pes : lista[static_cast<std::size_t>(intervalo(ctx.rng, 0, static_cast<int>(lista.size()) - 1))];
    ini.direcaoBote = normalizar(subtrair(ctx.alvo, saida));
    return saida;
}

// Anuncia o padrão `i`: aviso no chão com o alvo REGISTRADO (o golpe não segue Bill depois).
void comecarPadrao(Inimigo& ini, ContextoIA& ctx, int i) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    int quantos = 0;
    const PadraoChefe& pc = padroesDoChefe(ini.tipo, quantos)[i];
    ini.padrao = i;
    ini.ultimoPadrao = i;
    ini.estado = EstadoIA::Preparando;
    ini.timer = pc.preparo;
    ini.alvoRegistrado = ctx.alvo;
    const Vec2 dir = normalizar(subtrair(ctx.alvo, ini.pes));
    ini.direcao = direcaoDominante(dir, ini.direcao);
    const Vec2 c = corpo(ini);
    switch (pc.forma) {
        case FormaPadrao::Faixa: case FormaPadrao::Linha: case FormaPadrao::Investida:
            anunciar(ini, ctx, areaLinha(c, dir, pc.comprimento, pc.largura), p.dano, pc.preparo, pc.prende);
            break;
        case FormaPadrao::Leque:   // 3 bandagens: −25°, 0°, +25°
            for (int k = -1; k <= 1; ++k) {
                const float a = 0.436f * static_cast<float>(k);
                const Vec2 d{dir.x * std::cos(a) - dir.y * std::sin(a), dir.x * std::sin(a) + dir.y * std::cos(a)};
                anunciar(ini, ctx, areaLinha(c, d, pc.comprimento, pc.largura), p.dano, pc.preparo, 0.0f);
            }
            break;
        case FormaPadrao::Circulo:   // anel em volta dos pés do chefe
            anunciar(ini, ctx, areaCirculo(ini.pes, pc.comprimento), p.dano, pc.preparo, 0.0f);
            break;
        case FormaPadrao::Ondas:   // 2 anéis: o de dentro com o preparo, o de fora 0,5 s depois
            anunciar(ini, ctx, areaAnel(ini.pes, RAIO_SEGURO_ONDAS, RAIO_SEGURO_ONDAS + pc.largura), p.dano, pc.preparo, 0.0f);
            anunciar(ini, ctx, areaAnel(ini.pes, RAIO_SEGURO_ONDAS + pc.largura, pc.comprimento), p.dano, pc.preparo + 0.5f, 0.0f);
            break;
        case FormaPadrao::Nuvem:   // um círculo onde Bill está; ao cair fere pouco e fica a nuvem (lento 5 s)
            anunciar(ini, ctx, areaCirculo(ctx.alvo, pc.comprimento), p.dano / 2, pc.preparo, 0.0f);
            ctx.avisos.back().golpe.nuvem = DURACAO_NUVEM;
            break;
        case FormaPadrao::Cruz: case FormaPadrao::Xis: {   // duas linhas inteiras pelo centro do ralo
            constexpr float D = 0.70710678f;
            const Vec2 eixos[2][2] = {{{1.0f, 0.0f}, {0.0f, 1.0f}}, {{D, D}, {D, -D}}};
            const Vec2 centro{ini.pes.x, ini.pes.y - 8.0f};
            for (const Vec2& d : eixos[pc.forma == FormaPadrao::Xis ? 1 : 0])
                anunciar(ini, ctx, areaLinha(subtrair(centro, escalar(d, pc.comprimento)), d, 2.0f * pc.comprimento, pc.largura),
                         p.dano, pc.preparo, 0.0f);
            break;
        }
        case FormaPadrao::Inundar: {   // dois quadrantes opostos; o par alterna a cada inundação
            const bool par = (ini.inundacoes++ % 2) == 0;
            const Vec2 dirs[2] = {par ? Vec2{1.0f, -1.0f} : Vec2{-1.0f, -1.0f}, par ? Vec2{-1.0f, 1.0f} : Vec2{1.0f, 1.0f}};
            for (int k = 0; k < 2; ++k) {
                anunciar(ini, ctx, areaArco(ini.origem, normalizar(dirs[k]), pc.comprimento, ABERTURA_QUADRANTE), p.dano / 2,
                         pc.preparo, 0.0f);
                ctx.avisos.back().golpe.poca = DURACAO_INUNDACAO;
                ctx.avisos.back().golpe.inunda = k == 0;   // o 1º seca a inundação anterior; o 2º só soma
            }
            break;
        }
        case FormaPadrao::Golfada:   // cospe sangue onde Bill está: fere pouco e deixa a poça
            anunciar(ini, ctx, areaCirculo(ctx.alvo, pc.comprimento), p.dano / 2, pc.preparo, 0.0f);
            ctx.avisos.back().golpe.poca = DURACAO_GOLFADA;
            break;
        case FormaPadrao::Chuva: {   // brasas em círculos: a 1ª onde Bill está, as outras em volta, em sequência
            std::vector<Vec2> centros{ctx.alvo};
            for (int tentativa = 0; tentativa < 40 && static_cast<int>(centros.size()) < pc.invocar; ++tentativa) {
                const float a = 6.2831853f * real01(ctx.rng);
                const float r = pc.comprimento * (0.45f + 0.55f * real01(ctx.rng));
                const Vec2 c2{ctx.alvo.x + r * std::cos(a), ctx.alvo.y + r * std::sin(a)};
                const Celula cel = celulaDe(c2);
                if (!dentroDaSala(ctx.sala, cel.col, cel.lin) || tileEm(ctx.sala, cel.col, cel.lin).bloqueiaMovimento) continue;
                bool longe = true;   // círculos que não se sobrepõem: entre eles há por onde passar
                for (const Vec2& o : centros) longe &= comprimento(subtrair(o, c2)) >= 1.6f * pc.largura;
                if (longe) centros.push_back(c2);
            }
            for (std::size_t k = 0; k < centros.size(); ++k) {
                anunciar(ini, ctx, areaCirculo(centros[k], pc.largura), p.dano / 2, pc.preparo + INTERVALO_CHUVA * static_cast<float>(k), pc.prende);
                if (pc.prende > 0.0f) {   // a TEIA do Pennywise IV (24/11): gruda Bill no chão, sem fogo nem empurrão
                    ctx.avisos.back().golpe.recuo = 0.0f;
                    continue;
                }
                ctx.avisos.back().golpe.poca = DURACAO_FOGO;
                ctx.avisos.back().golpe.fogo = true;
                ctx.avisos.back().golpe.recuo = 30.0f;
            }
            break;
        }
        case FormaPadrao::CruzAlvo: {   // a cruz de raio `comprimento` em volta de onde Bill está agora
            const Vec2 eixos[2] = {{1.0f, 0.0f}, {0.0f, 1.0f}};
            for (const Vec2& d : eixos)
                anunciar(ini, ctx, areaLinha(subtrair(ctx.alvo, escalar(d, pc.comprimento)), d, 2.0f * pc.comprimento, pc.largura),
                         p.dano, pc.preparo, 0.0f);
            break;
        }
        case FormaPadrao::Invocar: case FormaPadrao::Mergulho: case FormaPadrao::Porta: case FormaPadrao::Salto:
            break;   // o preparo é a pose (agachar, afundar, sumir)
    }
}

// Os Cabelos que o Ralo chama sobem pelos ralos da arena (os spawns) mais perto de Bill, a 2+ células
// dele e sem outro inimigo em cima (dois no mesmo ralo alternavam a cruz e ninguém chegava, 12.2).
void invocarCabelos(const PadraoChefe& pc, ContextoIA& ctx) {
    if (!ctx.sala.modelo) return;
    std::vector<std::pair<float, Celula>> ralos;
    for (const PontoSpawn& sp : ctx.sala.modelo->spawns) {
        const Vec2 c = centroCelula(sp.celula);
        const float d = comprimento(subtrair(c, ctx.alvo));
        bool ocupado = false;
        if (ctx.vivos) {
            for (const Inimigo& o : *ctx.vivos) ocupado |= o.vivo && comprimento(subtrair(o.pes, c)) < TILE;
        }
        for (const Inimigo& o : *ctx.nascer) ocupado |= comprimento(subtrair(o.pes, c)) < TILE;
        if (d >= 2.0f * TILE && !ocupado) ralos.push_back({d, sp.celula});
    }
    std::sort(ralos.begin(), ralos.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (int k = 0; k < pc.invocar && k < static_cast<int>(ralos.size()); ++k)
        ctx.nascer->push_back(criarInimigo(pc.invocado, (*ctx.proximoId)++, centroCelula(ralos[static_cast<std::size_t>(k)].second)));
}

void decidirChefe(Inimigo& ini, ContextoIA& ctx, float dt, float distancia) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    int quantos = 0;
    const PadraoChefe* padroes = padroesDoChefe(ini.tipo, quantos);
    // Preparando ou golpeando, o corpo vira para onde o golpe vai (o IV gira o corpo de aranha inteiro).
    if ((ini.estado == EstadoIA::Preparando || ini.estado == EstadoIA::Atacando) && ini.submerso <= 0.0f)
        girarRumo(ini, subtrair(ini.alvoRegistrado, ini.pes), dt);
    // O Pennywise IV não troca de corpo nas fases: a velocidade é a dele (os outros: o Pennywise menor e rápido).
    const float velocidade = p.velocidade * (ini.fase == 2 && ini.tipo != TipoInimigo::PennywiseIV ? VELOCIDADE_FASE_2 : 1.0f);
    switch (ini.estado) {
        case EstadoIA::Perseguindo: {
            const int i = ini.recarga <= 0.0f ? escolherPadrao(ini, ctx, distancia) : -1;
            if (i < 0) {
                if (distancia > 40.0f && !inimigoFixo(ini)) irPara(ini, ctx, ctx.alvo, velocidade, dt);
                break;
            }
            comecarPadrao(ini, ctx, i);
            break;
        }
        case EstadoIA::Preparando: {
            ini.timer -= dt;
            if (ini.timer > 0.0f) break;
            const PadraoChefe& pc = padroes[ini.padrao];
            if (pc.forma == FormaPadrao::Mergulho) {   // afundou: marca onde Bill está e espera no fundo
                ini.alvoRegistrado = ctx.alvo;
                anunciar(ini, ctx, areaCirculo(ctx.alvo, RAIO_EMERGIR), p.dano, AVISO_EMERGIR, 0.0f);
                ini.submerso = AVISO_EMERGIR;
                ini.estado = EstadoIA::Atacando;
                ini.timer = AVISO_EMERGIR;
                break;
            }
            if (pc.forma == FormaPadrao::Salto) {   // saltou: no ar, marca onde Bill está agora e cai ali
                ini.alvoRegistrado = ctx.alvo;
                anunciar(ini, ctx, areaCirculo(ctx.alvo, pc.largura), p.dano, AVISO_SALTO, 0.0f);
                ini.submerso = AVISO_SALTO;   // no ar: não aparece nem é atingido
                ini.estado = EstadoIA::Atacando;
                ini.timer = AVISO_SALTO;
                break;
            }
            if (pc.forma == FormaPadrao::Porta) {   // sumiu: escolhe uma porta e anuncia a linha dela até Bill
                const Vec2 saida = pontoDaPorta(ini, ctx);
                const Vec2 dir = normalizar(subtrair(ctx.alvo, saida));
                anunciar(ini, ctx, areaLinha(saida, dir, pc.comprimento, pc.largura), p.dano, AVISO_PORTA, 0.0f);
                ini.alvoRegistrado = saida;
                ini.submerso = AVISO_PORTA;
                ini.estado = EstadoIA::Atacando;
                ini.timer = AVISO_PORTA;
                break;
            }
            if (pc.forma == FormaPadrao::Investida) {   // avança pela linha anunciada (para na parede)
                const Vec2 dir = normalizar(subtrair(ini.alvoRegistrado, ini.pes));
                ini.pes = moverComColisao(ctx.sala, ini.pes, CAIXA_INIMIGO, escalar(dir, pc.comprimento - 16.0f), p.voa);
            }
            if (pc.forma == FormaPadrao::Invocar && ctx.nascer && ctx.proximoId && pc.invocado == TipoInimigo::Cabelo) {
                invocarCabelos(pc, ctx);
            } else if (pc.forma == FormaPadrao::Invocar && ctx.nascer && ctx.proximoId) {
                for (int k = 0; k < pc.invocar; ++k) {
                    const float lado = k % 2 == 0 ? -1.0f : 1.0f;
                    Vec2 pos = moverComColisao(ctx.sala, ini.pes, CAIXA_INIMIGO, Vec2{lado * 40.0f, 12.0f}, false);
                    ctx.nascer->push_back(criarInimigo(pc.invocado, (*ctx.proximoId)++, pos));
                }
            }
            ini.estado = EstadoIA::Atacando;
            ini.timer = 0.2f;
            break;
        }
        case EstadoIA::Atacando:
            ini.timer -= dt;
            if (ini.submerso > 0.0f) ini.submerso -= dt;
            if (ini.timer <= 0.0f) {
                const PadraoChefe& pc = padroes[ini.padrao];
                if (pc.forma == FormaPadrao::Mergulho || pc.forma == FormaPadrao::Salto) {   // emerge/cai no lugar marcado (Bill pisava ali: é chão)
                    ini.pes = ini.alvoRegistrado;
                    ini.submerso = 0.0f;
                }
                if (pc.forma == FormaPadrao::Porta) {   // sai pela porta e dá o bote pela linha anunciada
                    const Vec2 dir = ini.direcaoBote;
                    ini.pes = moverComColisao(ctx.sala, ini.alvoRegistrado, CAIXA_INIMIGO, escalar(dir, pc.comprimento - 16.0f), p.voa);
                    ini.submerso = 0.0f;
                }
                if (pc.seguinte >= 0 && pc.seguinte < quantos) {   // combo: o 2º golpe já vem anunciado
                    comecarPadrao(ini, ctx, pc.seguinte);
                    break;
                }
                ini.estado = EstadoIA::Recuperando;
                ini.timer = pc.recuperacao;   // janela de punição
                ini.recarga = pc.recarga;     // respiro (o Invocar é longo: a arena não enche)
            }
            break;
        case EstadoIA::Recuperando:
            ini.timer -= dt;
            if (ini.timer <= 0.0f) ini.estado = EstadoIA::Perseguindo;
            break;
        default: break;
    }
}

// T03: a decisão específica de cada tipo é escolhida por esta tabela de ponteiros para função.
const Comportamento TABELA_COMPORTAMENTO[TOTAL_TIPOS_INIMIGO] = {decidirBalao, decidirRato, decidirSombra, decidirEspectro,
                                                                decidirSanguessuga, decidirAfogado, decidirBoneco, decidirCabelo,
                                                                decidirChefe, decidirChefe, decidirChefe, decidirChefe, decidirChefe,
                                                                decidirChefe, decidirChefe};

}  // namespace

const ParametrosInimigo& parametrosDe(TipoInimigo tipo) {
    const int i = static_cast<int>(tipo);
    return TABELA[(i >= 0 && i < TOTAL_TIPOS_INIMIGO) ? i : 0];
}

Inimigo criarInimigo(TipoInimigo tipo, IdEntidade id, Vec2 pes) {
    Inimigo ini;
    ini.id = id;
    ini.tipo = tipo;
    ini.pes = pes;
    ini.hp = parametrosDe(tipo).hp;
    ini.hpMax = ini.hp;
    ini.origem = pes;
    ini.surgindo = 0.7f;
    return ini;
}

bool padraoSemAlcance(FormaPadrao f) {
    return f == FormaPadrao::Invocar || f == FormaPadrao::Mergulho || f == FormaPadrao::Nuvem || f == FormaPadrao::Porta ||
           f == FormaPadrao::Inundar || f == FormaPadrao::Golfada || f == FormaPadrao::Chuva || f == FormaPadrao::Salto ||
           f == FormaPadrao::CruzAlvo;
}

bool inimigoFixo(const Inimigo& ini) {
    return parametrosDe(ini.tipo).velocidade <= 0.0f || (ini.tipo == TipoInimigo::RaloSangue && ini.fase == 1);
}

const PadraoChefe* padroesDoChefe(TipoInimigo tipo, int& quantos) {
    if (tipo == TipoInimigo::Mumia) {
        quantos = static_cast<int>(sizeof PADROES_MUMIA / sizeof PADROES_MUMIA[0]);
        return PADROES_MUMIA;
    }
    if (tipo == TipoInimigo::MedoAfogado) {
        quantos = static_cast<int>(sizeof PADROES_AFOGADO / sizeof PADROES_AFOGADO[0]);
        return PADROES_AFOGADO;
    }
    if (tipo == TipoInimigo::Leproso) {
        quantos = static_cast<int>(sizeof PADROES_LEPROSO / sizeof PADROES_LEPROSO[0]);
        return PADROES_LEPROSO;
    }
    if (tipo == TipoInimigo::RaloSangue) {
        quantos = static_cast<int>(sizeof PADROES_RALO / sizeof PADROES_RALO[0]);
        return PADROES_RALO;
    }
    if (tipo == TipoInimigo::Passaro) {
        quantos = static_cast<int>(sizeof PADROES_PASSARO / sizeof PADROES_PASSARO[0]);
        return PADROES_PASSARO;
    }
    if (tipo == TipoInimigo::Lobisomem) {
        quantos = static_cast<int>(sizeof PADROES_LOBISOMEM / sizeof PADROES_LOBISOMEM[0]);
        return PADROES_LOBISOMEM;
    }
    if (tipo == TipoInimigo::PennywiseIV) {
        quantos = static_cast<int>(sizeof PADROES_PENNYWISE_IV / sizeof PADROES_PENNYWISE_IV[0]);
        return PADROES_PENNYWISE_IV;
    }
    quantos = 0;
    return nullptr;
}

const char* nomeDoChefe(const Inimigo& ini) {
    if (ini.tipo == TipoInimigo::Mumia) return ini.fase == 1 ? "A Múmia" : "Pennywise I";
    if (ini.tipo == TipoInimigo::MedoAfogado) return ini.fase == 1 ? "O Afogado" : "Pennywise I+";
    if (ini.tipo == TipoInimigo::Leproso) return ini.fase == 1 ? "O Leproso" : "Pennywise II";
    if (ini.tipo == TipoInimigo::RaloSangue) return ini.fase == 1 ? "O Ralo de Sangue" : "Pennywise II+";
    if (ini.tipo == TipoInimigo::Passaro) return ini.fase == 1 ? "O Pássaro das Cinzas" : "Pennywise III";
    if (ini.tipo == TipoInimigo::Lobisomem) return ini.fase == 1 ? "O Lobisomem" : "Pennywise III+";
    return parametrosDe(ini.tipo).nome;
}

float escalaDoPennywise(TipoInimigo chefe) {
    switch (chefe) {
        case TipoInimigo::Mumia: return 0.92f;         // I: o mais fraco, contido
        case TipoInimigo::MedoAfogado: return 1.0f;    // I+
        case TipoInimigo::Leproso: return 1.06f;       // II
        case TipoInimigo::RaloSangue: return 1.12f;    // II+ (encharcado de sangue)
        case TipoInimigo::Passaro: return 1.16f;       // III
        case TipoInimigo::Lobisomem: return 1.22f;     // III+
        default: return 1.0f;                           // IV: folha própria 64×64
    }
}

Retangulo hurtboxInimigo(const Inimigo& ini) {
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    // Chefe: o medo é largo (64×64); o Pennywise da fase 2 é do tamanho de uma pessoa.
    // O Pennywise IV é 2×2 em todas as fases (não sai "de dentro" de nada).
    const bool grande = p.chefe && (ini.fase == 1 || ini.tipo == TipoInimigo::PennywiseIV);
    const float esc = (p.chefe && !grande) ? escalaDoPennywise(ini.tipo) : 1.0f;
    float h = (p.chefe && !grande) ? 42.0f * esc : p.alturaCorpo;
    float w = grande ? 34.0f : 22.0f * esc;
    if (ini.tipo == TipoInimigo::PennywiseIV) {   // o palhaço-aranha (24/11): o corpo inteiro é alvo
        w = 64.0f;
        h = 60.0f;
    }
    return {ini.pes.x - w / 2.0f, ini.pes.y - h, w, h};
}

bool podeSerAtingido(const Inimigo& ini) {
    // Luzes Mortas (PRD §4.5): o Pennywise IV só fica vulnerável logo depois de cada ataque.
    if (ini.tipo == TipoInimigo::PennywiseIV && ini.fase == 3 && ini.estado != EstadoIA::Recuperando) return false;
    return ini.vivo && ini.surgindo <= 0.0f && ini.transicao <= 0.0f && ini.submerso <= 0.0f;
}

void atualizarInimigo(Inimigo& ini, ContextoIA& ctx, float dt) {
    if (!ini.vivo) return;
    ini.animTempo += dt;
    ini.andando = false;
    if (ini.flash > 0.0f) ini.flash -= dt;
    if (ini.lento > 0.0f) ini.lento -= dt;
    if (ini.recarga > 0.0f) ini.recarga -= dt;
    if (ini.surgindo > 0.0f) { ini.surgindo -= dt; return; }
    if (ini.transicao > 0.0f) { ini.transicao -= dt; return; }   // chefe trocando de fase
    // Espectro em fumaça (13.2): o teleporte corre AQUI, em todo quadro. Dentro do decidir ele só
    // contava nos estados que chamam o comportamento: perdendo Bill de vista, a fumaça congelava e
    // ele andava invulnerável, reaparecendo longe de onde a fumaça tinha anunciado.
    if (ini.recargaTeleporte > 0.0f) ini.recargaTeleporte -= dt;
    if (ini.teleporte > 0.0f) {
        ini.teleporte -= dt;
        ini.submerso = std::max(0.0f, ini.teleporte);
        if (ini.teleporte <= 0.0f) {
            ini.pes = ini.destinoTeleporte;
            ini.submerso = 0.0f;
            ini.recarga = std::max(ini.recarga, 0.4f);   // um respiro antes da linha: dá para reagir
        }
        return;   // em fumaça não anda, não ataca, não é empurrado
    }

    // Recuo (knockback): desliza e perde força rapidamente; também respeita paredes.
    if (comprimento(ini.empurrao) > 1.0f) {
        ini.pes = moverComColisao(ctx.sala, ini.pes, CAIXA_INIMIGO, escalar(ini.empurrao, dt), parametrosDe(ini.tipo).voa);
        ini.empurrao = escalar(ini.empurrao, std::pow(0.0005f, dt));
    }
    if (ini.atordoado > 0.0f) { ini.atordoado -= dt; return; }

    // Percepção: distância de detecção + linha de visão (paredes bloqueiam).
    const ParametrosInimigo& p = parametrosDe(ini.tipo);
    const float distancia = comprimento(subtrair(ctx.alvo, ini.pes));
    const Celula aqui = celulaDe(ini.pes), alvo = celulaDe(ctx.alvo);
    // O chefe sempre sabe onde Bill está (a arena é dele).
    const bool ve = ctx.alvoAtivo && (p.chefe || (distancia <= p.deteccao * TILE &&
                                                  linhaDeVisao(ctx.sala, aqui, alvo, static_cast<int>(p.deteccao) + 1)));

    switch (ini.estado) {
        case EstadoIA::Vagando:
            if (ve) { ini.estado = EstadoIA::Perseguindo; break; }
            if (ctx.cacar && ctx.alvoAtivo && ini.tipo == TipoInimigo::Cabelo) {
                cabeloTrocaDeRalo(ini, ctx);
                break;
            }
            if (ctx.cacar && ctx.alvoAtivo) {
                // Caça com COMPROMISSO: vai até onde Bill estava e só reavalia a cada 2 s. Perseguindo
                // cada passo, um Afogado lento ia e voltava na margem enquanto Bill dava a volta no lago
                // (o menor caminho trocava de lado o tempo todo — impasse da 11.1).
                ini.timerCaca -= dt;
                if (ini.timerCaca <= 0.0f || ini.alvoCaca.col < 0) {
                    ini.alvoCaca = celulaDe(ctx.alvo);
                    ini.timerCaca = INTERVALO_CACA;
                }
                if (irPara(ini, ctx, centroCelula(ini.alvoCaca), p.velocidade * FATOR_VELOCIDADE_CACA, dt)) break;
                ini.alvoCaca = {-1, -1};
            }
            vagar(ini, ctx, dt);
            break;
        case EstadoIA::Perseguindo:
            if (!ve) {
                ini.estado = EstadoIA::Buscando;
                ini.tempoSemVer = 0.0f;
                break;
            }
            ini.ultimaVista = alvo;
            TABELA_COMPORTAMENTO[static_cast<int>(ini.tipo)](ini, ctx, dt, distancia);
            break;
        case EstadoIA::Buscando:
            ini.tempoSemVer += dt;
            if (ve) { ini.estado = EstadoIA::Perseguindo; break; }
            if (ini.tempoSemVer > TEMPO_BUSCA) {
                ini.estado = EstadoIA::Vagando;
                ini.destinoVagar = {-1, -1};
                break;
            }
            irPara(ini, ctx, centroCelula(ini.ultimaVista), p.velocidade * 0.8f, dt);
            break;
        default:   // Preparando, Atacando, Recuperando, Fugindo: o próprio tipo conduz
            TABELA_COMPORTAMENTO[static_cast<int>(ini.tipo)](ini, ctx, dt, distancia);
            break;
    }
}
