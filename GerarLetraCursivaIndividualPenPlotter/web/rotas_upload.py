"""Tela e rota de upload de imagens para o banco de caligrafias (T-004).

Usa `caligrafias/repositorio.py` (T-001) para criar uma caligrafia nova
(AC-001) ou acrescentar amostras a uma já existente (AC-002). Valida o
formato do arquivo (JPG/PNG) antes de aceitar (AC-003).
"""

from __future__ import annotations

from pathlib import Path

import cv2
import numpy as np
from flask import Blueprint, redirect, render_template, request, url_for
from werkzeug.utils import secure_filename

from caligrafias.modelos import Caligrafia
from caligrafias.repositorio import RepositorioCaligrafias

EXTENSOES_PERMITIDAS = {"jpg", "jpeg", "png"}


class ArquivoInvalido(Exception):
    """O arquivo enviado não é um JPG/PNG válido (AC-003)."""


def _extensao_permitida(nome_arquivo: str) -> bool:
    return (
        "." in nome_arquivo
        and nome_arquivo.rsplit(".", 1)[-1].lower() in EXTENSOES_PERMITIDAS
    )


def _destino_disponivel(pasta_uploads: Path, nome_arquivo: str) -> Path:
    destino = pasta_uploads / nome_arquivo
    contador = 1
    while destino.exists():
        destino = pasta_uploads / f"{destino.stem}-{contador}{destino.suffix}"
        contador += 1
    return destino


def salvar_upload(
    repositorio: RepositorioCaligrafias,
    pasta_uploads: Path,
    nome_caligrafia: str,
    nome_arquivo: str,
    conteudo: bytes,
) -> Caligrafia:
    """Valida e salva um upload (AC-003), criando a caligrafia (AC-001) ou
    acrescentando a imagem a uma já existente (AC-002)."""
    if not nome_arquivo or not _extensao_permitida(nome_arquivo):
        raise ArquivoInvalido(
            "Formato de arquivo não suportado. Envie uma imagem JPG ou PNG."
        )

    array = np.frombuffer(conteudo, dtype=np.uint8)
    imagem = cv2.imdecode(array, cv2.IMREAD_UNCHANGED) if array.size else None
    if imagem is None:
        raise ArquivoInvalido(
            "Não foi possível ler o arquivo enviado — verifique se é uma "
            "imagem JPG ou PNG válida (não corrompida)."
        )

    pasta_uploads.mkdir(parents=True, exist_ok=True)
    nome_seguro = secure_filename(nome_arquivo) or "upload"
    destino = _destino_disponivel(pasta_uploads, nome_seguro)
    destino.write_bytes(conteudo)

    return repositorio.registrar_upload(nome_caligrafia, str(destino))


def criar_blueprint_upload(
    repositorio: RepositorioCaligrafias, pasta_uploads: Path
) -> Blueprint:
    """Blueprint com o formulário (GET) e o processamento (POST) do upload."""
    blueprint = Blueprint("upload", __name__, template_folder="templates")

    @blueprint.route("/upload", methods=["GET"])
    def formulario():
        return render_template("upload.html")

    @blueprint.route("/upload", methods=["POST"])
    def enviar():
        nome_caligrafia = (request.form.get("nome") or "").strip()
        arquivo = request.files.get("arquivo")

        if not nome_caligrafia:
            return (
                render_template("upload.html", erro="Informe um nome para a caligrafia."),
                400,
            )
        if arquivo is None or arquivo.filename == "":
            return (
                render_template("upload.html", erro="Selecione um arquivo de imagem."),
                400,
            )

        try:
            caligrafia = salvar_upload(
                repositorio,
                pasta_uploads,
                nome_caligrafia,
                arquivo.filename,
                arquivo.read(),
            )
        except ArquivoInvalido as erro:
            return render_template("upload.html", erro=str(erro)), 400

        return redirect(url_for("upload.formulario", enviado=caligrafia.nome))

    return blueprint
