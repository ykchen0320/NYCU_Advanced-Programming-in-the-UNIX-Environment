mov     eax, [0x600000]
mov     ebx, [0x600000]
mov     ecx, [0x600000]
shl     eax, 4
shl     ebx, 3
shl     ecx, 1
add     eax, ebx
add     eax, ecx
mov     [0x600004], eax
done: