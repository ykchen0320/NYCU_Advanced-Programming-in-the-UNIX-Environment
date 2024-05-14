mov     eax, [0x600000]
mov     ebx, -5
imul    ebx
xor     edx, edx
mov     ecx, eax
mov     eax, [0x600004]
mov     ebx, [0x600008]
neg     eax
cdq
idiv    ebx
mov     eax, ecx
mov     ebx, edx
xor     edx, edx
cdq
idiv    ebx
mov     [0x60000c], eax
done: