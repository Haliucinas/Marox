# The C and C++ rules are already setup by default.

SRCFOLDER = src
SOURCES = start.o

CFLAGS =
LDFLAGS = -TLink.ld
ASFLAGS = -felf

all: $(SOURCES) link 

clean:
	-rm *.o kernel

link:
	ld $(LDFLAGS) -o kernel $(SRCFOLDER)/$(SOURCES)

.s.o:
	yasm $(ASFLAGS) $<