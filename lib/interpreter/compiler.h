#ifndef clox_compiler_h
#define clox_compiler_h
#include "vm.h"
#include "obj.h"

bool compile(const char* src, Chunk* chunk);

#endif