#pragma once
#include <stdint.h>

namespace keyboard {

    struct KeyEvent {
        uint16_t key;     // scan code (0x1C, 0x174 и т.п.)
        bool     pressed; // true = down, false = up
        bool     extended;// true = E0
    };

    void init();    // нужно вызвать при старте
    void poll();    // вызывается как можно чаще в loop()
    void beginFrame();  // вызывается в начале каждого кадра
    bool available();
    uint8_t read();
    bool readKey(KeyEvent& ev);
    bool isPressed(uint16_t key);
    bool isJustPressed(uint16_t key);
    bool isJustReleased(uint16_t key);
    bool getChar(char& out); // выводит только что введенный символ с учетом shift

    uint32_t getPs2AckCount();
    uint32_t getPs2WriteErrors();
}

