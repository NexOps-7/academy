data structures
    locals
        callframe -> stack index, short lived until return
    upval captured locals
        heap/obj arr -> ptr from closure, as long as closure refd
    globals
        table/dict -> name lookup, permanent
    strings
        func->chunk.constants.vals -> constant id index, as long as chunk exists
        ip = chunk.code
garbage collection
    any resizing triggers
types
    num 
    objs func closures frames
    vals 
        unions as
            primitives 
            objs
    boolean
    none
    cast 
        type VAL_OBJ VAL_NUM
        isType IS_OBJ IS_NIL
        val OBJ_VAL
        unions as AS_OBJ AS_NUM
vm stacks -> frame
    var  frame->slots[slot]
    upval *frame->closure->upvals[slot]-location
        define: 
            create var
            global x = 42, x -> constant table, val 42 -> push to stack, pop 42
        get: push byte val to stacktop
        set: 
            local: stacktop -> update the slot frame->slots[slot]
            global: update var
vm.stack vm.stackTop(sentinel and then stacktop)
    vals
        func call state
            stackTop slot 0 -> op
            constant table slot++
        callFrame
            vm.frames[] vm.frameCnt
            ptr to func closure being called
        upval
            upval = vm.openupvals 
vm: *objs frames[] frameCnt stack table **graystack
    frame: closure ip *slots
        Vm vm frame = frames[i] 
            vm.frame->ip
            closure: upvalcnt func *upvals
                vm.frame->closure->upvalcnt
                vm.frome->closure->func
compiler
    compiler->closure->upval->isLocal index
    compiler->func
table:
    obj bound menthod table
    obj instance fields table
    globals table
    strs/constant table
class:
    invoke
        if is_instance
            -> call val initializer 
                -> call frame closure ip slots
        else class
            -> method closure func

