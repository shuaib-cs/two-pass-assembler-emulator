; bubble.asm - bubble sort of 10 numbers, in place, ascending order.
; Written for the IITPSx instruction set (two-register stack A,B).
; EXPECTED: the 10 words at label 'arr' end up sorted:
; -7 5 11 12 22 22 25 34 64 90
; Every variable lives in memory, so each use is "ldc var / ldnl 0" and
; each store is "<value in A> / ldc var / stnl 0"  (stnl stores B at [A]).
;
; Algorithm:
;   pass = 9
;   while pass != 0:
;       k = pass ; p = &arr[0]
;       repeat k times: if p[0] > p[1] swap them ; p = p + 1
;       pass = pass - 1

        ldc  9
        ldc  pass
        stnl 0              ; pass = 9 (number of elements - 1)

outer:  ldc  pass
        ldnl 0              ; A = pass
        brz  done           ; pass == 0  ->  sorted, finish

        ldc  pass
        ldnl 0
        ldc  k
        stnl 0              ; k = pass  (comparisons left in this pass)
        ldc  arr
        ldc  p
        stnl 0              ; p = address of arr[0]

inner:  ldc  p
        ldnl 0              ; A = p
        ldnl 0              ; A = arr[j]     = p[0]
        ldc  x
        stnl 0              ; x = p[0]
        ldc  p
        ldnl 0              ; A = p
        ldnl 1              ; A = arr[j + 1] = p[1]
        ldc  y
        stnl 0              ; y = p[1]

        ldc  y
        ldnl 0              ; A = y
        ldc  x
        ldnl 0              ; A = x, B = y
        sub                 ; A = y - x
        brlz swap           ; y < x  ->  out of order, swap
        br   next

swap:   ldc  y
        ldnl 0              ; A = y
        ldc  p
        ldnl 0              ; A = p, B = y
        stnl 0              ; p[0] = y
        ldc  x
        ldnl 0              ; A = x
        ldc  p
        ldnl 0              ; A = p, B = x
        stnl 1              ; p[1] = x

next:   ldc  p
        ldnl 0
        adc  1
        ldc  p
        stnl 0              ; p = p + 1

        ldc  k
        ldnl 0
        adc  -1
        ldc  k
        stnl 0              ; k = k - 1
        ldc  k
        ldnl 0
        brz  endpass        ; k == 0  ->  this pass is finished
        br   inner

endpass: ldc pass
        ldnl 0
        adc  -1
        ldc  pass
        stnl 0              ; pass = pass - 1
        br   outer

done:   HALT

; ---- data ----
arr:    data 64
        data 34
        data 25
        data 12
        data 22
        data 11
        data 90
        data 5
        data -7
        data 22
pass:   data 0
k:      data 0
p:      data 0
x:      data 0
y:      data 0