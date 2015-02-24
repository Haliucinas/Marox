# The C and C++ rules are already setup by default.
# TODO: find "smart" way to load src folder recursively

SOURCES = src/start.o \
			src/main.o \
			src/include/common.o \
			src/include/screen.o

CFLAGS = -nostdlib -nostdinc -fno-builtin -fno-stack-protector -std=c11 -m32
LDFLAGS = -TLink.ld -melf_i386
ASFLAGS = -felf

all: $(SOURCES) link 

link:
	ld $(LDFLAGS) -o kernel $(SOURCES)

.s.o:
	yasm $(ASFLAGS) $< -o $@

clean:
	-rm src/*.o kernel