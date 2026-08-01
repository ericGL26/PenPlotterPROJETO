#pragma once
// =============================================================================
// hal/servo.h — Placeholder para controle da caneta (servo motor)
// Futuro: M3 (caneta baixo) / M5 (caneta alto)
// =============================================================================

#include <Arduino.h>

class PenServo {
public:
    // Inicializa o servo no pino configurado em config.h
    void init();

    // Levanta a caneta (M5)
    void penUp();

    // Abaixa a caneta (M3)
    void penDown();

    // Retorna true se a caneta está levantada
    bool isUp() const { return _isUp; }

private:
    bool _isUp = true;
    bool _initialized = false;
};

// Instância global (extern declarado em servo.cpp)
extern PenServo penServo;
