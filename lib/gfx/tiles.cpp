#include "tiles.h"
#include "font8x8.h"
#include "VGA.h"

static constexpr int TILE_W = 8;
static constexpr int TILE_H = 8;
static constexpr int GRID_H = 30;
static constexpr int GRID_W = 40;

static char tilemap[GRID_H][GRID_W];

void drawTile(
    VGA& vga,
    int sx,
    int sy,
    char ch,
    uint8_t fgColor,
    uint8_t bgColor,
    bool transparentBg
) {
    const uint8_t* glyph = font8x8::get(ch);

    for (int y = 0; y < TILE_H; y++) {
        uint8_t row = glyph[y];
        for (int x = 0; x < TILE_W; x++) {
            bool bit = row & (1 << x);
            if (bit) {
                vga.dot(sx + x, sy + y, fgColor);
            } else if (!transparentBg) {
                vga.dot(sx + x, sy + y, bgColor);
            }
        }
    }
}

void initTilemapTest() {
    char c = 0;
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            tilemap[y][x] = c;
            c++;
            if (c >= 128) c = 0;
        }
    }
}

void printText(const char* text, int x, int y) {
    int cx = x;
    int cy = y;

    while (*text) {
        char c = *text++;

        if (c == '\n') {
            cx = x;
            cy++;
            if (cy >= GRID_H) break;
            continue;
        }

        if (cx < 0 || cx >= GRID_W || cy < 0 || cy >= GRID_H)
            continue;

        tilemap[cy][cx] = c;
        cx++;
    }
}

void renderTilemap(
    VGA& vga,
    uint8_t fgColor,
    uint8_t bgColor
) {
    for (int ty = 0; ty < GRID_H; ty++) {
        for (int tx = 0; tx < GRID_W; tx++) {
            char c = tilemap[ty][tx];
            if (c == 0) continue;
            drawTile(
                vga,
                tx * TILE_W,
                ty * TILE_H,
                c,
                fgColor,
                bgColor,
                true
            );
        }
    }
}