#include <Arduino.h>
#include <AccelStepper.h>

#include "lerTratarDadosUGS.h"
#include "calcularPassoMotor.h"

const int PINO_PASSO_MOTORY = 15;
const int PINO_DIRECAO_MOTORY = 26; // GPIO 29 não existe no ESP32; usando GPIO 26

// Cria a instância do motor aqui dentro
AccelStepper motorY(1, PINO_PASSO_MOTORY, PINO_DIRECAO_MOTORY); //o 1 significa apenas que estamos usando um driver comum como o a4988

void inicializarMotorY() {
    motorY.setMaxSpeed(1000);
    motorY.setAcceleration(500);
}

bool motorYTerminouPercurso(){
    if(motorY.distanceToGo() == 0){
        return true;
    }else{
        return false;
    }
}

void atualizarMotorY(){
    motorY.run();
}