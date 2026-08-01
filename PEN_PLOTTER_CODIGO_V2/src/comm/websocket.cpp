// =============================================================================
// comm/websocket.cpp — Implementação do servidor WebSocket
//
// Protocolo (ESP32 → Navegador):
//   "ok"                       ← ACK de linha G-code
//   "error:E01:descricao"      ← Erro de parse ou execução
//   "busy"                     ← Fila cheia
//   "state:idle/running/paused"← Estado da máquina
//   "pos:x=25.40,y=-10.00"     ← Posição atual (a cada 200ms)
//   "progress:35"              ← Progresso percentual
//   "feed:3000"                ← Feed rate atual
//   "eof"                      ← Arquivo concluído
//
// Protocolo (Navegador → ESP32):
//   "cmd:run"                  ← Inicia execução
//   "cmd:pause"                ← Pausa
//   "cmd:resume"               ← Retoma
//   "cmd:cancel"               ← Cancela e limpa fila
//   "cmd:home"                 ← Homing (futuro)
//   "cmd:unlock"               ← Libera estado de erro
//   "cmd:status"               ← Solicita status imediato
//   "gcode:<linha>"            ← Linha G-code individual
// =============================================================================

#include "websocket.h"
#include "../config/config.h"
#include "../utils/logger.h"
#include "../state/machine_state.h"
#include "../kinematics/corexy.h"
#include "../motion/queue.h"
#include "../motion/executor.h"
#include "../gcode/parser.h"
#include "../gcode/commands.h"

// Instância global e ponteiro estático para o callback
WebSocketManager wsManager;
WebSocketManager* WebSocketManager::_instance = nullptr;

// -----------------------------------------------------------------------------
void WebSocketManager::init() {
    _instance = this;

    // Registra o callback de envio no Logger (rompe dependência circular)
    Logger::setCallback([](const char* msg) {
        if (_instance) _instance->broadcast(msg);
    });

    _wss.begin();
    _wss.onEvent(onEvent);

    Logger::debug("[WebSocket] Servidor WS iniciado na porta 81.");
}

// -----------------------------------------------------------------------------
void WebSocketManager::handle() {
    _wss.loop();
}

// -----------------------------------------------------------------------------
void WebSocketManager::broadcast(const char* msg) {
    _wss.broadcastTXT(msg);
}

void WebSocketManager::broadcast(const String& msg) {
    broadcast(msg.c_str());
}

void WebSocketManager::sendTo(uint8_t clientId, const char* msg) {
    _wss.sendTXT(clientId, msg);
}

// -----------------------------------------------------------------------------
void WebSocketManager::broadcastStatus() {
    // Posição física real (CoreXY, atualizada pelo executor em tempo real) —
    // não a posição lógica/comandada de machineState, que pode estar à frente
    // por causa de linhas já enfileiradas mas ainda não desenhadas.
    char buf[64];
    snprintf(buf, sizeof(buf), "pos:x=%.2f,y=%.2f",
             corexy.getX(), corexy.getY());
    broadcast(buf);

    // Estado
    snprintf(buf, sizeof(buf), "state:%s", machineState.getStateString());
    broadcast(buf);

    // Progresso
    snprintf(buf, sizeof(buf), "progress:%d",
             (int)motionQueue.progressPercent());
    broadcast(buf);

    // Feed rate
    snprintf(buf, sizeof(buf), "feed:%.0f", machineState.getFeedRate());
    broadcast(buf);
}

// -----------------------------------------------------------------------------
// Callback estático do WebSocketsServer
void WebSocketManager::onEvent(uint8_t clientId, WStype_t type,
                                uint8_t* payload, size_t length) {
    if (!_instance) return;

    switch (type) {
        case WStype_CONNECTED:
            Logger::debugf("[WS] Cliente %u conectado.", clientId);
            _instance->sendTo(clientId, "state:idle");
            break;

        case WStype_DISCONNECTED:
            Logger::debugf("[WS] Cliente %u desconectado.", clientId);
            break;

        case WStype_TEXT:
            payload[length] = '\0'; // garante terminação
            _instance->processMessage(clientId, (const char*)payload);
            break;

        default:
            break;
    }
}

// -----------------------------------------------------------------------------
void WebSocketManager::processMessage(uint8_t clientId, const char* msg) {
    _lastClientId = clientId;

    if (strncmp(msg, "cmd:", 4) == 0) {
        routeControlCommand(clientId, msg + 4);
    } else if (strncmp(msg, "gcode:", 6) == 0) {
        routeGCodeLine(clientId, msg + 6);
    } else {
        sendTo(clientId, "error:E99:Formato invalido. Use cmd: ou gcode:");
    }
}

// -----------------------------------------------------------------------------
void WebSocketManager::routeControlCommand(uint8_t clientId, const char* cmd) {
    if (strcmp(cmd, "run") == 0) {
        if (machineState.setState(MachineState::RUNNING)) {
            executor.resume();
            sendTo(clientId, "state:running");
        } else {
            sendTo(clientId, "error:E04:Estado invalido para run");
        }

    } else if (strcmp(cmd, "pause") == 0) {
        executor.pause();
        sendTo(clientId, "state:paused");

    } else if (strcmp(cmd, "resume") == 0) {
        executor.resume();
        sendTo(clientId, "state:running");

    } else if (strcmp(cmd, "cancel") == 0) {
        executor.cancel();
        sendTo(clientId, "state:idle");

    } else if (strcmp(cmd, "unlock") == 0) {
        if (machineState.getState() == MachineState::ERROR) {
            machineState.setState(MachineState::IDLE);
            sendTo(clientId, "state:idle");
        }

    } else if (strcmp(cmd, "status") == 0) {
        broadcastStatus();

    } else if (strcmp(cmd, "home") == 0) {
        // TODO Fase 2: implementar homing
        sendTo(clientId, "error:E99:Homing nao implementado");

    } else {
        sendTo(clientId, "error:E99:Comando desconhecido");
    }
}

// -----------------------------------------------------------------------------
void WebSocketManager::routeGCodeLine(uint8_t clientId, const char* line) {
    // Verifica se a fila está cheia antes de parsear
    if (motionQueue.isFull()) {
        sendTo(clientId, "busy");
        return;
    }

    // Parseia a linha
    GCodeCommand cmd = gcodeParser.parse(line);

    if (!cmd.valid) {
        char errMsg[64];
        snprintf(errMsg, sizeof(errMsg), "error:E01:%s", cmd.errorMsg);
        sendTo(clientId, errMsg);
        return;
    }

    // Despacha para o handler correto
    bool ok = commandDispatcher.dispatch(cmd);

    if (ok) {
        sendTo(clientId, "ok"); // ACK — navegador pode enviar próxima linha
    } else {
        sendTo(clientId, "error:E03:Falha ao enfileirar comando");
    }
}
