; test10.asm - mean of 5 numbers (10 20 30 40 50): sum them, then divide
;              by 5 with div.
; EXPECTED: assembles; at HALT register A = 30.
        ldc  a
        ldc  p
        stnl 0              ; p = address of a[0]
        ldc  5
        ldc  k
        stnl 0              ; k = 5 elements left
loop:   ldc  k
        ldnl 0
        brz  done
        ldc  sum
        ldnl 0              ; A = sum
        ldc  p
        ldnl 0              ; A = p, B = sum
        ldnl 0              ; A = *p, B = sum
        add                 ; A = sum + *p
        ldc  sum
        stnl 0              ; sum = sum + *p
        ldc  p
        ldnl 0
        adc  1
        ldc  p
        stnl 0              ; p = p + 1
        ldc  k
        ldnl 0
        adc  -1
        ldc  k
        stnl 0              ; k = k - 1
        br   loop
done:   ldc  sum
        ldnl 0              ; A = 150
        ldc  5              ; B = 150, A = 5
        div                 ; A = 150 / 5
        HALT
a:      data 10
        data 20
        data 30
        data 40
        data 50
sum:    data 0
p:      data 0
k:      data 0
