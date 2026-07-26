#include "lerTratarDadosUGS.h"
#include "servoMotor.h"

#include <Arduino.h>
#include <ESP32Servo.h>

Servo servoCaneta;

const int PINO_SERVO = 23;

const int ANGULO_LEVANTADA = 0;
const int ANGULO_ABAIXADA  = 90;

bool _canetaAbaixada = false;

void levantarServoCaneta(){
    servoCaneta.write(ANGULO_LEVANTADA);
    _canetaAbaixada = false;
}

void inicializarServoMotor(){
    servoCaneta.attach(PINO_SERVO);
    levantarServoCaneta();
}

void abaixarServoCaneta(){
    servoCaneta.write(ANGULO_ABAIXADA);
    _canetaAbaixada = true;
}

// Corrigido: renomeada de 'estadoCaneta()' para 'canetaEstaAbaixada()' (igual ao header)
bool canetaEstaAbaixada(){
    return _canetaAbaixada;
}