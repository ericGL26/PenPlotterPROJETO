// =============================================================================
// gcode/parser.cpp — Implementação do parser de G-code
// =============================================================================

#include "parser.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// Instância global
GCodeParser gcodeParser;

// -----------------------------------------------------------------------------
GCodeCommand GCodeParser::parse(const char* line) {
    GCodeCommand cmd;

    // Copia a linha para um buffer mutável
    char buf[128];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    // 1. Remove comentários
    stripComments(buf);

    // 2. Converte para maiúsculas
    toUpperCase(buf);

    // 3. Remove espaços no início
    char* p = buf;
    while (*p == ' ' || *p == '\t') p++;

    // 4. Linha vazia após limpeza
    if (*p == '\0') {
        cmd.type  = GCodeType::EMPTY;
        cmd.valid = true;
        return cmd;
    }

    // 4b. Delimitador de início/fim de programa ('%') — comum em arquivos de CAM/CNC
    if (strcmp(p, "%") == 0) {
        cmd.type  = GCodeType::EMPTY;
        cmd.valid = true;
        return cmd;
    }

    // 5. Extrai código G
    const char* gPtr = findParam(p, 'G');
    const char* mPtr = findParam(p, 'M');

    if (gPtr != nullptr) {
        int gNum = (int)extractFloat(gPtr + 1);
        cmd.type = resolveGCode(gNum);
    } else if (mPtr != nullptr) {
        int mNum = (int)extractFloat(mPtr + 1);
        cmd.type = resolveMCode(mNum);
    } else {
        // Linha sem G nem M — pode ser só parâmetros (ex: "X10 Y20" após G1 modal)
        // Por simplicidade, marcamos como UNKNOWN
        cmd.type = GCodeType::UNKNOWN;
        snprintf(cmd.errorMsg, sizeof(cmd.errorMsg), "No G/M code found");
        cmd.valid = false;
        return cmd;
    }

    if (cmd.type == GCodeType::UNKNOWN) {
        snprintf(cmd.errorMsg, sizeof(cmd.errorMsg), "Unsupported G/M code");
        cmd.valid = false;
        return cmd;
    }

    // 6. Extrai parâmetros X, Y, Z, I, J, F
    const char* xPtr = findParam(p, 'X');
    const char* yPtr = findParam(p, 'Y');
    const char* zPtr = findParam(p, 'Z');
    const char* iPtr = findParam(p, 'I');
    const char* jPtr = findParam(p, 'J');
    const char* fPtr = findParam(p, 'F');

    if (xPtr) { cmd.x = extractFloat(xPtr + 1); cmd.hasX = true; }
    if (yPtr) { cmd.y = extractFloat(yPtr + 1); cmd.hasY = true; }
    if (zPtr) { cmd.z = extractFloat(zPtr + 1); cmd.hasZ = true; }
    if (iPtr) { cmd.i = extractFloat(iPtr + 1); cmd.hasI = true; }
    if (jPtr) { cmd.j = extractFloat(jPtr + 1); cmd.hasJ = true; }
    if (fPtr) { cmd.feed = extractFloat(fPtr + 1); cmd.hasF = true; }

    cmd.valid = true;
    return cmd;
}

// -----------------------------------------------------------------------------
void GCodeParser::stripComments(char* buf) {
    // Remove bloco entre parênteses
    char* start = strchr(buf, '(');
    if (start) {
        char* end = strchr(start, ')');
        if (end) {
            memmove(start, end + 1, strlen(end + 1) + 1);
        } else {
            *start = '\0';
        }
    }
    // Remove comentário após ponto-e-vírgula
    char* semi = strchr(buf, ';');
    if (semi) *semi = '\0';
}

// -----------------------------------------------------------------------------
void GCodeParser::toUpperCase(char* buf) {
    for (int i = 0; buf[i] != '\0'; i++) {
        buf[i] = (char)toupper((unsigned char)buf[i]);
    }
}

// -----------------------------------------------------------------------------
const char* GCodeParser::findParam(const char* str, char letter) {
    const char* p = str;
    while (*p != '\0') {
        if (*p == letter) {
            char next = *(p + 1);
            if (isdigit((unsigned char)next) || next == '-' || next == '.') {
                return p;
            }
        }
        p++;
    }
    return nullptr;
}

// -----------------------------------------------------------------------------
float GCodeParser::extractFloat(const char* ptr) {
    return strtof(ptr, nullptr);
}

// -----------------------------------------------------------------------------
GCodeType GCodeParser::resolveGCode(int g_num) {
    switch (g_num) {
        case 0:  return GCodeType::G0;
        case 1:  return GCodeType::G1;
        case 2:  return GCodeType::G2;
        case 3:  return GCodeType::G3;
        case 21: return GCodeType::G21;
        case 90: return GCodeType::G90;
        case 91: return GCodeType::G91;
        case 92: return GCodeType::G92;
        default: return GCodeType::UNKNOWN;
    }
}

GCodeType GCodeParser::resolveMCode(int m_num) {
    switch (m_num) {
        case 3:  return GCodeType::M3;
        case 5:  return GCodeType::M5;
        default: return GCodeType::UNKNOWN;
    }
}
