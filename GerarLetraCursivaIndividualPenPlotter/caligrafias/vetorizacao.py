"""Vetorização do traço da letra (feature banco-caligrafias).

Dado um recorte confirmado (imagem de uma letra), binariza e aplica
esqueletização (thinning, algoritmo de Zhang-Suen) para reduzir o traço da
caneta à sua linha central, depois percorre o esqueleto de uma ponta à
outra para obter uma sequência ordenada de pontos. As duas extremidades da
sequência são os pontos de entrada/saída — sem marcação manual (ASM-002).
"""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np

# Ordem dos 8 vizinhos ao redor de um pixel, começando ao norte e seguindo
# no sentido horário (P2..P9 na notação clássica do algoritmo Zhang-Suen).
_OFFSETS_VIZINHANCA = (
    (-1, 0), (-1, 1), (0, 1), (1, 1), (1, 0), (1, -1), (0, -1), (-1, -1),
)


def _binarizar(imagem: np.ndarray) -> np.ndarray:
    """Converte o recorte para uma máscara binária (1 = traço de caneta)."""
    if imagem.ndim == 3:
        cinza = cv2.cvtColor(imagem, cv2.COLOR_BGR2GRAY)
    else:
        cinza = imagem

    binaria = cv2.adaptiveThreshold(
        cinza,
        1,
        cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
        cv2.THRESH_BINARY_INV,
        blockSize=25,
        C=10,
    )
    return binaria.astype(np.uint8)


def _vizinhos(pixel: tuple[int, int], padded: np.ndarray) -> list[int]:
    y, x = pixel
    return [int(padded[y + dy, x + dx]) for dy, dx in _OFFSETS_VIZINHANCA]


def _esqueletizar(binaria: np.ndarray) -> np.ndarray:
    """Reduz uma máscara binária à sua linha central (thinning de Zhang-Suen)."""
    imagem = binaria.copy()
    altura, largura = imagem.shape

    houve_mudanca = True
    while houve_mudanca:
        houve_mudanca = False
        for sub_iteracao in (0, 1):
            padded = np.pad(imagem, 1)
            marcados: list[tuple[int, int]] = []
            for y in range(altura):
                for x in range(largura):
                    if imagem[y, x] == 0:
                        continue
                    vizinhos = _vizinhos((y + 1, x + 1), padded)
                    b = sum(vizinhos)
                    if not (2 <= b <= 6):
                        continue
                    transicoes = sum(
                        1
                        for i in range(8)
                        if vizinhos[i] == 0 and vizinhos[(i + 1) % 8] == 1
                    )
                    if transicoes != 1:
                        continue
                    p2, p4, p6, p8 = vizinhos[0], vizinhos[2], vizinhos[4], vizinhos[6]
                    if sub_iteracao == 0:
                        if p2 * p4 * p6 != 0 or p4 * p6 * p8 != 0:
                            continue
                    else:
                        if p2 * p4 * p8 != 0 or p2 * p6 * p8 != 0:
                            continue
                    marcados.append((y, x))
            if marcados:
                houve_mudanca = True
                for y, x in marcados:
                    imagem[y, x] = 0

    return imagem


def _grau(pixel: tuple[int, int], pontos: set[tuple[int, int]]) -> int:
    y, x = pixel
    return sum(
        1
        for dy, dx in _OFFSETS_VIZINHANCA
        if (y + dy, x + dx) in pontos
    )


def _ordenar_esqueleto(esqueleto: np.ndarray) -> list[tuple[int, int]]:
    """Percorre o esqueleto de uma ponta à outra, devolvendo (y, x) em ordem."""
    coords = [(int(y), int(x)) for y, x in zip(*np.nonzero(esqueleto))]
    if not coords:
        return []

    pontos = set(coords)
    extremidades = [p for p in coords if _grau(p, pontos) == 1]
    inicio = extremidades[0] if extremidades else coords[0]

    visitados = {inicio}
    caminho = [inicio]
    atual = inicio
    while True:
        y, x = atual
        proximo = next(
            (
                (y + dy, x + dx)
                for dy, dx in _OFFSETS_VIZINHANCA
                if (y + dy, x + dx) in pontos and (y + dy, x + dx) not in visitados
            ),
            None,
        )
        if proximo is None:
            break
        visitados.add(proximo)
        caminho.append(proximo)
        atual = proximo

    return caminho


def vetorizar_recorte(
    imagem: np.ndarray,
) -> tuple[list[list[float]], list[float], list[float]]:
    """Extrai o traço vetorial de um recorte confirmado (AC-007).

    Binariza o recorte, esqueletiza o traço da caneta e devolve a sequência
    ordenada de pontos `[x, y]`, junto com os pontos de entrada e saída —
    as duas extremidades do esqueleto (ASM-002). Lança `ValueError` quando o
    recorte não contém nenhum traço reconhecível.
    """
    binaria = _binarizar(imagem)
    esqueleto = _esqueletizar(binaria)
    caminho = _ordenar_esqueleto(esqueleto)

    if not caminho:
        raise ValueError("recorte não contém nenhum traço de caneta reconhecível")

    pontos = [[float(x), float(y)] for y, x in caminho]
    entrada = pontos[0]
    saida = pontos[-1]
    return pontos, entrada, saida


def vetorizar_arquivo(
    caminho_imagem: str | Path,
) -> tuple[list[list[float]], list[float], list[float]]:
    """Carrega um recorte do disco e devolve seu traço vetorial (AC-007)."""
    imagem = cv2.imread(str(caminho_imagem))
    if imagem is None:
        raise ValueError(f"não foi possível ler a imagem: {caminho_imagem}")
    return vetorizar_recorte(imagem)
