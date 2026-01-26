#include "VGA.h"
#include "TextTiles.h"
#include "palette.h"
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

TextTiles text;

void setup() {
	LOG.begin(115200);
	if(!vga.init(pins, mode)) while(1) delay(1);
	vga.start();

    LOG.println("Started!!!");

    // printText("Hello World", 3, 5);
    //GUIText::printUTF8("ПРИВЕТ МИР и так далее", 1, 10, COLOR_BLUE);
    //GUIText::print("This is my PC", 1, 12, COLOR_RED);


    paletteInit();
    text.init(
        vga,
        320, 240,   // экран
        8, 8        // размер тайла
    );    

    vga.clear(0);
    text.print("This is my PC", 1, 1, COLOR_RED);

    text.foregroundVisible(true);
    TextTiles::CharTile tl;
    tl.ch = '_';
    tl.color = COLOR_BLUE;

    text.drawTileForeground(1, 3, tl);

    text.render();
    vga.show();

}

unsigned long fps_last_time = 0;
unsigned long fps_frames = 0;
unsigned long fps_frames_final = 0;

void update60fps(float dt) {



}

void loop() {
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
