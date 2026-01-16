#include "luaManager.h" 
#include "VGA.h"

extern "C" { 
    #include "lua.h" 
    #include "lauxlib.h" 
    #include "lualib.h" 
}

namespace luaManager {

    static lua_State* L = nullptr;
    static VGA* g_vga = nullptr;   // доступ из Lua-C API

    // ===== Lua C API =====

    static int l_clearFast(lua_State* Ls) {
        int c = luaL_checkinteger(Ls, 1);
        g_vga->clearFast(c);
        return 0;
    }

    static int l_fillRect(lua_State* Ls) {
        int x = luaL_checkinteger(Ls, 1);
        int y = luaL_checkinteger(Ls, 2);
        int w = luaL_checkinteger(Ls, 3);
        int h = luaL_checkinteger(Ls, 4);
        int c = luaL_checkinteger(Ls, 5);
        g_vga->fillRect(x, y, w, h, c);
        return 0;
    }

    void luaInit(VGA& vga) {
        g_vga = &vga;
        L = luaL_newstate();
        luaL_openlibs(L);

        lua_register(L, "clear",     l_clearFast);
        lua_register(L, "fillRect",  l_fillRect);

        static const char game_lua[] = R"lua(
            local x, y = 50, 50
            local w, h = 30, 30
            local vx, vy = 2, 2

            function update(dt)
                x = x + vx * dt * 60
                y = y + vy * dt * 60

                if x <= 0 or x + w >= 320 then vx = -vx end
                if y <= 0 or y + h >= 240 then vy = -vy end
            end

            function show()
                clear(0)
                fillRect(
                    math.floor(x),
                    math.floor(y),
                    w, h, 255
                )
            end
        )lua";

        if (luaL_dostring(L, game_lua) != LUA_OK) {
            //LOG.println(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    void callUpdate(float dt) {
        lua_getglobal(L, "update");
        lua_pushnumber(L, dt);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            //LOG.println(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    void callShow() {
        lua_getglobal(L, "show");
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            // LOG.println(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

}




