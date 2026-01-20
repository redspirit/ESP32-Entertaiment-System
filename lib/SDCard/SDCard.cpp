#include <SDCard.h>
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

#define MAX_DIR_ENTRIES 64
#define MAX_NAME_LEN   32

static File currentFile;

namespace SDCard {

    static bool s_inited = false;

    struct DirEntry {
        char name[MAX_NAME_LEN];
        bool isDir;
    };

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
        if (!s_inited)
            return;

        File dir = SD.open(path);
        if (!dir || !dir.isDirectory())
            return;

        DirEntry entries[MAX_DIR_ENTRIES];
        int count = 0;

        File entry;
        while ((entry = dir.openNextFile()) && count < MAX_DIR_ENTRIES) {
            strncpy(entries[count].name, entry.name(), MAX_NAME_LEN);
            entries[count].name[MAX_NAME_LEN - 1] = 0;
            entries[count].isDir = entry.isDirectory();
            count++;
            entry.close();
        }
        dir.close();

        // -------------------------------------------------
        // Сортировка:
        // 1) директории раньше файлов
        // 2) по алфавиту
        // -------------------------------------------------
        for (int i = 0; i < count - 1; ++i) {
            for (int j = i + 1; j < count; ++j) {

                bool swap = false;

                if (entries[i].isDir != entries[j].isDir) {
                    // директории выше
                    if (!entries[i].isDir)
                        swap = true;
                }
                else {
                    // одинаковый тип — сортировка по имени
                    if (strcasecmp(entries[i].name, entries[j].name) > 0)
                        swap = true;
                }

                if (swap) {
                    DirEntry tmp = entries[i];
                    entries[i] = entries[j];
                    entries[j] = tmp;
                }
            }
        }

        // -------------------------------------------------
        // Вывод
        // -------------------------------------------------
        for (int i = 0; i < count; ++i) {
            callback(entries[i].name, entries[i].isDir);
        }
    }

    bool dirExists(const char* path) {
        if (!s_inited)
            return false;

        File f = SD.open(path);
        if (!f)
            return false;

        bool isDir = f.isDirectory();
        f.close();
        return isDir;
    }

    bool fileExists(const char* path) {
        if (!s_inited)
            return false;

        File f = SD.open(path, FILE_READ);
        if (!f)
            return false;

        bool ok = !f.isDirectory();
        f.close();
        return ok;
    }

    size_t fileSize(const char* path) {
        if (!s_inited)
            return 0;

        File f = SD.open(path, FILE_READ);
        if (!f)
            return 0;

        size_t size = f.size();
        f.close();
        return size;
    }

    bool readTextFileLimited(const char* path, char* dst, size_t maxLen, size_t* outSize) {
        if (!s_inited)
            return false;

        File f = SD.open(path, FILE_READ);
        if (!f || f.isDirectory())
            return false;

        size_t size = f.size();
        if (size > maxLen) {
            f.close();
            return false;
        }

        f.readBytes(dst, size);
        dst[size] = 0;

        if (outSize)
            *outSize = size;

        f.close();
        return true;
    }


    bool mkdir(const char* path) {
        if (!s_inited)
            return false;

        // SD.mkdir вернёт false, если не удалось
        return SD.mkdir(path);
    }

    bool rmdirEmpty(const char* path) {
        if (!s_inited)
            return false;

        File dir = SD.open(path);
        if (!dir || !dir.isDirectory())
            return false;

        // проверяем, что директория пустая
        File entry = dir.openNextFile();
        if (entry) {
            entry.close();
            dir.close();
            return false; // не пустая
        }

        dir.close();
        return SD.rmdir(path);
    }

    bool removeFile(const char* path) {
        if (!s_inited)
            return false;

        File f = SD.open(path);
        if (!f || f.isDirectory()) {
            if (f) f.close();
            return false;
        }

        f.close();
        return SD.remove(path);
    }    

    bool writeTextFile(const char* path, const char* text) {
        if (!s_inited)
            return false;

        // если файл существует — удаляем
        if (SD.exists(path)) {
            SD.remove(path);
        }

        File f = SD.open(path, FILE_WRITE);
        if (!f)
            return false;

        if (text && *text) {
            f.print(text);
            f.print("\n");
        }

        f.close();
        return true;
    }

    bool appendTextFile(const char* path, const char* text) {
        if (!s_inited)
            return false;

        File f = SD.open(path, FILE_WRITE);
        if (!f)
            return false;

        f.seek(f.size()); // в конец

        if (text && *text) {
            f.print(text);
            f.print("\n");
        }

        f.close();
        return true;
    }

}