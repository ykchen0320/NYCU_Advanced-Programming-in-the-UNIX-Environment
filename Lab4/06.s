mov     eax, [0x600000]
mov     ebx, [0x600004]
mov     ecx, [0x600008]
neg     eax
sub     ebx, ecx
add     eax, ebx
mov     [0x60000c], eax
done: