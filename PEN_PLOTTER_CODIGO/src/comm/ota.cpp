// =============================================================================
// comm/ota.cpp — ArduinoOTA setup
// =============================================================================

#include "ota.h"
#include "../utils/logger.h"
#include <ArduinoOTA.h>

OtaManager otaManager;

void OtaManager::init() {
    ArduinoOTA.setHostname("esp32-plotter");

    ArduinoOTA.onStart([]() {
        Logger::debug("[OTA] Iniciando atualização...");
    });

    ArduinoOTA.onEnd([]() {
        Logger::debug("[OTA] Concluído. Reiniciando...");
    });

    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Logger::debugf("[OTA] Progresso: %u%%", (progress / (total / 100)));
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Logger::debugf("[OTA] Erro[%u]", error);
    });

    ArduinoOTA.begin();
    Logger::debug("[OTA] Pronto.");
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}
