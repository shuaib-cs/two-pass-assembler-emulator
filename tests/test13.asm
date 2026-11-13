; test13.asm - signed mul and div with negative numbers.
; EXPECTED: assembles; at HALT register A = 21  (-6 * 7 = -42, then -42 / -2).
        ldc  -6
        ldc  7
        mul                 ; A = -42
        ldc  -2             ; B = -42, A = -2
        div                 ; A = -42 / -2 = 21
        HALT
