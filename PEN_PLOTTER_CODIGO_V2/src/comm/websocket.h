#pragma once
// =============================================================================
// comm/websocket.h — Servidor WebSocket bidirecional
// Responsabilidade: receber mensagens do navegador, rotear para parser/executor,
//                   e enviar status/ACK de volta ao navegador.
// =============================================================================

#include <Arduino.h>
#include <WebSocketsServer.h>
#include "../config/config.h"

class WebSocketManager {
public:
    // Inicializa o servidor WebSocket na porta WS_PORT
    void init();

    // Deve ser chamado no loop() do Core 1
    void handle();

    // Envia mensagem para TODOS os clientes conectados
    void broadcast(const char* msg);
    void broadcast(const String& msg);

    // Envia mensagem para um cliente específico
    void sendTo(uint8_t clientId, const char* msg);

    // Envia status completo (posição, estado, progresso) — chamado a cada 200ms
    void broadcastStatus();

private:
    WebSocketsServer _wss{WS_PORT};
    uint8_t _lastClientId = 0;

    // Callback estático — WebSocketsServer exige ponteiro de função
    static void onEvent(uint8_t clientId, WStype_t type,
                        uint8_t* payload, size_t length);

    // Processa mensagem recebida
    void processMessage(uint8_t clientId, const char* msg);

    // Roteador: separa comandos de controle de linhas G-code
    void routeControlCommand(uint8_t clientId, const char* cmd);
    void routeGCodeLine(uint8_t clientId, const char* line);

    // Instância estática para o callback
    static WebSocketManager* _instance;
};

extern WebSocketManager wsManager;
