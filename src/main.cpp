#include "VGA.h"
#include "GUILayer.h"
#include "GUIText.h"
#include "palette.h"
#include "console.h"
#include "luaManager.h"
#include "SDCard.h"
#include "keyboard.h"
#include "scancodes.h"
#include <Arduino.h>

#define LOG Serial0

const PinConfig pins(
    -1, -1, 4, 5, 6,
    -1, -1, -1, 7, 8, 9,
    -1, -1, -1, 10, 11,
    12, 13 // H, V
);

VGA vga;
Mode mode = Mode::MODE_320x240x60;


void setup() {
	LOG.begin(115200);
	vga.bufferCount = 2;
	if(!vga.init(pins, mode)) while(1) delay(1);
	vga.start();

    LOG.println("Started!!!");

    // printText("Hello World", 3, 5);
    //GUIText::printUTF8("ПРИВЕТ МИР и так далее", 1, 10, COLOR_BLUE);
    //GUIText::print("This is my PC", 1, 12, COLOR_RED);

    SDCard::init();
    // SDCard::listDir("/", [](const char* name, bool isDir) {
    //     LOG.println(name);
    // });

    paletteInit();
    keyboard::init();

    // console::print("Loading");
    // console::print("...");
    // console::printLn(" OK");
    // console::printLn();
    // console::printLn();
    // console::printLn("Welcome");
    // console::print("> ");

    //initTilemapTest();
    //initTilemapFontTable();
    GUIText::printPaletteTable();

    luaManager::luaInit(vga);
    // luaManager::loadAndRunFromSD("/demo.lua");

}

unsigned long fps_last_time = 0;
unsigned long fps_frames = 0;
unsigned long fps_frames_final = 0;

void update60fps(float dt) {

    if(keyboard::isPressed(Key::ENTER)) {
        LOG.print('*');
    }

    luaManager::callUpdate(dt);
    luaManager::callShow();

    GUI::render(vga);

    vga.show();

}

void loop() {
    static unsigned long last = 0;
    unsigned long now = millis();

    keyboard::KeyEvent ev;
    while (keyboard::readKey(ev)) {
        if (ev.pressed) {
            KeyChar kc = KeyMap[ev.key];
            if (kc.normal) {
                LOG.print(kc.normal);
            }
        }
    }

    if (now - last < 16) return; // ~60 FPS
    float dt = (now - last) * 1e-3f;
    last = now;

    if (dt > 0.05f) dt = 0.05f; // clamp

    update60fps(dt);

	fps_frames++;
	if (millis() - fps_last_time >= 1000) {
		// LOG.print("FPS: "); LOG.println(fps_frames);

        // LOG.print("Free heap: ");
        // LOG.println(heap_caps_get_free_size(MALLOC_CAP_8BIT));
        // LOG.print("Stack high water: ");
        // LOG.println(uxTaskGetStackHighWaterMark(NULL));

		fps_frames_final = fps_frames;
		fps_frames = 0;
		fps_last_time = millis();
	}
}
