#include "GUIText.h"
#include "GUILayer.h"

namespace GUIText {

    GUI::Tile* map = GUI::getTilemap();

    static char hexDigit(uint8_t v) {
        return (v < 10) ? ('0' + v) : ('A' + (v - 10));
    }

    void printFontTable(uint8_t color_index) {
        int row = 1;

        for (int base = 0; base < 256 && row < GUI::GRID_H; base += 16, row++) {
            int x = 1;

            auto put = [&](char c) {
                if (x < GUI::GRID_W) {
                    TILE(map, x, row).ch = c;
                    TILE(map, x, row).color = color_index;
                    x++;
                }
            };

            // 0xNN
            put('0');
            put('x');
            put(hexDigit((base >> 4) & 0xF));
            put(hexDigit(base & 0xF));

            put(' ');

            // 16 символов строки
            for (int i = 0; i < 16 && x < GUI::GRID_W; i++) {
                put((char)(base + i));
            }
        }
    }


    uint8_t utf8ToCp866(const char*& p) {
        uint8_t c = (uint8_t)*p++;

        // ASCII
        if (c < 0x80)
            return c;

        // Ожидаем двухбайтовый UTF-8
        uint8_t c2 = (uint8_t)*p++;

        // А..Я
        if (c == 0xD0 && c2 >= 0x90 && c2 <= 0xAF)
            return 0x80 + (c2 - 0x90);

        // а..п
        if (c == 0xD0 && c2 >= 0xB0 && c2 <= 0xBF)
            return 0xA0 + (c2 - 0xB0);

        // р..я
        if (c == 0xD1 && c2 >= 0x80 && c2 <= 0x8F)
            return 0xE0 + (c2 - 0x80);

        // Ё
        if (c == 0xD0 && c2 == 0x81)
            return 0xF0;

        // ё
        if (c == 0xD1 && c2 == 0x91)
            return 0xF1;

        return '?';
    }

    void print(const char* text, int x, int y, uint8_t color_index) {
        int cx = x;
        int cy = y;

        while (*text) {
            char c = *text++;

            if (c == '\n') {
                cx = x;
                cy++;
                if (cy >= GUI::GRID_H) break;
                continue;
            }

            if (cx < 0 || cx >= GUI::GRID_W || cy < 0 || cy >= GUI::GRID_H)
                continue;

            TILE(map, cx, cy).ch = c;
            TILE(map, cx, cy).color = color_index;
            cx++;
        }
    }

    void printUTF8(const char* text, int x, int y, uint8_t color_index) {
        int cx = x;
        int cy = y;

        while (*text && cy < GUI::GRID_H) {

            if (*text == '\n') {
                text++;
                cx = x;
                cy++;
                continue;
            }

            uint8_t ch = utf8ToCp866(text);

            if (cx >= 0 && cx < GUI::GRID_W && cy >= 0 && cy < GUI::GRID_H) {
                TILE(map, cx, cy).ch = ch;
                TILE(map, cx, cy).color = color_index;
            }

            cx++;
            if (cx >= GUI::GRID_W) {
                cx = x;
                cy++;
            }
        }
    }

}