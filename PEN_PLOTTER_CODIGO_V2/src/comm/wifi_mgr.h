#pragma once
// =============================================================================
// comm/wifi_mgr.h — Gerenciador de Wi-Fi com reconexão automática
// =============================================================================

#include <Arduino.h>

class WifiManager {
public:
    // Conecta ao Wi-Fi definido em config.h. Bloqueia até conectar ou timeout.
    bool connect();

    // Deve ser chamado no loop() — verifica e reconecta se necessário
    void handle();

    bool isConnected() const;

    // Retorna o IP como String (ex: "192.168.1.150")
    String getIP() const;
};

extern WifiManager wifiManager;
