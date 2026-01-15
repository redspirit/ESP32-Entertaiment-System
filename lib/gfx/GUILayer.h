#pragma once
#include <stdint.h>

class VGA;

namespace GUI {

    constexpr int GRID_W = 40;
    constexpr int GRID_H = 30;

    struct Tile {
        uint8_t ch;    // символ из шрифта CP866
        uint8_t color; // индекс цвета из палитры (не сам цвет)
    };

    Tile* getTilemap();

    void clear();
    void drawTile(VGA& vga, int sx, int sy, Tile t, bool transparentBg);
    void render(VGA& vga);
}

