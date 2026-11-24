// Aparência do Bill (SDD §24.4; critério M08): função pura, então se testa sem janela.
#include "core/pixels.h"
#include "jogo/aparencia.h"
#include "teste.h"

namespace {
Jogador billPronto() {
    Jogador j;
    recalcularJogador(j);
    j.hp = j.stats.hpMax;
    return j;
}
}  // namespace

TESTE(aparencia_sem_itens_e_a_base) {
    const AparenciaBill a = aparenciaDe(billPronto(), Inventario{}, 1.0f);
    VERIFICAR(!a.escudo && a.melhorias == 0 && a.aura == 0.0f && !a.rastro && !a.revelaInimigos);
    VERIFICAR(a.forcaTinta == 0 && !a.contorno);
}

TESTE(aparencia_escudo_e_melhoria_aparecem) {
    Jogador j = billPronto();
    Inventario inv;
    coletarNoInventario(j, inv, TipoItem::Escudo, 1);
    coletarNoInventario(j, inv, TipoItem::MelhoriaArma, 2);
    const AparenciaBill a = aparenciaDe(j, inv, 0.0f);
    VERIFICAR(a.escudo && a.melhorias == 2);
}

TESTE(aparencia_coragem_acende_e_apaga_em_30_s) {
    Jogador j = billPronto();
    Inventario inv;
    coletarNoInventario(j, inv, TipoItem::FichaCoragem, 1);
    usarItem(j, inv, TipoItem::FichaCoragem);
    AparenciaBill a = aparenciaDe(j, inv, 0.3f);
    VERIFICAR(a.aura > 0.0f && a.tinta == COR_TINTA_CORAGEM && a.forcaTinta > 0);
    for (int q = 0; q < 31 * 60; ++q) atualizarPowerUps(j, DT);
    a = aparenciaDe(j, inv, 0.3f);
    VERIFICAR(a.aura == 0.0f && a.forcaTinta == 0);
}

TESTE(aparencia_prioridade_dano_cura_coragem_e_pacto) {
    Jogador j = billPronto();
    j.coragem = 10.0f;
    j.cura = 0.4f;
    j.flash = DT;
    VERIFICAR(aparenciaDe(j, Inventario{}, 0.0f).tinta == COR_TINTA_DANO);   // o dano é o mais urgente
    j.flash = 0.0f;
    VERIFICAR(aparenciaDe(j, Inventario{}, 0.0f).tinta == COR_TINTA_CURA);
    j.cura = 0.0f;
    VERIFICAR(aparenciaDe(j, Inventario{}, 0.0f).tinta == COR_TINTA_CORAGEM);
    j.pactoUsado = true;
    const AparenciaBill a = aparenciaDe(j, Inventario{}, 0.0f);
    VERIFICAR(a.contorno && a.corContorno == COR_CONTORNO_PACTO);
    j.tonico = 5.0f;
    j.oculos = 5.0f;
    VERIFICAR(aparenciaDe(j, Inventario{}, 0.0f).rastro && aparenciaDe(j, Inventario{}, 0.0f).revelaInimigos);
}

TESTE(pixels_tingir_preserva_alfa_e_respeita_a_forca) {
    const std::uint32_t vermelhoMeio = 0x80800000u;   // alfa 128, vermelho 128 (pré-multiplicado)
    VERIFICAR(tingir(vermelhoMeio, 0xFFFFFF, 0) == vermelhoMeio);
    const std::uint32_t branco = tingir(vermelhoMeio, 0xFFFFFF, 255);
    VERIFICAR((branco >> 24) == 0x80 && ((branco >> 16) & 0xFF) == 128 && ((branco >> 8) & 0xFF) == 128 && (branco & 0xFF) == 128);
    VERIFICAR(tingir(0x00000000u, 0xFFFFFF, 255) == 0u);   // transparente continua transparente
    const std::uint32_t meio = tingir(0xFF000000u, 0x00FF00, 128);   // preto opaco → meio verde
    VERIFICAR(((meio >> 8) & 0xFF) >= 126 && ((meio >> 8) & 0xFF) <= 129 && ((meio >> 16) & 0xFF) == 0);
}
