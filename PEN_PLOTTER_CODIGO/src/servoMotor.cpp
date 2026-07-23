#include "lerTratarDadosUGS.h" // corrigido: era "lerTratarDados.h" (arquivo inexistente)

#include <Arduino.h>
#include <ESP32Servo.h>

Servo servoCaneta;

const int PINO_SERVO = 23;

const int ANGULO_LEVANTADA = 0;
const int ANGULO_ABAIXADA  = 90;

// Renomeada para '_canetaAbaixada' para evitar conflito com a função 'canetaEstaAbaixada()'
bool _canetaAbaixada = false;

void inicializarServoMotor(){
    servoCaneta.attach(PINO_SERVO);
    levantarServoCaneta(); // corrigido: era 'levantarCaneta()' (função inexistente)
}

void levantarServoCaneta(){
    servoCaneta.write(ANGULO_LEVANTADA);
    _canetaAbaixada = false;
}

void abaixarServoCaneta(){
    servoCaneta.write(ANGULO_ABAIXADA);
    _canetaAbaixada = true;
}

// Corrigido: renomeada de 'estadoCaneta()' para 'canetaEstaAbaixada()' (igual ao header)
bool canetaEstaAbaixada(){
    return _canetaAbaixada;
}