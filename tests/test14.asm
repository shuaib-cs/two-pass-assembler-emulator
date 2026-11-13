; test14.asm - procedure call and return, using the stack (adj, stl, ldl).
; 'call' leaves the argument in B and the return address in A.
; EXPECTED: assembles; at HALT register A = 42  (double(21)).
        ldc  21
        call double         ; B = 21 (argument), A = return address
        HALT
double: adj  -1             ; reserve one local word on the stack
        stl  0              ; save return address; A = argument
        ldc  2              ; B = argument, A = 2
        mul                 ; A = argument * 2
        ldl  0              ; B = result, A = return address
        adj  1              ; release the local word
        return              ; PC = return address, A = result
