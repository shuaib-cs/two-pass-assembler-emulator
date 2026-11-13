; test09.asm - polynomial 3x^2 + 2x + 1 at x = 5 by Horner's rule:
;              ((3 * x) + 2) * x + 1.   Uses mul.
; EXPECTED: assembles; at HALT register A = 86.
        ldc  3
        ldc  x
        ldnl 0              ; A = x, B = 3
        mul                 ; A = 3x = 15
        adc  2              ; A = 17
        ldc  x
        ldnl 0              ; A = x, B = 17
        mul                 ; A = 17 * 5 = 85
        adc  1              ; A = 86
        HALT
x:      data 5
