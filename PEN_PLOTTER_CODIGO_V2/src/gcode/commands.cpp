// =============================================================================
// gcode/commands.cpp — Implementação dos handlers de G-code
// =============================================================================

#include "commands.h"
#include "../motion/queue.h"
#include "../state/machine_state.h"
#include "../kinematics/corexy.h"
#include "../hal/servo.h"
#include "../config/config.h"
#include <math.h>

// Instância global
CommandDispatcher commandDispatcher;

// -----------------------------------------------------------------------------
void CommandDispatcher::init() {
    // Nada a inicializar na versão atual
}

// -----------------------------------------------------------------------------
bool CommandDispatcher::dispatch(const GCodeCommand& cmd) {
    switch (cmd.type) {
        case GCodeType::G0:    return handle_G0(cmd);
        case GCodeType::G1:    return handle_G1(cmd);
        case GCodeType::G2:    return handle_G2(cmd);
        case GCodeType::G3:    return handle_G3(cmd);
        case GCodeType::G21:   return handle_G21(cmd);
        case GCodeType::G90:   return handle_G90(cmd);
        case GCodeType::G91:   return handle_G91(cmd);
        case GCodeType::G92:   return handle_G92(cmd);
        case GCodeType::M3:    return handle_M3(cmd);
        case GCodeType::M5:    return handle_M5(cmd);
        case GCodeType::EMPTY: return true; // linha vazia → ok silencioso
        default:               return false;
    }
}

// -----------------------------------------------------------------------------
// G0 — Movimento rápido. Sem Z: caneta sempre levantada (comportamento clássico
// de plotter). Com Z: o valor decide (compatibilidade com G-code de CNC/CAM).
bool CommandDispatcher::handle_G0(const GCodeCommand& cmd) {
    if (cmd.hasZ) {
        applyPenFromZ(cmd);
    } else {
        penServo.penUp();
    }

    float tx = resolveCoord(machineState.getPosX(), cmd.x, cmd.hasX,
                            machineState.isAbsolute());
    float ty = resolveCoord(machineState.getPosY(), cmd.y, cmd.hasY,
                            machineState.isAbsolute());

    if (!enqueueMove(tx, ty, MAX_FEED_RATE_MM_MIN, /*rapid=*/true)) return false;
    // Atualiza a posição lógica (comandada) imediatamente — não espera a
    // execução física, para que a próxima linha (relativa ou arco) resolva
    // corretamente mesmo com várias linhas já enfileiradas à frente.
    machineState.setPosition(tx, ty);
    return true;
}

// -----------------------------------------------------------------------------
// G1 — Movimento linear. Se vier Z (G-code de CNC/CAM), traduz para caneta
// cima/baixo; sem Z, o estado da caneta é controlado só por M3/M5 explícitos.
bool CommandDispatcher::handle_G1(const GCodeCommand& cmd) {
    applyPenFromZ(cmd);

    float feed = cmd.hasF ? cmd.feed : machineState.getFeedRate();
    if (feed <= 0) feed = DEFAULT_FEED_RATE_MM_MIN;
    machineState.setFeedRate(feed);

    float tx = resolveCoord(machineState.getPosX(), cmd.x, cmd.hasX,
                            machineState.isAbsolute());
    float ty = resolveCoord(machineState.getPosY(), cmd.y, cmd.hasY,
                            machineState.isAbsolute());

    if (!enqueueMove(tx, ty, feed, /*rapid=*/false)) return false;
    machineState.setPosition(tx, ty);
    return true;
}

// -----------------------------------------------------------------------------
// G2 — Arco horário (CW)
bool CommandDispatcher::handle_G2(const GCodeCommand& cmd) {
    return handleArc(cmd, /*clockwise=*/true);
}

// -----------------------------------------------------------------------------
// G3 — Arco anti-horário (CCW)
bool CommandDispatcher::handle_G3(const GCodeCommand& cmd) {
    return handleArc(cmd, /*clockwise=*/false);
}

// -----------------------------------------------------------------------------
// Decompõe um arco G2/G3 (definido por centro relativo I/J) em segmentos de
// reta curtos, cada um enfileirado como um MotionBlock normal. O planner/
// Bresenham nunca precisa saber que veio de um arco.
bool CommandDispatcher::handleArc(const GCodeCommand& cmd, bool clockwise) {
    applyPenFromZ(cmd);

    float feed = cmd.hasF ? cmd.feed : machineState.getFeedRate();
    if (feed <= 0) feed = DEFAULT_FEED_RATE_MM_MIN;
    machineState.setFeedRate(feed);

    float startX = machineState.getPosX();
    float startY = machineState.getPosY();

    float endX = resolveCoord(startX, cmd.x, cmd.hasX, machineState.isAbsolute());
    float endY = resolveCoord(startY, cmd.y, cmd.hasY, machineState.isAbsolute());

    // I/J são sempre o offset do centro em relação ao ponto inicial,
    // independente do modo absoluto/relativo (convenção padrão de G-code).
    float centerX = startX + (cmd.hasI ? cmd.i : 0.0f);
    float centerY = startY + (cmd.hasJ ? cmd.j : 0.0f);

    float radius = hypotf(startX - centerX, startY - centerY);
    if (radius < 0.001f) {
        // Sem raio válido (I/J ausentes ou nulos) — trata como reta simples
        if (!enqueueMove(endX, endY, feed, false)) return false;
        machineState.setPosition(endX, endY);
        return true;
    }

    float startAngle = atan2f(startY - centerY, startX - centerX);
    float endAngle   = atan2f(endY   - centerY, endX   - centerX);

    float sweep = endAngle - startAngle;
    if (clockwise) {
        while (sweep > 0.0f) sweep -= 2.0f * PI;
        if (sweep == 0.0f)   sweep = -2.0f * PI; // círculo completo
    } else {
        while (sweep < 0.0f) sweep += 2.0f * PI;
        if (sweep == 0.0f)   sweep = 2.0f * PI;  // círculo completo
    }

    float arcLength = fabsf(sweep) * radius;
    int segments = (int)ceilf(arcLength / ARC_SEGMENT_MAX_MM);
    if (segments < 1) segments = 1;

    for (int i = 1; i <= segments; i++) {
        float t     = (float)i / (float)segments;
        float angle = startAngle + sweep * t;
        float px = centerX + radius * cosf(angle);
        float py = centerY + radius * sinf(angle);

        if (!enqueueMove(px, py, feed, false)) return false;
    }

    // Posição lógica final do arco (não cada corda interna) — a próxima linha
    // do G-code deve resolver a partir daqui, não do início do arco.
    machineState.setPosition(endX, endY);
    return true;
}

// -----------------------------------------------------------------------------
// G21 — Unidades em mm. No-op: o firmware já trabalha nativamente em mm.
bool CommandDispatcher::handle_G21(const GCodeCommand& cmd) {
    (void)cmd;
    return true;
}

// -----------------------------------------------------------------------------
// Traduz Z em caneta cima/baixo para compatibilidade com G-code de CNC/CAM
// (ex: Inkscape Gcodetools), que controla a ferramenta pelo eixo Z em vez de
// M3/M5 explícitos entre trajetos.
void CommandDispatcher::applyPenFromZ(const GCodeCommand& cmd) {
    if (!cmd.hasZ) return;
    if (cmd.z <= PEN_ENGAGE_Z_MM) penServo.penDown();
    else                          penServo.penUp();
}

// -----------------------------------------------------------------------------
// G90 — Modo coordenadas absolutas
bool CommandDispatcher::handle_G90(const GCodeCommand& cmd) {
    (void)cmd;
    machineState.setAbsolute(true);
    return true;
}

// -----------------------------------------------------------------------------
// G91 — Modo coordenadas relativas
bool CommandDispatcher::handle_G91(const GCodeCommand& cmd) {
    (void)cmd;
    machineState.setAbsolute(false);
    return true;
}

// -----------------------------------------------------------------------------
// G92 — Zerar posição lógica (sem mover os motores)
bool CommandDispatcher::handle_G92(const GCodeCommand& cmd) {
    float nx = cmd.hasX ? cmd.x : 0.0f;
    float ny = cmd.hasY ? cmd.y : 0.0f;
    machineState.setPosition(nx, ny);
    // Atualiza também a posição interna do CoreXY
    corexy.setPosition(nx, ny);
    return true;
}

// -----------------------------------------------------------------------------
// M3 — Caneta baixo
bool CommandDispatcher::handle_M3(const GCodeCommand& cmd) {
    (void)cmd;
    penServo.penDown();
    return true;
}

// -----------------------------------------------------------------------------
// M5 — Caneta alto
bool CommandDispatcher::handle_M5(const GCodeCommand& cmd) {
    (void)cmd;
    penServo.penUp();
    return true;
}

// -----------------------------------------------------------------------------
float CommandDispatcher::resolveCoord(float current, float value,
                                      bool hasValue, bool absolute) const {
    if (!hasValue) return current;       // sem coordenada → mantém posição atual
    if (absolute)  return value;         // G90: valor é absoluto
    return current + value;              // G91: valor é relativo
}

// -----------------------------------------------------------------------------
bool CommandDispatcher::enqueueMove(float target_x, float target_y,
                                    float feed, bool rapid) {
    // Espera (com timeout) por espaço na fila em vez de falhar na hora — um
    // único G2/G3 pode gerar mais segmentos do que cabem de uma vez em
    // QUEUE_SIZE, e o executor vai liberando espaço conforme desenha.
    uint32_t start = millis();
    while (motionQueue.isFull()) {
        if (millis() - start > ENQUEUE_TIMEOUT_MS) return false;
        delay(5);
    }

    MotionBlock block;
    block.target_x  = target_x;
    block.target_y  = target_y;
    block.feed_rate = feed;
    block.rapid     = rapid;
    block.valid     = true;

    return motionQueue.push(block);
}
