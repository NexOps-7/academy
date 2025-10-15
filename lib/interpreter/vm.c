#include "common.h"
#include "vm.h"

// global VM obj, instead of passing ptr to numerous func
// not the best practice
VM vm;

void initVM() {

}
void freeVM() {

}
static InterpretRes run() {
    
}
InterpretRes interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = vm.chunk->code;
    return run();
}