#pragma once

#define MAX_ARGS 8

struct ParsedCommand {
    int argc;
    char* argv[MAX_ARGS];
};

bool parseCommand(char* line, ParsedCommand& out);
