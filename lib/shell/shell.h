#pragma once
#include <stdint.h>

namespace shell {

    void init();
    void update(float dt);   // каждый кадр
    void onChar(char c);     // символы
    void onKey(uint16_t key);// спец-клавиши
    const char* getCwd();
    void setCwd(const char* path);
    void resolvePath(const char* input, char* out);
}
