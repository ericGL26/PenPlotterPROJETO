// =============================================================================
// hal/servo.cpp — Implementação stub do servo da caneta
// TODO Fase 2: integrar ESP32Servo quando o hardware estiver disponível
// =============================================================================

#include "servo.h"
#include "../config/config.h"

// Instância global
PenServo penServo;

void PenServo::init() {
    // TODO: quando o servo for conectado, descomente o código abaixo:
    // _servo.attach(SERVO_PIN);
    // _servo.write(SERVO_PEN_UP);
    _isUp = true;
    _initialized = true;
}

void PenServo::penUp() {
    if (!_initialized) return;
    // TODO: _servo.write(SERVO_PEN_UP);
    _isUp = true;
}

void PenServo::penDown() {
    if (!_initialized) return;
    // TODO: _servo.write(SERVO_PEN_DOWN);
    _isUp = false;
}
