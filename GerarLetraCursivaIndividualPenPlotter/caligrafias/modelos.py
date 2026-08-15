"""Modelos de dados do banco de caligrafias (feature banco-caligrafias)."""

from __future__ import annotations

from dataclasses import asdict, dataclass, field

# Conjunto suportado: letras minúsculas do alfabeto português, com ou sem
# acento/cedilha (fora de escopo: maiúsculas, números, pontuação).
CONJUNTO_LETRAS: tuple[str, ...] = (
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n",
    "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "á", "à", "â", "ã", "é", "ê", "í", "ó", "ô", "õ", "ú", "ç",
)


@dataclass
class Regiao:
    """Um retângulo de recorte dentro de uma imagem de origem."""

    x: int
    y: int
    largura: int
    altura: int

    def to_dict(self) -> dict:
        return asdict(self)

    @staticmethod
    def from_dict(dados: dict) -> "Regiao":
        return Regiao(
            x=dados["x"],
            y=dados["y"],
            largura=dados["largura"],
            altura=dados["altura"],
        )


@dataclass
class LetraVariante:
    """Uma amostra vetorial de uma letra, capturada de uma região confirmada."""

    id: str
    letra: str
    pontos: list[list[float]]
    entrada: list[float]
    saida: list[float]
    imagem_origem: str
    regiao: Regiao

    def to_dict(self) -> dict:
        return {
            "id": self.id,
            "letra": self.letra,
            "pontos": [list(p) for p in self.pontos],
            "entrada": list(self.entrada),
            "saida": list(self.saida),
            "imagem_origem": self.imagem_origem,
            "regiao": self.regiao.to_dict(),
        }

    @staticmethod
    def from_dict(dados: dict) -> "LetraVariante":
        return LetraVariante(
            id=dados["id"],
            letra=dados["letra"],
            pontos=[list(p) for p in dados["pontos"]],
            entrada=list(dados["entrada"]),
            saida=list(dados["saida"]),
            imagem_origem=dados["imagem_origem"],
            regiao=Regiao.from_dict(dados["regiao"]),
        )


@dataclass
class Caligrafia:
    """Uma caligrafia salva: um nome, as imagens enviadas e as variantes
    de letra já capturadas, agrupadas por letra."""

    nome: str
    imagens: list[str] = field(default_factory=list)
    variantes: dict[str, list[LetraVariante]] = field(default_factory=dict)

    def cobertura(self) -> dict[str, int]:
        """Quantas variantes existem para cada letra do conjunto suportado (AC-010)."""
        return {letra: len(self.variantes.get(letra, [])) for letra in CONJUNTO_LETRAS}

    def letras_faltando(self) -> list[str]:
        """Letras do conjunto suportado sem nenhuma variante (AC-011)."""
        return [letra for letra in CONJUNTO_LETRAS if not self.variantes.get(letra)]

    def to_dict(self) -> dict:
        return {
            "nome": self.nome,
            "imagens": list(self.imagens),
            "variantes": {
                letra: [variante.to_dict() for variante in lista]
                for letra, lista in self.variantes.items()
            },
        }

    @staticmethod
    def from_dict(dados: dict) -> "Caligrafia":
        variantes = {
            letra: [LetraVariante.from_dict(v) for v in lista]
            for letra, lista in dados.get("variantes", {}).items()
        }
        return Caligrafia(
            nome=dados["nome"],
            imagens=list(dados.get("imagens", [])),
            variantes=variantes,
        )
