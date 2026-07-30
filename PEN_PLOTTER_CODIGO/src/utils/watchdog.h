#pragma once
// =============================================================================
// utils/watchdog.h — Wrapper do Task Watchdog Timer (TWDT) do ESP32
// Responsabilidade: resetar o dispositivo se a executor_task travar por mais
//                   de WATCHDOG_TIMEOUT_S segundos sem alimentar o watchdog.
// =============================================================================

#include <Arduino.h>

class Watchdog {
public:
    // Inicializa o TWDT com o timeout definido em config.h (chamar 1x no setup)
    void init();

    // Inscreve a task atual no TWDT — deve ser chamado de dentro da task monitorada
    void subscribe();

    // Alimenta o TWDT em nome da task atual — deve ser chamado periodicamente
    void feed();
};

extern Watchdog watchdog;
