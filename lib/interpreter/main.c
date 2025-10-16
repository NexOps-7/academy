// 100x two orders of magnitude faster
// cache: spatial locality/nearby block mem, buffer between processor & memory
// L1 cache: instructions(l-cache) data(d-cache) L3 cache: data memory
// overhead, ptr(mem addr) fields push objs away, out of cache
// emulator vm
#include "common.h"
#include "chunk.h"
#include "vm.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
    initVM();
    Chunk chunk;
    initChunk(&chunk);
    // ret the index
    int cons = addCons(&chunk, 1.2);
    // first byte opcode: how many operand bytes & what its like
    // which constant to load from chunk arr/constant pool
    // OP_CONS -> 1.2
    writeChunk(&chunk, OP_CONS, 123);
    // 2nd byte index operand in the constant pool
    writeChunk(&chunk, cons, 123);
    // 3rd single one byte return instruction
    writeChunk(&chunk, OP_RET, 123);
    // print 3 bytes (offset0000 line123 opcode_name index val)
    // == test chunk ==
    // 0000 123 OP_CONS     0 '1.2'
    // 0002     | OP_RET
    disassembleChunk(&chunk, "test chunk");
    interpret(&chunk);
    freeVM();
    freeChunk(&chunk);
    return 0;
}