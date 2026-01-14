#include "tiles.h"
// #include "font8x8.h"
#include "font8x8cyr.h"
// #include "font8x8cp866.h"
#include "VGA.h"

static constexpr int TILE_W = 8;
static constexpr int TILE_H = 8;
static constexpr int GRID_H = 30;
static constexpr int GRID_W = 40;

static char tilemap[GRID_H][GRID_W];

void clearTilemap() {
    memset(tilemap, 0, GRID_H * GRID_W);
}

void drawTile(
    VGA& vga,
    int sx,
    int sy,
    char ch,
    uint8_t fgColor,
    uint8_t bgColor,
    bool transparentBg
) {
    const uint8_t* glyph = font8x8cyr::get(ch);

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

void drawTileFast(
    VGA& vga,
    int sx,
    int sy,
    char ch,
    uint8_t fgColor,
    uint8_t bgColor,
    bool transparentBg
) {
    const uint8_t* glyph = font8x8cyr::get(ch);

    for (int y = 0; y < 8; y++) {
        uint8_t row = glyph[y];

        uint8_t* dst = vga.dmaBuffer->getLineAddr8(sy + y, vga.backBuffer) + sx;

        if (transparentBg) {
            for (int x = 0; x < 8; x++) {
                if (row & (1 << (7-x)))
                    dst[x] = fgColor;
            }
        } else {
            for (int x = 0; x < 8; x++) {
                dst[x] = (row & (1 << (7-x))) ? fgColor : bgColor;
            }
        }
    }
}


static char hexDigit(uint8_t v) {
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

void initTilemapFontTable() {
    int row = 1;

    for (int base = 0; base < 256 && row < GRID_H; base += 16, row++) {
        int x = 1;

        // 0xNN
        tilemap[row][x++] = '0';
        tilemap[row][x++] = 'x';
        tilemap[row][x++] = hexDigit((base >> 4) & 0xF);
        tilemap[row][x++] = hexDigit(base & 0xF);

        // пробел
        tilemap[row][x++] = ' ';

        // 16 символов строки
        for (int i = 0; i < 16 && x < GRID_W; i++) {
            uint8_t c = base + i;
            tilemap[row][x] = (char)c;
            x++;
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
            // drawTile(
            drawTileFast(
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