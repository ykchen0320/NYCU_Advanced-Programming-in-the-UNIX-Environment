mov     ecx, 0
loop1:
  cmp     cx, 16
  jge     fin
  shl     ax, 1
  jc      one
  mov     byte ptr [0x600000 + ecx], '0'
  inc     ecx
  jmp     loop1
one:
  mov     byte ptr [0x600000 + ecx], '1'
  inc     ecx
  jmp     loop1
fin:
done: