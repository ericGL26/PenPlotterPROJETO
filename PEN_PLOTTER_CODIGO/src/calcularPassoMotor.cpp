#include <Arduino.h>
#include "calcularPassoMotor.h"

#include "motorX.h"
#include "motorY.h"
#include "lerTratarDadosUGS.h"

//formula para calcular passo:     PASSOS = distancia em mm * Passos por mm
//EX (Motor de 200 passos, Driver em 1/16, Correia GT2 (2mm) e polia de 20 dentes.)
//Passos por mm = (200*16) = 3200 = 80 passos por MM
//                  2*20      40

const float passosPorMilimetroX = 80; //valor calculado de acordo com a maquina
const float passosPorMilimetroY = 80; //valor calculado de acordo com a maquina

long calcularPassoMotorX(const GCodeStruct& structGCode){
    // Se o comando nao tem coordenada X, mantem a posicao atual do motor (nao se move no eixo X)
    if (!structGCode.possuiX) {
        return motorX.currentPosition();
    }
    return (long)(structGCode.x * passosPorMilimetroX);
};

long calcularPassoMotorY(const GCodeStruct& structGCode){
    // Se o comando nao tem coordenada Y, mantem a posicao atual do motor (nao se move no eixo Y)
    if (!structGCode.possuiY) {
        return motorY.currentPosition();
    }
    return (long)(structGCode.y * passosPorMilimetroY);
}