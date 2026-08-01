#pragma once
// =============================================================================
// kinematics/corexy.h — Transformada cinemática CoreXY
// Responsabilidade: converter coordenadas cartesianas (X,Y) em passos
//                   para os dois motores físicos (A, B).
// NUNCA conhece: G-code, WebSocket, fila, estado.
// =============================================================================

#include <Arduino.h>

// Resultado da transformada CoreXY: passos para cada motor
struct CoreXYSteps {
    long stepsA;    // Motor A (ΔX + ΔY em passos)
    long stepsB;    // Motor B (ΔX - ΔY em passos)
    bool dirA;      // true = frente, false = trás
    bool dirB;
};

class CoreXY {
public:
    // Inicializa a posição interna em (0, 0)
    void init();

    // Converte um movimento cartesiano para passos dos motores.
    // target_x, target_y: coordenadas absolutas em mm
    // Retorna a estrutura com passos e direções para cada motor.
    CoreXYSteps translate(float target_x, float target_y);

    // Atualiza a posição lógica atual (chamado pelo executor após cada bloco)
    void setPosition(float x, float y);

    // Getters de posição atual em mm
    float getX() const { return _pos_x; }
    float getY() const { return _pos_y; }

    // Zera a posição lógica (G92)
    void zeroPosition();

private:
    float _pos_x = 0.0f;
    float _pos_y = 0.0f;
};

// Instância global
extern CoreXY corexy;
