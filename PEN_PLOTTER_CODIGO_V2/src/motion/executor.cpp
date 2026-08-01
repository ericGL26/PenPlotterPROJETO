// =============================================================================
// motion/executor.cpp — Task FreeRTOS no Core 0
//
// REGRA CRÍTICA: NUNCA chamar funções de Wi-Fi/WebSocket daqui.
//               A comunicação com o Core 1 é feita via variáveis voláteis
//               e a queue de movimento (thread-safe).
// =============================================================================

#include "executor.h"
#include "queue.h"
#include "../kinematics/planner.h"
#include "../kinematics/corexy.h"
#include "../state/machine_state.h"
#include "../utils/watchdog.h"
#include "../config/config.h"

// Instância global
Executor executor;

// -----------------------------------------------------------------------------
void Executor::startTask() {
    xTaskCreatePinnedToCore(
        taskFunction,           // função da task
        "ExecutorTask",         // nome
        4096,                   // stack em bytes
        this,                   // parâmetro (ponteiro para esta instância)
        configMAX_PRIORITIES - 1, // prioridade máxima
        nullptr,                // handle (não precisamos guardar)
        0                       // Core 0 = PRO_CPU (tempo real)
    );
}

// -----------------------------------------------------------------------------
// Wrapper estático exigido pelo FreeRTOS
void Executor::taskFunction(void* param) {
    static_cast<Executor*>(param)->run();
    vTaskDelete(nullptr); // nunca deve chegar aqui
}

// -----------------------------------------------------------------------------
void Executor::run() {
    MotionBlock block;

    // Inscreve esta task no Task Watchdog Timer (watchdog.init() já rodou no setup)
    watchdog.subscribe();

    for (;;) {
        watchdog.feed();

        // Se cancelamento pedido, limpa tudo e vai para Idle
        if (_cancel) {
            _cancel  = false;
            _running = false;
            _paused  = false;
            planner.abort();
            motionQueue.clear();
            // Os blocos ainda não executados foram descartados — a posição lógica
            // (usada pelo dispatcher para resolver a próxima linha/arco) precisa
            // voltar a bater com a posição física real do CoreXY.
            machineState.setPosition(corexy.getX(), corexy.getY());
            machineState.setState(MachineState::IDLE);
        }

        // Aguarda saída de pausa
        while (_paused) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }

        // Tenta consumir um bloco da fila
        if (motionQueue.pop(block)) {
            _running = true;
            machineState.setState(MachineState::RUNNING);

            // A posição lógica (machineState) já foi atualizada no dispatch,
            // no momento em que o comando foi aceito — não aqui. A posição
            // física real (para status/UI) é lida diretamente do CoreXY.
            planner.execute(block);

            // Se a fila esvaziou após este bloco → Idle
            if (motionQueue.isEmpty()) {
                _running = false;
                machineState.setState(MachineState::IDLE);
            }
        } else {
            // Fila vazia — yield para não consumir CPU inutilmente
            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
}

// -----------------------------------------------------------------------------
void Executor::pause() {
    _paused = true;
    machineState.setState(MachineState::PAUSED);
}

void Executor::resume() {
    _paused = false;
    machineState.setState(MachineState::RUNNING);
}

void Executor::cancel() {
    _cancel = true; // processado no início do próximo loop
}
