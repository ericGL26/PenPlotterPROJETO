#include "lerTratarDadosUGS.h"
#include "motorX.h"
#include "calcularPassoMotor.h"
#include "motorY.h"
#include "controleMotores.h"
#include "servoMotor.h"

#define TAMANHO_BUFFER 128 // indica o tamanho maximo do buffer
char buffer[TAMANHO_BUFFER]; 
int bufferIndex = 0; 
bool movimentoEmExecucao = false; // Variavel de controle, movimento dos motores
static bool ultimoFoiCR = false;

// Funcao auxiliar para buscar parametros como 'G1', 'X10.5', 'M3' no comando
static char* buscarParametro(char* comando, char letra, bool apenasDigito = false) {
    char* p = comando;
    while ((p = strchr(p, letra)) != NULL) {
        char proximo = *(p + 1);
        if (apenasDigito) {
            if (isdigit((unsigned char)proximo)) {
                return p;
            }
        } else {
            if (isdigit((unsigned char)proximo) || proximo == '-' || proximo == '.') {
                return p;
            }
        }
        p++; // avança para procurar proxima ocorrencia
    }
    return NULL;
}

// Limpa comentarios do GCode (ex: (comentario) e ;comentario)
static void limparComando(char* comando) {
    // 1. Remove comentarios entre parenteses ( ... )
    char* inicioParenteses = strchr(comando, '(');
    if (inicioParenteses != NULL) {
        char* fimParenteses = strchr(inicioParenteses, ')');
        if (fimParenteses != NULL) {
            for (char* p = inicioParenteses; p <= fimParenteses; p++) {
                *p = ' ';
            }
        } else {
            *inicioParenteses = '\0';
        }
    }

    // 2. Remove comentarios apos ponto e virgula ; ...
    char* pontoVirgula = strchr(comando, ';');
    if (pontoVirgula != NULL) {
        *pontoVirgula = '\0';
    }
}

static void aguardarEProcessarBuffer() {
    // Se houver movimento em execucao do comando anterior,
    // aguarda a conclusao antes de processar o novo comando.
    while (movimentoEmExecucao) {
        atualizarMotores();

        // Trata '?' se chegar durante a espera do movimento
        if (Serial.available() > 0 && Serial.peek() == '?') {
            Serial.read();
            float posX = (float)motorX.currentPosition() / 80.0f;
            float posY = (float)motorY.currentPosition() / 80.0f;
            Serial.print("<Run|WPos:");
            Serial.print(posX, 3);
            Serial.print(',');
            Serial.print(posY, 3);
            Serial.print(",0.000|Bf:15,128|FS:0,0>\r\n");
        }

        if (motorXTerminouPercurso() && motorYTerminouPercurso()) {
            movimentoEmExecucao = false;
            Serial.println("ok");
            break;
        }
    }

    processarGCode(buffer);
    bufferIndex = 0;
}

void lerSerialUGS() {
    while (Serial.available() > 0) {
        char caractere = Serial.read();

        // '?' é tratado SEMPRE, mesmo durante o movimento, para o status do UGS se atualizar
        if (caractere == '?') {
            float posX = (float)motorX.currentPosition() / 80.0f;
            float posY = (float)motorY.currentPosition() / 80.0f;
            Serial.print('<');
            Serial.print(movimentoEmExecucao ? "Run" : "Idle");
            Serial.print("|WPos:");
            Serial.print(posX, 3);
            Serial.print(',');
            Serial.print(posY, 3);
            // Bf:15,128 informa ao UGS que ha espaco livre no buffer (evita travamento do UGS)
            Serial.print(",0.000|Bf:15,128|FS:0,0>\r\n");
            continue;
        }

        if (caractere == '\r') {
            ultimoFoiCR = true;
            buffer[bufferIndex] = '\0';
            aguardarEProcessarBuffer();
        } 
        else if (caractere == '\n') {
            if (ultimoFoiCR) {
                // '\n' apos '\r' (CRLF), ja foi processado pelo '\r'
                ultimoFoiCR = false;
            } else {
                // Linha terminada apenas por '\n' (padrão Linux/UGS)
                buffer[bufferIndex] = '\0';
                aguardarEProcessarBuffer();
            }
        } 
        else {
            ultimoFoiCR = false;
            if (bufferIndex < TAMANHO_BUFFER - 1) {
                buffer[bufferIndex] = caractere;
                bufferIndex++;
            }
        }
    }
}

// Funcao para atribuir valores corretos a struct GCode
void processarGCode(char* comando) {
    // 1. Limpa comentarios
    limparComando(comando);

    // 2. Converte todo o texto para maiusculo
    for (int i = 0; comando[i] != '\0'; i++) {
        comando[i] = toupper((unsigned char)comando[i]);
    }

    // Trim de espacos no inicio
    char* cmdPtr = comando;
    while (*cmdPtr == ' ' || *cmdPtr == '\t') {
        cmdPtr++;
    }

    // 3. Trata comandos especiais do GRBL ($G, $I, $$, $#, $C, $X, etc.)
    if (*cmdPtr == '$') {
        if (strcmp(cmdPtr, "$$") == 0) {
            // Retorna configuracoes padrao do GRBL para o UGS preencher suas estruturas internas
            Serial.println("$0=10");
            Serial.println("$1=25");
            Serial.println("$2=0");
            Serial.println("$3=0");
            Serial.println("$4=0");
            Serial.println("$5=0");
            Serial.println("$6=0");
            Serial.println("$10=1");
            Serial.println("$11=0.010");
            Serial.println("$12=0.002");
            Serial.println("$13=0");
            Serial.println("$20=0");
            Serial.println("$21=0");
            Serial.println("$22=0");
            Serial.println("$23=0");
            Serial.println("$24=25.000");
            Serial.println("$25=500.000");
            Serial.println("$26=250");
            Serial.println("$27=1.000");
            Serial.println("$30=1000");
            Serial.println("$31=0");
            Serial.println("$32=0");
            Serial.println("$100=80.000");
            Serial.println("$101=80.000");
            Serial.println("$102=80.000");
            Serial.println("$110=1000.000");
            Serial.println("$111=1000.000");
            Serial.println("$112=1000.000");
            Serial.println("$120=500.000");
            Serial.println("$121=500.000");
            Serial.println("$122=500.000");
            Serial.println("$130=200.000");
            Serial.println("$131=200.000");
            Serial.println("$132=200.000");
            Serial.println("ok");
        } else if (strcmp(cmdPtr, "$G") == 0) {
            Serial.println("[GC:G0 G54 G17 G21 G90 G94 M3 M5 T0 F0 S0]");
            Serial.println("ok");
        } else if (strcmp(cmdPtr, "$I") == 0) {
            Serial.println("[VER:1.1f.20260726:]");
            Serial.println("ok");
        } else if (strcmp(cmdPtr, "$#") == 0) {
            Serial.println("[G54:0.000,0.000,0.000]");
            Serial.println("[G55:0.000,0.000,0.000]");
            Serial.println("[G56:0.000,0.000,0.000]");
            Serial.println("[G57:0.000,0.000,0.000]");
            Serial.println("[G58:0.000,0.000,0.000]");
            Serial.println("[G59:0.000,0.000,0.000]");
            Serial.println("[G28:0.000,0.000,0.000]");
            Serial.println("[G30:0.000,0.000,0.000]");
            Serial.println("[G92:0.000,0.000,0.000]");
            Serial.println("[TLO:0.000]");
            Serial.println("[PRB:0.000,0.000,0.000:0]");
            Serial.println("ok");
        } else if (strcmp(cmdPtr, "$C") == 0) {
            Serial.println("[MSG:Check mode disabled]");
            Serial.println("ok");
        } else if (strcmp(cmdPtr, "$X") == 0) {
            Serial.println("[MSG:Caution: Unlocked]");
            Serial.println("ok");
        } else {
            Serial.println("ok");
        }
        return;
    }

    // Se o comando estiver vazio apos limpar comentarios/espacos, responde ok
    if (*cmdPtr == '\0') {
        Serial.println("ok");
        return;
    }

    GCodeStruct structGCode;
    char* ponteiro;

    // Busca o comando G (ex: G0, G1, G21) - apenas se seguido de digito
    ponteiro = buscarParametro(cmdPtr, 'G', true);
    if (ponteiro != NULL) {
        structGCode.comandoG = atoi(ponteiro + 1);
    }

    // Busca o comando M (ex: M3, M5) - apenas se seguido de digito
    ponteiro = buscarParametro(cmdPtr, 'M', true);
    if (ponteiro != NULL) {
        structGCode.comandoM = atoi(ponteiro + 1);
    }

    // Busca a coordenada X
    ponteiro = buscarParametro(cmdPtr, 'X', false);
    if (ponteiro != NULL) {
        structGCode.possuiX = true;
        structGCode.x = atof(ponteiro + 1);
    }

    // Busca a coordenada Y
    ponteiro = buscarParametro(cmdPtr, 'Y', false);
    if (ponteiro != NULL) {
        structGCode.possuiY = true;
        structGCode.y = atof(ponteiro + 1);
    }

    // Busca a coordenada Z
    ponteiro = buscarParametro(cmdPtr, 'Z', false);
    if (ponteiro != NULL) {
        structGCode.possuiZ = true;
        structGCode.z = atof(ponteiro + 1);
    }

    // Busca a velocidade (Feed rate)
    ponteiro = buscarParametro(cmdPtr, 'F', false);
    if (ponteiro != NULL) {
        structGCode.possuiF = true;
        structGCode.feed = atof(ponteiro + 1);
    }

    // Agora que a struct esta pronta mandamos para a funcao que mexe os motores
    ExecutarComandosRecebidosPeloUGS(structGCode);
}

void ExecutarComandosRecebidosPeloUGS(const GCodeStruct& structGCode) {
    // --- Controle do Servo da Caneta ---
    if (structGCode.comandoG == 0) {
        levantarServoCaneta();
    }
    if (structGCode.possuiZ) {
        if (structGCode.z < 0) {
            abaixarServoCaneta();
        } else {
            levantarServoCaneta();
        }
    }
    if (structGCode.comandoM == 3) {
        abaixarServoCaneta();
    }
    if (structGCode.comandoM == 5) {
        levantarServoCaneta();
    }

    // --- Movimentacao dos Motores ---
    if (structGCode.comandoG == 0 || structGCode.comandoG == 1 || structGCode.comandoG == 2 || structGCode.comandoG == 3) {
        long passosX = calcularPassoMotorX(structGCode);
        long passosY = calcularPassoMotorY(structGCode);

        // Verifica se ha movimento real necessario
        long deltaX = labs(passosX - motorX.currentPosition());
        long deltaY = labs(passosY - motorY.currentPosition());

        if (deltaX > 0 || deltaY > 0) {
            moverMotores(passosX, passosY);
            movimentoEmExecucao = true;
            // O 'ok' sera enviado quando os motores terminarem a trajetoria
        } else {
            // Nao precisa mover os motores (ex: G0 Z5 ou G1 com mesma coordenada)
            movimentoEmExecucao = false;
            Serial.println("ok");
        }
    } else {
        // Para qualquer outro comando (ex: G21, G90, M3/M5 isolado, etc.)
        movimentoEmExecucao = false;
        Serial.println("ok");
    }
}