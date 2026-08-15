"""Testes do app factory e integração das telas (T-007)."""

from __future__ import annotations

import io

import cv2
import numpy as np
import pytest

from web.app import criar_app


def _bytes_imagem(formato: str = ".png") -> bytes:
    imagem = np.full((200, 200, 3), 255, dtype=np.uint8)
    # traço em "U": simula uma letra com dois pontos extremos bem definidos.
    cv2.line(imagem, (40, 40), (40, 100), color=(0, 0, 0), thickness=3)
    cv2.line(imagem, (40, 100), (100, 100), color=(0, 0, 0), thickness=3)
    cv2.line(imagem, (100, 100), (100, 40), color=(0, 0, 0), thickness=3)
    ok, codificada = cv2.imencode(formato, imagem)
    assert ok
    return codificada.tobytes()


@pytest.fixture
def cliente(tmp_path):
    app = criar_app(
        raiz_dados=tmp_path / "dados",
        pasta_uploads=tmp_path / "uploads",
        pasta_saidas=tmp_path / "saidas",
    )
    with app.test_client() as client:
        yield client


def _upload(cliente, nome: str, nome_arquivo: str = "folha.png"):
    return cliente.post(
        "/upload",
        data={
            "nome": nome,
            "arquivo": (io.BytesIO(_bytes_imagem()), nome_arquivo),
        },
        content_type="multipart/form-data",
    )


def test_upload_integrado_cria_caligrafia_visivel_na_consulta(cliente):
    """@spec:AC-001 fazer upload pela app integrada cria a caligrafia e ela aparece na tela de consulta."""
    resposta = _upload(cliente, "minha-letra")
    assert resposta.status_code in (302, 303)

    resposta_consulta = cliente.get("/consulta")
    assert resposta_consulta.status_code == 200
    assert "minha-letra" in resposta_consulta.get_data(as_text=True)


def test_extracao_integrada_sugere_regioes_da_imagem_enviada(cliente):
    """@spec:AC-004 após o upload pela app integrada, a tela de extração mostra sugestões para a mesma imagem enviada."""
    _upload(cliente, "minha-letra")

    resposta = cliente.get("/extracao/minha-letra")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert "data-regioes=" in corpo
    inicio = corpo.index("data-regioes='") + len("data-regioes='")
    assert corpo[inicio] != "]" or corpo[inicio : inicio + 2] != "[]"


def test_confirmar_regiao_integrado_salva_variante_e_reflete_na_consulta(cliente):
    """@spec:AC-007 confirmar uma região pela rota de extração integrada salva a variante e ela passa a contar na cobertura da consulta."""
    _upload(cliente, "minha-letra")

    resposta = cliente.post(
        "/extracao/minha-letra/confirmar",
        json={"imagem": 0, "letra": "u", "x": 20, "y": 20, "largura": 100, "altura": 100},
    )
    assert resposta.status_code == 200
    assert resposta.get_json()["sucesso"] is True

    detalhes = cliente.get("/consulta/minha-letra")
    assert detalhes.status_code == 200
    assert 'class="letra-com-variante" data-letra="u"' in detalhes.get_data(as_text=True)


def test_consulta_integrada_mostra_cobertura_apos_fluxo_completo(cliente):
    """@spec:AC-010 a tela de consulta integrada mostra a cobertura por letra depois do fluxo de upload e extração."""
    _upload(cliente, "minha-letra")
    cliente.post(
        "/extracao/minha-letra/confirmar",
        json={"imagem": 0, "letra": "u", "x": 20, "y": 20, "largura": 100, "altura": 100},
    )

    resposta = cliente.get("/consulta")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert "minha-letra" in corpo
    assert "u: 1" in corpo


def test_caligrafia_construida_no_app_integrado_pode_gerar_texto(cliente):
    """T-012: uma caligrafia construída via upload + extração na app integrada já pode ser usada em /gerar."""
    _upload(cliente, "minha-letra")
    for letra in "u":
        cliente.post(
            "/extracao/minha-letra/confirmar",
            json={"imagem": 0, "letra": letra, "x": 20, "y": 20, "largura": 100, "altura": 100},
        )

    resposta = cliente.post("/gerar", data={"caligrafia": "minha-letra", "texto": "u"})

    assert resposta.status_code == 200
    assert b"Baixar imagem" in resposta.data
