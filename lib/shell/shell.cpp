#include "LOG.h"
#include "shell.h"
#include "console.h"
#include "keyboard.h"
#include "scancodes.h"
#include "GUIText.h"
#include "shell_parser.h"
#include "shell_commands.h"
#include <string.h>

#define SHELL_CMD_MAX 64
#define PROMPT_LEN 2
#define PROMPT_STR "> "

namespace shell {

    static char cmd[SHELL_CMD_MAX];
    static int  len = 0;
    
    static int cursor_x = 0;
    static int cursor_y = 0;
    static int cursor_pos = 0; // позиция внутри cmd[]

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

    void init() {
        memset(cmd, 0, sizeof(cmd));
        len = 0;

        int cx, cy;
        console::getCursor(cx, cy);

        cursor_x   = PROMPT_LEN;
        cursor_y   = cy;
        cursor_pos = 0;

        console::print(PROMPT_STR);
        GUIText::setCursor(true);
        GUIText::moveCursor(cursor_x, cursor_y);        
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
        console::setCursor(PROMPT_LEN + cursor_pos, cursor_y);
        console::print(&cmd[cursor_pos]);

        cursor_pos++;
        cursor_x = PROMPT_LEN + cursor_pos;

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
        console::setCursor(PROMPT_LEN + cursor_pos, cursor_y);
        console::print(&cmd[cursor_pos]);

        // затираем последний символ (он остался на экране)
        console::clearCharAt(PROMPT_LEN + len, cursor_y);

        cursor_x = PROMPT_LEN + cursor_pos;
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

        console::print(PROMPT_STR);
        cursor_x = PROMPT_LEN;
        cursor_y = cy;

        console::setCursor(cursor_x, cursor_y);
        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyLeft() {
        if (cursor_pos == 0)
            return;

        cursor_pos--;
        cursor_x = PROMPT_LEN + cursor_pos;

        console::setCursor(cursor_x, cursor_y);
        GUIText::moveCursor(cursor_x, cursor_y);
    }

    void onKeyRight() {
        if (cursor_pos >= len)
            return;

        cursor_pos++;
        cursor_x = PROMPT_LEN + cursor_pos;

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
