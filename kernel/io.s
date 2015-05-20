; vim:syntax=n__asm__

section .text
align 4

global inPortB
inPortB:
    mov     edx, dword [esp+4]
    in      al, dx
    ret

global outPortB
outPortB:
    mov     eax, dword [esp+8]
    mov     edx, dword [esp+4]
    out     dx, al
    ret

global inPortWrite
inPortWrite:
    mov     edx, dword [esp+4]
    in      ax, dx
    ret

global outPortWrite
outPortWrite:
    mov     eax, dword [esp+8]
    mov     edx, dword [esp+4]
    out     dx, ax
    ret

global inPortRead
inPortRead:
    mov     edx, dword [esp+4]
    in      eax, dx
    ret

global outPortRead
outPortRead:
    mov     eax, dword [esp+8]
    mov     edx, dword [esp+4]
    out     dx, eax
    ret

global delayIO
delayIO:
    xor     eax, eax
    out     byte 0x80, al
    ret
