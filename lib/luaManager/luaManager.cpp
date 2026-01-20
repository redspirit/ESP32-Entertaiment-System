#include "luaManager.h" 
#include "VGA.h"
#include "SDCard.h"

extern "C" { 
    #include "lua.h" 
    #include "lauxlib.h" 
    #include "lualib.h" 
}

static const char* sdLuaReader(lua_State*, void*, size_t* size) {
    alignas(4) static char buf[512];

    if (!SDCard::available()) {
        *size = 0;
        return nullptr;
    }

    *size = SDCard::read(buf, sizeof(buf));
    return buf;
}

namespace luaManager {

    static lua_State* L = nullptr;
    static VGA* g_vga = nullptr;   // доступ из Lua-C API

    static int refUpdate = LUA_NOREF;
    static int refShow   = LUA_NOREF;

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
    }

    static void cacheCallbacks() {
        // update
        lua_getglobal(L, "update");
        if (lua_isfunction(L, -1))
            refUpdate = luaL_ref(L, LUA_REGISTRYINDEX);
        else {
            lua_pop(L, 1);
            refUpdate = LUA_NOREF;
        }

        // show
        lua_getglobal(L, "show");
        if (lua_isfunction(L, -1))
            refShow = luaL_ref(L, LUA_REGISTRYINDEX);
        else {
            lua_pop(L, 1);
            refShow = LUA_NOREF;
        }
    }

    void callUpdate(float dt) {
        if (!L || refUpdate == LUA_NOREF) return;

        lua_rawgeti(L, LUA_REGISTRYINDEX, refUpdate);
        lua_pushnumber(L, dt);

        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            lua_pop(L, 1);
        }
    }

    void callShow() {
        if (!L || refShow == LUA_NOREF) return;

        lua_rawgeti(L, LUA_REGISTRYINDEX, refShow);

        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            lua_pop(L, 1);
        }
    }

    bool loadFromSD(lua_State* L, const char* path) {
        if (!SDCard::open(path))
            return false;

        lua_settop(L, 0); // очистить стек
        int r = lua_load(L, sdLuaReader, nullptr, path, nullptr);
        SDCard::close();

        return r == LUA_OK;
    }

    bool loadAndRunFromSD(const char* path) {
        if (!L) return false;

        if (!loadFromSD(L, path))
            return false;

        // выполнить chunk
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            // LOG.println(lua_tostring(L, -1));
            lua_pop(L, 1);
            return false;
        }

        cacheCallbacks();
        return true;
    }

    bool loadAndRunFromString(const char* code) {
        if (!L || !code) return false;

        lua_settop(L, 0); // очистить стек

        if (luaL_dostring(L, code) != LUA_OK) {
            // LOG.println(lua_tostring(L, -1));
            lua_pop(L, 1);
            return false;
        }

        cacheCallbacks();
        return true;
    }

    bool runExpression(const char* expr, char* out, size_t outSize) {
        if (!L || !expr || !out || outSize == 0)
            return false;

        lua_settop(L, 0);

        // формируем: return <expr>
        char buf[256];
        snprintf(buf, sizeof(buf), "return %s", expr);

        if (luaL_dostring(L, buf) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            if (err && outSize > 1) {
                strncpy(out, err, outSize - 1);
                out[outSize - 1] = 0;
            }
            lua_pop(L, 1);
            return false;
        }

        // ожидаем результат в стеке
        if (lua_gettop(L) == 0) {
            strncpy(out, "nil", outSize);
            return true;
        }

        // преобразуем результат в строку (Lua way)
        const char* str = luaL_tolstring(L, -1, nullptr);
        if (!str) {
            strncpy(out, "<non-printable>", outSize);
            return true;
        }

        strncpy(out, str, outSize - 1);
        out[outSize - 1] = 0;

        lua_pop(L, 2); // результат + строка
        return true;
    }


}