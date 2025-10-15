#ifdef clox_val_h
#define clox_val_h

#include "common.h"

typedef double Val;
typedef struct {
    int cnt;
    int cap;
    Val* vals;
} ValArr;

void initValArr(ValArr* arr);
void writeValArr(ValArr* arr, Val val);
void freeValArr(ValArr* arr);
void printVal(Val val) {
    printf("%g", val);
}

#endif