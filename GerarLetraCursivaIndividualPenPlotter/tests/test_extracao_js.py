"""Testes da lógica de estado do canvas de extração (T-005), em Node.

`web/static/extracao.js` roda no navegador, mas exporta funções puras de
estado via `module.exports` quando carregado fora de um DOM — usamos isso
para testar diretamente com Node, sem precisar de um navegador.
"""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

import pytest

CAMINHO_JS = Path(__file__).resolve().parent.parent / "web" / "static" / "extracao.js"


def _rodar_node(script: str):
    node = shutil.which("node")
    if node is None:
        pytest.skip("node não está disponível neste ambiente")

    resultado = subprocess.run(
        [node, "-e", script],
        capture_output=True,
        text=True,
        timeout=30,
    )
    assert resultado.returncode == 0, resultado.stderr
    return json.loads(resultado.stdout)


def test_descartar_regiao_some_da_lista_de_pendencias():
    """@spec:AC-009 descartar uma região sugerida a remove da lista de pendências e não gera nenhum dado a salvar."""
    script = f"""
    const {{ criarEstadoRegioes, descartarRegiao, regioesPendentes }} = require({json.dumps(str(CAMINHO_JS))});

    let estado = criarEstadoRegioes([
      {{ x: 10, y: 10, largura: 20, altura: 20 }},
      {{ x: 50, y: 50, largura: 20, altura: 20 }},
    ]);
    estado = descartarRegiao(estado, 0);

    const pendentes = regioesPendentes(estado);
    console.log(JSON.stringify({{
      total: estado.length,
      pendentesIds: pendentes.map((item) => item.id),
    }}));
    """
    resultado = _rodar_node(script)

    assert resultado["total"] == 2
    assert resultado["pendentesIds"] == [1]


def test_ajustar_regiao_altera_o_retangulo_usado():
    """@spec:AC-006 ajustar as bordas de uma região sugerida troca o retângulo guardado no estado, não o original."""
    script = f"""
    const {{ criarEstadoRegioes, ajustarRegiao }} = require({json.dumps(str(CAMINHO_JS))});

    let estado = criarEstadoRegioes([{{ x: 10, y: 10, largura: 20, altura: 20 }}]);
    estado = ajustarRegiao(estado, 0, {{ x: 15, largura: 40 }});

    console.log(JSON.stringify(estado[0].regiao));
    """
    resultado = _rodar_node(script)

    assert resultado == {"x": 15, "y": 10, "largura": 40, "altura": 20}


def test_regiao_manual_e_adicionada_quando_nao_ha_sugestoes():
    """@spec:AC-005 desenhar manualmente uma região quando não há sugestões a adiciona como pendente."""
    script = f"""
    const {{ criarEstadoRegioes, adicionarRegiaoManual, regioesPendentes }} = require({json.dumps(str(CAMINHO_JS))});

    let estado = criarEstadoRegioes([]);
    estado = adicionarRegiaoManual(estado, {{ x: 5, y: 5, largura: 30, altura: 30 }});

    console.log(JSON.stringify({{
      total: estado.length,
      pendentes: regioesPendentes(estado).length,
      origem: estado[0].origem,
    }}));
    """
    resultado = _rodar_node(script)

    assert resultado == {"total": 1, "pendentes": 1, "origem": "manual"}
