#include <Arduino.h>
#include <AccelStepper.h>

const int PINO_PASSO_MOTORY = 15;
const int PINO_DIRECAO_MOTORY = 29;

// Cria a instância do motor aqui dentro
AccelStepper motorY(1, PINO_PASSO_MOTORY, PINO_DIRECAO_MOTORY); //o 1 significa apenas que estamos usando um driver comum como o a4988

void inicializarMotorY() {
    motorY.setMaxSpeed(1000);
    motorY.setAcceleration(500);
}

void moverMotorY(long passosY){
    motorY.moveTo(passosY);
}

void atualizarMotorY(){
    motorY.run();
}