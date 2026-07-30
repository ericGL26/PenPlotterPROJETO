#pragma once
// =============================================================================
// utils/logger.h — Roteador de log (WebSocket + Serial debug)
// Substitui Serial.println() para envio de mensagens ao navegador.
// =============================================================================

#include <Arduino.h>

class Logger {
public:
    // Envia mensagem para o cliente WebSocket conectado (se houver)
    static void sendWS(const char* msg);
    static void sendWS(const String& msg);

    // Formata e envia (printf-style)
    static void sendWSf(const char* fmt, ...);

    // Registra internamente via Serial (para debug com cabo USB)
    static void debug(const char* msg);
    static void debugf(const char* fmt, ...);

    // Ponteiro de callback que o módulo comm/websocket registra
    // Evita dependência circular: logger → websocket não existe
    using SendCallback = void (*)(const char*);
    static void setCallback(SendCallback cb) { _callback = cb; }

private:
    static SendCallback _callback;
};
