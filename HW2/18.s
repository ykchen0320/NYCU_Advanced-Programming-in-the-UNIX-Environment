mov     rax, 0
mov     rbx, 0
mov     rcx, 22
mov     rsi, 2
mov     rdi, 3
call    recur
jmp     fin
recur:
  cmp     rcx, 1
  jl      case0
  je      case1
  jg      case2
  ret
case2:
  push    rax
  xor     rax, rax
  push    rcx
  dec     rcx
  call    recur
  pop     rcx
  pop     rbx
  mul     rsi
  xor     rdx, rdx
  add     rax, rbx
  push    rax
  xor     rax, rax
  push    rcx
  sub     rcx, 2
  call    recur
  pop     rcx
  pop     rbx
  mul     rdi
  xor     rdx, rdx
  add     rax, rbx
  ret
case1:
  mov     rax, 1
  ret
case0:
  xor     rax, rax
  ret
fin:
done: