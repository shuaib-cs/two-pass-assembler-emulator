; test08.asm - sum of the numbers 1..10 using a loop (adding n numbers).
; EXPECTED: assembles; at HALT register A = 55.
        ldc  10
        ldc  i
        stnl 0              ; i = 10
loop:   ldc  i
        ldnl 0
        brz  end            ; i == 0 -> finished
        ldc  sum
        ldnl 0              ; A = sum
        ldc  i
        ldnl 0              ; A = i, B = sum
        add                 ; A = sum + i
        ldc  sum
        stnl 0              ; sum = sum + i
        ldc  i
        ldnl 0
        adc  -1
        ldc  i
        stnl 0              ; i = i - 1
        br   loop
end:    ldc  sum
        ldnl 0              ; A = result
        HALT
sum:    data 0
i:      data 0
