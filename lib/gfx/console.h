#pragma once

#include <stdint.h>

namespace console {
    void clear();
    void setColor(uint8_t color);
    void print(const char* text);
    void print(char c);
    void printLn(const char* text);
    void printLn();
    void clearCharAt(int x, int y);
    void setCursor(int x, int y);
    void getCursor(int& x, int& y);    
}