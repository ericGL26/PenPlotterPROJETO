#include "lerTratarDadosUGS.h"
#include "motorX.h"
#include "calcularPassoMotor.h"
#include "motorY.h"
#define TAMANHO_BUFFER 64 //indica o tamanho maximo do buffer
char buffer[TAMANHO_BUFFER]; //Reserva 64 bytes de memoria para o buffer
int bufferIndex = 0; //o bufferIndex indica a proxima posicao livre na memoria

void lerSerialUGS() {
    while (Serial.available() > 0) {
        char caractere = Serial.read();

        if(caractere == '\n' || caractere == '\r'){
            if (bufferIndex > 0) {
                buffer[bufferIndex] = '\0';

                processarGCode(buffer);

                bufferIndex = 0;
        }
    }else{
        if(bufferIndex < TAMANHO_BUFFER -1){
            buffer[bufferIndex] = caractere; //guarda o caractere no buffer no index bufferIndex
            bufferIndex++;
            }
        }
    }
}

//Funcao para atribuir valores corretos a struct GCode
void processarGCode(char* comando){
    GCodeStruct structGCode;
    char* ponteiro;

    //Converte todo o texto para maiusculo caso o UGS envie x ou y minusculo
    for(int i = 0; comando[i] != '\0'; i++){
        comando[i] = toupper(comando[i]);
    }

    //Busca o comando G
    ponteiro = strchr(comando, 'G'); //strchr significa string character, busca o caractere G na string comando
    if(ponteiro != NULL){
        structGCode.comandoG = atoi(ponteiro + 1); //Se encontrar a coordenada G no comando vindo do UGS, substituira o valor -1 por 1 na struct padrao que é comandoG = -1, sinalizando que deve se mover tambem nesse eixo
    }

    // Busca o comando M
    ponteiro = strchr(comando, 'M');
    if (ponteiro != NULL) {
        structGCode.comandoM = atoi(ponteiro + 1);
    }

    // Busca a coordenada X
    ponteiro = strchr(comando, 'X');
    if (ponteiro != NULL) {
        structGCode.possuiX = true;
        structGCode.x = atof(ponteiro + 1); // 'atof' converte o texto após o 'X' para Float
    }

    // Busca a coordenada Y
    ponteiro = strchr(comando, 'Y');
    if (ponteiro != NULL) {
        structGCode.possuiY = true;
        structGCode.y = atof(ponteiro + 1);
    }

    // Busca a velocidade (Feed rate)
    ponteiro = strchr(comando, 'F');
    if (ponteiro != NULL) {
        structGCode.possuiF = true;
        structGCode.feed = atof(ponteiro + 1);
    }
    // Agora que a struct e o calculo da quantidade de passos do motor está pronta, mandamos para a função que mexe os motores
    ExecutarComandosRecebidosPeloUGS(structGCode);

    // RESPONDE OK PARA O UGS LIBERAR A PRÓXIMA LINHA
    Serial.println("ok");
}

void ExecutarComandosRecebidosPeloUGS(const GCodeStruct& structGCode){
    //chama a funcao que calcula a quantidade de passos que o motor tem que dar para alcancar a localizacao do desenho
    long passosX = calcularPassoMotorX(structGCode);
    long passosY = calcularPassoMotorY(structGCode);

    moverMotorX(passosX);
    moverMotorY(passosY);
}