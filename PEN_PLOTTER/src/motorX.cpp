#include <Arduino.h>
#include <AccelStepper.h>

const int PINO_PASSO_MOTORX = 13;
const int PINO_DIRECAO_MOTORX = 21;

// Cria a instância do motor aqui dentro
AccelStepper motorX(1, PINO_PASSO_MOTORX, PINO_DIRECAO_MOTORX); //o 1 significa apenas que estamos usando um driver comum como o a4988

void inicializarMotorX() {
    motorX.setMaxSpeed(1000);
    motorX.setAcceleration(500);
}

void moverMotorX(long passosX){
    motorX.moveTo(passosX);
}

void atualizarMotorX(){
    motorX.run();
}