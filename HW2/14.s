mov     eax, [0x600000]
mov     ecx, [0x600004]
neg     ecx
imul    ecx
xor     edx, edx
mov     ecx, [0x600008]
sub     ecx, ebx
cdq
idiv    ecx
mov     [0x600008], eax
done: