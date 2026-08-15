"""Testes de renderização: traços, ligaduras e sorteio de variante (T-010)."""

from __future__ import annotations

from caligrafias.modelos import LetraVariante, Regiao
from geracao.render import curva_ligadura, montar_palavra, sortear_variante


class _SorteioSequencial:
    """Stub de aleatório: cicla por índices fixos em vez de sortear de verdade."""

    def __init__(self, indices: list[int]):
        self._indices = indices
        self._i = 0

    def choice(self, sequencia):
        indice = self._indices[self._i % len(self._indices)]
        self._i += 1
        return sequencia[indice]


def _variante(id_: str, letra: str, entrada, saida) -> LetraVariante:
    return LetraVariante(
        id=id_,
        letra=letra,
        pontos=[list(entrada), list(saida)],
        entrada=list(entrada),
        saida=list(saida),
        imagem_origem="folha.jpg",
        regiao=Regiao(x=0, y=0, largura=10, altura=10),
    )


def test_ocorrencias_da_mesma_letra_sorteiam_variantes_diferentes():
    """@spec:AC-018 cada ocorrência de uma letra com mais de uma variante pode sortear uma diferente."""
    v0 = _variante("a-0", "a", [0, 0], [1, 1])
    v1 = _variante("a-1", "a", [0, 0], [1, 1])
    aleatorio = _SorteioSequencial([0, 1, 0, 1])

    escolhidas = [sortear_variante([v0, v1], aleatorio) for _ in range(4)]

    assert [v.id for v in escolhidas] == ["a-0", "a-1", "a-0", "a-1"]


def test_sortear_variante_sem_nenhuma_disponivel_falha():
    """@spec:AC-018 sortear sem nenhuma variante disponível é um erro explícito, não um resultado vazio."""
    try:
        sortear_variante([])
    except ValueError:
        pass
    else:
        raise AssertionError("esperava ValueError ao sortear lista vazia")


def test_curva_ligadura_comeca_na_saida_e_termina_na_entrada():
    """@spec:AC-017 a curva de ligadura começa exatamente no ponto de saída e termina no de entrada."""
    pontos = curva_ligadura(saida=[0.0, 0.0], entrada=[10.0, 4.0], resolucao=8)

    assert pontos[0] == [0.0, 0.0]
    assert pontos[-1] == [10.0, 4.0]
    assert len(pontos) == 9


def test_primeira_letra_da_palavra_nao_tem_ligadura():
    """@spec:AC-017 a primeira letra de uma palavra não recebe ligadura (não há letra anterior a conectar)."""
    variantes = {
        "o": [_variante("o-0", "o", [0, 0], [2, 0])],
        "i": [_variante("i-0", "i", [0, 0], [1, 0])],
    }

    posicionadas = montar_palavra("oi", variantes)

    assert posicionadas[0].ligadura_anterior is None
    assert posicionadas[1].ligadura_anterior is not None


def test_ligadura_conecta_saida_da_letra_anterior_a_entrada_da_atual():
    """@spec:AC-017 a ligadura entre duas letras liga o ponto de saída da primeira ao de entrada da segunda."""
    variantes = {
        "o": [_variante("o-0", "o", entrada=[0, 0], saida=[2, 1])],
        "i": [_variante("i-0", "i", entrada=[5, 3], saida=[6, 3])],
    }

    posicionadas = montar_palavra("oi", variantes)
    ligadura = posicionadas[1].ligadura_anterior

    assert ligadura[0] == [2, 1]
    assert ligadura[-1] == [5, 3]
