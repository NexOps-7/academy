#ifndef clox_obj_h
#define clox_obj_h

#include "common.h"
#include "val.h"

// valType union AS_OBJ -> type -> obj -> ObjStr
#define OBJ_TYPE(val)   (AS_OBJ(val)->type)
#define IS_STR(val)     isObjType(val, OBJ_STR)
#define AS_STR(val)     ((ObjStr*)AS_OBJ(val))
#define AS_CSTR(val)    (((ObjStr*)AS_OBJ(val))->chars)

typedef enum {
    OBJ_STR,
} ObjType;

struct Obj {
    ObjType type;
    // linked list node, the Obj struct itself
    struct Obj* next;
};

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
static ObjStr* allocStr(char* chars, int length, int hash);
int hashStr(const char* key, int length);
ObjStr* copyStr(const char* chars, int length);
ObjStr* takeStr(char* chars, int length);
void printObj(Val val);

#endif