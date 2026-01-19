#pragma once

#include <stdint.h>
#include <cstddef>

namespace SDCard {
    bool init();

    bool open(const char* path);
    size_t read(void* dst, size_t len);
    bool available();
    void close();

    bool readFile(const char* path, char* dst, size_t maxLen);
    void listDir(const char* path, void (*callback)(const char* name, bool isDir));
    bool dirExists(const char* path);
}