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
    bool handle_G2(const GCodeCommand& cmd);   // Arco horário (CW)
    bool handle_G3(const GCodeCommand& cmd);   // Arco anti-horário (CCW)
    bool handle_G21(const GCodeCommand& cmd);  // Unidades em mm (no-op)
    bool handle_G90(const GCodeCommand& cmd);  // Modo absoluto
    bool handle_G91(const GCodeCommand& cmd);  // Modo relativo
    bool handle_G92(const GCodeCommand& cmd);  // Zerar posição
    bool handle_M3(const GCodeCommand& cmd);   // Caneta baixo
    bool handle_M5(const GCodeCommand& cmd);   // Caneta alto

    // Implementação compartilhada de G2/G3: decompõe o arco em segmentos de
    // reta curtos (ARC_SEGMENT_MAX_MM) e enfileira um MotionBlock por segmento.
    bool handleArc(const GCodeCommand& cmd, bool clockwise);

    // Traduz o eixo Z (quando presente) em caneta cima/baixo, para compatibilidade
    // com G-code de CAM/CNC que não usa M3/M5 explícitos entre trajetos.
    void applyPenFromZ(const GCodeCommand& cmd);

    // Helper: resolve coordenada absoluta ou relativa
    float resolveCoord(float current, float value, bool hasValue, bool absolute) const;

    // Helper: enfileira um bloco de movimento. Espera (com timeout) por espaço
    // na fila se necessário — um único arco pode gerar mais blocos do que cabem
    // de uma vez em QUEUE_SIZE.
    bool enqueueMove(float target_x, float target_y,
                     float feed, bool rapid);
};

// Instância global
extern CommandDispatcher commandDispatcher;
