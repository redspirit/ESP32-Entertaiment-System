#include "scancodes.h"

KeyChar KeyMap[512];

void initKeyMap() {
    // Letters
    KeyMap[Key::A] = {'a','A'};
    KeyMap[Key::B] = {'b','B'};
    KeyMap[Key::C] = {'c','C'};
    KeyMap[Key::D] = {'d','D'};
    KeyMap[Key::E] = {'e','E'};
    KeyMap[Key::F] = {'f','F'};
    KeyMap[Key::G] = {'g','G'};
    KeyMap[Key::H] = {'h','H'};
    KeyMap[Key::I] = {'i','I'};
    KeyMap[Key::J] = {'j','J'};
    KeyMap[Key::K] = {'k','K'};
    KeyMap[Key::L] = {'l','L'};
    KeyMap[Key::M] = {'m','M'};
    KeyMap[Key::N] = {'n','N'};
    KeyMap[Key::O] = {'o','O'};
    KeyMap[Key::P] = {'p','P'};
    KeyMap[Key::Q] = {'q','Q'};
    KeyMap[Key::R] = {'r','R'};
    KeyMap[Key::S] = {'s','S'};
    KeyMap[Key::T] = {'t','T'};
    KeyMap[Key::U] = {'u','U'};
    KeyMap[Key::V] = {'v','V'};
    KeyMap[Key::W] = {'w','W'};
    KeyMap[Key::X] = {'x','X'};
    KeyMap[Key::Y] = {'y','Y'};
    KeyMap[Key::Z] = {'z','Z'};

    // Numbers
    KeyMap[Key::NUM_1] = {'1', '!'};
    KeyMap[Key::NUM_2] = {'2', '@'};
    KeyMap[Key::NUM_3] = {'3', '#'};
    KeyMap[Key::NUM_4] = {'4', '$'};
    KeyMap[Key::NUM_5] = {'5', '%'};
    KeyMap[Key::NUM_6] = {'6', '^'};
    KeyMap[Key::NUM_7] = {'7', '&'};
    KeyMap[Key::NUM_8] = {'8', '*'};
    KeyMap[Key::NUM_9] = {'9', '('};
    KeyMap[Key::NUM_0] = {'0', ')'};

    // Symbols
    KeyMap[Key::SPACE]     = {' ',' '};
    // KeyMap[Key::ENTER]     = {'\n','\n'};
    // KeyMap[Key::TAB]       = {'\t','\t'};
    KeyMap[Key::MINUS]     = {'-','_'};
    KeyMap[Key::EQUAL]     = {'=','+'};
    KeyMap[Key::LBRACKET]  = {'[','{'};
    KeyMap[Key::RBRACKET]  = {']','}'};
    KeyMap[Key::BACKSLASH] = {'\\','|'};
    KeyMap[Key::SEMI]      = {';',':'};
    KeyMap[Key::QUOTE]     = {'\'','"'};
    KeyMap[Key::COMMA]     = {',','<'};
    KeyMap[Key::DOT]       = {'.','>'};
    KeyMap[Key::SLASH]     = {'/','?'};
    KeyMap[Key::GRAVE]     = {'`','~'};   
}