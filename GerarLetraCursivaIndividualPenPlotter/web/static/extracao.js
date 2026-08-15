/*
 * Canvas da tela de extração (T-005).
 *
 * As funções de estado abaixo são puras (sem DOM) de propósito: cobrem o
 * ajuste de retângulo (AC-006), o desenho manual quando não há sugestões
 * (AC-005) e o descarte de uma região que some da lista de pendências sem
 * salvar nada (AC-009). São exportadas via `module.exports` para serem
 * testadas com Node, e usadas pela parte de DOM mais abaixo no navegador.
 */

function criarEstadoRegioes(regioes) {
  return regioes.map((regiao, indice) => ({
    id: indice,
    regiao: { ...regiao },
    origem: "sugerida",
    status: "pendente",
  }));
}

function adicionarRegiaoManual(estado, regiao) {
  const proximoId =
    estado.reduce((maior, item) => Math.max(maior, item.id), -1) + 1;
  return [
    ...estado,
    { id: proximoId, regiao: { ...regiao }, origem: "manual", status: "pendente" },
  ];
}

function ajustarRegiao(estado, id, novoRetangulo) {
  return estado.map((item) =>
    item.id === id ? { ...item, regiao: { ...item.regiao, ...novoRetangulo } } : item
  );
}

function descartarRegiao(estado, id) {
  return estado.map((item) =>
    item.id === id ? { ...item, status: "descartada" } : item
  );
}

function confirmarRegiaoLocal(estado, id) {
  return estado.map((item) =>
    item.id === id ? { ...item, status: "confirmada" } : item
  );
}

function regioesPendentes(estado) {
  return estado.filter((item) => item.status === "pendente");
}

if (typeof module !== "undefined" && module.exports) {
  module.exports = {
    criarEstadoRegioes,
    adicionarRegiaoManual,
    ajustarRegiao,
    descartarRegiao,
    confirmarRegiaoLocal,
    regioesPendentes,
  };
}

if (typeof document !== "undefined") {
  (function iniciarTelaExtracao() {
    const corpo = document.body;
    const regioesIniciais = JSON.parse(corpo.dataset.regioes || "[]");
    const letrasSuportadas = JSON.parse(corpo.dataset.letrasSuportadas || "[]");
    const urlConfirmar = corpo.dataset.confirmarUrl;
    const indiceImagem = corpo.dataset.indiceImagem;

    let estado = criarEstadoRegioes(regioesIniciais);

    const listaPendencias = document.getElementById("lista-pendencias");
    const canvas = document.getElementById("canvas-extracao");

    function renderizarLista() {
      if (!listaPendencias) return;
      listaPendencias.innerHTML = "";
      regioesPendentes(estado).forEach((item) => {
        const linha = document.createElement("li");
        linha.dataset.regiaoId = String(item.id);
        linha.textContent = `x:${item.regiao.x} y:${item.regiao.y} `;

        const campoLetra = document.createElement("input");
        campoLetra.maxLength = 1;
        campoLetra.className = "campo-letra";

        const botaoConfirmar = document.createElement("button");
        botaoConfirmar.textContent = "Confirmar";
        botaoConfirmar.addEventListener("click", () => confirmar(item, campoLetra.value));

        const botaoDescartar = document.createElement("button");
        botaoDescartar.textContent = "Descartar";
        botaoDescartar.addEventListener("click", () => descartar(item.id));

        linha.append(campoLetra, botaoConfirmar, botaoDescartar);
        listaPendencias.appendChild(linha);
      });
    }

    function descartar(id) {
      estado = descartarRegiao(estado, id);
      renderizarLista();
    }

    function confirmar(item, letra) {
      if (!letrasSuportadas.includes(letra)) {
        window.alert(
          "Caractere não suportado. Letras aceitas: " + letrasSuportadas.join(", ")
        );
        return;
      }

      fetch(urlConfirmar, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({
          imagem: indiceImagem,
          letra,
          x: item.regiao.x,
          y: item.regiao.y,
          largura: item.regiao.largura,
          altura: item.regiao.altura,
        }),
      })
        .then((resposta) => resposta.json())
        .then((dados) => {
          if (dados.erro) {
            window.alert(dados.erro);
            return;
          }
          estado = confirmarRegiaoLocal(estado, item.id);
          renderizarLista();
        });
    }

    if (canvas) {
      let arrastando = null;

      canvas.addEventListener("mousedown", (evento) => {
        arrastando = { x: evento.offsetX, y: evento.offsetY };
      });

      canvas.addEventListener("mouseup", (evento) => {
        if (!arrastando) return;
        const regiao = {
          x: Math.min(arrastando.x, evento.offsetX),
          y: Math.min(arrastando.y, evento.offsetY),
          largura: Math.abs(evento.offsetX - arrastando.x),
          altura: Math.abs(evento.offsetY - arrastando.y),
        };
        arrastando = null;
        if (regiao.largura > 0 && regiao.altura > 0) {
          estado = adicionarRegiaoManual(estado, regiao);
          renderizarLista();
        }
      });
    }

    renderizarLista();
  })();
}
