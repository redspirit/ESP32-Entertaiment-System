#include "VGA.h"
#include "console.h"
#include "palette.h"
#include "keyboard.h"
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

Console console;
Keyboard kb;

void setup() {
	LOG.begin(115200);
	if(!vga.init(pins, mode)) while(1) delay(1);
	vga.start();

    LOG.println("Started!!!");

    // printText("Hello World", 3, 5);
    //GUIText::printUTF8("ПРИВЕТ МИР и так далее", 1, 10, COLOR_BLUE);
    //GUIText::print("This is my PC", 1, 12, COLOR_RED);


    paletteInit();

    console.init(vga, 8, 8, COLOR_WHITE);
    console.printLn("Hello ZX-style console!");
    console.printLn("Red spirit");
    console.setColor(COLOR_CYAN);
    console.printLn("Colored line");
    console.useDefaultColor();
    console.printLn("Pipp     kaka");

    console.setCursorVisible(true);

    //console.show();
    //vga.show();

    kb.init();

}

void update60fps(float dt) {
    vga.clear(0);
    console.cursorUpdate(dt);

    kb.beginFrame();
    
    char c;
    if (kb.getChar(c)) {
        console.print(c);
    }

    console.show();
    vga.show();

}

unsigned long fps_last_time = 0;
unsigned long fps_frames = 0;
unsigned long fps_frames_final = 0;

void loop() {
    kb.poll();
    static unsigned long last = 0;
    unsigned long now = millis();
    if (now - last < 16) return; // ~60 FPS
    float dt = (now - last) * 1e-3f;
    last = now;

    if (dt > 0.05f) dt = 0.05f; // clamp

    update60fps(dt);

	fps_frames++;
	if (millis() - fps_last_time >= 1000) {
		fps_frames_final = fps_frames;
		fps_frames = 0;
		fps_last_time = millis();
	}
}
