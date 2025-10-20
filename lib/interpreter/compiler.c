#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"
#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token cur;
    Token prev;
    bool hadErr;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT, // = 
    // or and < > ==
    PREC_TERM, // + -
    PREC_FACTOR, // *
    PREC_UNARY, // - !
    PREC_CALL, // . ()
    PREC_PRIMARY
} precedence;

typedef void (*ParseFn) ();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

// single global var of the struct
Parser parser;
Chunk* compilingChunk;

static void errAt(Token token, const char* msg) {
    // suppress other errs, keep on trucking, bytecode never get executed
    if (parser.panicMode) return;
    parser.panicMode = true;
    fprintf(stderr, "[line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERR) {

    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
        hadErr = true;
    }
}
// report at where just consumed
static void err(const char* msg) {
    errAt(&parser.prev, msg);
}
static void errAtCur(const char* msg) {
    errAt(&parser.cur, msg);
}
static void advance() {
    // store cur tp prev before looping, so cur++
    parser.prev = parser.cur;
    for (;;){
        parser.cur = scanToken();
        // reporting err
        if (parser.cur.type != TOKEN_ERR) break;
        errAtCur(parser.cur.start);
    }
}

static Chunk* curChunk() {
    return compilingChunk;
}
static void emitByte(uint8_t byte) {
    // given byte: opcode/operand instruction
    // prev.line: runtime err associated with it
    writeChunk(curChunk(), byte, parser.prev.line);
}
static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}
static void emitRet() {
    emitByte(OP_RET);
}
static void endCompiler() {
    emitRet();
#ifdef DEBUG_PRINT_CODE
    if (!parse.hadErr) {
        disassembleChunk(curChunk(), "code");
    }
#endif
}
static uint8_t makeCons(Value val) {
    // addCons(): chunk.h->vm.h->compile.h
    // write val to the chunk array/cons table
    // return index
    int cons = addCons(curChunk(), val);
    // 0-255, up to 256 cons in the chunk, single byte for the operand
    // OP_CONS_16 store index as two-byte, scale to larger
    if (cons > UIN8_MAX) {
        err("Too many cons in one chunk");
        return 0;
    }
    // constant table/array
    return (uint8_t)cons;
}
static void emitCons(Value val) {
    emitBytes(OP_CONS, makeCons(val));
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_MINUS] = {unary, NULL, PREC_TERM},
    [TOKEN_STR] = {str, NULL, PREC_NONE},
    [TOKEN_NUM] = {num, NULL, PREC_NONE},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_ERR] = {NULL, NULL, PREC_NONE},
}
static ParseRule* getRule(TokenType type){
    return &rules[type];
}
// todo
static void prefixRule() {
    c = getRule(parser.prev.type)
    switch(c) {
        case TOKEN_MINUS:
            parse.cur = -parse.cur;
        default:
            return;
    }
}
// todo
static void infixRule() {
    switch(c) {
        case TOKEN_STAR:
            res = parse.cur * parser.prev;
    }
    return res;
}
// Precedence: enum, numerically successively larger
static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.prev.type)->prefix;
    if (prefixRule == NULL) {
        err("Expect expr");
        return;
    }
    prefixRule();
    while (precedence <= getRule(parser.cur.type)->precedence) {
        advance();
        parseFn infixRule = getRule(parser.prev.type)->infix;
        infixRule();
    }
}
// consume TOKEN_NUM token in scanner, look up & call num() in func ptr arr to compile
static void num() {
    // discard whitespaces, interpret a floating-point val in a byte str
    // str to double
    // endptr: null, dont care about where num ends
    double val = strtod(parser.prev.start, NULL);
    // val.h
    emitCons(NUM_VAL(val));
}
// +1-2: trim the quotation
static void str() {
    emitCons(OBJ_VAL(copyStr(parser.prev.start+1, parser.prev.length-2)));
}
// prefix: paren
static void grouping() {
    // compile: generate bytecode
    expression();
    consume(TOKEN_LEFT_PAREN, "Expect ) after expr");
}
// prefix: unary
static void unary() {
    TokenType opType = parser.prev.type;
    // compile op recursively
    parsePrecedence(PREC_UNARY);
    switch(opType) {
        // scanner TOKEN_BANG -> chunk OP_NOT -> compiler unary -> vm runtime -> debug
        // OP_NOT/NEGATE last: compiler negate the val to execution lastly
        case TOKEN_BANG: emitByte(OP_NOT); break;
        case TOKEN_BANG_EQUAL: emitByte(OP_EQUAL, OP_NOT); break;
        case TOKEN_MINUS: emitByte(OP_NEGATE); break;
        default: return;
    }
}
// infix: table of func ptrs, prefix-TokenType, infix-TokenType
static binary() {
    TokenType opType = parser.prev.type;
    parsePrecedence(PREC_TERM);
}
static void literal() {
    switch (parser.prev.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); brea;
        default: return;
    }
}
// todo
static void expression(){
    parsePrecedence(PREC_ASSIGNMENT);
    if (!= TOKEN_EOF) ;
    switch(c) {
        case "(":
            if (!= ")") parsePrecedence(PREC_CALL);
            emitBytes(OP_LEFT_PAREN, arr);
        case "-": 
            if (t+1 == " ") 
                parsePrecedence(PREC+_TERM);
            else 
                parsePrecedence(PREC_UNARY);
        case isDigit():
            num();
            binary();
            parsePrecedence(PREC_TERM);
    }
    return;
}
static void statement() {

}
static void declaration() {
    
}
// only scan the token when compiler needs one
void compile(const char* src, Chunk* chunk) {
    initScanner(src);
    compilingChunk = chunk;
    // mode ends when synchronized statement boundaries
    parser.panicMode = false;
    parser.hadErr = false;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expr");
    endCompiler();
    return !parser.hadErr;

    // hand-written scanner test
    /*
        int line = -1;
        for (;;) {
            Token token = scanToken();
            if (token.line != line) {
                printf("%4d ", token.line);
                line = token.line;
            } else {
                printf("    | ");
            }
            // *s: length num precision in next arg -> token.len
            // token.start: the str to print and the length is token.len
            printf("%2d. '%.*s'\n", token.type, token.len, token.start);
            // eof print empty token ''
            if (token.type == TOKEN_EOF) break;
        }
    */
    
}