// =============================================================================
// kinematics/corexy.cpp — Implementação da cinemática CoreXY
//
// Fórmulas CoreXY:
//   Motor A = ΔX + ΔY  (passos)
//   Motor B = ΔX - ΔY  (passos)
//
// Exemplos:
//   Movimento +X puro:        A = +steps,  B = +steps  (ambos giram igual)
//   Movimento +Y puro:        A = +steps,  B = -steps  (giram em sentidos opostos)
//   Diagonal +X+Y:            A = +2×steps, B = 0      (só A se move)
// =============================================================================

#include "corexy.h"
#include "../config/config.h"

// Instância global
CoreXY corexy;

void CoreXY::init() {
    _pos_x = 0.0f;
    _pos_y = 0.0f;
}

CoreXYSteps CoreXY::translate(float target_x, float target_y) {
    // Deslocamento em mm a partir da posição atual
    float delta_x_mm = target_x - _pos_x;
    float delta_y_mm = target_y - _pos_y;

    // Converte deslocamento de mm para passos (inteiro, arredondamento correto)
    long delta_x_steps = lroundf(delta_x_mm * STEPS_PER_MM);
    long delta_y_steps = lroundf(delta_y_mm * STEPS_PER_MM);

    // Aplicar transformada CoreXY
    // Motor A gira na soma dos deltas
    // Motor B gira na diferença dos deltas
    long raw_a = delta_x_steps + delta_y_steps;
    long raw_b = delta_x_steps - delta_y_steps;

    CoreXYSteps result;
    result.dirA   = (raw_a >= 0);
    result.dirB   = (raw_b >= 0);
    result.stepsA = labs(raw_a);
    result.stepsB = labs(raw_b);

    return result;
}

void CoreXY::setPosition(float x, float y) {
    _pos_x = x;
    _pos_y = y;
}

void CoreXY::zeroPosition() {
    _pos_x = 0.0f;
    _pos_y = 0.0f;
}
