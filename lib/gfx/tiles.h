#pragma once
#include <stdint.h>

class VGA;

// Инициализация тестовой карты (ASCII 0..127)
void initTilemapTest();

// Рисование одного символа 8x8
void drawTile(
    VGA& vga,
    int sx,
    int sy,
    char ch,
    uint8_t fgColor,
    uint8_t bgColor,
    bool transparentBg
);

// Рендер всей тайловой карты
void renderTilemap(
    VGA& vga,
    uint8_t fgColor,
    uint8_t bgColor
);

void printText(const char* text, int x, int y);