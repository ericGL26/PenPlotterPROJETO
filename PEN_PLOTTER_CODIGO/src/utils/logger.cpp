// =============================================================================
// utils/logger.cpp
// =============================================================================

#include "logger.h"
#include <stdarg.h>
#include <stdio.h>

Logger::SendCallback Logger::_callback = nullptr;

// -----------------------------------------------------------------------------
void Logger::sendWS(const char* msg) {
    if (_callback) {
        _callback(msg);
    }
}

void Logger::sendWS(const String& msg) {
    sendWS(msg.c_str());
}

void Logger::sendWSf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    sendWS(buf);
}

// -----------------------------------------------------------------------------
void Logger::debug(const char* msg) {
    Serial.println(msg);
}

void Logger::debugf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Serial.println(buf);
}
