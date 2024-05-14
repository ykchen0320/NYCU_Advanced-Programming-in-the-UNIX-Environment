test    eax, eax
jns     positive1
mov     dword ptr [0x600000], -1
jmp     end1
positive1:
  mov     dword ptr [0x600000], 1
end1:
test    ebx, ebx
jns     positive2
mov     dword ptr [0x600004], -1
jmp     end2
positive2:
  mov     dword ptr [0x600004], 1
end2:
test    ecx, ecx
jns     positive3
mov     dword ptr [0x600008], -1
jmp     end3
positive3:
  mov     dword ptr [0x600008], 1
end3:
test    edx, edx
jns     positive4
mov     dword ptr [0x60000c], -1
jmp     end4
positive4:
  mov     dword ptr [0x60000c], 1
end4:
done: