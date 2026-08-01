#pragma once
// =============================================================================
// comm/ota.h — Suporte a ArduinoOTA (atualização Over-The-Air)
// =============================================================================

#include <Arduino.h>

class OtaManager {
public:
    void init();   // Configura ArduinoOTA
    void handle(); // Deve ser chamado no loop()
};

extern OtaManager otaManager;
