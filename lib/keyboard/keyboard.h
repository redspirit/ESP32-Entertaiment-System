#pragma once
#include <stdint.h>

namespace keyboard {

    struct KeyEvent {
        uint16_t key;     // scan code (0x1C, 0x174 и т.п.)
        bool     pressed; // true = down, false = up
        bool     extended;// true = E0
    };

    void init();
    bool available();
    uint8_t read();
    bool readKey(KeyEvent& ev);
    bool isPressed(uint16_t key);
}

