// =============================================================================
// kinematics/planner.cpp — Bresenham 2D com perfil de velocidade trapezoidal
//
// Algoritmo Bresenham:
//   O motor com mais passos é o "dominante" (eixo longo).
//   O motor subordinado distribui seus passos linearmente ao longo do dominante.
//   Isso garante trajetória reta e que ambos os motores terminem simultaneamente.
//
// Perfil de velocidade (trapézio):
//   Fase 1 - Aceleração: v aumenta de v_start até v_max
//   Fase 2 - Sustentação: v constante em v_max
//   Fase 3 - Desaceleração: v diminui de v_max até v_end
//   O delay entre steps é recalculado a cada step usando cinemática.
// =============================================================================

#include "planner.h"
#include "../config/config.h"
#include <math.h>

// Instância global
Planner planner;

// -----------------------------------------------------------------------------
void Planner::init(Motor* motorA, Motor* motorB) {
    _motorA = motorA;
    _motorB = motorB;
    _aborted = false;
}

// -----------------------------------------------------------------------------
bool Planner::execute(const MotionBlock& block) {
    if (!_motorA || !_motorB) return false;
    _aborted = false;

    // 1. Traduz coordenadas cartesianas → passos dos motores via CoreXY
    CoreXYSteps ks = corexy.translate(block.target_x, block.target_y);

    // Se não há movimento real, retorna imediatamente
    if (ks.stepsA == 0 && ks.stepsB == 0) {
        corexy.setPosition(block.target_x, block.target_y);
        return true;
    }

    // 2. Define direção dos motores
    _motorA->enable();
    _motorB->enable();
    _motorA->setDir(ks.dirA);
    _motorB->setDir(ks.dirB);

    // 3. Determina feed rate
    float feed = block.rapid
        ? MAX_FEED_RATE_MM_MIN
        : constrain(block.feed_rate, MIN_FEED_RATE_MM_MIN, MAX_FEED_RATE_MM_MIN);

    // 4. Executa Bresenham com perfil trapezoidal
    runBresenham(ks.stepsA, ks.stepsB, ks.dirA, ks.dirB, feed);

    // 5. Atualiza posição lógica apenas se o movimento não foi abortado
    if (!_aborted) {
        corexy.setPosition(block.target_x, block.target_y);
    }

    return !_aborted;
}

// -----------------------------------------------------------------------------
void Planner::abort() {
    _aborted = true;
}

// -----------------------------------------------------------------------------
// Bresenham 2D com perfil de velocidade trapezoidal
// -----------------------------------------------------------------------------
void Planner::runBresenham(long stepsA, long stepsB,
                           bool dirA, bool dirB,
                           float feed_mm_min) {
    (void)dirA; (void)dirB; // direção já setada antes de chamar esta função

    // --- Identifica motor dominante (mais passos) e subordinado ---
    long dominant, subordinate;
    bool aIsDominant;

    if (stepsA >= stepsB) {
        dominant    = stepsA;
        subordinate = stepsB;
        aIsDominant = true;
    } else {
        dominant    = stepsB;
        subordinate = stepsA;
        aIsDominant = false;
    }

    // --- Variáveis do Bresenham ---
    long error = 2 * subordinate - dominant;
    long stepsDone = 0;

    // --- Perfil trapezoidal de velocidade ---
    // Converte aceleração de mm/s² para steps/s²
    const float accel_steps_s2 = ACCELERATION_MM_S2 * STEPS_PER_MM;

    // Velocidade máxima em steps/s
    float v_max_steps = (feed_mm_min / 60.0f) * STEPS_PER_MM;

    // Ponto de crossover: quantos steps para atingir v_max partindo de 0
    // Usando v² = 2·a·d  →  d = v²/(2a)
    long accel_steps = (long)(v_max_steps * v_max_steps / (2.0f * accel_steps_s2));
    if (accel_steps > dominant / 2) {
        accel_steps = dominant / 2; // triangular, sem fase de sustentação
    }

    // Velocidade de pico alcançável (caso triangular)
    float v_peak = sqrtf(2.0f * accel_steps_s2 * accel_steps);
    if (v_peak > v_max_steps) v_peak = v_max_steps;

    // Velocidade inicial (steps/s) — começa baixo para evitar skip
    float v_current = (feed_mm_min / 60.0f) * STEPS_PER_MM * 0.1f;
    if (v_current < 100.0f) v_current = 100.0f;

    // --- Loop principal: um passo do motor dominante por iteração ---
    for (long i = 0; i < dominant; i++) {
        if (_aborted) return;

        // --- Pulsa motor dominante ---
        if (aIsDominant) {
            _motorA->step();
        } else {
            _motorB->step();
        }
        stepsDone++;

        // --- Bresenham: decide se pulsa o motor subordinado ---
        if (error >= 0) {
            if (aIsDominant) {
                _motorB->step();
            } else {
                _motorA->step();
            }
            error -= 2 * dominant;
        }
        error += 2 * subordinate;

        // --- Cálculo do delay (perfil trapezoidal) ---
        // Fase 1: aceleração
        if (i < accel_steps) {
            // v² = v0² + 2·a·1  (1 step de distância)
            v_current = sqrtf(v_current * v_current + 2.0f * accel_steps_s2);
            if (v_current > v_peak) v_current = v_peak;
        }
        // Fase 3: desaceleração (espelho do accel a partir do fim)
        else if (i >= dominant - accel_steps) {
            float v_new = sqrtf(v_current * v_current - 2.0f * accel_steps_s2);
            if (!isnan(v_new) && v_new > 100.0f) {
                v_current = v_new;
            }
        }
        // Fase 2: velocidade constante (sem alteração de v_current)

        // Converte velocidade para delay em µs
        uint32_t delay_us = (uint32_t)(1000000.0f / v_current);
        if (delay_us < 50)  delay_us = 50;   // limite superior de velocidade
        if (delay_us > 100000) delay_us = 100000; // limite inferior

        // Subtrai o tempo já gasto nos pulsos STEP (2× STEP_PULSE_US por motor)
        uint32_t pulse_time = 4 * STEP_PULSE_US;
        if (delay_us > pulse_time) {
            delayMicroseconds(delay_us - pulse_time);
        }
    }
}

// -----------------------------------------------------------------------------
uint32_t Planner::feedToStepDelayUs(float feed_mm_min) const {
    if (feed_mm_min <= 0) return 100000;
    float steps_per_sec = (feed_mm_min / 60.0f) * STEPS_PER_MM;
    return (uint32_t)(1000000.0f / steps_per_sec);
}
