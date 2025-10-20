#include <stdio.h>
#include <string.h>
#include "mem.h"
#include "obj.h"
#include "val.h"
#include "vm.h"
#include "table.h"

// macros pre-processed, func compiled not pre
// generic code for any types, template in c++
// avoid redundantly cast void* back to type
// cast to the correct type* & compute sizeof()
#define ALLOC_OBJ(type, objType) \
    (type*)allocObj(sizeof(type), objType)

// actual func for the macros
static Obj* allocObj(size_t size, objType type) {
    // ralloc() returns void ptr: void* realloc()
    // alloc any kinds of objs
    Obj* obj = (Obj*)ralloc(NULL, 0, size);
    obj->type = type;
    // insert obj into the linked list objs
    // ptr to the head of already existed linked list
    obj->next = vm.objs;
    // update the objs ptr to the new obj
    vm.objs = obj;
    return obj;
}

// obj constructor
static ObjStr* allocStr(char* chars, int length, uint32_t hash) {
    ObjStr* str = ALLOC_OBJ(objStr, OBJ_STR);
    str->length = length;
    str->chars = chars;
    str->hash = hash;
    tableSet(&vm.strs, str, NIL_VAL);
    return str;
}
static uint32_t hashStr(const char* key, int length) {
    uint32_t hash = 2166136261u;
    for (int i=0; i<length; i++) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619;
    }
    return hash;
}

// look up in the str table first, if no, alloc and store in the table
ObjStr* copyStr(const char* chars, int length) {
    // return a ref to the str, instead of copying
    uint32_t hash = hashStr(chars, length);
    ObjStr* interned = tableFindStr(&vm.strs, chars, length, hash);
    if (interned != NULL) return interned;
    // alloc a new arr on heap
    // ALLOC() returns void ptr: void* realloc()  
    // with given type & cnt, just big enough
    char* heapChars = ALLOC(char, length+1);
    // memcpy(dest, src, size_t cnt);
    memcpy(heapChars, chars, length);
    // trailing terminator
    heapChars[length] = '\0';
    return allocStr(heapChars, length, hash);
}
// take ownership
ObjStr* takeStr(char* chars, int length) {
    uint32_t hash = hashStr(chars, length);
    ObjStr* interned = tableFindStr(&vm.strs, chars, length, hash);
    if (interned != NULL) {
        // no need duplicate str, free mem for str
        FREE_ARR(char, chars, length+1);
        return interned;
    }
    return allocStr(chars, length, hash);
}
void printObj(Val val) {
    switch(OBJ_TYPE(val)) {
        case OBJ_STR:
            // CSTR -> chars, not type
            printf("%s", AS_CSTR(val));
            break;
    }
}