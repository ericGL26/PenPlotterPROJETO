#ifndef CALCULAR_PASSOS_MOTOR_H
#define CALCULAR_PASSOS_MOTOR_H

#include <Arduino.h>
#include "lerTratarDadosUGS.h"

int calcularPassoMotorX(const GCodeStruct& structGCode);
int calcularPassoMotorY(const GCodeStruct& structGCode);
#endif