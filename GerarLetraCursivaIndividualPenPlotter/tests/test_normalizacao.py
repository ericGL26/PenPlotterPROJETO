"""Testes de normalização e validação de texto (T-008)."""

from __future__ import annotations

import pytest

from caligrafias.modelos import Caligrafia, LetraVariante, Regiao
from geracao.normalizacao import (
    CoberturaInsuficiente,
    TextoInvalido,
    caracteres_invalidos,
    normalizar,
    normalizar_e_validar,
)


def _variante(letra: str) -> LetraVariante:
    return LetraVariante(
        id=f"{letra}-0001",
        letra=letra,
        pontos=[[0.0, 0.0], [1.0, 1.0]],
        entrada=[0.0, 0.0],
        saida=[1.0, 1.0],
        imagem_origem="folha.jpg",
        regiao=Regiao(x=0, y=0, largura=10, altura=10),
    )


def _caligrafia_com_letras(*letras: str) -> Caligrafia:
    caligrafia = Caligrafia(nome="teste")
    for letra in letras:
        caligrafia.variantes[letra] = [_variante(letra)]
    return caligrafia


def test_maiusculas_sao_convertidas_automaticamente():
    """@spec:AC-014 texto com maiúsculas é convertido para minúsculas sem erro."""
    assert normalizar("Olá Mundo") == "olá mundo"


def test_bloqueio_por_letra_sem_variante_na_caligrafia():
    """@spec:AC-015 letra usada no texto sem nenhuma variante na caligrafia bloqueia a geração."""
    caligrafia = _caligrafia_com_letras("o", "l", "a")
    with pytest.raises(CoberturaInsuficiente) as exc:
        normalizar_e_validar("ola mundo", caligrafia)
    assert set(exc.value.letras) == {"m", "u", "n", "d"}


def test_texto_com_cobertura_completa_e_aceito():
    """@spec:AC-015 texto cuja caligrafia cobre todas as letras usadas é aceito, sem erro."""
    caligrafia = _caligrafia_com_letras("o", "l", "a")
    assert normalizar_e_validar("ola", caligrafia) == "ola"


def test_bloqueio_por_caractere_fora_do_conjunto_suportado():
    """@spec:AC-016 número/pontuação/símbolo no texto bloqueia a geração com a lista de inválidos."""
    caligrafia = _caligrafia_com_letras("o", "l", "a")
    with pytest.raises(TextoInvalido) as exc:
        normalizar_e_validar("ola 123!", caligrafia)
    assert set(exc.value.caracteres) == {"1", "2", "3", "!"}


def test_espaco_nunca_e_reportado_como_invalido():
    """@spec:AC-016 espaço entre palavras nunca é tratado como caractere não suportado."""
    assert caracteres_invalidos("ola mundo") == []
