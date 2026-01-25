#include "VGA.h"
#include "GUILayer.h"
#include "GUIText.h"
#include "palette.h"
#include "console.h"
#include "luaManager.h"
#include "SDCard.h"
#include "keyboard.h"
#include "shell.h"
#include "scancodes.h"
#include "LOG.h"
#include <Arduino.h>

const PinConfig pins(
    -1, -1, 4, 5, 6,
    -1, -1, -1, 7, 8, 9,
    -1, -1, -1, 10, 11,
    12, 13 // H, V
);

VGA vga;
Mode mode = Mode::MODE_320x240x60;
//Mode mode = Mode::MODE_640x480x60;


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

    console::setColor(COLOR_GREEN);
    console::printLn("========================================");
    console::printLn("  RETRO CONSOLE OS");
    console::printLn("  ESP32-S3 SYSTEM SHELL");
    console::printLn("");
    console::printLn("  CPU   : ESP32-S3");
    console::printLn("  VIDEO : TILEMAP VGA");
    console::printLn("  INPUT : PS/2 KEYBOARD");
    console::printLn("  MEDIA : SD CARD");
    console::printLn("");
    console::printLn("  (c) 2026  Alexey");
    console::printLn("========================================");
    console::printLn("");
    console::printLn("Type HELP for available commands.");
    console::printLn();
    console::useDefaultColor();

    shell::init();

    luaManager::luaInit(vga);
    // luaManager::loadAndRunFromSD("/demo.lua");

}

unsigned long fps_last_time = 0;
unsigned long fps_frames = 0;
unsigned long fps_frames_final = 0;

void update60fps(float dt) {
    // keyboard::KeyEvent ev;
    // while (keyboard::readKey(ev)) {
    //     if (ev.pressed) {
    //         KeyChar kc = KeyMap[ev.key];
    //         if (kc.normal) {
    //             //LOG.print(kc.normal);
    //             console::print(kc.normal);
    //         }
    //         if (ev.key == Key::ENTER) {
    //             console::printLn();
    //         }
    //     }
    // }

    // char c;
    // while (keyboard::getChar(c)) {
    //     console::print(c);
    // }

    // debugNum++;
    // char buf[32];
    // snprintf(buf, sizeof(buf), "LINE #%d", debugNum);
    // console::printLn(buf);

    // if(keyboard::isPressed(Key::RIGHT)) {
    //     console::print('.');
    // }    
    // if(keyboard::isJustPressed(Key::RIGHT)) {

    // }    
    // if(keyboard::isJustPressed(Key::LEFT)) {

    // }


    shell::update(dt);

    luaManager::callUpdate(dt);
    luaManager::callShow();
    vga.clearFast(0); // раньше это было в lua но пока тут оставим

    GUI::render(vga);
    GUIText::renderCursor(vga);

    vga.show();
}

void loop() {
    keyboard::poll();

    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last < 16) return; // ~60 FPS
    float dt = (now - last) * 1e-3f;
    last = now;

    if (dt > 0.05f) dt = 0.05f; // clamp

    update60fps(dt);
    keyboard::beginFrame();

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
