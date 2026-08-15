"""Renderização: traços, ligaduras e sorteio de variante (T-010, US-006).

AC-017: uma ligadura conecta o ponto de saída de uma letra ao ponto de
entrada da próxima, dentro da mesma palavra — nunca entre palavras
(ASM-010, a "pena" levanta no espaço).
AC-018: cada ocorrência de uma letra sorteia uma variante entre as
disponíveis na caligrafia, em vez de repetir sempre a mesma.

`desenhar_pagina` rasteriza as linhas já quebradas (T-009) numa imagem —
sua corretude ponta a ponta (AC-013/AC-021) é provada pelo teste de
integração de T-011, não aqui.
"""

from __future__ import annotations

import random
from dataclasses import dataclass

from PIL import Image, ImageDraw

from caligrafias.modelos import LetraVariante
from geracao.layout import ALTURA_PAGINA_PX, LARGURA_PAGINA_PX, MARGEM_PX, Linha

RESOLUCAO_LIGADURA = 12
ALTURA_LETRA_PX = 60.0
ALTURA_LINHA_PX = 90.0
ESPACO_PALAVRA_PX = 40.0


def sortear_variante(variantes: list[LetraVariante], aleatorio=random) -> LetraVariante:
    """Sorteia uma variante entre as disponíveis para a letra (AC-018)."""
    if not variantes:
        raise ValueError("nenhuma variante disponível para sortear")
    return aleatorio.choice(variantes)


def curva_ligadura(
    saida: list[float], entrada: list[float], resolucao: int = RESOLUCAO_LIGADURA
) -> list[list[float]]:
    """Curva de Bézier quadrática simples ligando `saida` a `entrada`
    (AC-017, ASM-008) — o ponto de controle é o meio do segmento, sem
    tentar simular a física da caneta."""
    controle = [(saida[0] + entrada[0]) / 2, (saida[1] + entrada[1]) / 2]
    pontos: list[list[float]] = []
    for i in range(resolucao + 1):
        t = i / resolucao
        x = (1 - t) ** 2 * saida[0] + 2 * (1 - t) * t * controle[0] + t**2 * entrada[0]
        y = (1 - t) ** 2 * saida[1] + 2 * (1 - t) * t * controle[1] + t**2 * entrada[1]
        pontos.append([x, y])
    return pontos


@dataclass
class LetraPosicionada:
    """Uma variante sorteada para uma ocorrência de letra dentro de uma
    palavra, com a ligadura que a conecta à letra anterior (None na
    primeira letra da palavra, ASM-010)."""

    variante: LetraVariante
    ligadura_anterior: list[list[float]] | None


def montar_palavra(
    palavra: str,
    caligrafia_variantes: dict[str, list[LetraVariante]],
    aleatorio=random,
) -> list[LetraPosicionada]:
    """Sorteia uma variante para cada letra da palavra (AC-018) e liga
    cada letra à anterior dentro da mesma palavra (AC-017)."""
    resultado: list[LetraPosicionada] = []
    anterior: LetraVariante | None = None
    for letra in palavra:
        variante = sortear_variante(caligrafia_variantes[letra], aleatorio)
        ligadura = curva_ligadura(anterior.saida, variante.entrada) if anterior else None
        resultado.append(LetraPosicionada(variante=variante, ligadura_anterior=ligadura))
        anterior = variante
    return resultado


def largura_variante(variante: LetraVariante) -> float:
    """Largura da variante nas unidades NATIVAS do traço capturado (pixels
    do recorte original — variam de captura pra captura, não é uma escala
    de renderização)."""
    xs = [p[0] for p in variante.pontos] or [0.0]
    return max(xs) - min(xs)


def _altura_variante(variante: LetraVariante) -> float:
    ys = [p[1] for p in variante.pontos] or [0.0]
    return max(ys) - min(ys)


def fator_escala(variante: LetraVariante) -> float:
    """Fator que normaliza o tamanho NATIVO da variante (pixels do
    recorte original de onde ela foi extraída — pode ser qualquer
    tamanho) para a altura de letra alvo da página renderizada. Sem essa
    normalização por letra, duas caligrafias capturadas em recortes de
    tamanhos diferentes sairiam com letras de tamanhos bem diferentes."""
    altura_nativa = _altura_variante(variante)
    return ALTURA_LETRA_PX / altura_nativa if altura_nativa > 0 else 1.0


def largura_variante_renderizada(variante: LetraVariante) -> float:
    """Largura da variante já normalizada para o tamanho de renderização
    (o que efetivamente ocupa na página, ao contrário de `largura_variante`)."""
    return largura_variante(variante) * fator_escala(variante)


def _desenhar_traco(desenho: ImageDraw.ImageDraw, pontos: list[list[float]], x: float, y: float, escala: float) -> None:
    if len(pontos) < 2:
        return
    deslocados = [(x + p[0] * escala, y + p[1] * escala) for p in pontos]
    desenho.line(deslocados, fill=0, width=2, joint="curve")


def desenhar_pagina(
    linhas: list[Linha],
    caligrafia_variantes: dict[str, list[LetraVariante]],
    aleatorio=random,
) -> Image.Image:
    """Desenha as linhas já quebradas (T-009) numa página A4, escolhendo
    uma variante por ocorrência de letra (AC-018) e ligando as letras
    dentro de cada palavra (AC-017). Cada letra é normalizada pela
    própria altura (`fator_escala`), então recortes de tamanhos
    diferentes na captura original saem no mesmo tamanho na página — e é
    por isso que a ligadura é recalculada aqui, já em coordenadas da
    página (a `ligadura_anterior` de `montar_palavra` fica em unidades
    nativas de cada variante, que podem ter escalas diferentes entre si;
    só dá pra ligar sem quebra visual depois de posicionar as duas)."""
    imagem = Image.new("L", (LARGURA_PAGINA_PX, ALTURA_PAGINA_PX), color=255)
    desenho = ImageDraw.Draw(imagem)

    y = float(MARGEM_PX)
    for linha in linhas:
        x = float(MARGEM_PX)
        for palavra in linha.palavras:
            saida_anterior_pagina: tuple[float, float] | None = None
            for letra_pos in montar_palavra(palavra, caligrafia_variantes, aleatorio):
                variante = letra_pos.variante
                escala = fator_escala(variante)
                entrada_pagina = (x + variante.entrada[0] * escala, y + variante.entrada[1] * escala)
                saida_pagina = (x + variante.saida[0] * escala, y + variante.saida[1] * escala)

                if saida_anterior_pagina is not None:
                    ligadura_pagina = curva_ligadura(list(saida_anterior_pagina), list(entrada_pagina))
                    desenho.line([tuple(p) for p in ligadura_pagina], fill=0, width=2, joint="curve")

                _desenhar_traco(desenho, variante.pontos, x, y, escala)
                x += largura_variante_renderizada(variante)
                saida_anterior_pagina = saida_pagina
            x += ESPACO_PALAVRA_PX
        y += ALTURA_LINHA_PX
    return imagem
