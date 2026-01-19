DEBUG_MODE = 1

.if DEBUG_MODE
    print_str debug_msg  ; This only compiles if DEBUG_MODE is not 0
.endif
