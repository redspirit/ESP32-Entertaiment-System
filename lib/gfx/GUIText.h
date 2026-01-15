#pragma once
#include <GUILayer.h>
#include <stdint.h>

namespace GUIText {
    #define TILE(m, x, y) ((m)[(y) * GUI::GRID_W + (x)])

    void printFontTable(uint8_t color_index);
    void printPaletteTable();
    void print(const char* text, int x, int y, uint8_t color_index);
    void printUTF8(const char* text, int x, int y, uint8_t color_index);
}