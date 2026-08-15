"""App factory do banco de caligrafias e do gerador de texto (T-007, T-012).

Registra os blueprints de upload (T-004, US-001), extração (T-005,
US-002/US-003), consulta (T-006, US-004) e geração de texto (T-011,
US-005/US-008) numa única aplicação Flask, todos compartilhando o mesmo
`RepositorioCaligrafias` — é o que faz uma imagem enviada em `/upload`
aparecer disponível para `/extracao/<nome>`, as variantes confirmadas lá
aparecerem na cobertura de `/consulta`, e essas mesmas variantes poderem
ser usadas para escrever um texto novo em `/gerar`.
"""

from __future__ import annotations

from pathlib import Path

from flask import Flask, redirect, url_for

from caligrafias.repositorio import RepositorioCaligrafias
from web.rotas_consulta import criar_blueprint_consulta
from web.rotas_extracao import criar_blueprint_extracao
from web.rotas_gerar import criar_blueprint_gerar
from web.rotas_upload import criar_blueprint_upload

RAIZ_DADOS_PADRAO = Path("dados/caligrafias")
PASTA_UPLOADS_PADRAO = Path("dados/uploads")
PASTA_SAIDAS_PADRAO = Path("dados/saidas")


def criar_app(
    raiz_dados: Path | str = RAIZ_DADOS_PADRAO,
    pasta_uploads: Path | str = PASTA_UPLOADS_PADRAO,
    pasta_saidas: Path | str = PASTA_SAIDAS_PADRAO,
) -> Flask:
    """Monta a aplicação com as quatro telas ligadas ao mesmo repositório."""
    repositorio = RepositorioCaligrafias(raiz_dados)

    app = Flask(__name__, template_folder="templates", static_folder="static")
    app.register_blueprint(criar_blueprint_upload(repositorio, Path(pasta_uploads)))
    app.register_blueprint(criar_blueprint_extracao(repositorio))
    app.register_blueprint(criar_blueprint_consulta(repositorio))
    app.register_blueprint(criar_blueprint_gerar(repositorio, Path(pasta_saidas)))

    @app.route("/")
    def indice():
        return redirect(url_for("consulta.lista"))

    return app


if __name__ == "__main__":
    criar_app().run(debug=True)
