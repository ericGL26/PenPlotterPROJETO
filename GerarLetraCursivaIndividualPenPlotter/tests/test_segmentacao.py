"""Testes da segmentação automática de regiões candidatas."""

from __future__ import annotations

import cv2
import numpy as np
import pytest

from caligrafias.modelos import Regiao
from caligrafias.segmentacao import segmentar_arquivo, segmentar_regioes


def _folha_em_branco(largura: int = 300, altura: int = 300) -> np.ndarray:
    """Simula uma foto de folha de papel sem nenhuma marca de caneta."""
    return np.full((altura, largura), 255, dtype=np.uint8)


def _folha_com_letras(largura: int = 300, altura: int = 300) -> np.ndarray:
    """Simula uma foto de folha com dois traços de caneta bem separados."""
    imagem = _folha_em_branco(largura, altura)
    cv2.rectangle(imagem, (30, 40), (55, 90), color=0, thickness=-1)
    cv2.rectangle(imagem, (150, 160), (185, 210), color=0, thickness=-1)
    return imagem


def test_sugestoes_aparecem_apos_upload(tmp_path):
    """@spec:AC-004 uma imagem com traços de caneta gera retângulos sugeridos ao redor deles."""
    imagem = _folha_com_letras()

    regioes = segmentar_regioes(imagem)

    assert len(regioes) == 2
    assert all(isinstance(regiao, Regiao) for regiao in regioes)
    primeira, segunda = regioes
    assert (primeira.x, primeira.y) == (30, 40)
    assert (segunda.x, segunda.y) == (150, 160)


def test_sugestoes_aparecem_apos_upload_a_partir_de_arquivo(tmp_path):
    """@spec:AC-004 o mesmo resultado é obtido carregando a imagem recém-enviada do disco."""
    caminho = tmp_path / "folha.png"
    cv2.imwrite(str(caminho), _folha_com_letras())

    regioes = segmentar_arquivo(caminho)

    assert len(regioes) == 2


def test_nenhuma_sugestao_encontrada_permite_desenho_manual():
    """@spec:AC-005 uma folha em branco não gera nenhuma região candidata, sem lançar erro."""
    imagem = _folha_em_branco()

    regioes = segmentar_regioes(imagem)

    assert regioes == []


def test_nenhuma_sugestao_encontrada_a_partir_de_arquivo(tmp_path):
    """@spec:AC-005 uma imagem em branco carregada do disco também devolve lista vazia."""
    caminho = tmp_path / "folha_em_branco.png"
    cv2.imwrite(str(caminho), _folha_em_branco())

    regioes = segmentar_arquivo(caminho)

    assert regioes == []


def test_segmentar_arquivo_inexistente_lanca_erro(tmp_path):
    """Um caminho que não é uma imagem legível não deve ser confundido com 'nenhuma sugestão'."""
    caminho = tmp_path / "nao-existe.png"

    with pytest.raises(ValueError):
        segmentar_arquivo(caminho)
