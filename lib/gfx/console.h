#pragma once

#include <stdint.h>

namespace console {
    void clear();
    void setColor(uint8_t color);
    void useDefaultColor(); // устанавливает текущий цвет цветом по умолчанию
    void print(const char* text);
    void print(char c);
    void printLn(const char* text);
    void printLn();
    void print(int value);
    void printLn(int value);
    void clearCharAt(int x, int y);
    void setCursor(int x, int y);
    void getCursor(int& x, int& y);
}