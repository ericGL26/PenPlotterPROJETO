#ifndef CONTROLE_MOTORES_H
#define CONTROLE_MOTORES_H

#include <Arduino.h>

void inicializarControleMotores();
void moverMotores(long passosX, long passosY);
bool atualizarMotores(); // deve ser chamada a cada iteracao do loop()

#endif