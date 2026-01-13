#include "VGA.h"
#include <Arduino.h>

extern "C" { 
    #include "lua.h" 
    #include "lauxlib.h" 
    #include "lualib.h" 
}

#include "esp_heap_caps.h"

#define LOG Serial0

const PinConfig pins(
    -1, -1, 4, 5, 6,
    -1, -1, -1, 7, 8, 9,
    -1, -1, -1, 10, 11,
    12, 13 // H, V
);

VGA vga;
Mode mode = Mode::MODE_320x240x60;

lua_State* L;

int l_clearFast(lua_State* Ls) {
    int c = luaL_checkinteger(Ls, 1);
    vga.clearFast(c);
    return 0;
}

int l_fillRect(lua_State* Ls) {
    int x = luaL_checkinteger(Ls, 1);
    int y = luaL_checkinteger(Ls, 2);
    int w = luaL_checkinteger(Ls, 3);
    int h = luaL_checkinteger(Ls, 4);
    int c = luaL_checkinteger(Ls, 5);
    vga.fillRect(x, y, w, h, c);
    return 0;
}

void luaInit() {
    L = luaL_newstate();
    luaL_openlibs(L);

    lua_register(L, "clear", l_clearFast);
    lua_register(L, "fillRect",  l_fillRect);

    const char game_lua[] PROGMEM = R"lua(
        local x, y = 50, 50
        local w, h = 30, 30
        local vx, vy = 3, 2

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
        LOG.println(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
}


void setup() {
	LOG.begin(115200);
	vga.bufferCount = 2;
	if(!vga.init(pins, mode)) while(1) delay(1);
	vga.start();

    LOG.println("Started!!!");

    luaInit();
}

unsigned long fps_last_time = 0;
unsigned long fps_frames = 0;
unsigned long fps_frames_final = 0;

void loop() {
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last < 16) return; // ~60 FPS
    float dt = (now - last) * 1e-3f;
    last = now;

    if (dt > 0.05f) dt = 0.05f; // clamp

    lua_getglobal(L, "update");
    lua_pushnumber(L, dt);
    if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        LOG.println(lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    lua_getglobal(L, "show");
    if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
        LOG.println(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    vga.show();

	fps_frames++;
	if (millis() - fps_last_time >= 1000) {
		LOG.print("FPS: "); LOG.println(fps_frames);

        // LOG.print("Free heap: ");
        // LOG.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
        // LOG.print("Stack high water: ");
        // LOG.println(uxTaskGetStackHighWaterMark(NULL));

		fps_frames_final = fps_frames;
		fps_frames = 0;
		fps_last_time = millis();
	}
}
