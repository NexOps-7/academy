#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

// only scan the token when compiler needs one
void compile(const char* src) {
    initScanner(src);
    int line = -1;
    for (;;) {
        Token token = scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("    | ");
        }
        // *: length num precision in next arg -> token.len
        // token.start: the str to print and the length is token.len
        printf("%2d. '%.*s'\n", token.type, token.len, token.start);
        // eof print empty token ''
        if (token.type == TOKEN_EOF) break;
    }
}