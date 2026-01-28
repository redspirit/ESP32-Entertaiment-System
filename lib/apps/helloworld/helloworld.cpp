#include "helloworld.h"
#include "palette.h"

HelloWorld::HelloWorld(VGA& vga)
    : _vga(vga),
      _tiles() {
}

HelloWorld::~HelloWorld() {
}

bool HelloWorld::init() {
    paletteInit();
    _tiles.init(_vga, 8, 8);

    _tiles.print("Hello World!", 1, 1, COLOR_GREEN);
    return true;
}

void HelloWorld::update(float dt) {
    _vga.clear(0);
    _tiles.render();
    _vga.show();
}

void HelloWorld::tick() {

}