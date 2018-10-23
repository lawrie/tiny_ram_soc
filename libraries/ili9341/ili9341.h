#ifndef __TINYSOC_ILI9341__
#define __TINYSOC_ILI9341__

#include <stdint.h>

#define reg_dc (*(volatile uint32_t*)0x05000008)
#define reg_rst (*(volatile uint32_t*)0x0500000c)
#define reg_xfer (*(volatile uint32_t*)0x05000000)
#define reg_fast_xfer (*(volatile uint32_t*)0x05000004)

#define WIDTH 320
#define HEIGHT 240

#define ILI9341_SOFTRESET          0x01
#define ILI9341_SLEEPIN            0x10
#define ILI9341_SLEEPOUT           0x11
#define ILI9341_NORMALDISP         0x13
#define ILI9341_INVERTOFF          0x20
#define ILI9341_INVERTON           0x21
#define ILI9341_GAMMASET           0x26
#define ILI9341_DISPLAYOFF         0x28
#define ILI9341_DISPLAYON          0x29
#define ILI9341_COLADDRSET         0x2A
#define ILI9341_PAGEADDRSET        0x2B
#define ILI9341_MEMORYWRITE        0x2C
#define ILI9341_PIXELFORMAT        0x3A
#define ILI9341_FRAMECONTROL       0xB1
#define ILI9341_DISPLAYFUNC        0xB6
#define ILI9341_ENTRYMODE          0xB7
#define ILI9341_POWERCONTROL1      0xC0
#define ILI9341_POWERCONTROL2      0xC1
#define ILI9341_VCOMCONTROL1       0xC5
#define ILI9341_VCOMCONTROL2       0xC7
#define ILI9341_MEMCONTROL         0x36
#define ILI9341_MADCTL             0x36

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

void lcd_send_cmd(uint8_t r);

void lcd_send_data(uint8_t d);

void lcd_reset(void);

void lcd_init(void);

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

void lcd_clear(uint16_t c, int s, int w);

void lcd_clear_screen(uint16_t c);

void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color);

void lcd_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bc);

void lcd_draw_text(int16_t x, int16_t y, const char *text, uint16_t c, uint16_t bc);

#endif
