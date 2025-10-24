data structures
    locals
        callframe -> stack index, short lived until return
    upval captured locals
        heap/obj arr -> ptr from closure, as long as closure refd
    globals
        table/dict -> name lookup, permanent
    strings
        constant pool/chunk -> constant id index, as long as chunk exists
garbage collection
    any resizing triggers
types
    objs func closures
    vals unions
        primitives 
        objs
    cast 
        type VAL_OBJ
        IS_OBJ IS_NIL
        val OBJ_VAL
        unions as AS_OBJ AS_NUM
vm stacks:
    func call state
        vm.stack vm.stackTop
    callFrame
        vm.frames[] vm.frameCnt
        ptr to func closure being called
    upval
        upval = vm.openupvals          
vm.frame->closure->upvalcnt
compilier
    compiler->closure->upval->isLocal index
    compiler->func