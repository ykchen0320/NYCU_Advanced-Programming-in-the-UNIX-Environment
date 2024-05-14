mov     ah, 90
cmp     ch, ah
jg      low_to_high
add     ch, 32
jmp     fin
low_to_high:
sub     ch, 32
fin:
done: