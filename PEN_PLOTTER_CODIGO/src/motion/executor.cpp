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

            bool ok = planner.execute(block);
            if (ok) {
                // Mantém a FSM sincronizada com a posição real alcançada pelo CoreXY
                machineState.setPosition(block.target_x, block.target_y);
            }

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
