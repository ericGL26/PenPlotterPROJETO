#include <Arduino.h>
#include <MultiStepper.h>

#include "motorX.h"
#include "motorY.h"

MultiStepper steppers;

void inicializarControleMotores(){
    steppers.addStepper(motorX);
    steppers.addStepper(motorY);
}

void moverMotores(long passosX, long passosY){
    long destino[2];
    
    destino[0] = passosX;
    destino[1] = passosY;

    steppers.moveTo(destino);
    // REMOVIDO: steppers.runSpeedToPosition() — era bloqueante e travava o loop()
    // O avanço dos motores agora é feito por atualizarMotores() chamada no loop()
}

// Chama o run() do MultiStepper — deve ser chamada a cada iteração do loop()
// Retorna true se os motores ainda estão em movimento
bool atualizarMotores(){
    return steppers.run();
}