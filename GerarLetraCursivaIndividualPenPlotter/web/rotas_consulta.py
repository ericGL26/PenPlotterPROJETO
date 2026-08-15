"""Tela e rota de consulta do banco de caligrafias (T-006).

Usa `caligrafias/repositorio.py` (T-001) para listar as caligrafias com sua
cobertura por letra (AC-010), destacar as letras sem nenhuma variante
(AC-011) e excluir uma variante salva por engano (AC-012).
"""

from __future__ import annotations

from flask import Blueprint, redirect, render_template, url_for

from caligrafias.modelos import CONJUNTO_LETRAS
from caligrafias.repositorio import RepositorioCaligrafias, VarianteNaoEncontrada


def criar_blueprint_consulta(repositorio: RepositorioCaligrafias) -> Blueprint:
    """Blueprint com a lista de caligrafias (GET), os detalhes de uma
    caligrafia (GET) e a exclusão de uma variante (POST)."""
    blueprint = Blueprint("consulta", __name__, template_folder="templates")

    @blueprint.route("/consulta", methods=["GET"])
    def lista():
        caligrafias = repositorio.listar()
        return render_template(
            "consulta.html",
            caligrafias=caligrafias,
            letras_suportadas=CONJUNTO_LETRAS,
        )

    @blueprint.route("/consulta/<nome>", methods=["GET"])
    def detalhes(nome):
        caligrafia = repositorio.carregar(nome)
        return render_template(
            "consulta.html",
            caligrafia=caligrafia,
            cobertura=caligrafia.cobertura(),
            letras_faltando=set(caligrafia.letras_faltando()),
            letras_suportadas=CONJUNTO_LETRAS,
        )

    @blueprint.route("/consulta/<nome>/<letra>/<variante_id>/excluir", methods=["POST"])
    def excluir(nome, letra, variante_id):
        try:
            repositorio.excluir_variante(nome, letra, variante_id)
        except VarianteNaoEncontrada:
            pass
        return redirect(url_for("consulta.detalhes", nome=nome))

    return blueprint
