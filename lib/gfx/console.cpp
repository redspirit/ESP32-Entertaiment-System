#include <console.h>
#include <string.h>
#include <GUILayer.h>
#include <GUIText.h>
#include <palette.h>

namespace console {

static uint8_t text_color = 28;

// кольцевой буфер символов
static char buffer[GUI::GRID_H][GUI::GRID_W];

static int head  = 0;   // первая видимая строка
static int count = 0;   // количество строк в буфере

// курсор
static int cx = 0;
static int cy = 0;

static void clearLine(int row) {
    memset(buffer[row], ' ', GUI::GRID_W);
}

void clear() {
    for (int i = 0; i < GUI::GRID_H; i++)
        clearLine(i);

    head = 0;
    count = 0;
    cx = 0;
    cy = 0;
}

void setColor(uint8_t color) {
    text_color = color;
}

static void scrollUp() {
    head = (head + 1) % GUI::GRID_H;
    count--;
    if (cy > 0) cy--;
}

static void newLine() {
    cx = 0;

    if (count < GUI::GRID_H) {
        cy = count;
        clearLine((head + count) % GUI::GRID_H);
        count++;
    } else {
        scrollUp();
        cy = count;
        clearLine((head + count) % GUI::GRID_H);
        count++;
    }
}


void flush() {
    GUI::Tile* map = GUI::getTilemap();

    for (int y = 0; y < GUI::GRID_H; y++) {
        int src = (head + y) % GUI::GRID_H;
        for (int x = 0; x < GUI::GRID_W; x++) {
            TILE(map, x, y).ch    = buffer[src][x];
            TILE(map, x, y).color = text_color;
        }
    }
}

void print(const char* text) {
    if (count == 0)
        newLine();

    for (const char* p = text; *p; ++p) {
        if (*p == '\n') {
            newLine();
            continue;
        }

        if (cx >= GUI::GRID_W)
            newLine();

        int row = (head + cy) % GUI::GRID_H;
        buffer[row][cx++] = *p;
    }

    flush();
}

void printLn(const char* text) {
    if (text)
        print(text);

    newLine();
    flush();
}

void printLn() {
    newLine();
    flush();
}

}