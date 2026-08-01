# Arquitetura de Firmware: ESP32 CoreXY Plotter

> Documento de design técnico — versão 1.0  
> Estilo de referência: GRBL / FluidNC

---

## 1. Visão Geral

Este firmware transforma um ESP32 em um controlador autossuficiente de plotter CoreXY. Todo o controle é feito via interface Web hospedada no próprio dispositivo, eliminando dependência de softwares externos como o Universal G-code Sender.

```
┌─────────────────────────────────────────────┐
│              Navegador (Cliente)             │
│         HTML + CSS + JS puro                │
└──────────────────┬──────────────────────────┘
                   │  Wi-Fi  (WebSocket / HTTP)
┌──────────────────▼──────────────────────────┐
│                  ESP32                       │
│  ┌───────────┐  ┌──────────┐  ┌──────────┐  │
│  │ WebServer │  │WebSocket │  │   OTA    │  │
│  └─────┬─────┘  └────┬─────┘  └──────────┘  │
│        │             │                       │
│  ┌─────▼─────────────▼──────────────────┐   │
│  │           Command Router              │   │
│  └──────────────────┬────────────────────┘   │
│                     │                        │
│  ┌──────────────────▼────────────────────┐   │
│  │         G-code Parser                 │   │
│  └──────────────────┬────────────────────┘   │
│                     │                        │
│  ┌──────────────────▼────────────────────┐   │
│  │           Command Queue               │   │
│  └──────────────────┬────────────────────┘   │
│                     │                        │
│  ┌──────────────────▼────────────────────┐   │
│  │      Motion Planner (Executor)        │   │
│  └──────────────────┬────────────────────┘   │
│                     │                        │
│  ┌──────────────────▼────────────────────┐   │
│  │      CoreXY Kinematics                │   │
│  └──────────────────┬────────────────────┘   │
│                     │                        │
│  ┌──────────────────▼────────────────────┐   │
│  │      Step Generator (Timer ISR)       │   │
│  └──────┬───────────────────┬────────────┘   │
│         │                   │                │
│  ┌──────▼──────┐    ┌───────▼──────┐         │
│  │  Motor A    │    │   Motor B    │         │
│  │ A4988 (X)   │    │  A4988 (Y)   │         │
│  └─────────────┘    └──────────────┘         │
└─────────────────────────────────────────────┘
```

---

## 2. Estrutura de Arquivos — Arquitetura Modular

```
PEN_PLOTTER_FIRMWARE/
├── src/
│   ├── main.cpp              ← Ponto de entrada, inicialização, loop principal
│   │
│   ├── config/
│   │   └── config.h          ← Todas as constantes de hardware e tuning
│   │
│   ├── hal/                  ← Hardware Abstraction Layer
│   │   ├── motor.h / .cpp    ← Driver A4988: STEP, DIR, ENABLE
│   │   └── servo.h / .cpp    ← Futuro: controle da caneta (placeholder)
│   │
│   ├── kinematics/
│   │   ├── corexy.h / .cpp   ← Conversão X/Y → Motor A / Motor B
│   │   └── planner.h / .cpp  ← Bresenham + controle de velocidade
│   │
│   ├── motion/
│   │   ├── queue.h / .cpp    ← Fila circular de blocos de movimento
│   │   └── executor.h / .cpp ← Executa blocos da fila via timer ISR
│   │
│   ├── gcode/
│   │   ├── parser.h / .cpp   ← Tokenizer + interpretador de G-code
│   │   └── commands.h / .cpp ← Tabela de dispatch de comandos
│   │
│   ├── comm/
│   │   ├── wifi_mgr.h / .cpp ← Conexão Wi-Fi + reconexão automática
│   │   ├── ota.h / .cpp      ← ArduinoOTA
│   │   ├── webserver.h / .cpp← Servidor HTTP (serve o HTML)
│   │   └── websocket.h / .cpp← WebSocket bidirecional + protocolo
│   │
│   ├── state/
│   │   └── machine_state.h / .cpp ← FSM central: Idle/Run/Pause/Error/Homing
│   │
│   └── utils/
│       ├── logger.h / .cpp   ← Log via WebSocket (substitui Serial)
│       └── watchdog.h / .cpp ← ESP32 hardware watchdog wrapper
│
├── data/                     ← Filesystem (LittleFS)
│   └── index.html            ← Interface Web completa (minificada)
│
├── platformio.ini            ← Configuração PlatformIO
└── partitions.csv            ← Tabela de partições customizada
```

### Por que esta estrutura é melhor que a proposta original

| Decisão | Justificativa |
|---|---|
| Pasta `hal/` separada | Isola o hardware real; facilita simulação/testes unitários |
| `kinematics/` separado de `motion/` | CoreXY é uma transformada matemática pura; o planner é execução |
| `comm/` agrupa toda comunicação | Nenhum módulo de movimento precisa saber sobre WebSocket |
| `state/` como FSM explícita | Evita flags `isRunning`, `isPaused` espalhadas — estado é uma fonte da verdade |
| `data/` com LittleFS | Permite atualizar o HTML via OTA sem reflashear o firmware |

---

## 3. Responsabilidade de Cada Módulo

### 3.1 `config.h` — Constantes Globais

**Não é uma classe. É o único header incluído por todos.**

```
Pinos de hardware (STEP, DIR, ENABLE)
Steps por mm (steps_per_mm_x, steps_per_mm_y)
Velocidade máxima (mm/min)
Aceleração máxima (mm/s²)
Tamanho da fila (QUEUE_SIZE)
Timeout do watchdog
SSID / senha Wi-Fi
Porta WebSocket
```

> [!IMPORTANT]
> `config.h` nunca inclui nenhum outro header do projeto. É o único módulo sem dependências internas.

---

### 3.2 `hal/motor` — Hardware Abstraction Layer

**Responsabilidade única:** abstrair os pinos GPIO do A4988.

```
Métodos:
  init()         → configura GPIOs como OUTPUT
  step()         → gera 1 pulso STEP (HIGH + delay + LOW)
  setDir(bool)   → define direção (HIGH/LOW no pino DIR)
  enable()       → ativa driver (LOW no ENABLE do A4988)
  disable()      → desativa driver (HIGH no ENABLE)
```

**Nunca conhece:** CoreXY, planner, G-code, WebSocket.

> [!NOTE]
> O A4988 requer que o pulso STEP tenha duração mínima de **1 µs** (HIGH) antes do LOW. Isso deve ser garantido aqui, não no planner.

---

### 3.3 `kinematics/corexy` — Cinemática CoreXY

**Responsabilidade única:** transformar coordenadas cartesianas (X, Y) em passos para os dois motores físicos (A, B).

Veja a seção completa de CoreXY no item **6**.

---

### 3.4 `kinematics/planner` — Planejador de Movimento

**Responsabilidade única:** dado um bloco de movimento (ponto inicial → ponto final + velocidade), calcular a sequência de pulsos de step e garantir sincronismo entre os dois motores.

Veja a seção completa de Planejamento no item **7**.

---

### 3.5 `motion/queue` — Fila de Comandos

**Responsabilidade única:** buffer circular de blocos de movimento entre o parser (produtor) e o executor (consumidor).

```
Estrutura de um bloco (MotionBlock):
  float target_x, target_y     ← destino em mm
  float feed_rate              ← velocidade em mm/min
  uint8_t type                 ← G0, G1, etc.

Operações:
  push(block)   → adiciona (thread-safe via mutex)
  pop(block)    → retira (chamado pelo executor)
  isFull()
  isEmpty()
  size()
```

**Por que fila circular?** Memória fixa em tempo de compilação; sem `malloc`/`free`; sem fragmentação de heap.

---

### 3.6 `motion/executor` — Executor de Movimento

**Responsabilidade única:** consumir blocos da fila e executá-los em tempo real via timer de hardware da ESP32.

- Roda em **Core 0** (dedicado ao tempo real)
- Utiliza `esp_timer` ou `timer_isr` para gerar pulsos STEP com precisão de microssegundos
- Não bloqueia o Core 1 (Wi-Fi/WebSocket)

---

### 3.7 `gcode/parser` — Parser de G-code

**Responsabilidade única:** converter uma string de texto G-code em uma estrutura de dados tipada.

```
Entrada:  "G1 X25.4 Y-10.0 F3000"
Saída:    GCodeCommand { type=G1, x=25.4, y=-10.0, f=3000 }
```

```
Processo de parsing:
1. Remover comentários (; e ())
2. Converter para maiúsculas
3. Tokenizar por letra + número
4. Extrair código principal (G, M)
5. Extrair parâmetros (X, Y, Z, F, S, P)
6. Validar range de valores
7. Retornar struct ou erro
```

**Nunca conhece:** WebSocket, Wi-Fi, motores.

---

### 3.8 `gcode/commands` — Tabela de Dispatch

**Responsabilidade única:** mapear um código G/M para uma função handler.

```
Implementação via tabela de ponteiros de função:
  handlers[G0]  = handle_G0;
  handlers[G1]  = handle_G1;
  handlers[G90] = handle_G90;
  handlers[G91] = handle_G91;
  handlers[G92] = handle_G92;
  handlers[M3]  = handle_M3;
  handlers[M5]  = handle_M5;
```

**Para adicionar um novo comando:** basta registrar um novo handler na tabela. Nenhum outro arquivo precisa ser alterado.

---

### 3.9 `comm/websocket` — Servidor WebSocket

**Responsabilidade única:** receber mensagens do navegador, encaminhar ao `CommandRouter`, e enviar respostas/status ao navegador.

Veja o protocolo completo no item **9**.

---

### 3.10 `state/machine_state` — Máquina de Estados Finita (FSM)

**Responsabilidade única:** ser a **única fonte da verdade** sobre o estado atual da máquina.

```
Estados:
  IDLE      → aguardando comandos
  RUNNING   → executando G-code
  PAUSED    → execução suspensa
  HOMING    → retorno ao zero (futuro)
  ERROR     → erro detectado
  LOCKED    → travado por erro crítico

Transições válidas:
  IDLE    → RUNNING  (comando 'run')
  RUNNING → PAUSED   (comando 'pause')
  PAUSED  → RUNNING  (comando 'resume')
  RUNNING → IDLE     (fila esvaziada)
  ANY     → ERROR    (falha detectada)
  ERROR   → IDLE     (comando 'unlock')
```

---

## 4. Regras de Dependência entre Módulos

```
                ┌─────────┐
                │ config.h│  ← Todos conhecem, mas ele não conhece ninguém
                └────┬────┘
                     │ (incluso por todos)
        ┌────────────┼─────────────┐
        ▼            ▼             ▼
   ┌─────────┐  ┌─────────┐  ┌──────────┐
   │  hal/   │  │  state/ │  │  utils/  │
   │  motor  │  │   fsm   │  │  logger  │
   └────┬────┘  └────┬────┘  └──────────┘
        │            │
        ▼            ▼
   ┌──────────────────────┐
   │  kinematics/         │
   │  corexy + planner    │
   └──────────┬───────────┘
              │
              ▼
   ┌──────────────────────┐
   │  motion/             │
   │  queue + executor    │
   └──────────┬───────────┘
              │
              ▼
   ┌──────────────────────┐
   │  gcode/              │
   │  parser + commands   │
   └──────────┬───────────┘
              │
              ▼
   ┌──────────────────────┐
   │  comm/               │
   │  websocket + server  │
   └──────────┬───────────┘
              │
              ▼
   ┌──────────────────────┐
   │  main.cpp            │
   └──────────────────────┘
```

### Regras absolutas de isolamento

| Módulo | Nunca deve conhecer |
|---|---|
| `hal/motor` | G-code, WebSocket, Wi-Fi, planner |
| `kinematics/corexy` | G-code, WebSocket, fila, estado |
| `gcode/parser` | WebSocket, motores, Wi-Fi |
| `motion/queue` | WebSocket, parser, Wi-Fi |
| `comm/websocket` | Motores, planner, cinemática |
| `state/machine_state` | WebSocket, motores, parser |

A comunicação entre camadas é sempre **unidirecional via interfaces**, nunca acoplamento direto entre módulos de camadas distintas.

---

## 5. Fluxo Completo de Execução

### 5.1 Boot

```
main.cpp::setup()
  │
  ├─ Config::load()           ← lê config.h
  ├─ Motor::init()            ← configura GPIOs
  ├─ WifiManager::connect()   ← conecta ao AP
  ├─ OTA::init()              ← inicia ArduinoOTA
  ├─ LittleFS::mount()        ← monta filesystem
  ├─ WebServer::start()       ← serve index.html
  ├─ WebSocket::start()       ← inicia WS server
  ├─ MachineState::set(IDLE)  ← estado inicial
  ├─ Queue::init()            ← inicializa fila
  ├─ Executor::startTask()    ← cria FreeRTOS task no Core 0
  └─ Watchdog::init()         ← inicia hardware watchdog
```

### 5.2 Recepção de um arquivo G-code (linha por linha)

```
Navegador
  │
  ├─ envia linha: "G1 X25.4 Y-10.0 F3000"  via WebSocket
  │
ESP32 (Core 1 — Wi-Fi)
  │
  ├─ WebSocket::onMessage()
  │   └─ CommandRouter::dispatch(msg)
  │       ├─ É comando de controle? (run/pause/resume/cancel/home/unlock)
  │       │   └─ MachineState::transition()
  │       └─ É linha G-code?
  │           └─ GCodeParser::parse(line)
  │               └─ resultado: GCodeCommand struct
  │                   └─ Commands::execute(cmd)
  │                       └─ CoreXY::translate(x, y)
  │                           └─ Queue::push(MotionBlock)
  │                               └─ WebSocket::send("ok")  ← ACK ao navegador
  │
ESP32 (Core 0 — Tempo Real)
  │
  ├─ Executor::task() ← roda continuamente
  │   └─ Queue::pop(block)
  │       └─ Planner::execute(block)
  │           ├─ Calcula sequência de steps (Bresenham)
  │           ├─ Chama Motor::step() nos momentos certos
  │           └─ Ao finalizar bloco:
  │               ├─ MachineState::updatePosition(x, y)
  │               └─ WebSocket::send("x:25.4\ny:-10.0\nprogress:35")
  │
  └─ Quando fila esvazia:
      └─ MachineState::set(IDLE)
          └─ WebSocket::send("idle")
```

### 5.3 Por que o ACK linha a linha é mais seguro que enviar o arquivo inteiro

Enviar o arquivo G-code completo de uma só vez para a ESP32:

| Risco | Impacto |
|---|---|
| ESP32 tem apenas ~300 KB de RAM livre | Arquivo grande → crash por heap overflow |
| Sem confirmação → sem detecção de erro | Uma linha mal interpretada passa silenciosamente |
| Sem controle de fluxo → buffer overflow no WebSocket | Pacotes descartados sem aviso |
| Impossível pausar/cancelar no meio | Para cancelar seria necessário fechar a conexão |
| Re-envio impossível sem re-upload | Sem rastreabilidade de qual linha falhou |

Com ACK linha a linha (modelo GRBL):

- A ESP32 **nunca recebe mais do que um comando por vez**
- O navegador **sabe exatamente qual linha está sendo executada**
- Qualquer erro retorna `error:<código>` e o envio para imediatamente
- Pause/Cancel podem ser injetados entre linhas
- Re-tentativa de uma linha específica é trivial

> [!IMPORTANT]
> Esta é exatamente a estratégia adotada pelo GRBL, Klipper (em seu streamer) e RepRap. Não é uma limitação — é uma decisão de design deliberada.

---

## 6. Cinemática CoreXY

### 6.1 O que é CoreXY

Em um plotter CoreXY, **os dois motores movem juntos** para produzir movimento em X ou Y. Não existe "Motor X" e "Motor Y" — existem Motor A e Motor B, cada um conectado a uma correia que atravessa o eixo diagonal.

### 6.2 Fórmulas de conversão

Dado um movimento cartesiano (ΔX, ΔY) desejado:

```
Motor A = ΔX + ΔY
Motor B = ΔX - ΔY
```

**Exemplos:**

| Movimento desejado | Motor A | Motor B |
|---|---|---|
| +X (direita) | +1 | +1 |
| -X (esquerda) | -1 | -1 |
| +Y (frente) | +1 | -1 |
| -Y (atrás) | -1 | +1 |
| Diagonal +X+Y | +2 | 0 (apenas A se move) |
| Diagonal +X-Y | 0 | +2 (apenas B se move) |

### 6.3 Conversão de mm para passos

```
steps_per_mm = (motor_steps × microstepping) / belt_pitch_mm × pulley_teeth

Exemplo com NEMA17 200 steps, 1/16 microstepping, correia GT2 (2mm), polia 20 dentes:
  steps_per_mm = (200 × 16) / (2 × 20) = 80 steps/mm

Para um movimento de ΔX = 10mm, ΔY = 5mm:
  ΔA_mm = 10 + 5 = 15mm  →  15 × 80 = 1200 steps para Motor A
  ΔB_mm = 10 - 5 = 5mm   →   5 × 80 =  400 steps para Motor B
```

### 6.4 Sincronização dos motores

Os dois motores **devem terminar simultaneamente**. Isto é garantido pelo algoritmo de Bresenham (veja seção 7).

O motor com **mais passos** é o "motor dominante" (eixo longo). O motor com **menos passos** tem seus pulsos distribuídos linearmente ao longo do movimento do motor dominante.

### 6.5 Direção

```cpp
// Motor A
if (delta_a > 0) Motor_A.setDir(FORWARD); else Motor_A.setDir(BACKWARD);

// Motor B  
if (delta_b > 0) Motor_B.setDir(FORWARD); else Motor_B.setDir(BACKWARD);

steps_a = abs(delta_a_steps);
steps_b = abs(delta_b_steps);
```

---

## 7. Planejamento de Movimento

### 7.1 Comparação de algoritmos

| Critério | DDA (Digital Differential Analyzer) | Bresenham | Planner GRBL |
|---|---|---|---|
| Matemática | Adição de ponto flutuante | Apenas inteiros | Inteiros + look-ahead |
| Precisão | Erros acumulativos por float | Alta precisão, sem float | Alta precisão |
| Velocidade de cálculo | Médio | Muito rápido | Lento (look-ahead) |
| RAM necessária | Baixa | Mínima | Alta (buffer de blocos) |
| Suavidade de movimento | Básica | Básica | Excelente (junction speed) |
| Complexidade de impl. | Baixa | Baixa | Alta |
| Ideal para | Prototipagem | **Esta aplicação** | Produção CNC de precisão |

### 7.2 Decisão: Bresenham com trapézio de velocidade

**Recomendação: Bresenham + perfil trapezoidal de velocidade.**

**Justificativa:**

1. **Bresenham** usa apenas aritmética inteira → nenhum erro de ponto flutuante acumulativo → precisão de 1 step garantida
2. **Trapézio de velocidade** (aceleração → velocidade constante → desaceleração) evita perda de passo nos motores
3. A complexidade é gerenciável por um desenvolvedor solo
4. Adequado para plotters (precisão > velocidade; sem fresagem que exige junction speed)
5. O look-ahead do GRBL é necessário para CNC com mudanças bruscas de direção em alta velocidade — desnecessário para um plotter a caneta

### 7.3 Bresenham 2D (dois motores)

```
Dados: steps_a, steps_b (já calculados pelo CoreXY)

dx = max(steps_a, steps_b)   ← motor dominante
dy = min(steps_a, steps_b)   ← motor subordinado

error = 2*dy - dx
step_a = 0
step_b = 0

Para cada step do motor dominante:
  Pulsa motor dominante
  step_a++

  if error >= 0:
    Pulsa motor subordinado
    step_b++
    error -= 2*dx

  error += 2*dy
```

Isso garante que o motor subordinado distribua seus passos **uniformemente** ao longo do movimento do dominante, resultando em uma linha reta perfeita.

### 7.4 Perfil Trapezoidal de Velocidade

```
Velocidade
    │
Vmax│         ┌─────────────┐
    │        /               \
    │       /                 \
    │      /    aceleração     \ desaceleração
    │─────/─────────────────────\──────
    │
    └──────────────────────────────────── Passos
         ^                       ^
      início                    fim
```

```
Parâmetros:
  v_start   = velocidade inicial (tipicamente 0 ou junction speed)
  v_max     = velocidade máxima do bloco
  v_end     = velocidade final (tipicamente 0)
  accel     = mm/s²  (definido em config.h)

Cálculo do intervalo entre steps:
  step_delay_us = 1_000_000 / (steps_per_mm × velocity_mm_s)
  
  A cada step, recalcula velocity usando:
  v² = v₀² + 2·a·d  (equação cinemática)
```

---

## 8. Gerenciamento de Tarefas FreeRTOS

```
Core 0 (PRO_CPU) — Tempo Real:
  Task: executor_task
    Prioridade: configMAX_PRIORITIES - 1  (máxima)
    Stack: 4096 bytes
    Ação: consome fila, gera pulsos STEP via timer ISR

Core 1 (APP_CPU) — Aplicação:
  Task: loop() do Arduino
    ├─ wifi_manager.handle()
    ├─ ota.handle()
    ├─ webserver.handle()
    ├─ websocket.handle()
    └─ status_broadcaster() ← envia posição/status a cada 200ms
```

> [!WARNING]
> Nunca chame funções de Wi-Fi/WebSocket de dentro do Core 0 ou de dentro de uma ISR. Isso causa panic do FreeRTOS. Use uma fila de mensagens para comunicação entre cores.

---

## 9. Protocolo WebSocket

### 9.1 ESP32 → Navegador (mensagens de status)

| Mensagem | Significado |
|---|---|
| `ok` | Linha G-code aceita e enfileirada |
| `error:<código>:<descrição>` | Linha rejeitada |
| `busy` | Fila cheia, aguarde |
| `state:idle` | Máquina parada |
| `state:running` | Executando G-code |
| `state:paused` | Em pausa |
| `state:homing` | Executando homing |
| `state:error` | Erro — aguardando unlock |
| `pos:x=25.40,y=-10.00` | Posição atual (a cada 200ms) |
| `progress:35` | Percentual do arquivo (0-100) |
| `feed:3000` | Feed rate atual em mm/min |
| `eof` | Arquivo G-code finalizado |

### 9.2 Navegador → ESP32 (comandos de controle)

| Mensagem | Ação |
|---|---|
| `cmd:run` | Inicia execução |
| `cmd:pause` | Pausa no próximo bloco seguro |
| `cmd:resume` | Retoma execução |
| `cmd:cancel` | Cancela e esvazia a fila |
| `cmd:home` | Executa homing (futuro) |
| `cmd:unlock` | Limpa estado de erro |
| `cmd:status` | Solicita status imediato |
| `gcode:<linha>` | Envia linha G-code individual |

### 9.3 Por que prefixar com `cmd:` e `gcode:`

Sem prefixo, `"G0 X10"` e `"run"` são ambíguos se o G-code tiver um comando de texto livre. O prefixo elimina ambiguidade no roteador de comandos e torna o protocolo extensível.

### 9.4 Códigos de erro

| Código | Descrição |
|---|---|
| `E01` | Comando G-code desconhecido |
| `E02` | Parâmetro fora do range |
| `E03` | Fila cheia |
| `E04` | Máquina não está em estado IDLE/RUNNING |
| `E05` | Valor de feed rate inválido |

---

## 10. Interface Web

### 10.1 Estrutura do HTML (index.html no LittleFS)

```
┌────────────────────────────────────────────┐
│  🖊️  ESP32 CoreXY Plotter                  │
│  ● Conectado  [Desconectar]                │
├────────────────────────────────────────────┤
│  ESTADO: IDLE     X: 25.40   Y: -10.00    │
│  FEED: 3000 mm/min                        │
│  ████████████░░░░░░  35%                  │
├────────────────────────────────────────────┤
│  📁 [Escolher arquivo .gcode]  arquivo.nc │
│  [▶ Executar] [⏸ Pausar] [▶ Continuar]   │
│  [⏹ Cancelar]                             │
├────────────────────────────────────────────┤
│  Console:                                  │
│  > G1 X25.4 Y-10.0 F3000                 │
│  < ok                                     │
│  > G1 X30.0 Y-5.0                        │
│  < ok                                     │
└────────────────────────────────────────────┘
```

### 10.2 Lógica JavaScript do streamer

```javascript
// Pseudocódigo do streamer linha-a-linha
let lines = [];
let currentLine = 0;
let waitingAck = false;

function sendNextLine() {
  if (waitingAck || currentLine >= lines.length) return;
  
  ws.send("gcode:" + lines[currentLine]);
  waitingAck = true;
}

ws.onmessage = function(event) {
  if (event.data === "ok") {
    waitingAck = false;
    currentLine++;
    updateProgress(currentLine / lines.length * 100);
    sendNextLine();
  }
  else if (event.data.startsWith("error:")) {
    stopStream();
    showError(event.data);
  }
  else if (event.data.startsWith("pos:")) {
    updatePositionDisplay(event.data);
  }
  else if (event.data.startsWith("state:")) {
    updateStateIndicator(event.data);
  }
};
```

---

## 11. Gerenciamento de Memória

### 11.1 O que fica em RAM (DRAM)

| Item | Tamanho estimado | Justificativa |
|---|---|---|
| Fila de MotionBlocks | ~2 KB (32 blocos × 64 bytes) | Acesso frequente pelo executor |
| Estado da máquina (FSM) | < 100 bytes | Leitura/escrita constante |
| Posição atual (X, Y) | 8 bytes (2× float) | Atualizado a cada step |
| Buffer WebSocket RX | ~256 bytes | Uma linha G-code por vez |
| Stack das tasks FreeRTOS | ~8 KB total | Obrigatório em RAM |

### 11.2 O que fica em Flash (PROGMEM / Rodata)

| Item | Justificativa |
|---|---|
| Tabela de handlers G-code | Só leitura, nunca muda |
| Strings de erro e status | Constantes de texto |
| Configurações padrão | `config.h` compilado no firmware |

Use `const char* msg PROGMEM = "ok";` para strings frequentes.

### 11.3 LittleFS vs SPIFFS vs SD

| Critério | SPIFFS | LittleFS | SD Card |
|---|---|---|---|
| Wear leveling | Não | **Sim** | Sim |
| Integridade pós-falha | Fraca | **Forte (journaling)** | Depende do FS |
| Velocidade | Médio | **Rápido** | Rápido |
| Suporte no ESP32 Arduino | Legado | **Recomendado** | Biblioteca separada |
| Armazenamento de HTML | ✅ | ✅ | ✅ |
| Arquivos G-code grandes | ❌ (limitado a ~1MB) | ❌ (limitado a ~1MB) | ✅ (GBs) |
| Complexidade de hardware | Nenhuma | Nenhuma | Adiciona pinos SPI |

**Decisão:**

- **Fase 1 (agora):** LittleFS para o `index.html` + G-code pequenos (até ~300KB). G-code recebido linha a linha via WebSocket → nunca armazenado completo na RAM.
- **Fase 2 (futuro):** SD Card para arquivos G-code grandes. O executor lê linha a linha do SD, mantendo apenas 1 bloco na RAM por vez.

> [!TIP]
> Com o modelo ACK linha a linha, **você nunca precisa armazenar o arquivo G-code completo na ESP32**. O navegador é o buffer. Isso resolve o problema de memória de forma elegante.

---

## 12. OTA (Over-The-Air Updates)

### 12.1 Estratégia de particionamento

```
# partitions.csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x1E0000   ← firmware ativo
app1,     app,  ota_1,   0x1F0000,0x1E0000   ← firmware de atualização
spiffs,   data, spiffs,  0x3D0000,0x30000    ← LittleFS (HTML)
```

### 12.2 Dois tipos de OTA

| Tipo | Quando usar |
|---|---|
| `ArduinoOTA` (UDP) | Desenvolvimento — atualiza firmware via Arduino IDE/PlatformIO |
| OTA via HTTP | Produção — atualiza firmware via endpoint REST da própria Web UI |

A atualização do `index.html` pode ser feita independentemente do firmware via upload HTTP para o LittleFS.

---

## 13. Watchdog e Tratamento de Erros

### 13.1 Hardware Watchdog

O ESP32 possui um Task Watchdog Timer (TWDT). Configurar para resetar o dispositivo se o executor ficar bloqueado por mais de N segundos:

```
Configuração sugerida:
  Timeout: 30 segundos
  Tarefa monitorada: executor_task
  Em caso de timeout: reset + log de erro no NVS
```

### 13.2 Hierarquia de erros

```
Nível 1 — Warning: linha G-code inválida
  → Retorna error:E01 ao navegador, continua executando

Nível 2 — Error: endstop atingido (futuro), feed inválido
  → MachineState::set(ERROR)
  → Para todos os motores imediatamente
  → WebSocket::send("state:error")
  → Aguarda 'cmd:unlock'

Nível 3 — Panic: stack overflow, heap corruption
  → Watchdog reseta o ESP32
  → Log salvo em NVS antes do reset
  → Ao reiniciar: WebSocket::send("state:error:panic")
```

---

## 14. Suporte Futuro

### 14.1 Servo para caneta (M3/M5)

M3 (caneta baixo) e M5 (caneta alto) já estão na tabela de dispatch. Hoje retornam `ok` sem ação. No futuro:

```
handle_M3() → Servo::write(PEN_DOWN_ANGLE)
handle_M5() → Servo::write(PEN_UP_ANGLE)
```

Nenhum outro módulo precisa ser alterado.

### 14.2 Homing (G28)

```
handle_G28():
  MachineState::set(HOMING)
  Executor::runHomingSequence()  // move até endstops
  MachineState::setPosition(0, 0)
  MachineState::set(IDLE)
```

### 14.3 Endstops

Adicionar ao `hal/`:
```
endstop.h / .cpp:
  init()        ← configura pinos com INPUT_PULLUP
  isTriggered() ← lê GPIO
  attachISR()   ← para detecção instantânea
```

O executor verifica endstops a cada step durante homing.

### 14.4 SD Card

```
Novo módulo: storage/sd_card.h / .cpp
  mount()
  openFile(path)
  readLine() → String
  close()
```

O streamer do SD funciona identicamente ao streamer WebSocket — lê uma linha, enfileira, aguarda o executor processar, lê a próxima.

---

## 15. Resumo das Decisões Técnicas

| Decisão | Escolha | Alternativa descartada | Motivo |
|---|---|---|---|
| Comunicação | WebSocket + ACK linha a linha | Serial / envio em bloco | Confiabilidade, controle de fluxo |
| Filesystem | LittleFS | SPIFFS | Wear leveling, journaling |
| Algoritmo de steps | Bresenham | DDA, GRBL planner | Precisão inteira, simplicidade |
| Perfil de velocidade | Trapézio | Step constante, S-curve | Evita skip de steps sem complexidade de S-curve |
| Multitarefa | FreeRTOS dual-core | Single-task loop() | Core 0 = RT, Core 1 = networking |
| Estado da máquina | FSM explícita | Flags booleanas | Fonte única da verdade, transições validadas |
| Armazenamento de G-code | No navegador (streaming) | SPIFFS/RAM | G-code nunca precisa caber na ESP32 |
| Atualização | ArduinoOTA + HTTP OTA | USB apenas | Atualização em campo sem hardware adicional |

---

## 16. Próximos Passos Recomendados

1. **Implementar `config.h`** — definir todos os pinos e constantes
2. **Implementar `hal/motor`** — validar no hardware real com osciloscópio
3. **Implementar `kinematics/corexy` + `planner`** — testar com movimento manual
4. **Implementar `motion/queue` + `executor`** — testar com G0 simples
5. **Implementar `gcode/parser`** — testar com arquivo G-code mínimo
6. **Implementar `comm/websocket`** — integrar protocolo
7. **Implementar `index.html`** — interface Web
8. **Integração completa** — testes end-to-end
9. **OTA** — adicionar após validação
10. **Servo + Endstops** — expansão futura
