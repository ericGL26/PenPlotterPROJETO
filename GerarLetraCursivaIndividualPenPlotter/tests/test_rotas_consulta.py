"""Testes da tela e rota de consulta (T-006)."""

from __future__ import annotations

import uuid

import pytest
from flask import Flask

from caligrafias.modelos import LetraVariante, Regiao
from caligrafias.repositorio import RepositorioCaligrafias
from web.rotas_consulta import criar_blueprint_consulta


def _variante(letra: str) -> LetraVariante:
    return LetraVariante(
        id=str(uuid.uuid4()),
        letra=letra,
        pontos=[[0.0, 0.0], [1.0, 1.0]],
        entrada=[0.0, 0.0],
        saida=[1.0, 1.0],
        imagem_origem="qualquer.png",
        regiao=Regiao(x=0, y=0, largura=10, altura=10),
    )


@pytest.fixture
def contexto(tmp_path):
    repositorio = RepositorioCaligrafias(tmp_path / "dados")

    app = Flask(__name__)
    app.register_blueprint(criar_blueprint_consulta(repositorio))

    with app.test_client() as client:
        yield client, repositorio


def test_lista_de_caligrafias_mostra_cobertura_por_letra(contexto):
    """@spec:AC-010 a lista de caligrafias mostra o nome de cada uma e a cobertura por letra."""
    cliente, repositorio = contexto
    repositorio.registrar_upload("minha-letra", "foto.png")
    repositorio.adicionar_variante("minha-letra", _variante("a"))
    repositorio.adicionar_variante("minha-letra", _variante("a"))
    repositorio.adicionar_variante("minha-letra", _variante("b"))

    resposta = cliente.get("/consulta")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert "minha-letra" in corpo
    assert "a: 2" in corpo
    assert "b: 1" in corpo
    assert "c: 0" in corpo


def test_letras_sem_variante_aparecem_destacadas_nos_detalhes(contexto):
    """@spec:AC-011 letras sem nenhuma variante aparecem destacadas como faltando na tela de detalhes."""
    cliente, repositorio = contexto
    repositorio.registrar_upload("minha-letra", "foto.png")
    repositorio.adicionar_variante("minha-letra", _variante("a"))

    resposta = cliente.get("/consulta/minha-letra")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert 'class="letra-com-variante" data-letra="a"' in corpo
    assert 'class="letra-faltando" data-letra="b"' in corpo


def test_excluir_variante_remove_permanentemente_e_some_da_cobertura(contexto):
    """@spec:AC-012 excluir uma variante salva por engano remove-a permanentemente e ela some da cobertura."""
    cliente, repositorio = contexto
    repositorio.registrar_upload("minha-letra", "foto.png")
    variante = _variante("a")
    repositorio.adicionar_variante("minha-letra", variante)

    resposta = cliente.post(
        f"/consulta/minha-letra/{variante.letra}/{variante.id}/excluir"
    )

    assert resposta.status_code in (302, 303)
    caligrafia = repositorio.carregar("minha-letra")
    assert caligrafia.variantes.get("a", []) == []
    assert "a" in caligrafia.letras_faltando()

    detalhes = cliente.get("/consulta/minha-letra")
    corpo = detalhes.get_data(as_text=True)
    assert 'class="letra-faltando" data-letra="a"' in corpo
