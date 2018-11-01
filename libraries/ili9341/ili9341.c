#include <ili9341/ili9341.h>
#include <ili9341/font.h>
#include <delay/delay.h>

void lcd_send_cmd(uint8_t r) {
        reg_dc = 0;
        reg_xfer = r;
}

void lcd_send_data(uint8_t d) {
        reg_dc = 1;
        reg_xfer = d;
}

void lcd_reset() {
        reg_rst = 0;
        delay(2);
        reg_rst = 1;
        for(int i=0;i<3;i++) reg_xfer = 0x00;
}

void lcd_init() {
        lcd_reset();

        delay(200);

        lcd_send_cmd(ILI9341_SOFTRESET);

        delay(50);

        lcd_send_cmd(ILI9341_DISPLAYOFF);

        lcd_send_cmd(ILI9341_POWERCONTROL1);
        lcd_send_data(0x23);

        lcd_send_cmd(ILI9341_POWERCONTROL2);
        lcd_send_data(0x10);

        lcd_send_cmd(ILI9341_VCOMCONTROL1);
        lcd_send_data(0x2B);
        lcd_send_data(0x2B);

        lcd_send_cmd(ILI9341_VCOMCONTROL2);
        lcd_send_data(0xC0);

        lcd_send_cmd(ILI9341_MEMCONTROL);
        lcd_send_data(ILI9341_MADCTL_MV | ILI9341_MADCTL_MY | ILI9341_MADCTL_MX | ILI9341_MADCTL_BGR);

        lcd_send_cmd(ILI9341_PIXELFORMAT);
        lcd_send_data(0x55);

        lcd_send_cmd(ILI9341_FRAMECONTROL);
        lcd_send_data(0x00);
        lcd_send_data(0x1B);

        lcd_send_cmd(ILI9341_ENTRYMODE);
        lcd_send_data(0x07);

        lcd_send_cmd(ILI9341_SLEEPOUT);

        delay(150);

        lcd_send_cmd(ILI9341_DISPLAYON);

        delay(500);
}

void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
        lcd_send_cmd(ILI9341_COLADDRSET);
        lcd_send_data(x1 >> 8);
        lcd_send_data(x1);
        lcd_send_data(x2 >> 8);
        lcd_send_data(x2);

        lcd_send_cmd(ILI9341_PAGEADDRSET);
        lcd_send_data(y1 >> 8);
        lcd_send_data(y1);
        lcd_send_data(y2 >> 8);
        lcd_send_data(y2);
}

void lcd_clear(uint16_t c, int s, int w) {
        lcd_set_window(s, 0, s+w-1, HEIGHT-1);
        lcd_send_cmd(ILI9341_MEMORYWRITE);
        uint8_t c1 = c >> 8;
        uint8_t c2 = c;

        reg_dc = 1;
        /*for(int i=0; i< HEIGHT*w; i++) {
                reg_xfer = c1;
                reg_xfer = c2;
        }*/
        reg_fast_xfer = ((HEIGHT*w) << 16) | c;
}

void lcd_clear_screen(uint16_t c) {
  for (int i=0;i<4;i++) lcd_clear(c, i*80, 80); // Full screen write sometimnes fails
}


void lcd_draw_pixel(int16_t x, int16_t y, uint16_t color) {
        if((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;

        lcd_set_window(x, y, WIDTH-1, HEIGHT-1);
        lcd_send_cmd(ILI9341_MEMORYWRITE);

        lcd_send_data(color >> 8);
        lcd_send_data(color);
}

void lcd_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bc) {
    if((x >= WIDTH)      || // Clip right
       (y >= HEIGHT)     || // Clip bottom
       ((x + 6 - 1) < 0) || // Clip left
       ((y + 8 - 1) < 0))   // Clip top
        return;

    for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
        uint8_t line = font[c * 5 + i];
        for(int8_t j=0; j<8; j++, line >>= 1)
            lcd_draw_pixel(x+i, y+j, line & 1 ? color : bc);
    }
}

void lcd_draw_text(int16_t x, int16_t y, const char *text, uint16_t c, uint16_t bc) {
    for(int i=0;text[i];i++) lcd_draw_char(x + i*6, y,text[i], c, bc);
}

