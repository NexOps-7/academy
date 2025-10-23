locals
    callframe -> stack index, short lived until return
upval captured locals
    heap/obj arr -> ptr from closure, as long as closure refd
globals
    table/dict -> name lookup, permanent
strings
    constant pool/chunk -> constant id index, as long as chunk exists
obj
    ->next ptr points to next