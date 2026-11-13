; test12.asm - division by zero must be detected by the emulator.
; EXPECTED: assembles; the emulator stops with "RUNTIME ERROR: division by zero".
        ldc  10
        ldc  0
        div                 ; 10 / 0  -> run-time error
        HALT
