"""Normalização e validação de texto (T-008, US-005).

AC-014: maiúsculas viram minúsculas automaticamente, sem bloquear.
AC-016: bloqueia se houver caractere fora do conjunto suportado.
AC-015: bloqueia se a caligrafia escolhida não tiver variante de alguma
letra usada no texto (só é checado depois de AC-016 passar).

Espaço e quebra de linha não são "letras" — servem de separador de
palavra/linha (US-007) e nunca são bloqueados por AC-016.
"""

from __future__ import annotations

from caligrafias.modelos import CONJUNTO_LETRAS, Caligrafia

_SEPARADORES = {" ", "\n", "\t"}
_LETRAS_SUPORTADAS = set(CONJUNTO_LETRAS)


class TextoInvalido(Exception):
    """Texto contém caractere fora do conjunto suportado (AC-016)."""

    def __init__(self, caracteres: list[str]):
        self.caracteres = caracteres
        super().__init__("caracteres não suportados: " + ", ".join(map(repr, caracteres)))


class CoberturaInsuficiente(Exception):
    """A caligrafia não tem nenhuma variante para alguma letra usada (AC-015)."""

    def __init__(self, letras: list[str]):
        self.letras = letras
        super().__init__("letras sem variante na caligrafia: " + ", ".join(letras))


def normalizar(texto: str) -> str:
    """Converte maiúsculas em minúsculas (AC-014)."""
    return texto.lower()


def caracteres_invalidos(texto_normalizado: str) -> list[str]:
    """Caracteres (já em minúsculas) fora do conjunto suportado, na ordem
    da primeira ocorrência, sem repetição (AC-016)."""
    permitidos = _LETRAS_SUPORTADAS | _SEPARADORES
    vistos: list[str] = []
    for c in texto_normalizado:
        if c not in permitidos and c not in vistos:
            vistos.append(c)
    return vistos


def letras_sem_variante(texto_normalizado: str, caligrafia: Caligrafia) -> list[str]:
    """Letras do texto (já validado por AC-016) sem nenhuma variante
    capturada na caligrafia (AC-015)."""
    usadas = {c for c in texto_normalizado if c in _LETRAS_SUPORTADAS}
    return [letra for letra in CONJUNTO_LETRAS if letra in usadas and not caligrafia.variantes.get(letra)]


def normalizar_e_validar(texto: str, caligrafia: Caligrafia) -> str:
    """Fluxo completo: normaliza, valida caracteres suportados (AC-016) e
    a cobertura da caligrafia (AC-015), nessa ordem. Devolve o texto
    normalizado se passar nas duas checagens; senão lança TextoInvalido ou
    CoberturaInsuficiente."""
    normalizado = normalizar(texto)
    invalidos = caracteres_invalidos(normalizado)
    if invalidos:
        raise TextoInvalido(invalidos)
    faltando = letras_sem_variante(normalizado, caligrafia)
    if faltando:
        raise CoberturaInsuficiente(faltando)
    return normalizado
