#ifndef LERTRATAR_DADOS_UGS_H
#define LERTRATAR_DADOS_UGS_H   

#include <Arduino.h>

// struct para tratar GCODE definida como o padrão do projeto
struct GCodeStruct {
    int comandoG = -1;
    int comandoM = -1;

    bool possuiX = false;
    bool possuiY = false;
    bool possuiF = false;

    float x = 0;
    float y = 0;
    float feed = 0;
};

void lerSerialUGS();
void processarGCode(char* comando);

void ExecutarComandosRecebidosPeloUGS(const GCodeStruct& structGCode);

#endif