"""Layout em página A4 (T-009, US-007).

Quebra o texto em linhas por palavra inteira dentro da área útil de uma
página A4 a 300dpi com margens de 2cm (ASM-007) e, quando o texto não
cabe inteiro, corta na última palavra que cabe e devolve o restante para
o aviso de truncamento (AC-020) — nunca pagina (Fora de escopo).

As larguras de letra/espaço são recebidas como parâmetro: quem mede a
largura real de cada variante (a partir dos pontos do traço) é a
renderização (T-010); aqui só o algoritmo de quebra de linha importa.
"""

from __future__ import annotations

from dataclasses import dataclass

DPI = 300
MM_POR_POLEGADA = 25.4
LARGURA_A4_MM = 210
ALTURA_A4_MM = 297
MARGEM_MM = 20


def _mm_para_px(mm: float) -> int:
    return round(mm / MM_POR_POLEGADA * DPI)


LARGURA_PAGINA_PX = _mm_para_px(LARGURA_A4_MM)
ALTURA_PAGINA_PX = _mm_para_px(ALTURA_A4_MM)
MARGEM_PX = _mm_para_px(MARGEM_MM)
LARGURA_UTIL_PX = LARGURA_PAGINA_PX - 2 * MARGEM_PX
ALTURA_UTIL_PX = ALTURA_PAGINA_PX - 2 * MARGEM_PX


@dataclass
class Linha:
    """Uma linha de texto já quebrada, pronta para ser desenhada."""

    palavras: list[str]

    @property
    def texto(self) -> str:
        return " ".join(self.palavras)


@dataclass
class ResultadoLayout:
    """Linhas que couberam na página e o texto que ficou de fora, se
    houver (AC-020). `texto_truncado` é None quando tudo coube."""

    linhas: list[Linha]
    texto_truncado: str | None


def montar_layout(
    texto_normalizado: str,
    largura_letra_px: float,
    largura_espaco_px: float,
    altura_linha_px: float,
    largura_util_px: float = LARGURA_UTIL_PX,
    altura_util_px: float = ALTURA_UTIL_PX,
) -> ResultadoLayout:
    """Quebra `texto_normalizado` em linhas por palavra inteira dentro da
    largura útil (AC-019). Se as linhas necessárias não couberem na
    altura útil, para na última linha que cabe e devolve o restante em
    `texto_truncado` (AC-020)."""
    palavras = [p for p in texto_normalizado.split(" ") if p]
    max_linhas = max(1, int(altura_util_px // altura_linha_px))

    linhas: list[Linha] = []
    linha_atual: list[str] = []
    largura_atual = 0.0

    for indice, palavra in enumerate(palavras):
        largura_palavra = len(palavra) * largura_letra_px
        largura_incremento = largura_palavra + (largura_espaco_px if linha_atual else 0.0)
        cabe_na_linha_atual = not linha_atual or (largura_atual + largura_incremento <= largura_util_px)

        if not cabe_na_linha_atual:
            if len(linhas) + 1 >= max_linhas:
                linhas.append(Linha(palavras=linha_atual))
                return ResultadoLayout(linhas=linhas, texto_truncado=" ".join(palavras[indice:]))
            linhas.append(Linha(palavras=linha_atual))
            linha_atual = []
            largura_atual = 0.0
            largura_incremento = largura_palavra

        linha_atual.append(palavra)
        largura_atual += largura_incremento

    if linha_atual:
        linhas.append(Linha(palavras=linha_atual))
    return ResultadoLayout(linhas=linhas, texto_truncado=None)
