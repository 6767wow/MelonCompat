PUBLIC rosev_mix64

.code
rosev_mix64 PROC
    mov rax, rcx
    xor rax, rdx
    rol rax, 13
    mov r8, 100000001b3h
    imul rax, r8
    ret
rosev_mix64 ENDP

END
