#include "TextTiles.h"
#include "VGA.h"
#include "font8x8.h"
#include "palette.h"
#include <string.h>
#include <stdlib.h>

TextTiles::~TextTiles() {
    if (tilemap_) {
        free(tilemap_);
        tilemap_ = nullptr;
    }
}

void TextTiles::init(
    VGA& vga,
    int screenW,
    int screenH,
    int tileW,
    int tileH
) {
    vga_ = &vga;

    tileW_ = tileW;
    tileH_ = tileH;

    gridW_ = screenW / tileW_; // округляется вниз до целого автоматически
    gridH_ = screenH / tileH_;

    // переаллоцируем, если нужно
    size_t count = gridW_ * gridH_;

    tilemap_ = (CharTile*)malloc(count * sizeof(CharTile));
    memset(tilemap_, 0, count * sizeof(CharTile));

    fgVisible_ = false;
}

void TextTiles::clear() {
    if (!tilemap_) return;
    memset(tilemap_, 0, gridW_ * gridH_ * sizeof(CharTile));
}

void TextTiles::drawTile(int x, int y, CharTile t) {
    if (x < 0 || y < 0 || x >= gridW_ || y >= gridH_)
        return;

    tileAt(x, y) = t;
}

void TextTiles::drawTileForeground(int x, int y, CharTile t) {
    fgX_ = x;
    fgY_ = y;
    fgTile_ = t;
}

void TextTiles::foregroundVisible(bool visible) {
    fgVisible_ = visible;
}

void TextTiles::print(const char* text, int x, int y, uint8_t color) {
    if (y < 0 || y >= gridH_)
        return;

    int cx = x;

    while (*text && cx < gridW_) {
        CharTile& t = tileAt(cx, y);
        t.ch = (uint8_t)*text++;
        t.color = color;
        cx++;
    }
}

void TextTiles::render() {
    if (!vga_ || !tilemap_)
        return;

    for (int ty = 0; ty < gridH_; ty++) {
        for (int tx = 0; tx < gridW_; tx++) {
            const CharTile& t = tileAt(tx, ty);
            if (t.ch == 0)
                continue;

            renderTile(tx * tileW_, ty * tileH_, t);
        }
    }

    if (fgVisible_) {
        renderTile(
            fgX_ * tileW_,
            fgY_ * tileH_,
            fgTile_
        );
    }
}

void TextTiles::renderTile(int px, int py, const CharTile& t) {
    const uint8_t* glyph = font8x8::get(t.ch);
    uint8_t color = getColorByPalette(t.color);

    for (int y = 0; y < tileH_; y++) {
        uint8_t row = glyph[y];
        uint8_t* dst = vga_->getLinePtr8(py + y) + px;

        for (int x = 0; x < tileW_; x++) {
            if (row & (1 << (7 - x)))
                dst[x] = color;
        }
    }
}
