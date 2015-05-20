CCHOME ?= $(HOME)/opt/cross

CC = $(CCHOME)/bin/i386-elf-gcc
ASM = nasm -g
#ASM = yasm

#DEFINES = -DMAROX -DQEMU_DEBUG -g
DEFINES = -DMAROX -DQEMU_DEBUG
CFLAGS = $(DEFINES) --std=c11 -nostdlib -ffreestanding -finline-functions -O0
NFLAGS = -felf
LFLAGS = -lgcc

KERNDIR = kernel
OBJDIR = obj
ISODIR = isodir

# INCLUDES = -I$(KERNDIR)/ -I$(CCHOME)/lib/gcc/i386-elf/4.9.1/include
INCLUDES = -I$(KERNDIR)/

KERN_SRCS := $(wildcard $(KERNDIR)/*.c) $(wildcard $(KERNDIR)/*.h) $(wildcard $(KERNDIR)/*.asm)
KERN_OBJS := $(addprefix $(OBJDIR)/,\
	start.o main.o io.o gdt.o idt.o irq.o int.o bget.o mem.o \
	paging.o syscall.o thread.o \
	timer.o kb.o rtc.o screen.o string.o print.o util.o)

KERNEL = kernel.bin

QEMU = qemu-system-i386
QARGS = -m 32 -initrd modules/hello.bin -debugcon stdio
# QARGS = -s -S -m 32 -initrd modules/hello.bin -debugcon stdio # -d int,cpu_reset

.PHONY: all
all: $(KERNEL)

$(KERNEL): $(KERN_OBJS)
	$(CC) $(CFLAGS) -o $@ -T $(KERNDIR)/linker.ld $^ $(LFLAGS)

$(OBJDIR)/%.o: $(KERNDIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(OBJDIR)/%.o: $(KERNDIR)/%.s
	$(ASM) $(NFLAGS) $< -o $@

$(KERN_OBJS): | $(OBJDIR)
$(OBJDIR):
	test -d $(OBJDIR) || mkdir $(OBJDIR)

modules:
	$(MAKE) -C modules/

.PHONY: run
run: $(KERNEL) modules
	$(QEMU) $(QARGS) -kernel $<

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(KERNEL)

.PHONY: help
help:
	@echo "MaroxOS Make Help:"
	@echo "  all: make MaroxOS kernel"
	@echo "  run: run MaroxOS using QEMU"
	@echo "  clean: clean up build files"
