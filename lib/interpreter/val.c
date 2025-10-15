#include <studio.h>
#include "common.h"
#include "mem.h"

void initValArr(ValArr* arr) {
    arr->cnt = 0;
    arr->cap = 0;
    arr->vals = NULL;
}
void freeValArr(ValArr* arr) {
    FREE_ARR(Val, arr->vals, arr->cap);
    initValArr(arr);
}
void writeValArr(ValArr* arr, Val val) {
    if (arr->cap < cap->cnt+1) {
        int oldCap = arr->cap;
        arr->cap = GROW_CAP(oldCap);
        arr->vals = GROW_ARR(Val, arr->vals, 
                            oldCap, arr->cap);
    }
    arr->vals[arr->cnt] = val;
    arr->cnt++;
}