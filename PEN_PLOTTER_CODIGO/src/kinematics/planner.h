#pragma once
// =============================================================================
// kinematics/planner.h — Planejador de movimento: Bresenham + perfil trapezoidal
// Responsabilidade: dado um bloco de movimento, calcular e executar a sequência
//                   de pulsos STEP de forma sincronizada entre os dois motores.
// =============================================================================

#include <Arduino.h>
#include "../hal/motor.h"
#include "corexy.h"

// Bloco de movimento processado pelo planner
struct MotionBlock {
    float   target_x;       // destino X em mm (coordenadas absolutas)
    float   target_y;       // destino Y em mm
    float   feed_rate;      // velocidade em mm/min
    bool    rapid;          // true = G0 (velocidade máxima), false = G1
    bool    valid;          // false = slot vazio na fila
};

class Planner {
public:
    // Inicializa referências aos dois motores físicos
    void init(Motor* motorA, Motor* motorB);

    // Executa um bloco de movimento completo (bloqueante — chamado pelo executor task)
    // Retorna true se o bloco foi executado com sucesso.
    bool execute(const MotionBlock& block);

    // Interrompe imediatamente qualquer movimento em andamento
    void abort();

    bool isAborted() const { return _aborted; }

private:
    Motor* _motorA = nullptr;
    Motor* _motorB = nullptr;
    volatile bool _aborted = false;

    // Algoritmo de Bresenham com perfil de velocidade trapezoidal
    void runBresenham(long stepsA, long stepsB, bool dirA, bool dirB,
                      float feed_mm_min);

    // Calcula o delay entre steps em µs a partir da velocidade em mm/min
    uint32_t feedToStepDelayUs(float feed_mm_min) const;
};

// Instância global
extern Planner planner;
