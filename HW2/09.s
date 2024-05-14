mov     ecx, 0
mov     ebx, 0x00000020
loop1:
  cmp     ecx, 15
  jge     fin
  mov     eax, [0x600000 + ecx]
  or      eax, ebx
  mov     [0x600010 + ecx], eax
  inc     ecx
  jmp     loop1
fin:
done: