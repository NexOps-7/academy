#include <stdio.h>
#include "debug.h"

static int simpleInstru(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}
int disassembleInstru(Chunk* chunk, int offset) {
    // the loc
    printf("%04d ", offset);
    // read one byte
    uinit8_t instru = chunk->code[offset];
    switch(instru) {
        case OP_RET:
            // read single byte
            return simpleInstru("OP_RET", offset);
        default:
            printf("Unknown opcode %d\n", instru);
            return offset + 1;
    }
}
void disassembleChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    // instructions diff sizes, not offset++
    for (int offset = 0; offset < chunk->cnt;) {
        offset = disassembleInstru(chunk, offset);
    }
}