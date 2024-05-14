mov     eax, [0x600000]
mov     ebx, [0x600008]
mov     ecx, eax
mov     eax, ebx
mov     ebx, ecx
mov     [0x600000], eax
mov     [0x600008], ebx
done: