#include "shell_commands.h"
#include "shell_parser.h"
#include "console.h"
#include "palette.h"
#include "esp_system.h"
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

static const ShellCommand commands[] = {
    { "CLS",  cmd_cls,  "Clear screen" },
    { "HELP",  cmd_help,  "Get help" },
    { "REBOOT", cmd_reboot, "Restart system" },
    // дальше: DIR, RUN, HELP ...
};

static const int commandCount = sizeof(commands) / sizeof(commands[0]);

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
