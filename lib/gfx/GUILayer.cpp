#include "GUILayer.h"
#include "font8x8.h"
#include "VGA.h"
#include "palette.h"

// Создается сетку 30х40 знакомест под символы текста и псевдографики
// Рендерит отдельным методом, применяет прозрачность

namespace GUI {

    static constexpr int TILE_W = 8;
    static constexpr int TILE_H = 8;

    static Tile tilemap[GRID_H][GRID_W];

    static bool transparentBg = true;

    Tile* getTilemap() {
        return &tilemap[0][0];
    }

    void clear() {
        memset(tilemap, 0, sizeof(tilemap));
    }

    void drawTile(VGA& vga, int sx, int sy, Tile t) {
        const uint8_t* glyph = font8x8::get(t.ch);
        uint8_t color = getColorByPalette(t.color); // получили цвет по индексу из палитры

        for (int y = 0; y < 8; y++) {
            uint8_t row = glyph[y];
            uint8_t* dst = vga.dmaBuffer->getLineAddr8(sy + y, vga.backBuffer) + sx;

            if (transparentBg) {
                for (int x = 0; x < 8; x++) {
                    if (row & (1 << (7-x)))
                        dst[x] = color;
                }
            } else {
                for (int x = 0; x < 8; x++) {
                    dst[x] = (row & (1 << (7-x))) ? color : 0x00; // bg color
                }
            }
        }
    }

    void render(VGA& vga) {
        for (int ty = 0; ty < GRID_H; ty++) {
            for (int tx = 0; tx < GRID_W; tx++) {
                Tile& t = tilemap[ty][tx];
                if (t.ch == 0) continue;
                drawTile(
                    vga,
                    tx * TILE_W,
                    ty * TILE_H,
                    t
                );
            }
        }
    }

}