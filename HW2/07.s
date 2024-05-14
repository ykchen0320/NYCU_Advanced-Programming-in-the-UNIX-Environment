shr     ax, 5
and     al, 0x7F
mov     [0x600000], al
done: