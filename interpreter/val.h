#ifdef clox_val_h
#define clox_val_h

#include "common.h"
#include "obj.h"

typedef struct Obj Obj;
typedef struct ObjStr ObjStr;

typedef enum {
    VAL_BOOL,
    VAL_NIL,
    VAL_NUM,
    VAL_OBJ
} ValType;

// union: save mem, same mem, only one val can be used at once
typedef struct {
    ValType type;
    union {
        bool boolean;
        double num;
        Obj* obj;
    } as;
} Val;

#define IS_BOOL(val) ((val).type == VAL_BOOL)
#define IS_NIL(val)  ((val).type == VAL_NIL)
#define IS_NUM(val)  ((val).type == VAL_NUM)
#define IS_OBJ(val)  ((val).type == VAL_OBJ)

#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUM(val)     ((val).as.num)
#define AS_OBJ(val)     ((val).as.obj)

#define BOOL_VAL(val)   ((Val){VAL_BOOL, {.boolean = val}})
#define NIL_VAL         ((Val){VAL_NIL, {.num = 0}})
#define NUM_VAL(val)    ((Val){VAL_NUM, {.num = val}})
#define OBJ_VAL(obj)    ((Val){VAL_OBJ, {.obj = (Obj*)obj}})

typedef double Val;
typedef struct {
    int cnt;
    int cap;
    Val* vals;
} ValArr;

void initValArr(ValArr* arr);
void freeValArr(ValArr* arr);
void writeValArr(ValArr* arr, Val val);

bool valuesEqual(Val a, Val b);
void printVal(Val val);

#endif