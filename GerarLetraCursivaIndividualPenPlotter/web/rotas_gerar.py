"""Tela e rota de geração de texto na caligrafia escolhida (T-011).

Integra normalizacao.py (T-008), layout.py (T-009) e render.py (T-010):
valida o texto contra o conjunto suportado e a cobertura da caligrafia
(AC-014/AC-015/AC-016), monta a página quebrando linhas (AC-019/AC-020)
com ligaduras e sorteio de variante (AC-017/AC-018), e disponibiliza o
PNG resultante para download (AC-013, AC-021) — junto com o aviso de
truncamento (AC-020), quando houver.
"""

from __future__ import annotations

import uuid
from pathlib import Path

from flask import Blueprint, render_template, request, send_from_directory

from caligrafias.modelos import LetraVariante
from caligrafias.repositorio import CaligrafiaNaoEncontrada, RepositorioCaligrafias
from geracao.layout import montar_layout
from geracao.normalizacao import CoberturaInsuficiente, TextoInvalido, normalizar_e_validar
from geracao.render import ESPACO_PALAVRA_PX, desenhar_pagina, largura_variante_renderizada

ALTURA_LINHA_PX = 90.0


def _largura_letra_media_px(variantes_por_letra: dict[str, list[LetraVariante]]) -> float:
    """Estimativa de largura por letra usada só para decidir a quebra de
    linha (AC-019) — já na mesma escala normalizada que a renderização
    (T-010) usa de verdade, então é uma boa aproximação da largura real
    média, não a largura exata de cada letra específica do texto."""
    larguras = [
        largura_variante_renderizada(variante)
        for variantes in variantes_por_letra.values()
        for variante in variantes
    ]
    return sum(larguras) / len(larguras) if larguras else ALTURA_LINHA_PX / 2


def gerar_imagem(repositorio: RepositorioCaligrafias, nome_caligrafia: str, texto: str):
    """Fluxo completo: valida (AC-014/015/016), quebra em linhas
    (AC-019/020) e desenha a página (AC-017/018). Devolve a imagem PIL
    pronta pra salvar e o texto truncado (None se coube tudo)."""
    caligrafia = repositorio.carregar(nome_caligrafia)
    normalizado = normalizar_e_validar(texto, caligrafia)

    largura_letra_px = _largura_letra_media_px(caligrafia.variantes)
    layout = montar_layout(
        normalizado,
        largura_letra_px=largura_letra_px,
        largura_espaco_px=ESPACO_PALAVRA_PX,
        altura_linha_px=ALTURA_LINHA_PX,
    )

    imagem = desenhar_pagina(layout.linhas, caligrafia.variantes)
    return imagem, layout.texto_truncado


def criar_blueprint_gerar(repositorio: RepositorioCaligrafias, pasta_saida: Path) -> Blueprint:
    """Blueprint com o formulário (GET), a geração (POST) e o download do
    PNG já gerado (GET /gerar/baixar/<arquivo>)."""
    # Resolvido para absoluto: `send_from_directory` trata um diretório
    # relativo como relativo ao root_path do Flask (a pasta de web/app.py),
    # não ao diretório de trabalho do processo — sem isso, o arquivo é
    # salvo num lugar e procurado em outro, e o download vira 404.
    pasta_saida = Path(pasta_saida).resolve()
    blueprint = Blueprint("gerar", __name__, template_folder="templates")

    @blueprint.route("/gerar", methods=["GET"])
    def formulario():
        caligrafias = repositorio.listar()
        return render_template("gerar.html", caligrafias=caligrafias)

    @blueprint.route("/gerar", methods=["POST"])
    def enviar():
        nome_caligrafia = (request.form.get("caligrafia") or "").strip()
        texto = request.form.get("texto") or ""
        caligrafias = repositorio.listar()

        if not nome_caligrafia:
            return render_template("gerar.html", caligrafias=caligrafias, erro="Escolha uma caligrafia."), 400
        if not texto.strip():
            return render_template("gerar.html", caligrafias=caligrafias, erro="Digite um texto."), 400

        try:
            imagem, texto_truncado = gerar_imagem(repositorio, nome_caligrafia, texto)
        except CaligrafiaNaoEncontrada:
            return render_template("gerar.html", caligrafias=caligrafias, erro="Caligrafia não encontrada."), 400
        except TextoInvalido as erro:
            mensagem = "Texto tem caracteres não suportados: " + ", ".join(repr(c) for c in erro.caracteres)
            return render_template("gerar.html", caligrafias=caligrafias, erro=mensagem), 400
        except CoberturaInsuficiente as erro:
            mensagem = "A caligrafia não tem variante para: " + ", ".join(erro.letras)
            return render_template("gerar.html", caligrafias=caligrafias, erro=mensagem), 400

        pasta_saida.mkdir(parents=True, exist_ok=True)
        nome_arquivo = f"{nome_caligrafia}-{uuid.uuid4().hex[:8]}.png"
        imagem.save(pasta_saida / nome_arquivo, format="PNG")

        return render_template(
            "gerar.html",
            caligrafias=caligrafias,
            imagem_gerada=nome_arquivo,
            aviso_truncamento=texto_truncado,
        )

    @blueprint.route("/gerar/baixar/<nome_arquivo>", methods=["GET"])
    def baixar(nome_arquivo):
        return send_from_directory(pasta_saida, nome_arquivo, as_attachment=True)

    return blueprint
