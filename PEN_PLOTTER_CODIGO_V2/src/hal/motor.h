#pragma once
// =============================================================================
// hal/motor.h — Hardware Abstraction Layer: Driver A4988
// Responsabilidade: abstrair os GPIOs do A4988.
// NUNCA conhece: G-code, WebSocket, CoreXY, planner.
// =============================================================================

#include <Arduino.h>

class Motor {
public:
    // Construtor: define os pinos STEP, DIR e (opcionalmente) ENABLE
    Motor(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin = 255);

    // Inicializa os pinos GPIO como OUTPUT e desabilita o driver
    void init();

    // Habilita o driver (LOW no pino ENABLE do A4988)
    void enable();

    // Desabilita o driver (HIGH no pino ENABLE — economiza corrente)
    void disable();

    // Define a direção: true = forward (HIGH no DIR), false = backward (LOW)
    void setDir(bool forward);

    // Gera 1 único pulso STEP com duração garantida >= STEP_PULSE_US
    void step();

    // Retorna a direção atual
    bool getDir() const { return _dir; }

private:
    uint8_t _stepPin;
    uint8_t _dirPin;
    uint8_t _enablePin;
    bool    _dir;
    bool    _hasEnable;
};
