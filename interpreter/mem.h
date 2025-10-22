#ifdef clox_mem_h
#define clox_mem_h

#include "common.h"
#include "obj.h"

#define ALLOC(type, cnt) \
    (type*)ralloc(NULL, 0, sizeof(type)*(cnt))
#define FREE(type, ptr) ralloc(ptr, sizeof(type), 0)
#define FREE_ARR(type, ptr, oldCnt) \
    // check NUll first
    ralloc(ptr, sizeof(type)*(oldCnt), 0)
#define GROW_CAP(cap) \
    ((cap) < 8 ? 8 : (cap) * 2)
// ptr: arr/struct
#define GROW_ARR(type, ptr, oldCnt, newCnt) \
    // type*: cast ret of ralloc() generic void* to the right ptr type
    (type*)ralloc(ptr, sizeof(type) * (oldCnt), \
        sizeof(type) * (newCnt))

void* ralloc(void* ptr, size_t oldSize, size_t newSize);
// free linked list from obj.h
static void freeObj(Obj* obj);
void freeObjs();

#endif