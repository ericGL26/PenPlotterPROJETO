// =============================================================================
// gcode/commands.cpp — Implementação dos handlers de G-code
// =============================================================================

#include "commands.h"
#include "../motion/queue.h"
#include "../state/machine_state.h"
#include "../kinematics/corexy.h"
#include "../hal/servo.h"
#include "../config/config.h"

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
// G0 — Movimento rápido (levanta a caneta automaticamente antes de mover)
bool CommandDispatcher::handle_G0(const GCodeCommand& cmd) {
    penServo.penUp(); // G0 sempre com caneta levantada

    float tx = resolveCoord(machineState.getPosX(), cmd.x, cmd.hasX,
                            machineState.isAbsolute());
    float ty = resolveCoord(machineState.getPosY(), cmd.y, cmd.hasY,
                            machineState.isAbsolute());

    return enqueueMove(tx, ty, MAX_FEED_RATE_MM_MIN, /*rapid=*/true);
}

// -----------------------------------------------------------------------------
// G1 — Movimento linear com caneta abaixada
bool CommandDispatcher::handle_G1(const GCodeCommand& cmd) {
    float feed = cmd.hasF ? cmd.feed : machineState.getFeedRate();
    if (feed <= 0) feed = DEFAULT_FEED_RATE_MM_MIN;
    machineState.setFeedRate(feed);

    float tx = resolveCoord(machineState.getPosX(), cmd.x, cmd.hasX,
                            machineState.isAbsolute());
    float ty = resolveCoord(machineState.getPosY(), cmd.y, cmd.hasY,
                            machineState.isAbsolute());

    return enqueueMove(tx, ty, feed, /*rapid=*/false);
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
    // Rejeita se a fila estiver cheia
    if (motionQueue.isFull()) return false;

    MotionBlock block;
    block.target_x  = target_x;
    block.target_y  = target_y;
    block.feed_rate = feed;
    block.rapid     = rapid;
    block.valid     = true;

    return motionQueue.push(block);
}
