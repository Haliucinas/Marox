; start.s -- Kernel start location. Also defines multiboot header.

MBOOT_HEADER_MAGIC equ 0x1BADB002	; Multiboot Magic value
MBOOT_HEADER_FLAGS equ 0x3			; Load kernel and modules on a page boundary and provide your kernel with memory info
MBOOT_CHECKSUM equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

[BITS 32]							; All instructions should be 32-bit.

GLOBAL mboot 						; Make 'mboot' accessible from C.
EXTERN code 						; Start of the '.text' section.
EXTERN bss 							; Start of the .bss section.
EXTERN end 							; End of the last loadable section.

mboot:
	dd MBOOT_HEADER_MAGIC 			; GRUB will search for this value on each
									; 4-byte boundary in your kernel file
	dd MBOOT_HEADER_FLAGS			; How GRUB should load your file / settings
	dd MBOOT_CHECKSUM				; To ensure that the above values are correct

	dd mboot						; Location of this descriptor
	dd code							; Start of kernel '.text' (code) section.
	dd bss							; End of kernel '.data' section.
	dd end							; End of kernel.
	dd start						; Kernel entry point (initial EIP).

GLOBAL start 						; Kernel entry point.
EXTERN kmain 						; This is the entry point of our C code

start:
	push ebx 						; Load multiboot header location

	; Execute the kernel:
	cli 							; Disable interrupts.
	call kmain 						; call our kmain() function.
	jmp $ 							; Enter an infinite loop, to stop the processor
									; executing whatever rubbish is in the memory
									; after our kernel!