#ifndef MOTORY_H
#define MOTORY_H

#include <Arduino.h>
#include <AccelStepper.h>

extern AccelStepper motorY; //exportamos o motorY para o arquivo controleMotores poder manipula-lo

void inicializarMotorY();
void atualizarMotorY();
bool motorYTerminouPercurso();


#endif