#include "LOG.h"
#include "shell.h"
#include "console.h"
#include "keyboard.h"
#include "scancodes.h"
#include "GUIText.h"
#include "palette.h"
#include "shell_parser.h"
#include "shell_commands.h"
#include <string.h>

#define SHELL_CMD_MAX 64
#define PROMPT_PATH_MAX 16
#define MAX_SEGMENTS 16
#define PROMPT_STR "> "
#define MAX_PATH 128

namespace shell {

    static char cmd[SHELL_CMD_MAX];
    static int  len = 0;
    
    static int cursor_x = 0;
    static int cursor_y = 0;
    static int cursor_pos = 0; // позиция внутри cmd[]
    static char cwd[MAX_PATH] = "/";
    static char promptPath[PROMPT_PATH_MAX + 1];

    static void redrawLine() {
        // перерисовываем текущую строку целиком
        console::print("\r");           // логический возврат
        console::print(cmd);
    }

    static void executeCommand(char* cmd) {
        ParsedCommand pc;

        console::printLn();
        if (!parseCommand(cmd, pc))
            return;

        shellExecute(pc);
    }

    void resolvePath(const char* input, char* out) {
        char temp[MAX_PATH];

        if (!input || input[0] == '\0') {
            strncpy(temp, cwd, MAX_PATH);
        } else if (input[0] == '/') {
            strncpy(temp, input, MAX_PATH);
        } else {
            if (strcmp(cwd, "/") == 0)
                snprintf(temp, MAX_PATH, "/%s", input);
            else
                snprintf(temp, MAX_PATH, "%s/%s", cwd, input);
        }

        temp[MAX_PATH - 1] = 0;

        const char* segments[MAX_SEGMENTS];
        int segCount = 0;

        char* p = temp;

        // пропускаем начальный '/'
        if (*p == '/')
            p++;

        while (*p && segCount < MAX_SEGMENTS) {
            char* start = p;

            // идём до '/' или конца
            while (*p && *p != '/')
                p++;

            if (*p) {
                *p = 0;
                p++;
            }

            if (strcmp(start, ".") == 0) {
                // ничего не делаем
            }
            else if (strcmp(start, "..") == 0) {
                if (segCount > 0)
                    segCount--;   // шаг назад
            }
            else if (*start) {
                segments[segCount++] = start;
            }
        }

        if (segCount == 0) {
            strcpy(out, "/");
            return;
        }

        char* dst = out;
        *dst++ = '/';

        for (int i = 0; i < segCount; ++i) {
            int l = strlen(segments[i]);
            memcpy(dst, segments[i], l);
            dst += l;

            if (i < segCount - 1)
                *dst++ = '/';
        }

        *dst = 0;
    }

    static const char* getPromptPath() {
        const char* full = getCwd();
        int len = strlen(full);

        // если путь короткий — показываем целиком
        if (len <= PROMPT_PATH_MAX) {
            return full;
        }

        // иначе: "~" + хвост пути
        // 1 символ под '~', остальное — хвост
        int tailLen = PROMPT_PATH_MAX - 1;
        const char* tail = full + (len - tailLen);

        promptPath[0] = '~';
        strncpy(&promptPath[1], tail, tailLen);
        promptPath[PROMPT_PATH_MAX] = '\0';

        return promptPath;
    }

    static int getPromptLength() {
        return strlen(getPromptPath()) + 3;
    }

    static void printPrompt() {
        console::setColor(COLOR_CYAN);
        console::print(getPromptPath());
        console::useDefaultColor();
        console::print(" > ");
    }

    void init() {
        memset(cmd, 0, sizeof(cmd));
        len = 0;

        int cx, cy;
        console::getCursor(cx, cy);

        printPrompt();
        cursor_x   = getPromptLength();
        cursor_y   = cy;
        cursor_pos = 0;

        GUIText::setCursor(true);
        GUIText::moveCursor(cursor_x, cursor_y);        
    }

    const char* getCwd() {
        return cwd;
    }

    void setCwd(const char* path) {
        if (!path || path[0] == '\0')
            return;

        strncpy(cwd, path, MAX_PATH);
        cwd[MAX_PATH - 1] = 0;
    }

    void onChar(char c) {
        if (len >= SHELL_CMD_MAX - 1)
            return;

        // сдвигаем хвост вправо
        memmove(
            &cmd[cursor_pos + 1],
            &cmd[cursor_pos],
            len - cursor_pos + 1 // +1 чтобы сдвинуть '\0'
        );

        cmd[cursor_pos] = c;
        len++;
        
        // перерисовываем хвост строки
        console::setCursor(getPromptLength() + cursor_pos, cursor_y);
        console::print(&cmd[cursor_pos]);

        cursor_pos++;
        cursor_x = getPromptLength() + cursor_pos;

        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyBack() {
        if (cursor_pos == 0)
            return;

        // сдвигаем хвост влево
        memmove(
            &cmd[cursor_pos - 1],
            &cmd[cursor_pos],
            len - cursor_pos + 1 // включая '\0'
        );

        len--;
        cursor_pos--;

        // перерисовываем хвост строки
        console::setCursor(getPromptLength() + cursor_pos, cursor_y);
        console::print(&cmd[cursor_pos]);

        // затираем последний символ (он остался на экране)
        console::clearCharAt(getPromptLength() + len, cursor_y);

        cursor_x = getPromptLength() + cursor_pos;
        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyEnter() {
        executeCommand(cmd);

        int cx, cy;
        console::getCursor(cx, cy);

        // сброс строки
        memset(cmd, 0, sizeof(cmd));
        len = 0;
        cursor_pos = 0;

        printPrompt();
        cursor_x = getPromptLength();
        cursor_y = cy;

        console::setCursor(cursor_x, cursor_y);
        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyLeft() {
        if (cursor_pos == 0)
            return;

        cursor_pos--;
        cursor_x = getPromptLength() + cursor_pos;

        console::setCursor(cursor_x, cursor_y);
        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyRight() {
        if (cursor_pos >= len)
            return;

        cursor_pos++;
        cursor_x = getPromptLength() + cursor_pos;

        console::setCursor(cursor_x, cursor_y);
        GUIText::moveCursor(cursor_x, cursor_y);
    }    

    void update(float dt) {
        GUIText::tick(dt);

        char c;
        while (keyboard::getChar(c)) {
            onChar(c);
        }

        if(keyboard::isJustPressed(Key::BACKSPACE)) {
            onKeyBack();
        }

        if(keyboard::isJustPressed(Key::ENTER)) {
            onKeyEnter();
        }

        if (keyboard::isJustPressed(Key::LEFT)) {
            onKeyLeft();
        }

        if (keyboard::isJustPressed(Key::RIGHT)) {
            onKeyRight();
        }

    }

}
