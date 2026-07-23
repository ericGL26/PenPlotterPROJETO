#include <Arduino.h>
#include "motorX.h"
#include "motorY.h"
#include "lerTratarDadosUGS.h"
#include "controleMotores.h"
#include "servoMotor.h"

void setup() {
  Serial.begin(115200);
  inicializarMotorX();
  inicializarMotorY();
  inicializarControleMotores();
  inicializarServoMotor();
}

void loop() {
  // Avanca os motores um passo por vez (nao bloqueante)
  atualizarMotores();

  if(movimentoEmExecucao == false){
    lerSerialUGS();
  }

  // Responde "ok" para o UGS liberar a proxima linha quando os motores terminaram
  if(movimentoEmExecucao && motorXTerminouPercurso() && motorYTerminouPercurso()){
      movimentoEmExecucao = false;
      Serial.println("ok");
  }
}