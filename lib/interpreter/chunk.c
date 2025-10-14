#include <stdlib.h>
#include "chunk.h"
#include "mem.h"

// store/grow new cap 
// -> alloc/grow new arr(copy -> del old arr) -> upd code
// -> store ele -> upd cnt
void initChunk(Chunk* chunk) {
    chunk->cnt = 0;
    chunk->cap = 0;
    chunk->code =  NULL;
}
void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uinit8_t, chunk->code, chunk->cap);
    // zero out the fields to empty state
    initChunk(chunk);
}
void writeChunk(Chunk* chunk, uinit8_t byte) {
    if (chunk->cap < chunk->cnt+1) {
        int oldCap = chunk->cap;
        chunk->cap = GROW_CAP(oldCap);
        chunk->code = GROW_ARR(uinit8_t, chunk->code,
        oldCap, chunk->cap);
    }
    chunk->code[chunk->cnt] = byte;
    chunk->cnt++;
}