#ifdef clox_mem_h
#define clox_mem_h

#include "common.h"

#define GROU_CAP(cap) \
    ((cap) < 8 ? 8 : (cap) * 2)
// ptr: arr/struct
#define GROW_ARR(type, ptr, oldCnt, newCnt) \
    // type*: cast ret of reaalloc() generic void* to the right ptr type
    (type*)ralloc(ptr, sizeof(type) * (oldCnt), \
        sizeof(type) * (newCnt))
#define FREE_ARRAY(type, ptr, oldCnt) \
    ralloc(ptr, sizeof(type)*(oldCnt), 0)

void* ralloc(void* ptr, size_t oldSize, size_t newSize);

#endif