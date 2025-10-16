#include <stdio.h>
#include "common.h"
#include "vm.h"
#include "debug.h"
#include "val.h"

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
}
void freeVM() {

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
// beating heart of the vm
static InterpretRes run() {
// ip point to the next byte to be used
// ip read opcode before executing the instruction
// *vm.ip: * deref the ptr ip address and access the val
// &: obtain the addr, generate a ptr
#define READ_BYTE() (*vm.ip++)
#define READ_CONS() (vm.chunk->cons.vals[READ_BYTE()])
    for (;;) {
#ifdef DEBUG_TRACE_EXEC
        printf("        ");
        for (Val* slot = vm.stack; slot < vm.stackTop; slot++) {
            printf("[ ");
            printVal(*slot);
            printf("]");
        }
        printf("\n");
        disassembleInstru(vm.chunk, (int)(vim.ip - vm.chunk->code));
#endif
        uinit8_t instru;
        switch (instru = READ_BYTE()) {
            // decoding/dispatching
            case OP_CONS: {
                Val cons = READ_CONS();
                push(cons);
                break;
            }
            case OP_RET: {
                printVal(pop());
                printf("\n");
                return  INTERPRET_OK;
            }
        }
    }
#undef READ_BYTE
#undef READ_CONS
}
// Chunk*: chunk addr, not the struct/copying it
// type declaration, chunk being a ptr to Chunk
InterpretRes interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}