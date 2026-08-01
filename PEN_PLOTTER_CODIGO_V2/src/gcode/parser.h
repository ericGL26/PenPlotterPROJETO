#pragma once
// =============================================================================
// gcode/parser.h — Parser de G-code
// Responsabilidade: converter uma string de texto em uma struct tipada.
// NUNCA conhece: WebSocket, motores, Wi-Fi.
// =============================================================================

#include <Arduino.h>

// Enum dos tipos de comando suportados
enum class GCodeType : uint8_t {
    UNKNOWN = 0,
    G0,     // Movimento rápido (sem caneta, salvo Z explícito)
    G1,     // Movimento linear controlado
    G2,     // Arco horário (CW) — I/J = offset do centro em relação ao ponto atual
    G3,     // Arco anti-horário (CCW) — mesma convenção de I/J
    G21,    // Unidades em mm (no-op — já é o padrão do firmware)
    G90,    // Modo absoluto
    G91,    // Modo relativo
    G92,    // Zerar posição
    M3,     // Caneta baixo (servo)
    M5,     // Caneta alto (servo)
    EMPTY   // Linha vazia, só comentário, ou delimitador '%'
};

// Resultado do parse de uma linha G-code
struct GCodeCommand {
    GCodeType type   = GCodeType::UNKNOWN;
    float     x      = 0.0f;
    float     y      = 0.0f;
    float     z      = 0.0f;
    float     i      = 0.0f;    // offset X do centro do arco (G2/G3)
    float     j      = 0.0f;    // offset Y do centro do arco (G2/G3)
    float     feed   = 0.0f;
    bool      hasX   = false;
    bool      hasY   = false;
    bool      hasZ   = false;
    bool      hasI   = false;
    bool      hasJ   = false;
    bool      hasF   = false;
    bool      valid  = false;   // false = erro de parse
    char      errorMsg[32] = "";
};

class GCodeParser {
public:
    // Parseia uma linha G-code.
    // Entrada:  "G1 X25.4 Y-10.0 F3000"
    // Saída:    GCodeCommand preenchida
    GCodeCommand parse(const char* line);

private:
    // Remove comentários (; e (...))
    void stripComments(char* buf);

    // Converte para maiúsculas
    void toUpperCase(char* buf);

    // Busca parâmetro por letra (ex: 'X', 'Y', 'F')
    // Retorna ponteiro para o char da letra, ou nullptr se não encontrado
    const char* findParam(const char* str, char letter);

    // Extrai float após uma letra (ex: "X25.4" → 25.4)
    float extractFloat(const char* ptr);

    // Mapeia número G/M para GCodeType
    GCodeType resolveGCode(int g_num);
    GCodeType resolveMCode(int m_num);
};

// Instância global
extern GCodeParser gcodeParser;
