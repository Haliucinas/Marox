// multiboot.h -- Declares the multiboot info structure.

#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_CONFIG  0x080
#define MULTIBOOT_FLAG_LOADER  0x100
#define MULTIBOOT_FLAG_APM     0x200
#define MULTIBOOT_FLAG_VBE     0x400

struct multiboot {
    u32int flags;
    u32int memLower;
    u32int memUpper;
    u32int bootBevice;
    u32int cmdline;
    u32int modsCount;
    u32int modsAddr;
    u32int num;
    u32int size;
    u32int addr;
    u32int shndx;
    u32int mmapLength;
    u32int mmapAddr;
    u32int drivesLength;
    u32int drivesAddr;
    u32int configTable;
    u32int bootLoaderName;
    u32int apmTable;
    u32int vbeControlInfo;
    u32int vbeModeInfo;
    u32int vbeMode;
    u32int vbeInterfaceSeg;
    u32int vbeInterfaceOff;
    u32int vbeInterfaceLen;
}  __attribute__((packed));

#endif
