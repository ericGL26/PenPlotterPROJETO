#pragma once
// =============================================================================
// config.h — Constantes globais do firmware ESP32 CoreXY Plotter
// Este arquivo é o único sem dependências internas.
// NUNCA inclua outros headers do projeto aqui.
// =============================================================================

// -----------------------------------------------------------------------------
// Wi-Fi
// -----------------------------------------------------------------------------
#define WIFI_SSID       "Eduardo"
#define WIFI_PASSWORD   "26301906"
#define WIFI_TIMEOUT_MS 15000   // Timeout de conexão (ms)

// -----------------------------------------------------------------------------
// Rede
// -----------------------------------------------------------------------------
#define HTTP_PORT       80
#define WS_PORT         81

// -----------------------------------------------------------------------------
// Hardware — Pinos dos motores (A4988)
// -----------------------------------------------------------------------------
// Motor A (fisicamente ligado ao que era Motor X)
#define MOTOR_A_STEP    13
#define MOTOR_A_DIR     12
#define MOTOR_A_ENABLE  14   // Opcional: pino ENABLE do A4988

// Motor B (fisicamente ligado ao que era Motor Y)
#define MOTOR_B_STEP    22
#define MOTOR_B_DIR     23
#define MOTOR_B_ENABLE  27   // Opcional: pino ENABLE do A4988

// Servo (futuro — caneta)
#define SERVO_PIN       26
#define SERVO_PEN_UP    90   // ângulo (graus) caneta levantada
#define SERVO_PEN_DOWN  0    // ângulo (graus) caneta abaixada

// -----------------------------------------------------------------------------
// Cinemática CoreXY
// -----------------------------------------------------------------------------
// Fórmula: steps_per_mm = (motor_steps × microstepping) / (belt_pitch × pulley_teeth)
// NEMA17 (200 steps) + A4988 1/16 + GT2 (2mm) + polia 20 dentes → 80 steps/mm
#define STEPS_PER_MM    80.0f

// Dimensões da área de trabalho (mm)
#define MACHINE_WIDTH_MM  200.0f
#define MACHINE_HEIGHT_MM 200.0f

// -----------------------------------------------------------------------------
// Velocidade e Aceleração
// -----------------------------------------------------------------------------
#define DEFAULT_FEED_RATE_MM_MIN  3000.0f   // mm/min
#define MAX_FEED_RATE_MM_MIN      6000.0f   // mm/min
#define MIN_FEED_RATE_MM_MIN       100.0f   // mm/min
#define ACCELERATION_MM_S2         500.0f   // mm/s²

// Pulso mínimo do STEP (µs) — A4988 requer ≥ 1µs HIGH
#define STEP_PULSE_US   2

// -----------------------------------------------------------------------------
// Fila de movimento
// -----------------------------------------------------------------------------
#define QUEUE_SIZE      64   // número máximo de blocos na fila circular

// -----------------------------------------------------------------------------
// Watchdog
// -----------------------------------------------------------------------------
#define WATCHDOG_TIMEOUT_S  30

// -----------------------------------------------------------------------------
// Status broadcast
// -----------------------------------------------------------------------------
#define STATUS_INTERVAL_MS  200  // envio de posição/status ao navegador

// -----------------------------------------------------------------------------
// Buffer G-code
// -----------------------------------------------------------------------------
#define GCODE_BUFFER_SIZE 128   // tamanho máximo de uma linha G-code (bytes)
