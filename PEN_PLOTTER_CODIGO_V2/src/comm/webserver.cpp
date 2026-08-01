// =============================================================================
// comm/webserver.cpp — Serve index.html do LittleFS via HTTP
// =============================================================================

#include "webserver.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include <LittleFS.h>

WebServerManager webServerManager;

void WebServerManager::init() {
    _server.on("/", [this]() { handleRoot(); });
    _server.onNotFound([this]() { handleNotFound(); });

    _server.begin();
    Logger::debug("[WebServer] Servidor HTTP iniciado na porta 80.");
}

void WebServerManager::handle() {
    _server.handleClient();
}

void WebServerManager::handleRoot() {
    if (LittleFS.exists("/index.html")) {
        File file = LittleFS.open("/index.html", "r");
        _server.streamFile(file, "text/html");
        file.close();
    } else {
        _server.send(404, "text/plain",
                     "index.html nao encontrado. Faca upload via LittleFS.");
    }
}

void WebServerManager::handleNotFound() {
    _server.send(404, "text/plain", "Not Found");
}
