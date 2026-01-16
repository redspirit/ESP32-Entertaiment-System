#pragma once

#include <stdint.h>

class VGA;

namespace luaManager {
    void luaInit(VGA& vga);
    void callUpdate(float dt);
    void callShow();
}