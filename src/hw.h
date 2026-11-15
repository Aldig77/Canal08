#ifndef HW_H
#define HW_H
typedef unsigned char u8; typedef signed char s8;
typedef unsigned short u16; typedef short s16;
typedef unsigned int u32; typedef int s32;

#ifdef HOST
extern u8 host_io[0x400]; extern u8 host_pal[0x400]; extern u8 host_vram[0x18000];
#define IO16(o) (*(volatile u16*)(host_io+(o)))
#define PAL  ((volatile u16*)host_pal)
#define VRAM ((volatile u32*)host_vram)
#define FAST
#define SBSS
#else
#define IO16(o) (*(volatile u16*)(0x04000000+(o)))
#define PAL  ((volatile u16*)0x05000000)
#define VRAM ((volatile u32*)0x06000000)
#define FAST __attribute__((section(".iwram"),target("arm"),long_call))
#define SBSS __attribute__((section(".sbss")))
#endif

#define REG_DISPCNT IO16(0x00)
#define REG_VCOUNT  IO16(0x06)
#define REG_KEYINPUT IO16(0x130)
#endif
