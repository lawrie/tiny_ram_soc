#ifndef __TINYSOC_BUTTON__
#define __TINYSOC_BUTTON__

#define reg_buttons (*(volatile uint32_t*)0x03000004)

#define BUTTON_UP 0x04
#define BUTTON_DOWN 0x10
#define BUTTON_LEFT 0x80
#define BUTTON_RIGHT 0x08
#define BUTTON_B 0x20
#define BUTTON_A 0x40

#endif

