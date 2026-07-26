#ifndef CALCULAR_PASSOS_MOTOR_H
#define CALCULAR_PASSOS_MOTOR_H

#include <Arduino.h>
#include "lerTratarDadosUGS.h"

long calcularPassoMotorX(const GCodeStruct& structGCode);
long calcularPassoMotorY(const GCodeStruct& structGCode);
#endif