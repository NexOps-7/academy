#ifndef clox_obj_h
#define clox_obj_h

#include "common.h"
#include "val.h"
#include "chunk.h"

// valType union AS_OBJ -> type -> obj -> ObjStr
#define OBJ_TYPE(val)   (AS_OBJ(val)->type)
#define AS_FUNC(val)    (ObjFunc*)AS_OBJ(val)
#define AS_CLOSURE(val) (ObjClosure*)AS_OBJ(val)
// extract the val from NativeFn func
#define AS_NATIVE(val)  \
        (((ObjNative*)AS_OBJ(val))->func) \
#define AS_STR(val)     ((ObjStr*)AS_OBJ(val))
#define AS_CSTR(val)    (((ObjStr*)AS_OBJ(val))->chars)
// support dynamic typing
#define IS_FUNC(val)    isObjType(val, OBJ_FUNC)
#define IS_CLOSURE(val) isObjType(val, OBJ_CLOSURE)
#define IS_NATIVE(val)  isObjType(val, OBJ_NATIVE)
#define IS_STR(val)     isObjType(val, OBJ_STR)

typedef enum {
    OBJ_FUNC,
    OBJ_CLOSURE,
    OBJ_NATIVE,
    OBJ_STR,
} ObjType;

struct Obj {
    ObjType type;
    // linked list node, ptr to the Obj struct itself
    struct Obj* next;
};

typedef struct {
    Obj obj;
    // no. of parameter func expects
    int arity;
    // not a ptr anymore, its a struct
    Chunk chunk;
    ObjStr* name;
} ObjFunc;

typedef struct {
    Obj obj;
    ObjFunc* func;
} ObjClosure;

typedef Val (*NativeFn)(int argCnt, Val* args);

typedef struct {
    Obj obj;
    NativeFn func;
} ObjNative;

struct ObjStr {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

// inline: compiler integrate the code into caller
// reduce exec time
// vm takes val, not ptr (Obj* obj)
static inline bool isObjType(Val val, ObjType type) {
    return IS_OBJ(val) && AS_OBJ(val)->type == type;
}

ObjFunc* newFunc();
ObjClosure* newClosure(ObjFun* func);
ObjNative* newNative();
static ObjStr* allocStr(char* chars, int length, int hash);
int hashStr(const char* key, int length);
ObjStr* copyStr(const char* chars, int length);
ObjStr* takeStr(char* chars, int length);
void printObj(Val val);

#endif