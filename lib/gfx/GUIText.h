#pragma once
#include <GUILayer.h>
#include <stdint.h>

namespace GUIText {
    #define TILE(m, x, y) ((m)[(y) * GUI::GRID_W + (x)])

    //printing
    void print(const char* text, int x, int y, uint8_t color_index);
    void printUTF8(const char* text, int x, int y, uint8_t color_index);

    // cursor
    void setCursor(bool isVisible);
    void moveCursor(int x, int y);
    void tick(float dt);
    void renderCursor(VGA& vga);
}