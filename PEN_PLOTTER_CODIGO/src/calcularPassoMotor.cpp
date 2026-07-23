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
    long passosX = structGCode.x * passosPorMilimetroX;
    return passosX;
};

long calcularPassoMotorY(const GCodeStruct& structGCode){
    long passosY = structGCode.y * passosPorMilimetroY;
    return passosY;
}