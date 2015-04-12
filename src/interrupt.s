; interrupt.s -- Contains interrupt service routine wrappers.

; This macro creates a stub for an ISR which does NOT pass it's own
; error code (adds a dummy errcode byte).
%macro ISR_NOERRCODE 1
	global isr%1
	isr%1:
		cli					; Disable interrupts firstly.
		push byte 0 		; Push a dummy error code.
		push byte %1 		; Push the interrupt number.
		jmp isrCommonStub	; Go to our common handler code.
%endmacro

; This macro creates a stub for an ISR which passes it's own
; error code.
%macro ISR_ERRCODE 1
	global isr%1
	isr%1:
		cli					; Disable interrupts.
		push byte %1 		; Push the interrupt number
		jmp isrCommonStub
%endmacro

; This macro creates a stub for an IRQ - the first parameter is
; the IRQ number, the second is the ISR number it is remapped to.
%macro IRQ 2
	global irq%1
	irq%1:
		cli
		push byte 0
		push byte %2
		jmp irqCommonStub
%endmacro

ISR_NOERRCODE 0
ISR_NOERRCODE 1
ISR_NOERRCODE 2
ISR_NOERRCODE 3
ISR_NOERRCODE 4
ISR_NOERRCODE 5
ISR_NOERRCODE 6
ISR_NOERRCODE 7
ISR_ERRCODE 8
ISR_NOERRCODE 9
ISR_ERRCODE 10
ISR_ERRCODE 11
ISR_ERRCODE 12
ISR_ERRCODE 13
ISR_ERRCODE 14
ISR_NOERRCODE 15
ISR_NOERRCODE 16
ISR_NOERRCODE 17
ISR_NOERRCODE 18
ISR_NOERRCODE 19
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; exceptional irq0
	global irq0
	irq0:
		cli
		push byte 0
		push byte 32
		jmp irq0Stub

IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47
 
; In isr.c
extern isrHandler
extern irqHandler
; In task.c
extern switchTask
extern newDirAddr

; This is our common ISR stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
isrCommonStub:
	pusha ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov ax, ds ; Lower 16-bits of eax = ds.
	push eax ; save the data segment descriptor

	mov ax, 0x10 ; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call isrHandler

	pop ebx ; reload the original data segment descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa ; Pops edi,esi,ebp...
	add esp, 8 ; Cleans up the pushed error code and pushed ISR number
	sti
	iret ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

; This is our common IRQ stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
irqCommonStub:
	pusha ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax

	mov ax, ds ; Lower 16-bits of eax = ds.
	push eax ; save the data segment descriptor

	mov ax, 0x10 ; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	call irqHandler

	pop ebx ; reload the original data segment descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa ; Pops edi,esi,ebp...
	add esp, 8 ; Cleans up the pushed error code and pushed ISR number
	sti
	iret ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP

irq0Stub:
	;Notice there is no IRQ number or error code - we don't need them

	pusha		  ;Push all standard registers

	mov ax, ds			   ; Lower 16-bits of eax = ds.
	push eax				 ; save the data segment descriptor

	mov ax, 0x10  ; load the kernel data segment descriptor
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax

	push esp	   ;Push pointer to old esp
	call switchTask ;Call C code

	mov esp, eax   ;Replace the stack with what the C code gave us. ebp is the same for all threads.
	mov eax, [newDirAddr]
	mov cr3, eax ; Install new directory

	mov al, 0x20   ;Port number AND command number to Acknowledge IRQ
	out 0x20, al	 ;Acknowledge IRQ, so we keep getting interrupts

	pop ebx		; reload the original data segment descriptor
	mov ds, bx
	mov es, bx
	mov fs, bx
	mov gs, bx

	popa		   ;Put the standard registers back
	add esp, 8
	sti
	iret		   ;Interrupt-Return