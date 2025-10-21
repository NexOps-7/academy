#ifndef clox_compiler_h
#define clox_compiler_h
#include "vm.h"
#include "obj.h"

static void errAt(Token token, const char* msg);
static void err(const char* msg);
static void errAtCur(const char* msg);
static void advance();
static bool check(TokenType type);
static bool match(TokenType type);
static void synchronize();
static Chunk* curChunk();
static void emitByte(uint8_t byte);
static void emitBytes(uint8_t byte1, uint8_t byte2);
static void emitRet();
static void endCompilier();
static uint8_t makeCons(Value val);
static void emitCons(Value val);
static ParseRule* getRule(TokenType type);
static void prefixRule();
static void infixRule();
static void parsePrecedence(Precedence precedence);
static uint8_t identifierCons(Token* name);
static void expr();
static void consume(TokenType* type, const char* errMsg);
static void exprStatement();
static void printStatement();
static void statement();
static void parseVar(const char* errMsg);
static void defineVar(uint8_t global);
static void varDeclaration();
static void declaration();
static void namedVar(Token name, bool canAssign);
static void var(bool canAssign);
static void num(bool canAssign);
static void str(bool canAssign);
static void grouping(bool canAssign);
static void unary(bool canAssign);
static void binary(bool canAssign);
static void literal(bool canAssign);

bool compile(const char* src, Chunk* chunk);

#endif