// 100x two orders of magnitude faster
// cache: spatial locality/nearby block mem, buffer between processor n memory
// L1 cache: instructions(l-cache) data(d-cache) L3 cache: data memory
// overhead, ptr(mem addr) fields push objs away, out of cache
// emulator vm
#include "common.h"
#include "chunk.h"

int main(int argc, const char* argv[]) {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RET);
    disassembleChunk(&chunk, "test ck");
    freeChunk(&chunk);
    return 0;
}