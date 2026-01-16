#pragma once

#include <stdint.h>

class VGA;
struct lua_State;

namespace luaManager {
    void luaInit(VGA& vga);
    void callUpdate(float dt);
    void callShow();
    bool loadFromSD(lua_State* L, const char* path);
    bool loadAndRunFromSD(const char* path);
    bool loadAndRunFromString(const char* code);
}