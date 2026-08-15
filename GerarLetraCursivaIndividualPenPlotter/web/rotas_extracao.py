"""Tela e rota de extração de letras (T-005).

Usa `caligrafias/segmentacao.py` (T-002) para sugerir regiões candidatas
(AC-004/AC-005), `caligrafias/vetorizacao.py` (T-003) para extrair o traço
vetorial do recorte confirmado (AC-007) e `caligrafias/repositorio.py`
(T-001) para persistir a variante. O ajuste de retângulo (AC-006) e o
desenho manual (AC-005) são feitos no canvas (`web/static/extracao.js`); o
que chega no POST de confirmação é sempre o recorte já ajustado. A
validação do caractere digitado (AC-008) roda aqui, no servidor.
"""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np
from flask import Blueprint, abort, jsonify, render_template, request, send_file

from caligrafias.modelos import CONJUNTO_LETRAS, LetraVariante, Regiao
from caligrafias.repositorio import RepositorioCaligrafias
from caligrafias.segmentacao import segmentar_arquivo
from caligrafias.vetorizacao import vetorizar_recorte

import uuid


class RegiaoInvalida(Exception):
    """A região informada não corresponde a nenhum traço de caneta reconhecível."""


class LetraInvalida(Exception):
    """O caractere informado não está no conjunto suportado (AC-008)."""

    def __init__(self) -> None:
        super().__init__(
            "Caractere não suportado. Use uma letra minúscula do alfabeto "
            "português, com ou sem acento/cedilha: " + ", ".join(CONJUNTO_LETRAS)
        )


def _carregar_imagem(caminho: str) -> np.ndarray:
    imagem = cv2.imread(caminho)
    if imagem is None:
        abort(404, f"não foi possível ler a imagem: {caminho}")
    return imagem


def _indice_imagem(caligrafia, indice: int | None) -> int:
    if not caligrafia.imagens:
        abort(404, "esta caligrafia ainda não tem nenhuma imagem enviada")
    if indice is None:
        return len(caligrafia.imagens) - 1
    if indice < 0 or indice >= len(caligrafia.imagens):
        abort(404, "índice de imagem inválido")
    return indice


def _recortar(imagem: np.ndarray, regiao: Regiao) -> np.ndarray:
    altura, largura = imagem.shape[:2]
    x0 = max(0, regiao.x)
    y0 = max(0, regiao.y)
    x1 = min(largura, regiao.x + regiao.largura)
    y1 = min(altura, regiao.y + regiao.altura)
    return imagem[y0:y1, x0:x1]


def validar_letra(letra: str) -> str:
    """Recusa qualquer caractere fora do conjunto suportado (AC-008)."""
    if letra not in CONJUNTO_LETRAS:
        raise LetraInvalida()
    return letra


def confirmar_regiao(
    repositorio: RepositorioCaligrafias,
    nome_caligrafia: str,
    imagem_origem: str,
    regiao: Regiao,
    letra: str,
) -> LetraVariante:
    """Extrai o traço vetorial do recorte (já ajustado — AC-006) e salva
    como nova variante da letra digitada (AC-007), depois de validar o
    caractere (AC-008)."""
    validar_letra(letra)

    imagem = _carregar_imagem(imagem_origem)
    recorte = _recortar(imagem, regiao)
    try:
        pontos, entrada, saida = vetorizar_recorte(recorte)
    except ValueError as erro:
        raise RegiaoInvalida(str(erro)) from erro

    variante = LetraVariante(
        id=str(uuid.uuid4()),
        letra=letra,
        pontos=pontos,
        entrada=entrada,
        saida=saida,
        imagem_origem=imagem_origem,
        regiao=regiao,
    )
    repositorio.adicionar_variante(nome_caligrafia, variante)
    return variante


def criar_blueprint_extracao(repositorio: RepositorioCaligrafias) -> Blueprint:
    """Blueprint com a tela de extração (GET), o envio da imagem (GET) e a
    confirmação de uma região rotulada (POST)."""
    blueprint = Blueprint(
        "extracao",
        __name__,
        template_folder="templates",
        static_folder="static",
        static_url_path="/static",
    )

    @blueprint.route("/extracao/<nome>", methods=["GET"])
    def tela(nome):
        caligrafia = repositorio.carregar(nome)
        indice = request.args.get("imagem", type=int)
        indice = _indice_imagem(caligrafia, indice)
        caminho = caligrafia.imagens[indice]

        regioes = segmentar_arquivo(caminho)

        return render_template(
            "extracao.html",
            nome=nome,
            indice_imagem=indice,
            regioes=[regiao.to_dict() for regiao in regioes],
            sem_sugestoes=not regioes,
            letras_suportadas=CONJUNTO_LETRAS,
        )

    @blueprint.route("/extracao/<nome>/imagem/<int:indice>", methods=["GET"])
    def imagem(nome, indice):
        caligrafia = repositorio.carregar(nome)
        _indice_imagem(caligrafia, indice)
        return send_file(Path(caligrafia.imagens[indice]).resolve())

    @blueprint.route("/extracao/<nome>/confirmar", methods=["POST"])
    def confirmar(nome):
        dados = request.get_json(silent=True) or request.form
        caligrafia = repositorio.carregar(nome)

        indice = _indice_imagem(caligrafia, int(dados.get("imagem", -1)))
        caminho = caligrafia.imagens[indice]
        letra = (dados.get("letra") or "").strip()
        regiao = Regiao(
            x=int(dados.get("x", 0)),
            y=int(dados.get("y", 0)),
            largura=int(dados.get("largura", 0)),
            altura=int(dados.get("altura", 0)),
        )

        try:
            variante = confirmar_regiao(repositorio, nome, caminho, regiao, letra)
        except LetraInvalida as erro:
            return jsonify(erro=str(erro)), 400
        except RegiaoInvalida as erro:
            return jsonify(erro=str(erro)), 400

        return jsonify(sucesso=True, variante_id=variante.id, letra=variante.letra)

    return blueprint
