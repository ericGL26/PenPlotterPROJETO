#pragma once
// =============================================================================
// comm/webserver.h — Servidor HTTP que serve o index.html do LittleFS
// =============================================================================

#include <Arduino.h>
#include <WebServer.h>
#include "../config/config.h"

class WebServerManager {
public:
    void init();   // Registra as rotas e inicia o servidor
    void handle(); // Deve ser chamado no loop()

private:
    WebServer _server{HTTP_PORT};

    void handleRoot();
    void handleNotFound();
};

extern WebServerManager webServerManager;
