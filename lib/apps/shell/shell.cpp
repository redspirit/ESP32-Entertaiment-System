#include "shell.h"
#include "palette.h"

Shell::Shell(VGA& vga)
    : _vga(vga),
      _console(),
      _kb() {
}

Shell::~Shell() {
}

bool Shell::init() {
    paletteInit();
    _console.init(_vga, 8, 8, COLOR_WHITE);
    _console.setCursorVisible(true);
    _kb.init();
    return true;
}

void Shell::update(float dt) {
    _vga.clear(0);
    _console.cursorUpdate(dt);

    _kb.beginFrame();
    
    char c;
    if (_kb.getChar(c)) {
        _console.print(c);
    }

    _console.show();
    _vga.show();
}

void Shell::tick() {
    _kb.poll();
}