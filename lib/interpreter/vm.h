#ifdef clox_vm_h
#define clox_vm_h

#include "chunk.h"

typedef struct {
    Chunk* chunk;
    // instruction ptr, keep track
    // point to the next instruction to be executed
    uint8_t* ip;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretRes;

void initVM();
void freeVM();
InterpretRes interpret(Chunk* chunk);

#endif