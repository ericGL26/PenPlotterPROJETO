"""Faz as tags @spec:AC-xxx / @principle:P-xxx do docstring do teste
aparecerem no título TAP (pytest-tap só usa path::nome por padrão) —
é isso que o onp-spec verify lê para provar cada critério de aceite."""

from __future__ import annotations

import re

import pytest

_TAG_RE = re.compile(r"@spec:AC-\d{3,}|@principle:P-\d{3,}")


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_makereport(item, call):
    outcome = yield
    report = outcome.get_result()
    doc = getattr(getattr(item, "obj", None), "__doc__", None)
    if not doc:
        return
    tags = " ".join(_TAG_RE.findall(doc))
    if not tags:
        return
    path, lineno, name = report.location
    report.location = (path, lineno, f"{name} {tags}")
