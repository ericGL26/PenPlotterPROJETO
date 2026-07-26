#ifndef MOTORX_H
#define MOTORX_H

#include <Arduino.h>
#include <AccelStepper.h>

#include "calcularPassoMotor.h"

extern AccelStepper motorX; //exportamos o motoX para o arquivo controleMotores poder manipula-lo

void inicializarMotorX();
void atualizarMotorX();
bool motorXTerminouPercurso();



#endif