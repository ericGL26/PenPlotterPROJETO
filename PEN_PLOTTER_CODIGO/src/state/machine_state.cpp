// =============================================================================
// state/machine_state.cpp — Implementação da FSM
// =============================================================================

#include "machine_state.h"

// Instância global
MachineState machineState;

// -----------------------------------------------------------------------------
void MachineState::init() {
    _state     = IDLE;
    _pos_x     = 0.0f;
    _pos_y     = 0.0f;
    _feed_rate = 0.0f;
    _absolute  = true;
}

// -----------------------------------------------------------------------------
bool MachineState::setState(State newState) {
    if (!isTransitionValid(_state, newState)) {
        return false; // transição inválida — mantém estado atual
    }
    _state = newState;
    return true;
}

// -----------------------------------------------------------------------------
bool MachineState::isTransitionValid(State from, State to) const {
    // Qualquer estado pode ir para ERROR
    if (to == ERROR) return true;

    switch (from) {
        case IDLE:
            return (to == RUNNING || to == HOMING);

        case RUNNING:
            return (to == PAUSED || to == IDLE || to == ERROR);

        case PAUSED:
            return (to == RUNNING || to == IDLE);

        case HOMING:
            return (to == IDLE || to == ERROR);

        case ERROR:
            return (to == IDLE); // só sai via 'unlock'

        case LOCKED:
            return false; // travado permanentemente (requer reset)

        default:
            return false;
    }
}

// -----------------------------------------------------------------------------
const char* MachineState::getStateString() const {
    switch (_state) {
        case IDLE:    return "idle";
        case RUNNING: return "running";
        case PAUSED:  return "paused";
        case HOMING:  return "homing";
        case ERROR:   return "error";
        case LOCKED:  return "locked";
        default:      return "unknown";
    }
}

// -----------------------------------------------------------------------------
void MachineState::setPosition(float x, float y) {
    _pos_x = x;
    _pos_y = y;
}
