#include <stdlib.h>
#include "chunk.h"
#include "mem.h"
#include "val.h"

// store/grow new cap 
// -> alloc/grow new arr(copy -> del old arr) -> upd code
// -> store ele -> upd cnt
void initChunk(Chunk* chunk) {
    chunk->cnt = 0;
    chunk->cap = 0;
    chunk->code =  NULL;
    chunk->line =  NULL;
    // &: access the mem addr
    // *: deref/access val ref by a ptr addr
    // ref: the obj, ptr with cyntactic sugar
    initValArr(&chunk->cons);
}
void freeChunk(Chunk* chunk) {
    FREE_ARR(uinit8_t, chunk->code, chunk->cap);
    FREE_ARR(int, chunk->lines, chunk->cap);
    freeValArr(&chunk->cons);
    // zero out the fields to empty state
    initChunk(chunk);
}
void writeChunk(Chunk* chunk, uinit8_t byte, int line) {
    if (chunk->cap < chunk->cnt+1) {
        int oldCap = chunk->cap;
        chunk->cap = GROW_CAP(oldCap);
        chunk->code = GROW_ARR(uinit8_t, chunk->code,
        oldCap, chunk->cap);
        chunk->line = GROW_ARR(int, chunk->line,
        oldCap, chunk->cap);
    }
    chunk->code[chunk->cnt] = byte;
    chunk->line[chunk->cnt] = line;
    chunk->cnt++;
}

int addCons(Chunk* chunk, Val val) {
    writeValArr(&chunk->cons, val);
    // ret the index of the constant
    return chunk->cons.cnt-1;
}