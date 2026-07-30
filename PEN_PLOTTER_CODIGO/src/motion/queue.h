#pragma once
// =============================================================================
// motion/queue.h — Fila circular de blocos de movimento (thread-safe)
// Responsabilidade: buffer entre o produtor (parser/WebSocket) e o
//                   consumidor (executor task no Core 0).
// NUNCA conhece: WebSocket, Wi-Fi, G-code.
// =============================================================================

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "../config/config.h"
#include "../kinematics/planner.h"

class MotionQueue {
public:
    // Inicializa a fila e cria o mutex FreeRTOS
    void init();

    // Enfileira um bloco. Retorna true se bem-sucedido, false se fila cheia.
    // Thread-safe: pode ser chamado do Core 1.
    bool push(const MotionBlock& block);

    // Remove e retorna o próximo bloco. Retorna false se fila vazia.
    // Thread-safe: pode ser chamado do Core 0.
    bool pop(MotionBlock& block);

    // Remove todos os blocos da fila (para cancel)
    void clear();

    // Consultas de estado
    bool isEmpty() const;
    bool isFull()  const;
    int  count()   const;

    // Progresso: retorna o percentual de blocos já consumidos
    // (útil para a barra de progresso da UI)
    float progressPercent() const;

    // Informa o total de blocos do arquivo atual (para cálculo de progresso)
    void setTotalBlocks(int total);

private:
    static const int SIZE = QUEUE_SIZE;

    MotionBlock  _buffer[QUEUE_SIZE];
    volatile int _head = 0;   // próximo índice para pop
    volatile int _tail = 0;   // próximo índice para push
    volatile int _count = 0;

    int          _totalBlocks = 0;
    int          _consumedBlocks = 0;

    SemaphoreHandle_t _mutex = nullptr;

    // Helpers internos (NÃO thread-safe — usar dentro do mutex)
    bool _isEmpty() const { return _count == 0; }
    bool _isFull()  const { return _count >= SIZE; }
};

// Instância global
extern MotionQueue motionQueue;
