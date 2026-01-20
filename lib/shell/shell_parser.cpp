#include "shell_parser.h"
#include <ctype.h>

bool parseCommand(char* line, ParsedCommand& out) {
    out.argc = 0;
    char* p = line;

    while (*p && out.argc < MAX_ARGS) {

        // пропуск пробелов
        while (*p == ' ' || *p == '\t')
            ++p;

        if (*p == '\0')
            break;

        // ---------------------------------
        // аргумент в кавычках
        // ---------------------------------
        if (*p == '"') {
            ++p; // пропускаем "

            out.argv[out.argc++] = p;

            // ищем закрывающую кавычку
            while (*p && *p != '"')
                ++p;

            if (*p == '"') {
                *p = '\0';
                ++p;
            }
        }
        // ---------------------------------
        // обычный аргумент
        // ---------------------------------
        else {
            out.argv[out.argc++] = p;

            while (*p && *p != ' ' && *p != '\t')
                ++p;

            if (*p) {
                *p = '\0';
                ++p;
            }
        }
    }

    return out.argc > 0;
}
