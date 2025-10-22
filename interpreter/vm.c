#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
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
    vm.frameCnt = 0;
}
void initVM() {
    // no need to alloc stack since its in the VM stuck
    // no need to clear unused cells, access only after vals in
    // indicate the stack is empty
    resetStack();
    vm.objs = NULL;
    initTable(&vm.globals);
    initTable(&vm.strs);
    defineNative("newClock", clockNative);
}
void freeVM() {
    freeTable(&vm.globals);
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

    // the last frame top-down print the line number and name
    for (int i=vm.frameCnt-1; i>=0; i--) {
        CallFrame* frame = &vm.frames[i];
        ObjFunc* func = frame->closure->func;
        // from the beginning to the executing one
        // -1: ip point to next, we want the last failed instruction
        size_t instruction = frame->ip - frame->func->chunk.code - 1;
        int line = func->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (func->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", func->name->chars);
        }
    }

    // ip: instruction ptr, next instruction to be executed
    // offset: vm.ip-vm.chunk->code
    // size_t instruction = vm.ip-vm.chunk->code-1;
    // int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] in script\n", line);
    resetStack();
}
static Val clockNative(int argCnt, Val* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}
static void defineNative(const char* name, NativeFn func) {
    push(OBJ_VAL(copyStr(name, (int)strlen(name))));
    push(OBJ_VAL(newNative(func)));
    // store in stack for the garbage collection to not free
    // tableSet(table, key, val);
    tableSet(&vm.globals, AS_STR(vm.stack[0]), vm.stack[1]);
    pop();
    pop();
}
static bool call(ObjClosure* closure, int argCnt) {
    // not statically typed, for too many/few args from user
    // func->arity in compiler, argCnt user runtime byte chunk arr
    if (argCnt != closure->func->arity) {
        runtimeErr("Expected %d arguments but got %d", closure->func->arity, argCnt);
        return false;
    }
    if (vm.frameCnt == FRAME_MAX) {
        runtimeErr("Stack overflow");
        return false;
    }
    // initialize the next callframe
    CallFrame* frame = &vm.frames[vm.frameCnt++];
    frame->closure = closure;
    frame->ip = closure->func->chunk.code;
    // make sure the args line up with func params
    // sum() callframe: sum 5 6 7
    // frame slots <-> stackTop == 1(op sum slot0) + argCnt(3)
    frame->slots = vm.stackTop - argCnt - 1
    return true;
}
static bool callVal(Val callee, int argCnt) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            // runtime no need to call OBJ_FUNC, since its wrapped in OBJ_CLOSURE
            // only live in constant table now and wrapped in closure
            // case OBJ_FUNC:
            //     return call(AS_FUNC(callee), argCnt);
            case OBJ_CLOSURE:
                return call(AS_CLOSURE(callee), argCnt);
            case OBJ_NATIVE:
                // call c func right now and here, no need for callframe
                // get the res, stuff it right back to the stack
                NativeFn native = AS_NATIVE(callee);
                Val res = native(argCnt, vm.stackTop - argCnt);
                vm.stackTop -= argCnt + 1;
                push(res);
                return true;
            default:
                non-callable obj type
                break;
        }
    }
    runtimeErr("Can only call func & clss");
    return false;;
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
    // top most callFrame 
    // -1: index 0, 1, 2, main, foo(), cur-> frames[2]
    // frames[3] would point to one past end of frames, invalid mem
    CallFrame* frame = &vm.frames[vm.frameCnt - 1];
// instruction ptr ip points to the next byte to be used
// read opcode before executing the instruction
// *vm.ip: * deref the ptr ip address and access the val
// &: obtain the addr, generate a ptr
#define READ_BYTE() (*frame->ip++)

// build 16-bit unsigned int, pull next two bytes from chunk
// short integer: 2 bytes
#define READ_SHORT() \
        (*frame->ip += 2, \
        // two bytes: 
        // [-2]: high byte [-1]: low byte
        // (0x01 << 8 | 0x02) -> left shift -> 0x0102 -> 258
        (uint16_t)((*frame->ip[-2] << 8) | *frame->ip[-1])) \

#define READ_CONSTANT() (frame->closure->func->chunk.constant.vals[READ_BYTE()])

// AS_STR: (ObjStr*)(Val).union{bool, double, obj*} constant table
#define READ_STR() AS_STR(READ_CONSTANT())

// c preprocessor macros
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
        // ->: ptr to a struct .:direct from a struct mem
        disassembleInstruction(&frame->closure->func->chunk, 
                        (int)(frame->ip - frame->closure->func->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            // decoding/dispatching
            case OP_CALL: {
                int argCnt = READ_BYTE();
                if (!callVal(peek(argCnt), argCnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CLOSURE: {
                // treat like constant
               ObjFunc* func = AS_FUNC(READ_CONSTANT());
               ObjClosure* closure = newClosure(func);
               push(OBJ_VAL(closure));
               break;
            }
            case OP_CONSTANT: {
                Val constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_FALSE:  push(BOOL_VAL(false)); break;
            case OP_POP:    pop(); break;
            case OP_JUMP: {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                // 16 bit operand
                uint16_t offset = READ_SHORT();
                // null or false
                // if (isFalsey(peek(0))) vm.ip += offset;
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                uint16_t offset = READ_SHORT();
                // read the next two bytes
                // frame->ip byte ptr, an addr ptr width 32-bit/64-bit, uint8_t* ptr to the bytes 258
                // to grow bigger jumps -> offset uint16_t up to 65k bytes to jump
                // one ele one byte, backwards
                frame->ip -= offset;
                break;
            }
            case OP_GET_LOC: {
                uint8_t slot = READ_BYTE();
                // push(vm.stack[slot]);
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOC: {
                // assignment expr produces a val
                // take from stacktop, store and leave the val on the stack, dont pop()
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                // get name from constant table
                ObjStr* name = READ_STR();
                // name stack on top as key in the hash table
                // overwrite if already exists
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_GET_GLOBAL: {
                // use constant table index of instruction operand to get var name as key
                ObjStr* name = READ_STR();
                Val val;
                // key doesnt exist
                if (!tableGet(&vm.globals, name, &val)) {
                    runtimeErr("Undefined var '%s' ", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(val);
                break;
            }
            case OP_SET_GLOBAL: {
                ObjStr* name = READ_STR();
                // tableSet(table, key, val)
                // ret isNewKey, var undefined
                if (tableSet(&vm.globals, name, peek(0))) {
                    // del the zombie val of the stored undefined global vars in table
                    tableDel(&vm.globals, name);
                    runtimeErr("Undefined var '%s' ", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
            case OP_PRINT:
                printVal(pop());
                printf("\n");
                break;
            case OP_RET: {
                // exit interpreter
                // ret val on top, pop it but hold on to it
                // discard callframes
                // push ret back to lower loc in the stack
                // update run() cached ptr to cur frame
                Val res = pop();
                vm.frameCnt--;
                if (vm.frameCnt == 0) {
                    pop();
                    // frames and main script poped, done
                    return  INTERPRET_OK;
                }
                vm.stackTop = frame->slots;
                push(res);
                frame = &vm.frames[vm.frameCnt-1];
            }
        }
    }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_STR
#undef BINARY_OP
}

// runtime
// push the returned compiled top level code to the stack
// initialize to execute the code
// ptr to the beginning of the func bytecode
// and at the bot of the val stack
InterpretRes interpret(const char* src) {
    ObjFunc* func = compile(src);
    if (func == NULL) return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL(func));
    ObjClosure* closure = newClosure(function);
    pop();
    push((OBJ_VAL)(closure));
    // initialize callFrame
    call(closure, 0);

    return run;

    // no chunk anymore
    // create a new chunk, pass it to vm
    // compiler fill the chunk with bytecode if no err
    // Chunk chunk;
    // initChunk(&chunk);
    // if (!compile(src, &chunk)) {
    //     freeChunk(&chunk);
    //     return INTERPRET_COMPILE_ERROR;
    // }

    // vm.chunk = &chunk;
    // vm.ip = vm.chunk->code;
    // InterpretRes res = run();
    // freeChunk(&chunk);
    // return res;

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
 