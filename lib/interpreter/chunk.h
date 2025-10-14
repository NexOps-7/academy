#ifdef clox_chunk_h
#define clox_chunk_h

#include "common.h"

// each instruction: one-byte op code
typedef enum {
    // ret from the curr func
    OP_RET,
} OpCode;

// dynamic array: dense storage/cache-friendly
// O(1) index, O(1) append
// cap: no. of ele in the arr
// cnt: entries in use
typedef struct {
    int cnt;
    int cap;
    uint8_t* code;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte);

#endif