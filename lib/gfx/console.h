#pragma once

#include <stdint.h>

namespace console {
    void clear();
    void setColor(uint8_t color);
    void print(const char* text);
    void printLn(const char* text);
    void printLn();
}