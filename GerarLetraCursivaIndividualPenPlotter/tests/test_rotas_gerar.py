"""Testes da tela e rota de geração de texto (T-011)."""

from __future__ import annotations

from pathlib import Path

import pytest
from flask import Flask

from caligrafias.modelos import Caligrafia, LetraVariante, Regiao
from caligrafias.repositorio import RepositorioCaligrafias
from web.rotas_gerar import criar_blueprint_gerar


def _variante(id_: str, letra: str) -> LetraVariante:
    return LetraVariante(
        id=id_,
        letra=letra,
        pontos=[[0.0, 0.0], [1.0, 0.5], [2.0, 0.0]],
        entrada=[0.0, 0.0],
        saida=[2.0, 0.0],
        imagem_origem="folha.jpg",
        regiao=Regiao(x=0, y=0, largura=10, altura=10),
    )


@pytest.fixture
def cliente(tmp_path):
    repositorio = RepositorioCaligrafias(tmp_path / "dados")
    caligrafia = Caligrafia(nome="minha-letra")
    for letra in "olamundo":
        caligrafia.variantes[letra] = [_variante(f"{letra}-0", letra)]
    repositorio.salvar(caligrafia)

    app = Flask(__name__)
    app.register_blueprint(criar_blueprint_gerar(repositorio, tmp_path / "saidas"))

    with app.test_client() as client:
        yield client


def test_gerar_imagem_a_partir_de_texto_e_caligrafia_escolhidos(cliente):
    """@spec:AC-013 escolher uma caligrafia salva, digitar um texto e confirmar produz uma imagem."""
    resposta = cliente.post("/gerar", data={"caligrafia": "minha-letra", "texto": "ola mundo"})

    assert resposta.status_code == 200
    assert b"Baixar imagem" in resposta.data


def test_imagem_gerada_fica_disponivel_para_download(cliente):
    """@spec:AC-021 depois de gerada, a imagem PNG fica disponível para download."""
    resposta_geracao = cliente.post("/gerar", data={"caligrafia": "minha-letra", "texto": "ola"})
    assert resposta_geracao.status_code == 200

    corpo = resposta_geracao.data.decode("utf-8")
    inicio = corpo.index("/gerar/baixar/")
    fim = corpo.index('"', inicio)
    link_download = corpo[inicio:fim]

    resposta_download = cliente.get(link_download)

    assert resposta_download.status_code == 200
    assert resposta_download.mimetype == "image/png"
    assert resposta_download.data[:8] == b"\x89PNG\r\n\x1a\n"


def test_download_funciona_com_pasta_de_saida_relativa(tmp_path, monkeypatch):
    """@spec:AC-021 o download funciona mesmo quando pasta_saida é um caminho relativo (como o padrão real do app).

    Regressão: send_from_directory trata diretório relativo como relativo
    ao root_path do Flask, não ao diretório de trabalho do processo — sem
    resolver para absoluto, o arquivo é salvo num lugar e procurado em
    outro, e o download vira 404 (só aparece com um caminho relativo de
    verdade; tmp_path é sempre absoluto, por isso os outros testes não
    pegam esse caso)."""
    monkeypatch.chdir(tmp_path)
    repositorio = RepositorioCaligrafias(Path("dados"))
    caligrafia = Caligrafia(nome="teste-relativo")
    caligrafia.variantes["a"] = [_variante("a-0", "a")]
    repositorio.salvar(caligrafia)

    app = Flask(__name__)
    app.register_blueprint(criar_blueprint_gerar(repositorio, Path("saidas-relativas")))

    with app.test_client() as cliente_relativo:
        resposta = cliente_relativo.post("/gerar", data={"caligrafia": "teste-relativo", "texto": "a"})
        assert resposta.status_code == 200

        corpo = resposta.data.decode("utf-8")
        inicio = corpo.index("/gerar/baixar/")
        fim = corpo.index('"', inicio)
        link_download = corpo[inicio:fim]

        resposta_download = cliente_relativo.get(link_download)

    assert resposta_download.status_code == 200
    assert resposta_download.data[:8] == b"\x89PNG\r\n\x1a\n"
