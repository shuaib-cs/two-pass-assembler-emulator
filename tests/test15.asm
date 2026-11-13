; test15.asm - spec sample 4: mul and div.
; EXPECTED: assembles; when emulated A = 21 at HALT.
ldc 6
ldc 7
mul ; A = 42
ldc 2
div ; A = 21
HALT