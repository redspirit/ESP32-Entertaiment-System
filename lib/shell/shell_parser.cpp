#include "shell_parser.h"
#include <ctype.h>

bool parseCommand(char* line, ParsedCommand& out) {
    out.argc = 0;

    char* p = line;

    while (*p && out.argc < MAX_ARGS) {

        // пропускаем пробелы
        while (*p == ' ' || *p == '\t')
            ++p;

        if (*p == '\0')
            break;

        // начало аргумента
        out.argv[out.argc++] = p;

        // идём до конца аргумента
        while (*p && *p != ' ' && *p != '\t')
            ++p;

        if (*p) {
            *p = '\0'; // ❗ in-place
            ++p;
        }
    }

    return out.argc > 0;
}
