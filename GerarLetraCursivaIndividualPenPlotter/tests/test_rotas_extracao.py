"""Testes da tela e rota de extração (T-005)."""

from __future__ import annotations

import json

import cv2
import numpy as np
import pytest
from flask import Flask

from caligrafias.modelos import Regiao
from caligrafias.repositorio import RepositorioCaligrafias
from web.rotas_extracao import criar_blueprint_extracao


def _salvar_imagem(caminho, imagem) -> None:
    ok = cv2.imwrite(str(caminho), imagem)
    assert ok


def _folha_em_branco(largura: int = 200, altura: int = 200) -> np.ndarray:
    return np.full((altura, largura, 3), 255, dtype=np.uint8)


def _folha_com_uma_letra(largura: int = 200, altura: int = 200) -> np.ndarray:
    imagem = _folha_em_branco(largura, altura)
    # traço em "U": simula uma letra com dois pontos extremos bem definidos.
    cv2.line(imagem, (40, 40), (40, 100), color=(0, 0, 0), thickness=3)
    cv2.line(imagem, (40, 100), (100, 100), color=(0, 0, 0), thickness=3)
    cv2.line(imagem, (100, 100), (100, 40), color=(0, 0, 0), thickness=3)
    return imagem


@pytest.fixture
def contexto(tmp_path):
    repositorio = RepositorioCaligrafias(tmp_path / "dados")
    pasta_uploads = tmp_path / "uploads"
    pasta_uploads.mkdir()

    app = Flask(__name__)
    app.register_blueprint(criar_blueprint_extracao(repositorio))

    with app.test_client() as client:
        yield client, repositorio, pasta_uploads


def _preparar_caligrafia(repositorio, pasta_uploads, nome, imagem):
    caminho = pasta_uploads / f"{nome}.png"
    _salvar_imagem(caminho, imagem)
    repositorio.registrar_upload(nome, str(caminho))
    return caminho


def test_sugestoes_aparecem_apos_upload(contexto):
    """@spec:AC-004 abrir a tela de extração de uma imagem recém-enviada mostra os retângulos sugeridos."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_com_uma_letra())

    resposta = cliente.get("/extracao/minha-letra")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert "data-regioes=" in corpo
    inicio = corpo.index("data-regioes='") + len("data-regioes='")
    fim = corpo.index("'", inicio)
    regioes = json.loads(corpo[inicio:fim].replace("&#34;", '"'))
    assert len(regioes) >= 1


def test_nenhuma_sugestao_permite_desenho_manual(contexto):
    """@spec:AC-005 uma imagem em branco não gera sugestões e a tela avisa que é possível desenhar manualmente."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_em_branco())

    resposta = cliente.get("/extracao/minha-letra")

    assert resposta.status_code == 200
    corpo = resposta.get_data(as_text=True)
    assert "desenhe manualmente" in corpo.lower() or "aviso" in corpo.lower()
    assert "data-regioes='[]'" in corpo


def test_confirmar_usa_o_recorte_ajustado_nao_o_original(contexto):
    """@spec:AC-006 o retângulo enviado na confirmação (ajustado pelo usuário) é o que é usado para extrair o traço, não a sugestão original."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_com_uma_letra())

    sugestao_original = Regiao(x=0, y=0, largura=10, altura=10)
    retangulo_ajustado = {"x": 30, "y": 30, "largura": 80, "altura": 80}
    assert (sugestao_original.x, sugestao_original.largura) != (
        retangulo_ajustado["x"],
        retangulo_ajustado["largura"],
    )

    resposta = cliente.post(
        "/extracao/minha-letra/confirmar",
        json={"imagem": 0, "letra": "a", **retangulo_ajustado},
    )

    assert resposta.status_code == 200
    caligrafia = repositorio.carregar("minha-letra")
    variante = caligrafia.variantes["a"][0]
    assert variante.regiao.x == retangulo_ajustado["x"]
    assert variante.regiao.y == retangulo_ajustado["y"]
    assert variante.regiao.largura == retangulo_ajustado["largura"]
    assert variante.regiao.altura == retangulo_ajustado["altura"]


def test_confirmar_regiao_salva_traco_vetorial(contexto):
    """@spec:AC-007 confirmar uma região com um caractere válido salva uma nova variante com o traço vetorial extraído."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_com_uma_letra())

    resposta = cliente.post(
        "/extracao/minha-letra/confirmar",
        json={"imagem": 0, "letra": "u", "x": 20, "y": 20, "largura": 100, "altura": 100},
    )

    assert resposta.status_code == 200
    dados = resposta.get_json()
    assert dados["sucesso"] is True

    caligrafia = repositorio.carregar("minha-letra")
    assert "u" in caligrafia.variantes
    variante = caligrafia.variantes["u"][0]
    assert len(variante.pontos) > 0
    assert variante.entrada == variante.pontos[0]
    assert variante.saida == variante.pontos[-1]


def test_caractere_fora_do_conjunto_suportado_e_recusado(contexto):
    """@spec:AC-008 digitar um caractere que não é letra minúscula PT-BR é recusado e a resposta lista os aceitos."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_com_uma_letra())

    for letra_invalida in ("A", "1", "!", "ab", ""):
        resposta = cliente.post(
            "/extracao/minha-letra/confirmar",
            json={
                "imagem": 0,
                "letra": letra_invalida,
                "x": 20,
                "y": 20,
                "largura": 100,
                "altura": 100,
            },
        )
        assert resposta.status_code == 400
        dados = resposta.get_json()
        assert "a" in dados["erro"]

    caligrafia = repositorio.carregar("minha-letra")
    assert caligrafia.variantes == {}


def test_descartar_regiao_nao_persiste_nada_no_banco(contexto):
    """@spec:AC-009 descartar uma região sugerida não salva nada no banco — só o desenho/confirmação persistem."""
    cliente, repositorio, pasta_uploads = contexto
    _preparar_caligrafia(repositorio, pasta_uploads, "minha-letra", _folha_com_uma_letra())

    resposta = cliente.get("/extracao/minha-letra")
    assert resposta.status_code == 200

    caligrafia = repositorio.carregar("minha-letra")
    assert caligrafia.variantes == {}
