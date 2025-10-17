#ifdef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "val.h"

typedef enum {
    // instructions
    // on which order to produce
    OP_CONS,
    OP_ADD,
    OP_DIV,
    OP_NEGATE,
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
    int* line;
    ValArr cons;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
int addCons(Chunk* chunk, Val val);

#endif