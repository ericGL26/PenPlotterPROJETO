"""Testes da vetorização do traço da letra."""

from __future__ import annotations

import cv2
import numpy as np
import pytest

from caligrafias.vetorizacao import vetorizar_arquivo, vetorizar_recorte


def _recorte_com_traco_reto(tamanho: int = 60) -> np.ndarray:
    """Simula o recorte confirmado de uma letra: um traço de caneta reto."""
    imagem = np.full((tamanho, tamanho), 255, dtype=np.uint8)
    cv2.line(imagem, (5, 5), (tamanho - 6, tamanho - 6), color=0, thickness=3)
    return imagem


def _recorte_em_branco(tamanho: int = 60) -> np.ndarray:
    """Simula um recorte sem nenhuma marca de caneta."""
    return np.full((tamanho, tamanho), 255, dtype=np.uint8)


def test_confirmar_regiao_salva_traco_vetorial_da_letra():
    """@spec:AC-007 confirmar um recorte gera a sequência ordenada de pontos do traço, com entrada/saída."""
    recorte = _recorte_com_traco_reto()

    pontos, entrada, saida = vetorizar_recorte(recorte)

    assert len(pontos) > 1
    assert all(isinstance(p, list) and len(p) == 2 for p in pontos)
    assert entrada == pontos[0]
    assert saida == pontos[-1]
    assert entrada != saida


def test_confirmar_regiao_salva_traco_vetorial_a_partir_de_arquivo(tmp_path):
    """@spec:AC-007 o mesmo traço vetorial é extraído a partir do recorte confirmado salvo em disco."""
    caminho = tmp_path / "recorte.png"
    cv2.imwrite(str(caminho), _recorte_com_traco_reto())

    pontos, entrada, saida = vetorizar_arquivo(caminho)

    assert len(pontos) > 1
    assert entrada == pontos[0]
    assert saida == pontos[-1]


def test_entrada_e_saida_ficam_nas_extremidades_opostas_do_traco():
    """@spec:AC-007 entrada e saída marcam as duas pontas do esqueleto, não pontos do meio (ASM-002)."""
    recorte = _recorte_com_traco_reto()

    pontos, entrada, saida = vetorizar_recorte(recorte)

    distancia = ((entrada[0] - saida[0]) ** 2 + (entrada[1] - saida[1]) ** 2) ** 0.5
    assert distancia > 40


def test_recorte_sem_traco_lanca_erro():
    """Um recorte em branco não tem traço vetorial para extrair — não deve fabricar pontos."""
    recorte = _recorte_em_branco()

    with pytest.raises(ValueError):
        vetorizar_recorte(recorte)


def test_vetorizar_arquivo_inexistente_lanca_erro(tmp_path):
    """Um caminho que não é uma imagem legível não deve ser confundido com 'sem traço'."""
    caminho = tmp_path / "nao-existe.png"

    with pytest.raises(ValueError):
        vetorizar_arquivo(caminho)
