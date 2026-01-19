#include "shell_commands.h"
#include "shell_parser.h"
#include "shell.h"
#include "console.h"
#include "palette.h"
#include "esp_system.h"
#include <SDCard.h>
#include <string.h>

// сигнатура обработчиков
typedef void (*CommandHandler)(int argc, const char* const* argv);

struct ShellCommand {
    const char*    name;
    CommandHandler handler;
    const char*    help;
};

static void cmd_cls(int argc, const char* const* argv);
static void cmd_help(int argc, const char* const* argv);
static void cmd_reboot(int argc, const char* const* argv);
static void cmd_font(int argc, const char* const* argv);
static void cmd_colors(int argc, const char* const* argv);
static void cmd_pwd(int argc, const char* const* argv);
static void cmd_cd(int argc, const char* const* argv);
static void cmd_dir(int argc, const char* const* argv);

static const ShellCommand commands[] = {
    { "HELP",  cmd_help,  "Get this help" },
    { "CLS",  cmd_cls,  "Clear screen" },
    { "REBOOT", cmd_reboot, "Restart system" },
    { "FONT",   cmd_font,  "Show font table" },
    { "COLORS",  cmd_colors,  "Show color palette" },
    { "PWD",  cmd_pwd,  "Show current directory" },
    { "CD",  cmd_cd,  "Change current directory" },
    { "DIR",  cmd_dir,  "List directory contents" },
    // дальше: DIR, RUN ...
};

static const int commandCount = sizeof(commands) / sizeof(commands[0]);
static char hexDigit(uint8_t v) {
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

static void dirCallback(const char* name, bool isDir) {
    if (isDir) {
        console::setColor(COLOR_YELLOW);
        console::print("<D> ");
    } else {
        console::print("    ");
    }

    console::setColor(COLOR_WHITE);
    console::printLn(name);
}

static void cmd_dir(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    const char* path = shell::getCwd();

    if (!SDCard::init()) {
        console::setColor(COLOR_RED);
        console::printLn("SD card not initialized");
        console::useDefaultColor();
        return;
    }

    console::printLn(""); 
    SDCard::listDir(path, dirCallback);
}

static void cmd_pwd(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    console::printLn(shell::getCwd());
}

static void cmd_cd(int argc, const char* const* argv) {
    if (argc < 2) {
        shell::setCwd("/");
        return;
    }

    char newPath[128];
    shell::resolvePath(argv[1], newPath);

    if (!SDCard::dirExists(newPath)) {
        console::setColor(COLOR_RED);
        console::print("Directory not found: ");
        console::printLn(newPath);
        console::useDefaultColor();;
        return;
    }

    shell::setCwd(newPath);
}

static void cmd_cls(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    console::clear();
}

static void cmd_help(int argc, const char* const* argv) {
    if (argc == 2) {
        for (int i = 0; i < commandCount; ++i) {
            if (strcasecmp(argv[1], commands[i].name) == 0) {

                console::setColor(COLOR_YELLOW);
                console::print(commands[i].name);
                console::setColor(COLOR_WHITE);
                console::print(" - ");
                console::printLn(commands[i].help);

                return;
            }
        }

        // команда не найдена
        console::setColor(COLOR_RED);
        console::print("No help available for command: ");
        console::printLn(argv[1]);
        console::setColor(COLOR_WHITE);
        return;
    }

    console::setColor(COLOR_CYAN);
    console::printLn("Available commands:");
    console::printLn("-------------------");
    console::setColor(COLOR_WHITE);

    // ищем максимальную длину имени команды
    int maxLen = 0;
    for (int i = 0; i < commandCount; ++i) {
        int len = strlen(commands[i].name);
        if (len > maxLen)
            maxLen = len;
    }

    // печатаем список
    for (int i = 0; i < commandCount; ++i) {

        console::setColor(COLOR_YELLOW);
        console::print(commands[i].name);
        console::setColor(COLOR_WHITE);

        int pad = maxLen - strlen(commands[i].name) + 2;
        for (int s = 0; s < pad; ++s)
            console::print(' ');

        console::printLn(commands[i].help);
    }
}

static void cmd_reboot(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    console::printLn("Rebooting...");
    esp_restart();
}

static void cmd_font(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    console::setColor(COLOR_CYAN);
    console::printLn("FONT TABLE (0x00 - 0xFF)");
    console::printLn("------------------------");

    for (int base = 0; base < 256; base += 16) {

        console::setColor(COLOR_YELLOW);
        console::print("0x");
        console::print(hexDigit((base >> 4) & 0xF));
        console::print(hexDigit(base & 0xF));
        console::print(" ");

        // 16 символов
        console::setColor(COLOR_WHITE);
        for (int i = 0; i < 16; i++) {
            char c = (char)(base + i);
            console::print(c);
        }

        console::printLn();
    }
    console::useDefaultColor();

}

static void cmd_colors(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    console::setColor(COLOR_CYAN);
    console::printLn("COLOR PALETTE (0x00 - 0xFF)");
    for (int i = 0; i < 27; i++) {
        console::print((char)205);
    }
    console::printLn();

    for (int base = 0; base < 256; base += 16) {
        // Заголовок строки: 0xNN
        console::setColor(COLOR_YELLOW);
        console::print("0x");
        console::print(hexDigit((base >> 4) & 0xF));
        console::print(hexDigit(base & 0xF));
        console::print(" ");

        // 16 цветов
        for (int i = 0; i < 16; i++) {
            uint8_t color = base + i;
            console::setColor(color);
            console::print((char)219); // █
        }

        console::printLn();
    }
    console::useDefaultColor();

}

// ======================
// Dispatcher
// ======================

void shellExecute(const ParsedCommand& cmd) {
    for (int i = 0; i < commandCount; ++i) {
        if (strcasecmp(cmd.argv[0], commands[i].name) == 0) {
            commands[i].handler(cmd.argc, cmd.argv);
            return;
        }
    }

    console::printLn("Command not found");
}
