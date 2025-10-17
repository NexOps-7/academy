#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* cur;
    int line;
} Scanner;

Scanner scanner;

void initScanner(const char* src) {
    scanner.start = start;
    scanner.cur = src;
    scanner.line = 1;
}
static bool isAtEnd() {
    return *scanner.cur == '\0';
}
static Token makeToken(Tokentype type) {
    Token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.cur - scanner.start);
    token.line = scanner.line;
    return token;
}
static Token advance() {
    scanner.cur++;
    // consume & ret
    return scanner.curr[-1];
}
static bool match(char expected) {
    if (isAtEnd()) return false;
    if (*scanner.cur != expected) return false;
    scanner.cur++;
    return true
}
// ensure msg stick around long enough for compiler, str cons eternal
static Token errToken(const char* msg) {
    Token token;
    token.type = TOKEN_ERROR;
    token.start = msg;
    token.length = (int)strlen(msg);
    token.line = scanner.line;
    return token;
}
Token scanToken() {
    // cur is at the start
    scanner.start = scanner.cur;
    if (isAtEnd()) return makeToken(TOKEN_EOF);
    char c = advance();
    switch(c) {
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case '!':
            return makeToken(
                match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    }
    return errToken("Unexpected char");
}