// =============================================================================
// comm/wifi_mgr.cpp
// =============================================================================

#include "wifi_mgr.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include <WiFi.h>

WifiManager wifiManager;

bool WifiManager::connect() {
    Logger::debug("[WiFi] Conectando...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > WIFI_TIMEOUT_MS) {
            Logger::debug("[WiFi] Timeout de conexão!");
            return false;
        }
        delay(500);
    }

    Logger::debugf("[WiFi] Conectado! IP: %s", WiFi.localIP().toString().c_str());
    return true;
}

void WifiManager::handle() {
    if (WiFi.status() != WL_CONNECTED) {
        Logger::debug("[WiFi] Reconectando...");
        WiFi.reconnect();
    }
}

bool WifiManager::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WifiManager::getIP() const {
    return WiFi.localIP().toString();
}
