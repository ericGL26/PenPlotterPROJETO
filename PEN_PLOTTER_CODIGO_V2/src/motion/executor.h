#pragma once
// =============================================================================
// motion/executor.h — Task FreeRTOS no Core 0 que consome a fila de movimento
// Responsabilidade: consumir blocos da MotionQueue e executá-los via Planner.
//                   Roda em Core 0 com máxima prioridade para tempo real.
// =============================================================================

#include <Arduino.h>

class Executor {
public:
    // Cria a FreeRTOS task no Core 0
    void startTask();

    // Sinaliza pausa (o executor termina o bloco atual e aguarda)
    void pause();

    // Retoma a execução após pausa
    void resume();

    // Aborta o movimento atual e limpa a fila
    void cancel();

    // Estado
    bool isRunning() const { return _running; }
    bool isPaused()  const { return _paused; }

private:
    volatile bool _running = false;
    volatile bool _paused  = false;
    volatile bool _cancel  = false;

    // Função estática passada ao xTaskCreatePinnedToCore
    static void taskFunction(void* param);

    // Loop interno da task
    void run();
};

// Instância global
extern Executor executor;
