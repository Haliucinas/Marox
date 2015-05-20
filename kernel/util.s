; vim:syntax=n__asm__

section .text
align 4

; return stack pointer (its value before calling this function)
global getESP
getESP:
    mov eax, esp    ; load ESP
    add eax, 4      ; add 4 bytes to jump over return address
    ret

; return contents of EFLAGS register
global getEFlags
getEFlags:
    pushfd      ; push eflags
    pop eax     ; pop contents into eax
    ret

; returns 0 or 3
global getRing
getRing:
    mov eax, cs
    and eax, 0x03
    ret

global khalt
khalt:
    cli     ; disable interrupts so the machine stays halted
    hlt     ; halt machine
    ret

; Attempt to triple-fault (doesn't really work)
global kreboot
kreboot:
    ;lidt no_idt
    lidt [no_idt]
    ret

section .data
align 4
;global  no_idt

no_idt:
    ;resw 2
    times 6 db 0
