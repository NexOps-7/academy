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

typedef enum {
    TYPE_FUNC,
    TYPE_SCRIPT
} FuncType;

typedef struct {
    Token name;
    int depth;
    bool isCaptured;
} Local;

// val needs to outlive the func lifetime
typedef struct {
    uint8_t index;
    bool isLocal;
} Upval;

typedef struct Compiler {
    struct Compiler* enclosing;
    ObjFun* func;
    FuncType type;

    Local locals[UINT8_CNT];
    Upval upvals[UINT8_CNT];
    int localCnt;
    int scopeDepth;
} Compiler;

// single global var of the struct
Parser parser;
Compiler* cur = NULL;
// Chunk* compilingChunk;
// static Chunk* curChunk() {
//     return compilingChunk;
// }
static Chunk* curChunk() {
    return &cur->func->chunk;
}
staic void initCompiler(Compiler* compiler) {
    compilier->enclosing = cur;
    compiler->func = NULL;
    compiler->type = type;
    compiler->localCnt = 0;
    compiler->scopeDepth = 0;
    compiler->func = newFunc();
    cur = compiler;
    if (type != TYPE_SCRIPT) {
        // copyStr: findStr() in table, memcpy(), realloc()
        // copy: func obj outlives the compiler, persist until runtime
        cur->func->name = copyStr(parser.prev.start, parser.prev.length);
    }
    // use locals[] slot 0 for vm internal use, localCnt+1
    Local* local = &cur->locals[cur->localCnt++];
    local->depth = 0;
    local->isCaptured = false;
    local->name.start = "";
    local->name.length = 0;
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
    // if no val, but return by the end of the body
    emitByte(OP_NIL);
    emitByte(OP_RET);
}
static ObjFunc* endCompiler() {
    emitRet();
    // compiler create func itself and return
    // before interpret() pass in a chunk to be written to
    ObjFunc* func = cur->func;
#ifdef DEBUG_PRINT_CODE
    if (!parser.hadErr) {
        disassembleChunk(curChunk(), func->name != NULL
                        ? func->name->chars : "<script>");
    }
#endif
    cur = cur->enclosing;
    return func;
}
static uint8_t makeConstant(Value val) {
    // addConstant(): chunk.h->vm.h->compile.h
    // write val to the chunk array/constant table
    // return index
    int constanttant = addConstant(curChunk(), val);
    // 0-255, up to 256 constant in the chunk, single byte for the operand
    // OP_CONSTANT_16 store index as two-byte, scale to larger
    if (constant > UINT8_MAX) {
        err("Too many constant in one chunk");
        return 0;
    }
    // constant table/array
    return (uint8_t)constant;
}
static void emitConstant(Value val) {
    emitBytes(OP_CONSTANT, makeConstant(val));
}
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]  = {grouping, call, PREC_CALL},
    [TOKEN_BANG]        = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]  = {NULL, binary, PREC_EQUALITY},
    [TOKEN_MINUS]       = {unary, NULL, PREC_TERM},
    [TOKEN_ID]          = {var, NULL, PREC_NONE},
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
static endScope() {
    // pop a scope
    cur->scopeDepth--;
    // cur scope always at the arr end, look for var declared at scope depth just left
    // when outer() returns, its stack frame destroyed, but inner() still needs local var, hoist to the heap
    // when returning, at locCnt-1, if iscaptured, upval ptr to local, move to heap, where var always be on stacktop, no op needed
    // else free the stack slots for locals, out of scope, no need for slot, discard them
    // decrement the arr length
    while (cur->localCnt > 0 &&
            cur->locals[cur->localCnt - 1].depth > cur->scopeDepth) {
                if (cur->locals[cur->locCnt-1].isCaptured) {
                    emitByte(OP_CLOSE_UPVAL);
                } else {
                    emitByte(OP_POP);
                }
                cur->localCnt--;
            }
}
static void retStatement() {
    if (cur->type == TYPE_SCRIPT) {
        err(" cant ret from top-level code");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitRet();
    } else {
        expr();
        consume(TOKEN_SEMICOLON, "Expect ';' after ret val");
        emitByte(OP_RET);
    }
}
static int emitJump(uint8_t instruction) {
    emitByte(instruction);
    // placeholder for two bytes offset operand
    // 16-bit offset jump up to 65,535 bytes
    emitByte(0xff);
    emitByte(0xff);
    return curChunk()->cnt - 2;
}
// after compiling, replace operand with cal jump offset
// use cur bytecode to determine how far to jump
static void patchJump(int offset) {
    // -2: the jump itself takes 2 bytes
    // offset: where the jump starts, first byte of the jump's 2-byte offset placeholder
    int jumpDis = curChunk()->cnt - offset - 2;
    if (jumpDis > UINT16_MAX) {
        err("Too much code to jump over");
    }
    // replace the placeholder jumpDis, store it back into bytecode
    curChunk()->code[offset] = (jumpDis >> 8) & 0xff;
    curChunk()->code[offset+1] = jumpDis & 0xff;
}
// if -> jump false -> then -> jump -> else
static void ifStatement() {
    // check type
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if' ");
    expr();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    // return placeholder, skip then if false
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    // 'then jump/skip then if false' only truly completes after emitting 'else jump'/skip else-block
    int elseJump = emitJump(OP_JUMP);
    // forward jump: jump over code dont want to execute/exit
    // execute condition
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) statement();
    // after compiling the else block
    patchJump(elseJump);
}
static void emitLoop(int loopStart) {
    emitByte(OP_LOOP);
    // jump back to start, 2 bytes for the jump offset itself
    int offset = curChunk()->cnt - loopStart + 2;
    if (offset > UINT15_MAX) err("Loop body too large");
    // encode: store 16-bit as two 8-bit bytes to bytecode stream and later read back and reconstruct 16-bit
    // discard the lowest 8 bits, move highbyte to the right/down, divide offset by 256
    // bitwise AND: 0x1234 -> 0x12 & low byte 0xff -> 0x12
    emitByte((offset >> 8) & 0xff);
    // low byte
    emitByte(offset & 0xff);
}
static void whileStatement() {
    // jump back to loopStart
    int loopStart = curChunk()->cnt;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while' ");
    expr();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition");
    // index of first placeholder, 2 bytes placeholders
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    // backward jump: how far back to the loopstart
    // execute body, to repeat the loop
    emitLoop(loopStart);
    // forward jump: jump over code dont want to execute/exit
    // execute condition
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
static uint8_t argList() {
    uint8_t argCnt = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expr();
            if (argCnt == 255) {
                err("Cant have more than 255 args");
            }
            argCnt++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after args");
    return argCnt;
}
static void call(bool canAssign) {
    uint8_t argCnt = argList();
    emitBytes(OP_CALL, argCnt);
}
static void markInitialized() {
    // var cuppa = "joe";
    //    1             2
    // 1->uninitialized
    // 2->initialized
    // avoid var a = "outer"; {var a = a;}
    // global
    if (cur->scopeDepth == 0) return;
    // local
    cur->locals[cur->localCnt-1].depth =
        cur->scopeDepth;
}

static void defineVar(uint8_t global){
    // global var can be defined after ref compiled before execution, good for manual recursion
    // fun showVar() {print global;}
    // var global = "after"; showVar();
    emitBytes(OP_DEFINE_GLOBAL, global);
    // local var already in temp, top of the stack
    // var a = 1 + 2; var b = 4; 3-a-OP_ADD -> local slot
    if (cur->scopeDepth > 0) {
        markInitialized();
        return;
    }
}
static void block() {
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block");
}
static void func(FuncType* type) {
    Compiler compiler;
    initCompiler(compiler);
    beginScope();
    // err msg
    consume(TOKEN_LEFT_PAREN, "Expect '(' after func name");
    // params: like local var declared in the outermost lexical scope without initializer
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            cur->func->arity++;
            if (cur->func->arity > 255) {
                errAtCur("Cant have more than 255 params");
            }
            uint8_t constant = parseVar("Expect param name");
            // emit byte, write chunk arr, markInitialized for local var
            defineVar(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after param");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before func body ");
    // recursive declaration() to the end of the body
    block();
    // emit return and create func
    ObjFunc* func = endCompiler();
    // makeConstant(): return the index from the obj into val in the constant table
    // emitBytes(): writeChunk() instruction opcode + operands
    emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(func)));

    for (int i=0; i<func->upalCnt; i++) {
        emitByte(compiler.upvals[i].isLocal ? 1 : 0);
        emitByte(compiler.upvals[i].index);
    }
}
static void funcDeclaration() {
    // func creates & stores a var
    // index in the constant table
    // errMsg -> add to locals[] -> token to val -> check size -> return index
    uint8_t global = parseVar("Expect func name");
    // can recursively call func itself, mark it right away, not like local vars
    markInitialized();
    func(TYPE_FUNC);
    // emit byte, write chunk arr, markInitialized for local var
    defineVar(global);
}
static void varDeclaration() {
    // index in the constant table
    // errMsg -> add to locals[] -> token to val -> check size -> return index
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
    if (match(TOKEN_FUNC)) {
        funcDeclaration();
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) synchronize();
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
    } else if (match(TOKEN_RET)) {
        retStatement();
    } else {
        exprStatement();
    }
}
static void identifierEqual(Token* a, Token* b) {
    if (a->length != b->length) return false;
    return memcmp(a->start, b->start, a->length) == 0;
}
static void addLocal(Token* name) {
    if (cur->localCnt == UINT8_CNT) {
        err("Too many local vars in func");
        return;
    }
    // local arr locals[] mirrors the stack slot indexes where locals live at runtime
    Local* local = &cur->locals[cur->localCnt++];
    local->name = name;
    // local->depth = cur->scopeDepth;
    // uninitialized state
    local->depth = -1;
    local->isCaptured = false;
}
// only for local: to remember the var exists, put it in locals, name, depth
static void declareVar() {
    // top level
    if (cur->scopeDepth == 0) return;
    Token* name = &parser.prev;
    // append to arr, from arr end backwards
    for (int i = cur->localCnt-1; i>=0; i--) {
        Local* local = &cur->locals[i];
        // local var can have same name, as long as diff scope
        // != -1 local, global others, not within scope?
        // from arr end backwards look for same name
        // stop when reach the arr beginning or var owned by another scope
        if (local->depth != -1 && local->depth < cur->scopeDepth) {
            break;
        }
        if (identifiersEqual(name, &local->name)) {
            err("Already a var with this name in this scope");
        }
    }
    addLocal(*name);
}
static void parseVar(const char* errMsg ) {
    consume(TOKEN_ID, errMsg);
    // only for local, put in locals[], record the existence of the var
    declareVar();
    // in local scope, no need to store var in constant table, ret dummy index
    if (cur->scopeDepth > 0) return 0;
    return identifierCons(&parser.prev);
}
static uint8_t resolveLocal(Compiler* compiler, Token* name) {
    // look thru the locals in scope
    // same name as identifier var ref token
    // backwards: find the last declared
    for (int i=compiler->localCnt-1; i>=0; i--) {
        Local* local = &compiler->locals[i];
        if (identifierEqual(name, &local->name)) {
            // -1: ref to a var in its own initializer
            if (local->depth == -1) {
                err("Cant read local var in its own initializer");
            }
            // found it
            return i;
        }
    }
    // not found
    return -1;
}
staic int addUpval(Compiler* compiler, uint8_t index, bool isLocal) {
    int upvalCnt = compiler->func->upvalCnt;
    for (int i=0; i<upvalCnt; i++) {
        Upval* upval = &compiler->upvals[i];
        // indexes in compiler arr matches the index where upval lives in ObjClosure runtime
        // as locals[] in runtime stack slot
        // find the same slot index upval
        if (upval->index == index && upval->isLocal == isLocal) {
            return i;
        }
    }
    if (upvalCnt == UINT8_CNT) {
        err('Too many closure vars in func');
        return 0;
    }
    compiler->upvals[upvalCnt].isLocal = isLocal;
    compiler->upvals[upvalCnt].index = index;
    return compiler->func->upvalCnt++;
}
// each var ref resolved as local, upval, global
static uint8_t resolveUpval(Compiler* compiler, Token* name) {
    // each func has a stack frame w/ locals
    // first look for a matching local var in the enclosing func, identifier resolved
    // when found, capture true, vm create an upval obj point to the local
    // recursively call on the enclosing compiler
    // until it finds local var to capture/run out of compilers
    // returns the upval index, returns all the way to the next call for outmost func declaration
    if (local != -1) {
        compiler->enclosing->locals[local].isCaptured = true;
        return addUpval(compiler, (uint8_t)local, true);
    }
    int upval = resolveUpval(compiler->enclosing, name);
    // found it
    if (upval != -1) {
        return addUpval(compiler, (uint8_t)upval, false);
    }
    // not found
    return -1;
}
// global var a = nil;
static void namedVar(Token name, bool canAssign) {
    uint8_t getOp, setOp;
    uint8_t arg = resolveLocal(cur, &name);
    // resolveLocal() return found the given name in local var
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
ObjFunc* compile(const char* src) {
    initScanner(src);
    Compiler compiler;
    initCompiler(&compiler, TYPE_SCRIPT);
    curChunk = chunk;
    // mode ends when synchronized statement boundaries
    parser.panicMode = false;
    parser.hadErr = false;
    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    ObjFunc* func = endCompiler();
    return parser.hadErr ? NULL : func;

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