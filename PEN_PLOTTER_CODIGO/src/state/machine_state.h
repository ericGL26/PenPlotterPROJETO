#pragma once
// =============================================================================
// state/machine_state.h — Máquina de estados finita (FSM)
// Responsabilidade: ser a ÚNICA fonte da verdade sobre o estado da máquina.
// NUNCA conhece: WebSocket, motores, parser.
// =============================================================================

#include <Arduino.h>

class MachineState {
public:
    enum State {
        IDLE    = 0,
        RUNNING = 1,
        PAUSED  = 2,
        HOMING  = 3,
        ERROR   = 4,
        LOCKED  = 5
    };

    // Inicializa em IDLE
    void init();

    // Tenta transição de estado. Retorna false se a transição é inválida.
    bool setState(State newState);

    // Retorna o estado atual
    State getState() const { return _state; }

    // Retorna string legível do estado (para WebSocket)
    const char* getStateString() const;

    // Posição atual (atualizada pelo executor após cada bloco)
    void setPosition(float x, float y);
    float getPosX() const { return _pos_x; }
    float getPosY() const { return _pos_y; }

    // Feed rate atual
    void  setFeedRate(float f) { _feed_rate = f; }
    float getFeedRate() const  { return _feed_rate; }

    // Modo relativo/absoluto
    void setAbsolute(bool abs) { _absolute = abs; }
    bool isAbsolute() const    { return _absolute; }

private:
    volatile State _state = IDLE;
    float _pos_x     = 0.0f;
    float _pos_y     = 0.0f;
    float _feed_rate = 0.0f;
    bool  _absolute  = true; // G90 por padrão

    // Tabela de transições válidas
    bool isTransitionValid(State from, State to) const;
};

// Instância global
extern MachineState machineState;
