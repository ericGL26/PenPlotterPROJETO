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
  
  // Envia a mensagem de boas-vindas do GRBL para o UGS abrir a conexão com sucesso
  Serial.println("Grbl 1.1f ['$' for help]");
}

void loop() {
  // Avanca os motores um passo por vez (nao bloqueante)
  atualizarMotores();

  // Sempre le a serial para poder responder ao ? do UGS durante o movimento
  lerSerialUGS();

  // Responde "ok" para o UGS liberar a proxima linha quando os motores terminaram
  if(movimentoEmExecucao && motorXTerminouPercurso() && motorYTerminouPercurso()){
      movimentoEmExecucao = false;
      Serial.println("ok");
  }
}