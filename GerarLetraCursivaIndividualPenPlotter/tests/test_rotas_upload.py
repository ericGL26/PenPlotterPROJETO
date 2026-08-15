"""Testes da tela e rota de upload (T-004)."""

from __future__ import annotations

import io

import cv2
import numpy as np
import pytest
from flask import Flask

from caligrafias.repositorio import RepositorioCaligrafias
from web.rotas_upload import criar_blueprint_upload


def _bytes_imagem_valida(formato: str = ".png") -> bytes:
    imagem = np.full((40, 40, 3), 255, dtype=np.uint8)
    cv2.rectangle(imagem, (5, 5), (20, 20), color=(0, 0, 0), thickness=-1)
    ok, codificada = cv2.imencode(formato, imagem)
    assert ok
    return codificada.tobytes()


@pytest.fixture
def cliente(tmp_path):
    repositorio = RepositorioCaligrafias(tmp_path / "dados")
    pasta_uploads = tmp_path / "uploads"

    app = Flask(__name__)
    app.register_blueprint(criar_blueprint_upload(repositorio, pasta_uploads))

    with app.test_client() as client:
        client.repositorio = repositorio
        yield client


def test_upload_cria_caligrafia_nova(cliente):
    """@spec:AC-001 enviar uma imagem válida com um nome inédito cria uma nova caligrafia."""
    resposta = cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(_bytes_imagem_valida()), "folha1.png"),
        },
        content_type="multipart/form-data",
    )

    assert resposta.status_code in (302, 303)
    caligrafia = cliente.repositorio.carregar("minha-letra")
    assert len(caligrafia.imagens) == 1


def test_upload_em_caligrafia_existente_acrescenta_amostras(cliente):
    """@spec:AC-002 enviar uma nova foto para um nome já usado acrescenta a imagem, sem duplicar a caligrafia."""
    cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(_bytes_imagem_valida()), "folha1.png"),
        },
        content_type="multipart/form-data",
    )

    resposta = cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(_bytes_imagem_valida()), "folha2.png"),
        },
        content_type="multipart/form-data",
    )

    assert resposta.status_code in (302, 303)
    assert len(cliente.repositorio.listar()) == 1
    caligrafia = cliente.repositorio.carregar("minha-letra")
    assert len(caligrafia.imagens) == 2


def test_upload_de_arquivo_invalido_e_recusado_por_extensao(cliente):
    """@spec:AC-003 um arquivo que não é JPG nem PNG é recusado e nenhuma caligrafia é criada."""
    resposta = cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(b"conteudo qualquer"), "documento.txt"),
        },
        content_type="multipart/form-data",
    )

    assert resposta.status_code == 400
    assert "JPG ou PNG".encode("utf-8") in resposta.data or "JPG" in resposta.get_data(as_text=True)
    assert not cliente.repositorio.existe("minha-letra")


def test_upload_de_arquivo_corrompido_e_recusado(cliente):
    """@spec:AC-003 um arquivo com extensão válida mas conteúdo corrompido é recusado."""
    resposta = cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(b"isso nao e uma imagem de verdade"), "folha.png"),
        },
        content_type="multipart/form-data",
    )

    assert resposta.status_code == 400
    assert not cliente.repositorio.existe("minha-letra")


def test_upload_de_arquivo_invalido_nao_altera_caligrafia_existente(cliente):
    """@spec:AC-003 recusar um upload inválido não altera uma caligrafia já existente com o mesmo nome."""
    cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(_bytes_imagem_valida()), "folha1.png"),
        },
        content_type="multipart/form-data",
    )

    resposta = cliente.post(
        "/upload",
        data={
            "nome": "minha-letra",
            "arquivo": (io.BytesIO(b"conteudo qualquer"), "documento.txt"),
        },
        content_type="multipart/form-data",
    )

    assert resposta.status_code == 400
    caligrafia = cliente.repositorio.carregar("minha-letra")
    assert len(caligrafia.imagens) == 1
