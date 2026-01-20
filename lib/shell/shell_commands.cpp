#include "shell_commands.h"
#include "shell_parser.h"
#include "shell.h"
#include "console.h"
#include "palette.h"
#include "esp_system.h"
#include <SDCard.h>
#include <string.h>
#include <luaManager.h>

#define MAX_PATH 128
#define TYPE_MAX_SIZE 1024
#define LUA_OUT_MAX 128

// сигнатура обработчиков
typedef void (*CommandHandler)(int argc, const char* const* argv);

struct ShellCommand {
    const char*    name;
    CommandHandler handler;
    const char*    help;
};

static bool s_dirEmpty;

static void cmd_cls(int argc, const char* const* argv);
static void cmd_help(int argc, const char* const* argv);
static void cmd_reboot(int argc, const char* const* argv);
static void cmd_font(int argc, const char* const* argv);
static void cmd_colors(int argc, const char* const* argv);
static void cmd_pwd(int argc, const char* const* argv);
static void cmd_cd(int argc, const char* const* argv);
static void cmd_dir(int argc, const char* const* argv);
static void cmd_type(int argc, const char* const* argv);
static void cmd_mkdir(int argc, const char* const* argv);
static void cmd_rd(int argc, const char* const* argv);
static void cmd_del(int argc, const char* const* argv);
static void cmd_write(int argc, const char* const* argv);
static void cmd_append(int argc, const char* const* argv);
static void cmd_lua(int argc, const char* const* argv);

static const ShellCommand commands[] = {
    { "HELP",  cmd_help,  "Get this help" },
    { "CLS",  cmd_cls,  "Clear screen" },
    { "REBOOT", cmd_reboot, "Restart system" },
    { "FONT",   cmd_font,  "Show font table" },
    { "COLORS",  cmd_colors,  "Show color palette" },
    { "PWD",  cmd_pwd,  "Show current directory" },
    { "CD",  cmd_cd,  "Change current directory" },
    { "DIR",  cmd_dir,  "List directory contents" },
    { "TYPE",  cmd_type,  "Display text file" },
    { "RD",    cmd_rd,    "Remove empty directory" },
    { "DEL",   cmd_del,   "Delete file" },
    { "MKDIR", cmd_mkdir, "Create directory" },
    { "WRITE",  cmd_write,  "Write text file or create empty" },
    { "APPEND", cmd_append, "Append text to file" },
    { "LUA", cmd_lua, "Execute Lua expression" },
};

static const int commandCount = sizeof(commands) / sizeof(commands[0]);
static char hexDigit(uint8_t v) {
    return (v < 10) ? ('0' + v) : ('A' + (v - 10));
}

static void dirCallback(const char* name, bool isDir) {
    s_dirEmpty = false;
    if (isDir) {
        console::setColor(COLOR_YELLOW);
        console::print("<D> ");
    } else {
        console::print("    ");
    }

    console::setColor(COLOR_WHITE);
    console::printLn(name);
}

static const char* getTextArg(int argc, const char* const* argv) {
    if (argc < 3)
        return nullptr;

    return argv[2];
}

static void cmd_lua(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: LUA <expression>");
        console::useDefaultColor();
        return;
    }

    // всё после LUA — выражение
    const char* expr = argv[1];

    char out[LUA_OUT_MAX];

    if (!luaManager::runExpression(expr, out, sizeof(out))) {
        console::setColor(COLOR_RED);
        console::printLn(out[0] ? out : "Lua error");
        console::useDefaultColor();
        return;
    }

    console::printLn(out);
}

static void cmd_write(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: WRITE <file> [text]");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    const char* text = getTextArg(argc, argv);

    if (!SDCard::init() || !SDCard::writeTextFile(path, text)) {
        console::setColor(COLOR_RED);
        console::print("Cannot write file: ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }
}

static void cmd_append(int argc, const char* const* argv) {
    if (argc < 3) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: APPEND <file> <text>");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    const char* text = getTextArg(argc, argv);

    if (!SDCard::init() || !SDCard::appendTextFile(path, text)) {
        console::setColor(COLOR_RED);
        console::print("Cannot append file: ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }
}

static void cmd_type(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: TYPE <file>");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    if (!SDCard::init()) {
        console::setColor(COLOR_RED);
        console::printLn("SD card not initialized");
        console::useDefaultColor();
        return;
    }

    char buffer[TYPE_MAX_SIZE + 1];

    if (!SDCard::readTextFileLimited(path, buffer, TYPE_MAX_SIZE)) {
        console::setColor(COLOR_RED);
        console::printLn("File not found or too large (max 1 KB)");
        console::useDefaultColor();
        return;
    }

    console::printLn("");
    console::printLn(buffer);
}

static void cmd_dir(int argc, const char* const* argv) {
    (void)argc;
    (void)argv;

    char path[MAX_PATH];

    // DIR или DIR <path>
    if (argc > 1) {
        shell::resolvePath(argv[1], path);
    } else {
        strncpy(path, shell::getCwd(), MAX_PATH);
        path[MAX_PATH - 1] = 0;
    }

    if (!SDCard::init()) {
        console::setColor(COLOR_RED);
        console::printLn("SD card not initialized");
        console::useDefaultColor();
        return;
    }

    // проверка, что это директория
    if (!SDCard::dirExists(path)) {
        console::setColor(COLOR_RED);
        console::print("Directory not found: ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }

    s_dirEmpty = true;
    SDCard::listDir(path, dirCallback);

    if (s_dirEmpty) {
        console::setColor(COLOR_YELLOW);
        console::printLn("<empty>");
        console::useDefaultColor();
    }
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

    char newPath[MAX_PATH];
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
    console::printRawChar((char)205, 24);
    console::printLn();

    for (int base = 0; base < 256; base += 16) {

        console::setColor(COLOR_YELLOW);
        console::print("0x");
        console::print(hexDigit((base >> 4) & 0xF));
        console::print(hexDigit(base & 0xF));
        console::print(" ");

        // 16 символов
        console::setColor(COLOR_WHITE);
        for (int i = 0; i < 16; i++) {
            console::printRawChar((char)(base + i));
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
    console::printRawChar((char)205, 27);
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

static void cmd_mkdir(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: MKDIR <dir>");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    if (!SDCard::init() || !SDCard::mkdir(path)) {
        console::setColor(COLOR_RED);
        console::print("Cannot create directory: ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }
}

static void cmd_rd(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: RD <dir>");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    if (!SDCard::init() || !SDCard::rmdirEmpty(path)) {
        console::setColor(COLOR_RED);
        console::print("Cannot remove directory (not empty?): ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }
}

static void cmd_del(int argc, const char* const* argv) {
    if (argc < 2) {
        console::setColor(COLOR_RED);
        console::printLn("Usage: DEL <file>");
        console::useDefaultColor();
        return;
    }

    char path[MAX_PATH];
    shell::resolvePath(argv[1], path);

    if (!SDCard::init() || !SDCard::removeFile(path)) {
        console::setColor(COLOR_RED);
        console::print("Cannot delete file: ");
        console::printLn(path);
        console::useDefaultColor();
        return;
    }
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
