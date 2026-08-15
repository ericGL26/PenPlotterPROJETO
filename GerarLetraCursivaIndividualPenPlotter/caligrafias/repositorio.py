"""Repositório de dados das caligrafias.

Armazenamento em arquivos locais (ASM-005): uma pasta por caligrafia em
`<raiz>/<nome>/metadados.json`. Não há SQLite nem banco externo.
"""

from __future__ import annotations

import json
from pathlib import Path

from caligrafias.modelos import Caligrafia, LetraVariante

NOME_ARQUIVO_METADADOS = "metadados.json"


class CaligrafiaNaoEncontrada(Exception):
    """Nenhuma caligrafia salva com o nome informado."""


class VarianteNaoEncontrada(Exception):
    """Nenhuma variante com o id informado para a letra informada."""


class RepositorioCaligrafias:
    def __init__(self, raiz: Path | str):
        self.raiz = Path(raiz)
        self.raiz.mkdir(parents=True, exist_ok=True)

    def _pasta(self, nome: str) -> Path:
        return self.raiz / nome

    def _arquivo_metadados(self, nome: str) -> Path:
        return self._pasta(nome) / NOME_ARQUIVO_METADADOS

    def existe(self, nome: str) -> bool:
        return self._arquivo_metadados(nome).exists()

    def carregar(self, nome: str) -> Caligrafia:
        arquivo = self._arquivo_metadados(nome)
        if not arquivo.exists():
            raise CaligrafiaNaoEncontrada(nome)
        dados = json.loads(arquivo.read_text(encoding="utf-8"))
        return Caligrafia.from_dict(dados)

    def salvar(self, caligrafia: Caligrafia) -> None:
        pasta = self._pasta(caligrafia.nome)
        pasta.mkdir(parents=True, exist_ok=True)
        arquivo = self._arquivo_metadados(caligrafia.nome)
        arquivo.write_text(
            json.dumps(caligrafia.to_dict(), ensure_ascii=False, indent=2),
            encoding="utf-8",
        )

    def listar(self) -> list[Caligrafia]:
        """Todas as caligrafias salvas, com sua cobertura por letra (AC-010)."""
        if not self.raiz.exists():
            return []
        nomes = sorted(
            pasta.name
            for pasta in self.raiz.iterdir()
            if pasta.is_dir() and (pasta / NOME_ARQUIVO_METADADOS).exists()
        )
        return [self.carregar(nome) for nome in nomes]

    def registrar_upload(self, nome: str, caminho_imagem: str) -> Caligrafia:
        """Cria uma caligrafia nova (AC-001) ou acrescenta a imagem a uma já
        existente, sem duplicar nem sobrescrever o que já existia (AC-002,
        ASM-001)."""
        if self.existe(nome):
            caligrafia = self.carregar(nome)
        else:
            caligrafia = Caligrafia(nome=nome)
        if caminho_imagem not in caligrafia.imagens:
            caligrafia.imagens.append(caminho_imagem)
        self.salvar(caligrafia)
        return caligrafia

    def adicionar_variante(self, nome: str, variante: LetraVariante) -> Caligrafia:
        caligrafia = self.carregar(nome)
        caligrafia.variantes.setdefault(variante.letra, []).append(variante)
        self.salvar(caligrafia)
        return caligrafia

    def excluir_variante(self, nome: str, letra: str, variante_id: str) -> Caligrafia:
        """Remove permanentemente uma variante salva por engano (AC-012)."""
        caligrafia = self.carregar(nome)
        variantes = caligrafia.variantes.get(letra, [])
        restantes = [v for v in variantes if v.id != variante_id]
        if len(restantes) == len(variantes):
            raise VarianteNaoEncontrada(f"{nome}/{letra}/{variante_id}")
        caligrafia.variantes[letra] = restantes
        self.salvar(caligrafia)
        return caligrafia
