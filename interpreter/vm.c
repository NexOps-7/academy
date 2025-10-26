#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "compiler.h"
#include "mem.h"
#include "obj.h"

// global VM obj, instead of passing ptr to numerous func
// not the best practice
VM vm;

static void resetStack() {
    // its in the VM stack, no need to alloc stack
    // access only after vals in, no need to clear unused cells, the stack is empty, 
    vm.openUpvals = NULL;
    vm.stackTop = vm.stack;
    vm.frameCnt = 0;
}
void initVM() {
    resetStack();
    vm.objs = NULL;
    vm.bytesAllocated = 0;
    vm.nextGC = 1024*1024;
    // garbage collection: white -> gray -> black process
    vm.grayCnt = 0;
    vm.grayCap = 0;
    vm.grayStack = NULL;
    vm.initStr = NULL;
    vm.initStr = copyStr("init", 4);
    initTable(&vm.globals);
    initTable(&vm.strs);
    defineNative("newNativeClock", clockNative);
}
void freeVM() {
    freeTable(&vm.globals);
    freeTable(&vm.strs);
    vm.initStr = NULL;
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
        // ip: instruction ptr, next instruction to be executed
        // offset: vm.ip-vm.chunk->code
        // size_t instruction = vm.ip-vm.chunk->code-1;
        // int line = vm.chunk->lines[instruction];
        // ->: ptr .:access val from struct, not a ptr
        size_t instruction = frame->ip - func->chunk.code - 1;
        int line = func->chunk.lines[instruction];
        fprintf(stderr, "[line %d] in ", line);
        if (func->name == NULL) {
            fprintf(stderr, "script\n");
        } else {
            fprintf(stderr, "%s()\n", func->name->chars);
        }
    }

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
            case OBJ_BOUND_METHOD: {
                ObjBoundMethod* bound = AS_BOUND_METHOD(callee);
                vm.stackTop[-argCnt-1] = bound->receiver;
                return call(bound->method, argCnt);
            }
            case OBJ_CLASS: {
                ObjClass* klass = AS_CLASS(callee);
                vm.stackTop[-argCnt-1] = OBJ_VAL(newInstance(klass));
                Val initializer;
                // tableGet(*tb, key, val);
                if (tableGet(&klass->methods, vm.initStr, &initializer)) {
                    return call(AS_CLOSURE(initializer), argCnt);
                } else if (argCnt != 0) {
                    runtimeErr("Expected 0 args, got %d", argCnt);
                    return false;
                }
                return true;
            }
            // runtime OBJ_FUNC only lives in constant table now
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
                // non-callable obj type
                break;
        }
    }
    runtimeErr("Can only call func & clss");
    return false;;
}
static bool invokeFromClass(ObjClass* klass, ObjStr* name, Int argCnt) {
    Val method;
    if (!tableGet(&klass->methods; name; &method)) {
        runtimeErr("undefined property '%s' ", name->chars);
        return false;
    }
    return call(AS_CLOSURE(method), argCnt);
}
static bool invoke(ObjStr* name, int argCnt) {
    Val receiver = peek(argCnt);
    if (!IS_INSTANCE(receiver)) {
        runtimeErr("Only instances have methods");
        return false;
    }
    ObjInstance* instance = AS_INSTANCE(receiver);
    Val val;
    if (tableGet(&instance->fields, name, &val)) {
        vm.stackTop[-argCnt-1] = val;
        return callVal(val, argCnt);
    }
    return invokeFromClass(instance->klass, name, argCnt);
}
static bool bindMethod(ObjClass* klass, ObjStr* name) {
    Val method;
    if (!tableGet(&klass->methods, name, &method)) {
        runtimeErr("Undefined property '%s' ", name->chars);
        return false;
    }
    ObjBoundMethod* bound = newBoundMethod(peek(0), AS_CLOSURE(method));
    pop();
    push(OBJ_VAL(bound));
    return true;
}
static ObjUpval* captureUpval(Val* local) {
    // make sure only one single ObjUpval for any given local slot
    // start from the head of the list, the stacktop, ptr comp, 
    // past every upval above local, the one we're looking for
    // update the next to cur/new one upval
    // then insert created upval into the list, before the obj pointed at by upval
    // at the head: prev is NULL, NULL->'next'/head vm.openupvals is new created upval
    // else prev->next
    ObjUpval* prevUpval = NULL;
    ObjUpval* upval = vm.openUpvals;
    while (upval != NULL && upval->location > local) [
        prevUpval = upval;
        upval = upval->next;
    ]
    if (upval != NULL && upval->location == local) {
        return upval;
    }
    ObjUpval* createdUpval = newUpval(local);
    createdUpval->next = upval;
    if (prevUpval == NULL) {
        vm.openUpvals = createdUpval;
    } else {
        prevUpval->next = createdUpval;
    }
    return createdUpval;
}
static void closeUpvals(Val* last) {
    while (vm.openUpvals != NULL &&
            vm.openUpvals->location >= last) {
        ObjUpval* upval = vm.openUpvals;
        // obj struct {location, closed, next}
        // get the location index and pass it to closed
        // location is a ref to closed
        // point to next
        upval->closed = *upval->location;
        upval->location = &upval->closed;
        vm.openUpvals = upval->next;
    }
}
static void defineMethod(ObjStr* name) {
    // stacktop val func closure method, when body compiled later on
    // class defined earlier
    // bind the method to the class
    // pop method, leave only class
    Val method = peek(0);
    ObjClass* klass = AS_CLASS(peek(1));
    tableSet(&klass->methods, name, method);
    pop();
}

void push(Val val) {
    // ptr* in header: access the val
    // increment the ptr to the next unused
    *vm.stackTop = val;
    vm.stackTop++;
}
Val pop() {
    // no need to rm it, just move down to mark not in use
    // return the val
    vm.stackTop--;
    return *vm.stackTop;
}
static Val peek(int distance) {
    // how far down from the stack top -(dis+1)
    return vm.stackTop[-1-distance];
}
static bool isFalsey(Val val) {
    // val.h null/false
    return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
} 
static void concat() {
    // need to alloc a new char arr on the heap, which trigger gc
    ObjStr* b = AS_STR(peek(0));
    ObjStr* a = AS_STR(peek(1));
    int length = a->length + b->length;
    // ptr chars, char type
    // +1: trailing terminator
    char* chars = ALLOC(char, length+1);
    // memcpy(void *dest, const void *src, size_t cnt);
    memcpy(chars, a->chars, a->length);
    memcpy(chars+a->length, b->chars, b->length);
    char[length] = '\0';
    // takeStr: construct an objstr*
    ObjStr* res = takeStr(chars, length);
    pop();
    pop();
    push(OBJ_VAL(res));
}

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
// READ_CONSTANT()
// ++: both read and advance by one step, return uint8_t
// +=2: dont deref directly, but advance the ptr position first, returns later (uin16_t)
#define READ_BYTE() (*frame->ip++)

// for jump, fetch a 16-bit unsigned int, pull next two bytes from chunk
// short integer: 2 bytes jump back and forth, calls and funcs
// vm.ip
#define READ_SHORT() \
        // += 2: skip two bytes
        // -: bytes before the updated ip
        // [-2]: high byte [-1]: low byte
        // (0x01 << 8 | 0x02) -> left shift -> 0x0102 -> 258
        (frame->ip += 2, \
        (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->func->chunk.constants.vals[READ_BYTE()])

// AS_STR: (ObjStr*)(Val).union{bool, double, obj*} constant table
#define READ_STR() AS_STR(READ_CONSTANT())

// c preprocessor macros
#define BINARY_OP(valType, op) \
    do { \
        if (!IS_NUM(peek(0)) || !IS_NUM(peek(1))) { \
            runtimeErr("operands must be num"); \
            return INTERPRET_RUNTIME_ERROR; \
        } \
        // right before a left eval first, right on top
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
        // vm.ip-vm.chunk->code: offset
        // ->: ptr to a struct .:direct from a struct mem
        disassembleInstruction(&frame->closure->func->chunk, 
                        (int)(frame->ip - frame->closure->func->chunk.code));
#endif
        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            // decoding/dispatching
            case OP_CALL: {
                int argCnt = READ_BYTE();
                // get callable add(2, 4) func[arg1, arg2, closure]
                // stack -> call add -> [6]
                // else not callable
                if (!callVal(peek(argCnt), argCnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                // update frame
                // when ret, pops the frame
                // main script frame 1 + add() calls frame 1 -> main 1
                frame = &vm.frames[vm.frameCnt-1];
                break;
            }
            case OP_INVOKE: {
                // invoke -> method argCnt call()
                ObjStr* method = READ_STR();
                int argCnt = READ_BYTE();
                if (!invoke(method, argCnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCnt - 1];
                break;
            }
            case OP_SUPER_INVOKE: {
                ObjStr* method = READ_STR();
                int argCnt = READ_BYTE();
                ObjClass* superclass = AS_CLASS(pop());
                if (!invokeFromClass(method, argCnt)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCnt - 1];
                break;
            }
            case OP_CLASS:
                push(OBJ_VAL(newClass(READ_STR())));
            case OP_INHERIT: {
                Val superclass = peek(1);
                if (!IS_CLASS(superclass)) {
                    runtimeErr("superclass must be a class");
                    return INTERPRET_RUNTIME_ERROR;
                }
                // subclass: stacktop last pushed in
                // superclass: -1
                // add superclass -> subclass
                // pop sublass
                ObjClass* subclass = AS_CLASS(peek(0));
                tableAddAll(&AS_CLASS(superclass)->methods, &subclass->methods);
                pop();
                break;
            }
            case OP_METHOD:
                // method: stacktop last pushed in
                // lass: -1
                defineMethod(READ_STR());
                break;
            case OP_CLOSURE: {
                // treat like constant
               ObjFunc* func = AS_FUNC(READ_CONSTANT());
               ObjClosure* closure = newClosure(func);
               push(OBJ_VAL(closure));
               for (int i=0; i<closure->upvalCnt; i++) {
                // read upval
                // if local, create a upval, slot zero + index offset
                // read right from the frame local var, caches a ref to callframe
                // else, use cur surrounding closure, callframe stacktop 
                uint8_t isLocal = READ_BYTE();
                uint8_t index = READ_BYTE();
                if (isLocal) {
                    closure->upvals[i] = 
                                    captureUpval(frame->slots + index);
                } else {
                    closure->upvals[i] = frame->closure->upvals[index];
                }
               }
               break;
            }
            case OP_CLOSE_UPVAL: {
                // stacktop is one below the top sentinel
                closeUpvals(vm.stackTop-1);
                pop();
                break;
            }
            case OP_GET_UPVAL: {
                uint8_t slot = READ_BYTE();
                // deref location ptr, access val in slot of cur func upvals[] arr
                push(*frame->closure->upvals[slot]->location);
                break;
            }
            case OP_SET_UPVAL: {
                uint8_t slot = READ_BYTE();
                // take stacktop val and store in slot pointed to by chosen upval
                // need to be fast
                *frame->closure->upvals[slot]->location = peek(0);
                break;
            }
            case OP_GET_PROPERTY: {
                if (!IS_INSTANCE(peek(0))) {
                    runtimeErr("Only instances have properties");
                    return INTERPRET_RUNTIME_ERROR
                }
                /*
                class Person {
                    var name = "Alice";
                }
                var p = Person();
                print p.name;
                */
                // peek(0): instance p
                // read var 'name' from constant table
                // look up 'name' in p's fields
                // pop p, push val of p Alice
                ObjInstance* instance = AS_INSTANCE(peek(0));
                ObjStr* name = READ_STR();
                Val val;
                if (tableGet(&instance->fields, name, &val)) {
                    pop();
                    push(val);
                    break;
                }
                if (!bindMethod(instance->klass, name)) {
                    return INTERPRET_RUNTIME_ERROR
                }
                break;
            }
            case OP_SET_PROPERTY: {
                // set p.name = "Alice" 
                // val stacktop instance next
                if (!IS_INSTANCE(peek(1))) {
                    runtimeErr("Only instances have properties");
                    return INTERPRET_RUNTIME_ERROR
                }
                ObjInstance instance = AS_INSTANCE(peek(1));
                // set name->key, val->val in instance table
                // pop both and push val
                tableSet(&instance->fields, READ_STR(), peek(0));
                Val val = pop();
                pop();
                push(val);
                break;
            }
            case OP_GET_SUPER: {
                ObjStr* name = READ_STR();
                // pushed last thru super.method()
                ObjClass* superclass = AS_CLASS(pop());
                if (!bindMethod(superclass, name)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CONSTANT: {
                Val constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_NIL: push(NIL_VAL); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
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
                // null or false, skip to else
                // if (isFalsey(peek(0))) vm.ip += offset;
                if (isFalsey(peek(0))) frame->ip += offset;
                break;
            }
            case OP_LOOP: {
                // read the next two bytes
                // frame->ip byte ptr, an addr ptr width 32-bit/64-bit 
                // uint8_t* ptr to the bytes 258
                // bigger jumps -> offset uint16_t up to 65k bytes to jump
                // one ele one byte 
                // backwards to the beginning
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }
            // vm get: push getted to stack
            // vm set: set the stacktop in the frame
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                // push(vm.stack[slot]);
                push(frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL: {
                // assignment expr produces a val
                // peek(0): stacktop, store and leave the val on the stack, dont pop()
                uint8_t slot = READ_BYTE();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_GLOBAL: {
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
            case OP_DEFINE_GLOBAL: {
                // x = 42 42 pushed to stack x in constant table
                // store 42 in x table, and pop 42
                // name: chunk.constants.vals(*vm.ip++).str
                ObjStr* name = READ_STR();
                // name from val table as key, stacktop as val
                // create or update, overwrite if already exists
                // store in table and pop the stacktop
                tableSet(&vm.globals, name, peek(0));
                pop();
                break;
            }
            case OP_SET_GLOBAL: {
                // x = 42, update if x exists, otherwise err
                // no popping
                ObjStr* name = READ_STR();
                // tableSet(table, key, val)
                // ret is NewKey true -> var undefined
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
            case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
            case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
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
                // discard callframes, CLOSE_UPVAL for each local var, also close func params, local inside func
                // ret val on top, pop it but hold on to it
                // pass in the first stack slot owned by the func to close upvals
                // push ret val back to lower local in the stack
                // update frame run() cached ptr to cur frame
                Val res = pop();
                closeUpvals(frame->slots);
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

void hack(bool b) {
    // ensure run() invoked, avoid unused func compiler err
    // start with b = true, run() twice
    run();
    if (b) hack(false);
}

// runtime scanning on demand
// push the returned compiled top level code to the stack
// initialize to execute the code
// ptr to the beginning of the func bytecode
// push func to create closure
// pop func, push closure
// call closure to init frame, run vm
InterpretRes interpret(const char* src) {
    ObjFunc* func = compile(src);
    if (func == NULL) return INTERPRET_COMPILE_ERROR;
    push(OBJ_VAL(func));
    ObjClosure* closure = newClosure(func);
    pop();
    push((OBJ_VAL)(closure));
    // initialize callFrame
    call(closure, 0);

    return run;

    // chunk test
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
 