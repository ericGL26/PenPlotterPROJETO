"""Testes de layout em página A4 (T-009)."""

from __future__ import annotations

from geracao.layout import montar_layout


def test_quebra_por_palavra_inteira_dentro_da_largura_util():
    """@spec:AC-019 texto quebra em linhas por palavra inteira, sem cortar nenhuma no meio."""
    resultado = montar_layout(
        "ab cd ef",
        largura_letra_px=1,
        largura_espaco_px=1,
        altura_linha_px=10,
        largura_util_px=5,
        altura_util_px=1000,
    )
    assert [linha.texto for linha in resultado.linhas] == ["ab cd", "ef"]
    assert resultado.texto_truncado is None


def test_layout_usa_area_util_a4_por_padrao():
    """@spec:AC-019 sem largura/altura customizada, o layout usa a área útil de uma página A4 a 300dpi."""
    resultado = montar_layout(
        "uma linha curta",
        largura_letra_px=20,
        largura_espaco_px=10,
        altura_linha_px=80,
    )
    assert resultado.linhas
    assert resultado.texto_truncado is None


def test_texto_que_nao_cabe_e_truncado_com_o_restante_reportado():
    """@spec:AC-020 texto que não cabe na página é truncado e o restante é devolvido no aviso."""
    resultado = montar_layout(
        "ab cd ef",
        largura_letra_px=1,
        largura_espaco_px=1,
        altura_linha_px=10,
        largura_util_px=5,
        altura_util_px=10,
    )
    assert [linha.texto for linha in resultado.linhas] == ["ab cd"]
    assert resultado.texto_truncado == "ef"


def test_texto_que_cabe_inteiro_nao_e_truncado():
    """@spec:AC-020 texto que cabe inteiro na página não gera nenhum aviso de truncamento."""
    resultado = montar_layout(
        "ab",
        largura_letra_px=1,
        largura_espaco_px=1,
        altura_linha_px=10,
        largura_util_px=5,
        altura_util_px=10,
    )
    assert resultado.texto_truncado is None
