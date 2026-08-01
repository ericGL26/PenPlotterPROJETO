// =============================================================================
// main.cpp — Ponto de entrada do firmware ESP32 CoreXY Plotter
//
// Sequência de boot:
//   1. Serial (debug)
//   2. LittleFS (filesystem)
//   3. HAL (motores)
//   4. CoreXY + Planner
//   5. Fila de movimento
//   6. Estado da máquina (FSM)
//   7. Wi-Fi
//   8. OTA
//   9. WebServer (HTTP)
//  10. WebSocket (WS)
//  11. Watchdog (TWDT — iniciado antes da executor_task)
//  12. Executor Task (Core 0)
//
// Loop principal roda no Core 1:
//   - Wi-Fi watchdog
//   - OTA
//   - WebServer
//   - WebSocket
//   - Broadcast de status a cada STATUS_INTERVAL_MS
// =============================================================================

#include <Arduino.h>
#include <LittleFS.h>

// Camadas (de baixo para cima)
#include "config/config.h"
#include "hal/motor.h"
#include "hal/servo.h"
#include "kinematics/corexy.h"
#include "kinematics/planner.h"
#include "motion/queue.h"
#include "motion/executor.h"
#include "gcode/parser.h"
#include "gcode/commands.h"
#include "state/machine_state.h"
#include "comm/wifi_mgr.h"
#include "comm/ota.h"
#include "comm/webserver.h"
#include "comm/websocket.h"
#include "utils/logger.h"
#include "utils/watchdog.h"

// --- Instâncias dos dois motores físicos ---
Motor motorA(MOTOR_A_STEP, MOTOR_A_DIR, MOTOR_A_ENABLE);
Motor motorB(MOTOR_B_STEP, MOTOR_B_DIR, MOTOR_B_ENABLE);

// Timestamp para o broadcast periódico de status
static uint32_t lastStatusBroadcast = 0;

// =============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Logger::debug("=== ESP32 CoreXY Plotter Firmware ===");

    // 1. LittleFS — necessário antes do WebServer
    if (!LittleFS.begin(true)) {
        Logger::debug("[FS] ERRO: Nao foi possivel montar LittleFS!");
    } else {
        Logger::debug("[FS] LittleFS montado.");
    }

    // 2. HAL — inicializa GPIOs dos motores
    motorA.init();
    motorB.init();
    Logger::debug("[HAL] Motores inicializados.");

    // 3. Servo (placeholder)
    penServo.init();
    Logger::debug("[HAL] Servo inicializado.");

    // 4. CoreXY
    corexy.init();
    Logger::debug("[CoreXY] Cinemática pronta.");

    // 5. Planner — injeta referências aos motores físicos
    planner.init(&motorA, &motorB);
    Logger::debug("[Planner] Planner pronto.");

    // 6. Fila de movimento
    motionQueue.init();
    Logger::debug("[Queue] Fila inicializada.");

    // 7. Dispatcher de G-code
    commandDispatcher.init();
    Logger::debug("[GCode] Dispatcher pronto.");

    // 8. Máquina de estados
    machineState.init();
    Logger::debug("[FSM] Estado inicial: IDLE.");

    // 9. Wi-Fi
    if (!wifiManager.connect()) {
        Logger::debug("[WiFi] Falha na conexão. Continuando sem Wi-Fi...");
    }

    // 10. OTA
    otaManager.init();

    // 11. WebServer HTTP
    webServerManager.init();

    // 12. WebSocket
    wsManager.init();

    Logger::debugf("[NET] Interface disponível em: http://%s",
                   wifiManager.getIP().c_str());

    // 13. Watchdog — deve ser iniciado ANTES da executor_task, que se inscreve
    //     no TWDT assim que começa a rodar (evita corrida entre os cores).
    watchdog.init();

    // 14. Executor no Core 0 (tempo real)
    executor.startTask();
    Logger::debug("[Executor] Task iniciada no Core 0.");

    Logger::debug("=== Boot concluído ===");
}

// =============================================================================
void loop() {
    // Wi-Fi watchdog
    wifiManager.handle();

    // OTA
    otaManager.handle();

    // Servidor HTTP
    webServerManager.handle();

    // WebSocket
    wsManager.handle();

    // Broadcast periódico de status (posição, estado, progresso)
    if (millis() - lastStatusBroadcast >= STATUS_INTERVAL_MS) {
        lastStatusBroadcast = millis();
        wsManager.broadcastStatus();
    }
}