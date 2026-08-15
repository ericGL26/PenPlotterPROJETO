"""Testes do repositório de dados das caligrafias."""

from __future__ import annotations

import pytest

from caligrafias.modelos import CONJUNTO_LETRAS, LetraVariante, Regiao
from caligrafias.repositorio import (
    RepositorioCaligrafias,
    VarianteNaoEncontrada,
)


def _variante(id_: str, letra: str) -> LetraVariante:
    return LetraVariante(
        id=id_,
        letra=letra,
        pontos=[[0.0, 0.0], [1.0, 1.0]],
        entrada=[0.0, 0.0],
        saida=[1.0, 1.0],
        imagem_origem="folha.jpg",
        regiao=Regiao(x=0, y=0, largura=10, altura=10),
    )


def test_upload_cria_caligrafia_nova(tmp_path):
    """@spec:AC-001 upload com nome inédito cria uma nova caligrafia associada à imagem."""
    repo = RepositorioCaligrafias(tmp_path)

    caligrafia = repo.registrar_upload("minha-letra", "folha1.jpg")

    assert caligrafia.nome == "minha-letra"
    assert caligrafia.imagens == ["folha1.jpg"]
    assert repo.existe("minha-letra")
    recarregada = repo.carregar("minha-letra")
    assert recarregada.imagens == ["folha1.jpg"]


def test_upload_em_caligrafia_existente_acrescenta_amostras(tmp_path):
    """@spec:AC-002 upload com nome já usado acrescenta a imagem, sem duplicar a caligrafia."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")
    repo.adicionar_variante("minha-letra", _variante("v1", "a"))

    caligrafia = repo.registrar_upload("minha-letra", "folha2.jpg")

    assert caligrafia.imagens == ["folha1.jpg", "folha2.jpg"]
    assert len(repo.listar()) == 1
    assert [v.id for v in caligrafia.variantes["a"]] == ["v1"]


def test_upload_repetido_da_mesma_imagem_nao_duplica_referencia(tmp_path):
    """@spec:AC-002 reenviar a mesma imagem não a lista duas vezes na caligrafia existente."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")

    caligrafia = repo.registrar_upload("minha-letra", "folha1.jpg")

    assert caligrafia.imagens == ["folha1.jpg"]


def test_lista_de_caligrafias_mostra_cobertura_por_letra(tmp_path):
    """@spec:AC-010 a lista de caligrafias mostra, para cada letra suportada, a contagem de variantes."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")
    repo.adicionar_variante("minha-letra", _variante("v1", "a"))
    repo.adicionar_variante("minha-letra", _variante("v2", "a"))
    repo.adicionar_variante("minha-letra", _variante("v3", "b"))

    caligrafias = repo.listar()

    assert len(caligrafias) == 1
    cobertura = caligrafias[0].cobertura()
    assert set(cobertura.keys()) == set(CONJUNTO_LETRAS)
    assert cobertura["a"] == 2
    assert cobertura["b"] == 1
    assert cobertura["c"] == 0


def test_letras_sem_variante_aparecem_destacadas(tmp_path):
    """@spec:AC-011 letras sem nenhuma variante aparecem na lista de letras faltando."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")
    repo.adicionar_variante("minha-letra", _variante("v1", "a"))

    caligrafia = repo.carregar("minha-letra")
    faltando = caligrafia.letras_faltando()

    assert "a" not in faltando
    assert "b" in faltando
    assert set(faltando) == set(CONJUNTO_LETRAS) - {"a"}


def test_excluir_variante_remove_permanentemente(tmp_path):
    """@spec:AC-012 excluir uma variante a remove da cobertura e ela não pode ser recarregada depois."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")
    repo.adicionar_variante("minha-letra", _variante("v1", "a"))
    repo.adicionar_variante("minha-letra", _variante("v2", "a"))

    repo.excluir_variante("minha-letra", "a", "v1")

    caligrafia = repo.carregar("minha-letra")
    assert [v.id for v in caligrafia.variantes["a"]] == ["v2"]
    assert caligrafia.cobertura()["a"] == 1


def test_excluir_variante_inexistente_falha(tmp_path):
    """@spec:AC-012 tentar excluir uma variante que não existe não altera dados e sinaliza erro."""
    repo = RepositorioCaligrafias(tmp_path)
    repo.registrar_upload("minha-letra", "folha1.jpg")
    repo.adicionar_variante("minha-letra", _variante("v1", "a"))

    with pytest.raises(VarianteNaoEncontrada):
        repo.excluir_variante("minha-letra", "a", "id-que-nao-existe")

    caligrafia = repo.carregar("minha-letra")
    assert [v.id for v in caligrafia.variantes["a"]] == ["v1"]
