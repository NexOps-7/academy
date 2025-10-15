#include <stdio.h>
#include "debug.h"
#include "val.h"

static int simpleInstru(const char* name, int offset) {
    // opcode/OP_RET
    printf("%s\n", name);
    // ret index of the next chunk
    return offset + 1;
}
static int consInstru(const char* name, Chunk* chunk, int offset) {
    uinit8_t cons = chunk->code[offset+1];
    // print opcode/OP_CONS, index
    print("%-16s %4d '", name, cons);
    // the actual val from the next chunk index
    printVal(vals[cons]);
    printf("\n");
    // two bytes --one for opcode, one for operand
    return offset + 2;
}
int disassembleInstru(Chunk* chunk, int offset) {
    // the loc/no. of bytes from the beginning
    printf("%04d ", offset);
    // source line
    // one line complies to a sequence of instructions
    // |: show the one has the same line as the preceding one
    if (offset > 0 && 
        chunk->line[offset] == chunk->line[offset-1]) {
            printf("    | ")
    } else {
        printf("%4d ", chunk->line[offset]);
    }
    // read one byte
    uinit8_t instru = chunk->code[offset];
    switch(instru) {
        case OP_CONS:
            return consInstru("OP_CONS", chunk, offset);
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