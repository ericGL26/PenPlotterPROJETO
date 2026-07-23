#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include <Arduino.h>

void inicializarServoMotor();
void levantarServoCaneta();
void abaixarServoCaneta();

bool canetaEstaAbaixada();

#endif