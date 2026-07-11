#include <Arduino.h>
#include "motorX.h"
#include "motorY.h"

void setup() {
  Serial.begin(115200);
  
  inicializarMotorX();
}

void loop() {
  atualizarMotorX();
}