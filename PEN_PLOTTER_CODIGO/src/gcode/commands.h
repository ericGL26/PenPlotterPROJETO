#pragma once
// =============================================================================
// gcode/commands.h — Tabela de dispatch de comandos G-code
// Responsabilidade: mapear GCodeType → handler e executar.
// Para adicionar um novo comando: registrar um novo handler abaixo.
// =============================================================================

#include <Arduino.h>
#include "parser.h"

class CommandDispatcher {
public:
    void init();

    // Executa o comando. Retorna true = ok, false = erro.
    // Envia ACK ou erro via WebSocket internamente via logger.
    bool dispatch(const GCodeCommand& cmd);

private:
    // --- Handlers individuais ---
    bool handle_G0(const GCodeCommand& cmd);   // Movimento rápido
    bool handle_G1(const GCodeCommand& cmd);   // Movimento linear
    bool handle_G90(const GCodeCommand& cmd);  // Modo absoluto
    bool handle_G91(const GCodeCommand& cmd);  // Modo relativo
    bool handle_G92(const GCodeCommand& cmd);  // Zerar posição
    bool handle_M3(const GCodeCommand& cmd);   // Caneta baixo
    bool handle_M5(const GCodeCommand& cmd);   // Caneta alto

    // Helper: resolve coordenada absoluta ou relativa
    float resolveCoord(float current, float value, bool hasValue, bool absolute) const;

    // Helper: enfileira um bloco de movimento
    bool enqueueMove(float target_x, float target_y,
                     float feed, bool rapid);
};

// Instância global
extern CommandDispatcher commandDispatcher;
