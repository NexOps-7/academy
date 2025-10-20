#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "val.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "mem.h"
#include "obj.h"

// global VM obj, instead of passing ptr to numerous func
// not the best practice
VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}
void initVM() {
    // no need to alloc stack since its in the VM stuck
    // no need to clear unused cells, access only after vals in
    // indicate the stack is empty
    resetStack();
    vm.objs = NULL;
}
void freeVM() {
    freeTable(&vm.strs);
    freeObjs();
}
static void runtimeErr(const char* format, ...) {
    // variadic func: pass arbitrary no. of args, to vfprintf(va_list)
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    // ip: instru ptr, next instru to be executed
    // offset: vm.ip-vm.chunk->code
    size_t instru = vm.ip-vm.chunk->code-1;
    int line = vm.chunk->lines[instru];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}
void push(Val val) {
    // access the val
    *vm.stackTop = val;
    // increment the ptr to the next unused
    vm.stackTop++;
}
Val pop() {
    // no need to rm it, just move down to mark not in use
    vm.stackTop--;
    // return the val
    return *vm.stackTop;
}
static Val peek(int distance) {
    // how far down from the stack top
    return vm.stackTop[-1-distance];
}
static bool isFalsey(Val val) {
    // val.h null/false
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
} 
static void concat() {
    // val.h -> obj.h
    ObjStr* b = AS_STR(pop());
    ObjStr* a = AS_STR(pop());
    int length = a->length + b->length;
    // +1: trailing terminator
    char* chars = ALLOC(char, length+1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars. b->length);
    char[length] = '\0';
    // takeStr: construct an obj
    ObjStr* res = takeStr(chars, length);
    push(OBJ_VAL(res));
}
// beating heart of the vm, 
// scanner -> val -> chunk -> stack, op, chunk -> debug-> compiler
static InterpretRes run() {
// ip points to the next byte to be used
// ip read opcode before executing the instruction
// *vm.ip: * deref the ptr ip address and access the val
// &: obtain the addr, generate a ptr
#define READ_BYTE() (*vm.ip++)
#define READ_CONS() (vm.chunk->cons.vals[READ_BYTE()])
// c preprocessor
#define BINARY_OP(valType, op) \
    do { \
        // right before a left eval first, right on top
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) { \
            runtimeErr("operands must be num"); \
            return INTERPRET_RUNTIME_ERROR; \
        }
        double b = AS_NUM(pop()); \
        double a = AS_NUM(pop()); \
        push(valType(a op b)); \
        // double b = pop(); \
        // double a = pop(); \
        // push(a op b);\
    // force semicolon/exit after one iteration
    } while (false)

    for (;;) {
#ifdef DEBUG_TRACE_EXEC
        printf("        ");
        for (Val* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printVal(*slot);
            printf("]");
        }
        printf("\n");
        // offset: vm.ip-vm.chunk->code
        disassembleInstru(vm.chunk, (int)(vm.ip-vm.chunk->code));
#endif
        uint8_t instru;
        switch (instru = READ_BYTE()) {
            // decoding/dispatching
            case OP_CONS: {
                Val cons = READ_CONS();
                push(cons);
                break;
            }
            case OP_FALSE:  push(BOOL_VAL(false)); break;
            case OP_EQUAL:  {
                Val b = pop();
                Val a = pop();
                push(BOOL_VAL(valsEqual(a, b)));
                break;
            }
            case OP_ADD: {
                if (IS_STR(peek(0)) && IS_STR(peek(1))) {
                    concat();
                } else if (IS_NUM(peek(0)) && IS_NUM(peek(1))) {
                    double b = AS_NUM(pop());
                    double a = AS_NUM(pop());
                    push(NUM_VAL(a + b));
                } else {
                    runtimeErr(
                        "Operands must be two nums/strs");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DIV:    BINARY_OP(NUM_VAL, /); break;
            // val BOOL_VAL -> chunk OP_NOT
            case OP_NOT:    
                push(BOOL_VAL(isFalsey(pop())));
                break;
            case OP_NEGATE:
                // val.h
                if (!IS_NUM(peek(0))) {
                    runtimeErr("Operand must be a num");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUM_VAL(-AS_NUM(pop())));
                break;
                // push(-pop()); break;
            case OP_RET: {
                printVal(pop());
                printf("\n");
                return  INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONS
#undef BINARY_OP
}

// create a new chunk, pass it to vm
// compiler fill the chunk with bytecode if no err
InterpretRes interpret(const char* src) {
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(src, &chunk)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;
    InterpretRes res = run();
    freeChunk(&chunk);
    return res;

    // hand-written compile test
    /*
        compile(src);
        return INTERPRET_OK; */
};

/*
    // hand-written vm chunk test
    // *chunk: a ptr to Chunk, access Chunk addr, not copying it
    // Chunk: type declaration
    InterpretRes interpret(Chunk* chunk) {
        vm.chunk = chunk;
        vm.ip = vm.chunk->code;
        return run();
    }
*/ 
 