"""Segmentação automática de regiões candidatas (feature banco-caligrafias).

Visão computacional clássica (ASM-003): binarização adaptativa +
`cv2.findContours`, sem modelo treinado especificamente para caligrafia.
Por isso pode errar — a lista devolvida é sempre revisada pelo usuário
(US-003). Uma lista vazia é um resultado válido (AC-005), não um erro:
libera o desenho manual de retângulos.
"""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

from caligrafias.modelos import Regiao

# Área mínima (em pixels) para uma região ser considerada uma letra
# plausível, e fração máxima da área total da imagem que uma única região
# pode ocupar — filtram ruído e manchas grandes demais para ser uma letra.
AREA_MINIMA_PADRAO = 40
FRACAO_AREA_MAXIMA_PADRAO = 0.2


def segmentar_regioes(
    imagem: np.ndarray,
    area_minima: int = AREA_MINIMA_PADRAO,
    fracao_area_maxima: float = FRACAO_AREA_MAXIMA_PADRAO,
) -> list[Regiao]:
    """Sugere retângulos candidatos a letras a partir de uma imagem (AC-004).

    Converte para escala de cinza, binariza com threshold adaptativo e usa
    componentes conexos/contornos (`cv2.findContours`) para propor
    retângulos, filtrando por tamanho mínimo/máximo plausível de uma letra.
    Devolve lista vazia quando nada plausível é encontrado (AC-005) — não
    lança exceção, pois é um resultado válido que libera o desenho manual.
    """
    if imagem.ndim == 3:
        cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    else:
        cinza = imagem

    altura, largura = cinza.shape[:2]
    area_maxima = largura * altura * fracao_area_maxima

    binaria = cv2.adaptiveThreshold(
        cinza,
        255,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY_INV,
        blockSize=25,
        C=10,
    )

    contornos, _ = cv2.findContours(
        binaria, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )

    candidatas: list[Regiao] = []
    for contorno in contornos:
        x, y, w, h = cv2.boundingRect(contorno)
        area = w * h
        if area < area_minima or area > area_maxima:
            continue
        candidatas.append(Regiao(x=int(x), y=int(y), largura=int(w), altura=int(h)))

    candidatas.sort(key=lambda r: (r.y, r.x))
    return candidatas


def segmentar_arquivo(caminho_imagem: str | Path) -> list[Regiao]:
    """Carrega uma imagem do disco e devolve as regiões candidatas (AC-004/AC-005)."""
    imagem = cv2.imread(str(caminho_imagem))
    if imagem is None:
        raise ValueError(f"não foi possível ler a imagem: {caminho_imagem}")
    return segmentar_regioes(imagem)
