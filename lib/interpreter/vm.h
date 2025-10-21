#ifdef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "val.h"
#include "table.h"

/*
fun echo(n) {
    print n;
    return n;
}
print echo(echo(1)+echo(2)) + echo(echo(4)+echo(5));
steps: number -> produce val
start: constant/res of add
length: prev val need to stay around to wait for the right
end: val consumed by op
stack: first in last out -> last in first out
pop off: consuming num from rightmost to left, return
push: instruction produce val, cons/op
stackTop: stack size varies, keep track of the top
ip: use ptr, easier to deref(access val) than int index offset calc
ptr: ele+1, to void ele-1 from empty stack
max: stackTop, ele+1, keep track of
*/

#define STACK_MAX 256

typedef struct {
    Chunk* chunk;
    // instruction ptr, keep track
    // point to the next instruction to be executed
    uint8_t* ip;
    Val stack[STACK_MAX];
    Val* stackTop;
    Table globals;
    Table strs;
    // ptr to the header of the objs linked list
    Obj* objs;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretRes;

extern VM vm;

void initVM();
void freeVM();

InterpretRes interpret(const char* src);
// InterpretRes interpret(Chunk* chunk);

void push(Val val);
void pop();
static void concat();

#endif