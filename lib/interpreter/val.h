#ifdef clox_val_h
#define clox_val_h

#include "common.h"

typedef double Val;
typedef struct {
    int cap;
    int cnt;
    Val* val;
} ValArray;

#endif