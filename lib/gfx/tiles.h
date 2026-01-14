#pragma once
#include <stdint.h>

class VGA;

// Инициализация тестовой карты (ASCII 0..127)
void initTilemapTest();
void clearTilemap();
void initTilemapFontTable();

// Рендер всей тайловой карты
void renderTilemap(
    VGA& vga,
    uint8_t fgColor,
    uint8_t bgColor
);

void printText(const char* text, int x, int y);