#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef void (*ParseFn) (bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Loc locs[UINT8_CNT];
    int locCnt;
    int scopeDepth;
} Compiler;

typedef struct {
    Token name;
    int depth;
} Loc;
// single global var of the struct
Parser parser;
Compilier* cur = NULL;
Chunk* compilingChunk;

staic void initCompiler(Compiler* compiler) {
    compiler->locCnt = 0;
    compiler->scopeDepth = 0;
    cur = compiler;
}
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
static bool check(TokenType type, type) {
    return parser.cur.type == type;
}
static bool match(TokenType type) {
    if (!check(type)) return false;
    advance();
    return true;
}
static void synchronize() {
    parser.panicMode = false;
    while (parser.cur.type != TOKEN_EOF) {
        if (parser.prev.type == TOKEN_SEMICOLON) return;
        switch (parser.cur.type) {
            case TOKEN_VAR:
            case TOKEN_FOR:
                return;
            default:
            // do nothing
                ;
        }
        advance();
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
    if (cons > UINT8_MAX) {
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
    [TOKEN_LEFT_PAREN]  = {grouping, NULL, PREC_NONE},
    [TOKEN_BANG]        = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]  = {NULL, binary, PREC_EQUALITY},
    [TOKEN_MINUS]       = {unary, NULL, PREC_TERM},
    [TOKEN_ID]          = {gvar, NULL, PREC_NONE},
    [TOKEN_STR]         = {str, NULL, PREC_NONE},
    [TOKEN_NUM]         = {num, NULL, PREC_NONE},
    [TOKEN_STAR]        = {NULL, binary, PREC_FACTOR},
    [TOKEN_AND]         = {NULL, and_, PREC_AND},
    [TOKEN_OR]         = {NULL, or_, PREC_OR},
    [TOKEN_FALSE]       = {literal, NULL, PREC_NONE},
    [TOKEN_ERR]         = {NULL, NULL, PREC_NONE},
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
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);
    while (precedence <= getRule(parser.cur.type)->precedence) {
        advance();
        parseFn infixRule = getRule(parser.prev.type)->infix;
        infixRule(canAssign);
    }
    // = still not getting consumed
    // a * b = c + d, a * b not a valid assignment 
    if (canAssign && match(TOKEN_EQUAL)) {
        err("Invalid assignment target.");
    }
}
static uint8_t identifierCons(Token* name) {
    // makeCons: add to constant table return index
    // copyStr: findStr() in table, memcpy(), realloc()
    return makeCons(OBJ_VAL(copyStr(name->start, name->length)));
}
// todo
static void expr(){
    parsePrecedence(PREC_ASSIGNMENT);
    while (!= TOKEN_EOF){
        makeCons(parser.prev--);
    }
    return;
}
static void consume(TokenType type, const char* errMsg) {
    if (parser.cur.type != type) errAtCur(errMsg);
}
// OP_POP: an expr followed by ;, it evals expr and discards res
static void exprStatement() {
    expr();
    consume(TOKEN_SEMICOLON, "Expect ';' after expr");
    emitByte(OP_POP);
}
// stack effect: 
// OP_ADD pops 2 pushes 1, leaves 1 smaller
// statement: OP_POP produces no val on the stack
// expr: res stack top pop, print
static void printStatement() {
    expr();
    consume(TOKEN_SEMICOLON, "Expect ';' after expr");
    emitByte(OP_PRINT);
}
static void beginScope() {
    cur->scopeDepth++;
}
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}
static endScope() {
    // pop a scope
    cur->scopeDepth--;
    // cur scope always at the arr end, look for var declared at scope depth just left
    // out of scope, no need for slot, discard them, decrem the arr length
    while (cur->locCnt > 0 &&
            cur->locs[cur->locCnt - 1].depth > cur->scopeDepth) {
                emitByte(OP_POP);
                cur->locCnt--;
            }
}
// expr =, for, if, print, ret, while
static void statement() {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    }
    else {
        exprStatement();
    }
}
static int emitJump(uint8_t instru) {
    emitByte(instru);
    // placeholder for two bytes offset operand
    // 16-bit offset jump up to 65,535 bytes
    emitByte(0xff);
    emitByte(0xff);
    // index of first placeholder -2 placeholders
    return curChunk()->cnt - 2;
}
// after compiling, replace operand with cal jump offset
// use cur bytecode to determine how far to jump
static void patchJump(int offset) {
    int jumpDis = curChunk()->cnt - offset - 2;
    if (jumpDis > UINT16_MAX) {
        err("Too much code to jump over");
    }
    // high byte write offset & update bytecode
    // &0xff: mask to ensure only lower 8 bits used
    curChunk()->code[offset] = (jumpDis >> 8) & 0xff;
    // low byte
    curChunk()->code[offset+1] = jumpDis & 0xff;
}
// if -> jump false -> then -> jump -> else
static void ifStatement() {
    // check type
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if' ");
    expr();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) statement();
    patchJump(elseJump);
}
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    int offset = curChunk()->cnt - loopStart + 2;
    if (offset > UINT15_MAX) err("Loop body too large");
    emitByte((offset >> 8) & 0xff);
    emitByte(offset & 0xff);
}
static void whileStatement() {
    int loopStart = curChunk()->cnt;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while' ");
    expr();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitLoop(loopStart);
    patchJump(exitJump);
    emitByte(OP_POP);
}
static void and_(bool canAssign) {
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
}
// if left is truthy, skip right
static void or_(bool canAssign) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);
}
static void addLoc(Token* name) {
    if (cur->locCnt == UINT8_CNT) {
        err("Too many loc vars in func");
        return;
    }
    Loc* loc = &cur->locs[cur->locCnt++];
    loc->name = name;
    // loc->depth = cur->scopeDepth;
    // uninitialized state
    loc->depth = -1;
}
// only for local: to remember the var exists, put it in locs, name, depth
static void declareVar() {
    // top level
    if (cur->scopeDepth == 0) return;
    Token* name = &parser.prev;
    // append to arr, from arr end backwards
    for (int i = cur->locCnt-1; i>=0; i--) {
        Loc* loc = &cur->locs[i];
        // loc var can have same name, as long as diff scope
        // != -1 local, global others, not within scope?
        // from arr end backwards look for same name
        // stop when reach the arr beginning or var owned by another scope
        if (loc->depth != -1 && loc->depth < cur->scopeDepth) {
            break;
        }
        if (identifiersEqual(name, &loc->name)) {
            err("Already a var with this name in this scope");
        }
    }
    addLoc(*name);
}
static void parseVar(const char* errMsg ) {
    consume(TOKEN_ID, errMsg);
    // only for loc, put in locs[], record the existence of the var
    declareVar();
    // in loc scope, no need to store var in constant table, ret dummy index
    if (cur->scopeDepth > 0) return 0;
    return identifierCons(&parser.prev);
}
// var cuppa = "joe";
//    1             2
// 1->uninitialized
// 2->initialized
// avoid var a = "outer"; {var a = a;}
static void markInitialized() {
    cur->locs[cur->locCnt-1].depth =
        cur->scopeDepth;
}
// global var can be defined after ref compiled before execution, good for manual recursion
// fun showVar() {print global;}
// var global = "after"; showVar();
static void defineVar(uint8_t global){
    emitBytes(OP_DEFINE_GLOBAL, global);
    // local var already in temp, top of the stack
    // var a = 1 + 2; var b = 4; 3-a-OP_ADD -> loc slot
    if (cur->scopeDepth > 0) {
        markInitialized();
        return;
    }
}
static void varDeclaration() {
    // the index in the constant table
    uint8_t global = parseVar("Expect var name");
    if (match(TOKEN_EQUAL)) {
        expr();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after var declaration");
    defineVar(global);
}
static void forStatement() {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for' ");
    consume(TOKEN_SEMICOLON, "Expect ';' ");
    if (match(TOKEN_SEMICOLON)) {
        // no initializer
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        exprStatement();
    }
    int loopStart = curChunk()->cnt;
    int exitJump = -1;
    // exit the loop
    if (!match(TOKEN_SEMICOLON)) {
        expr();
        consume(TOKEN_SEMICOLON, "Expect ';' ");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        // no ;, must be condition
        // discard val if condition is true
        emitByte(OP_POP);
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int increStart = curChunk()->cnt;
        expr();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after for ");
        emitLoop(loopstart);
        loopStart = increStart;
        patchJump(bodyJump);
    }
    statement();
    emitLoop(loopstart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
}
// class, func, var, statement
static void declaration() {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
}
static void identifierEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}
static uint8_t resolveLoc(Compiler* compiler, Token* name) {
    // look thru the locs in scope
    // same name as identifier var ref token
    // backwards: find the last declared
    for (int i=compiler->locCnt-1; i>=0; i--) {
        Loc* loc = &compiler->locs[i];
        if (identifierEqual(name, &loc->name)) {
            // -1: ref to a var in its own initializer
            if (loc->depth == -1) {
                err("Cant read loc var in its own initializer");
            }
            // found it
            return i;
        }
    }
    // not found
    return -1;
}
// var a = nil;
static void namedVar(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    uint8_t arg = resolveLoc(cur, &name);
    // resolveLoc() return found the given name in local var
    if (arg != -1) {
        getOp = OP_GET_LOC;
        setOp = OP_SET_LOC;
    } else {
        arg = identifierCons(&name);
        getOp = OP_GET_GLOBAL;
        setOp = OP_SET_GLOBAL;
    }
    // var menu.brunch(sunday).beverage = "mimosa"
    //     --get/target expr-- --set---    --val--
    // only consumed = on lower precedence
    // look for = right after identifier
    // compile val and emit assignment instruction
    if (canAssign && match(TOKEN_EQUAL)) {
        expr();
        emitBytes(setOp, (uint8_t)arg);
    // else higher precedence, ignore =, return to parsePrecedence()
    // emit for var access, load
    } else {
        emitBytes(getOp, (uint8_t)arg);
    }
}
static void var(bool canAssign) {
    namedVar(parser.prev, canAssign);
}
// consume TOKEN_NUM token in scanner, look up & call num() in func ptr arr to compile
static void num(bool canAssign) {
    // discard whitespaces, interpret a floating-point val in a byte str
    // str to double
    // endptr: null, dont care about where num ends
    double val = strtod(parser.prev.start, NULL);
    // val.h
    emitCons(NUM_VAL(val));
}
// +1-2: trim the quotation
static void str(bool canAssign) {
    emitCons(OBJ_VAL(copyStr(parser.prev.start+1, parser.prev.length-2)));
}
// prefix: paren
static void grouping(bool canAssign) {
    // compile: generate bytecode
    expr();
    consume(TOKEN_LEFT_PAREN, "Expect ) after expr");
}
// prefix: unary
static void unary(bool canAssign) {
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
static binary(bool canAssign) {
    TokenType opType = parser.prev.type;
    parsePrecedence(PREC_TERM);
}
static void literal(bool canAssign) {
    switch (parser.prev.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); brea;
        default: return;
    }
}
// only scan the token when compiler needs one
bool compile(const char* src, Chunk* chunk) {
    initScanner(src);
    Compiler compiler;
    initCompiler(&compiler);
    compilingChunk = chunk;
    // mode ends when synchronized statement boundaries
    parser.panicMode = false;
    parser.hadErr = false;
    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
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