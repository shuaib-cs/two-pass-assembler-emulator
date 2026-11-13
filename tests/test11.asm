; test11.asm - factorial of 5 with a loop (uses mul).
; EXPECTED: assembles; at HALT register A = 120.
        ldc  1
        ldc  res
        stnl 0              ; res = 1
        ldc  5
        ldc  n
        stnl 0              ; n = 5
loop:   ldc  n
        ldnl 0
        brz  done
        ldc  res
        ldnl 0              ; A = res
        ldc  n
        ldnl 0              ; A = n, B = res
        mul                 ; A = res * n
        ldc  res
        stnl 0              ; res = res * n
        ldc  n
        ldnl 0
        adc  -1
        ldc  n
        stnl 0              ; n = n - 1
        br   loop
done:   ldc  res
        ldnl 0
        HALT
res:    data 0
n:      data 0
