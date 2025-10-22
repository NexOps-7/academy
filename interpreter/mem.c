#include <stdlib.h>
#include "mem.h"
#include "vm.h"

// if oldSize == 0, equals to malloc()
void* ralloc(void* ptr, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    // return void 
    void* res = realloc(ptr, newSize);
    if (res == NULL) exit(1);
    return res;
}
// free obj node -> mem
static void freeObj(Obj* obj) {
    switch(obj->type) {
        case OBJ_FUNC: {
            ObjFunc* func = (ObjFunc*)obj;
            freeChunk(&func->chunk);
            FREE(ObjFunc, obj);
            break;
        }
        case OBJ_CLOSURE: {
            FREE(ObjClosure, obj);
            break;
        }
        case OBJ_NATIVE: {
            FREE(ObjNative, obj);
            break;
        }
        case OBJ_STR: {
            ObjStr* str = (ObjStr*)obj;
            FREE_ARR(char, str->chars, str->length+1);
            FREE(ObjStr, obj);
            break;
        }
    }
}
// free objs linked list
void freeObjs() {
    // ptr to the objs
    Obj* obj = vm.objs;
    while (obj != NULL) {
        Obj* next = obj->next;
        freeObj(obj);
        // go to the next
        obj = next;
    }
}