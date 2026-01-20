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

    bool mkdir(const char* path);
    bool rmdirEmpty(const char* path);
    bool removeFile(const char* path);

    bool fileExists(const char* path);
    size_t fileSize(const char* path);

    bool writeTextFile(const char* path, const char* text);
    bool appendTextFile(const char* path, const char* text);    

    // читает файл, если размер <= maxLen
    // возвращает false если файл не существует или слишком большой
    bool readTextFileLimited(
        const char* path,
        char* dst,
        size_t maxLen,
        size_t* outSize = nullptr
    );

}