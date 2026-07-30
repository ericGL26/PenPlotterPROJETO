// =============================================================================
// hal/motor.cpp — Implementação do driver A4988
// =============================================================================

#include "motor.h"
#include "../config/config.h"

Motor::Motor(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin)
    : _stepPin(stepPin),
      _dirPin(dirPin),
      _enablePin(enablePin),
      _dir(true),
      _hasEnable(enablePin != 255)
{}

void Motor::init() {
    pinMode(_stepPin,   OUTPUT);
    pinMode(_dirPin,    OUTPUT);
    digitalWrite(_stepPin, LOW);
    digitalWrite(_dirPin,  LOW);

    if (_hasEnable) {
        pinMode(_enablePin, OUTPUT);
        disable(); // começa desabilitado para não aquecer em idle
    }
}

void Motor::enable() {
    if (_hasEnable) {
        digitalWrite(_enablePin, LOW); // A4988: LOW = habilitado
    }
}

void Motor::disable() {
    if (_hasEnable) {
        digitalWrite(_enablePin, HIGH); // A4988: HIGH = desabilitado
    }
}

void Motor::setDir(bool forward) {
    _dir = forward;
    digitalWrite(_dirPin, forward ? HIGH : LOW);
}

void Motor::step() {
    // Pulso STEP: HIGH por pelo menos STEP_PULSE_US microssegundos, depois LOW
    digitalWrite(_stepPin, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(_stepPin, LOW);
    delayMicroseconds(STEP_PULSE_US); // tempo de setup antes do próximo step
}
