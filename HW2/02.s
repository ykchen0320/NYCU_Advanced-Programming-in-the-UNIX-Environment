mov     ecx, 0
loop1:
  cmp     ecx, 10
  jge     fin
  mov     edx, 0
loop2:
  cmp     edx, 9
  jge     loop1_tail
  mov     eax, [0x600000 + edx * 4]
  mov     ebx, [0x600004 + edx * 4]
  cmp     eax, ebx
  jle     noswap
  cmp     eax, ebx
  jg      swap
  noswap:
  mov     [0x600000 + edx * 4], eax
  mov     [0x600004 + edx * 4], ebx
  jmp     loop2_tail
  swap:
  mov     [0x600000 + edx * 4], ebx
  mov     [0x600004 + edx * 4], eax
  jmp     loop2_tail
loop2_tail:
  inc     edx
  jmp     loop2
loop1_tail:
  inc     ecx
  jmp     loop1
fin:
done: