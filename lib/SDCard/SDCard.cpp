#include <SD.h>
#include <SPI.h>

extern "C" { 
    #include "lua.h" 
    #include "lualib.h" 
}

// ===== SD SPI pins =====
#define SD_CS    21
#define SD_SCK   18
#define SD_MOSI  17
#define SD_MISO  16

static File currentFile;

namespace SDCard {

    static bool s_inited = false;

    bool init() {
        SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
        s_inited = SD.begin(SD_CS);
        return s_inited;
    }

    bool open(const char* path) {
        if (!s_inited) return false;
        if (currentFile) currentFile.close();
        currentFile = SD.open(path, FILE_READ);
        return currentFile;
    }

    size_t read(void* dst, size_t len) {
        return currentFile.read((uint8_t*)dst, len);
    }

    bool available() {
        return currentFile && currentFile.available();
    }

    void close() {
        if (currentFile) currentFile.close();
    }

    bool readFile(const char* path, char* dst, size_t maxLen) {
        File f = SD.open(path, FILE_READ);
        if (!f) return false;

        size_t size = f.size();
        if (size >= maxLen) {
            f.close();
            return false;
        }

        f.readBytes(dst, size);
        dst[size] = 0;
        f.close();
        return true;
    }

    void listDir(const char* path, void (*callback)(const char* name, bool isDir)) {

        File dir = SD.open(path);
        if (!dir || !dir.isDirectory()) return;

        File entry;
        while ((entry = dir.openNextFile())) {
            callback(entry.name(), entry.isDirectory());
            entry.close();
        }
        dir.close();
    }

}