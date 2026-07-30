// =============================================================================
// utils/watchdog.cpp — Implementação do wrapper do TWDT
// =============================================================================

#include "watchdog.h"
#include "../config/config.h"
#include "logger.h"
#include <esp_task_wdt.h>

Watchdog watchdog;

void Watchdog::init() {
    esp_task_wdt_init(WATCHDOG_TIMEOUT_S, /*panic=*/true);
    Logger::debugf("[Watchdog] TWDT iniciado (timeout=%ds).", WATCHDOG_TIMEOUT_S);
}

void Watchdog::subscribe() {
    esp_task_wdt_add(nullptr); // inscreve a task atual
}

void Watchdog::feed() {
    esp_task_wdt_reset();
}
