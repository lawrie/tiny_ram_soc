#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>
#include "font.h"

// a pointer to this is a null pointer, but the compiler does not
// know that because "sram" is a linker symbol from sections.lds.
extern uint32_t sram;

#define reg_spictrl (*(volatile uint32_t*)0x02000000)
#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)
#define reg_leds (*(volatile uint32_t*)0x03000000)
#define reg_buttons (*(volatile uint32_t*)0x03000004)

#define reg_dc (*(volatile uint32_t*)0x05000008)
#define reg_rst (*(volatile uint32_t*)0x0500000c)
#define reg_xfer (*(volatile uint32_t*)0x05000000)
#define reg_fast_xfer (*(volatile uint32_t*)0x05000004)

#define reg_sdcard_cs (*(volatile uint32_t*)0x06000004)
#define reg_sdcard_xfer (*(volatile uint32_t*)0x06000008)
#define reg_sdcard_prescale (*(volatile uint32_t*)0x06000000)
#define reg_sdcard_mode (*(volatile uint32_t*)0x0600000c)

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

// Buttons
#define BUTTON_UP 0x04
#define BUTTON_DOWN 0x10
#define BUTTON_LEFT 0x80
#define BUTTON_RIGHT 0x08
#define BUTTON_B 0x20
#define BUTTON_A 0x40

uint32_t set_irq_mask(uint32_t mask); asm (
    ".global set_irq_mask\n"
    "set_irq_mask:\n"
    ".word 0x0605650b\n"
    "ret\n"
);

void irq_handler(uint32_t irqs, uint32_t* regs) { }

void delay(uint32_t n) {
  for (uint32_t i = 0; i < (n << 6); i++) asm volatile ("");
}

void send_cmd(uint8_t r) {
	reg_dc = 0;
	reg_xfer = r;
}

void send_data(uint8_t d) {
	reg_dc = 1;
	reg_xfer = d;
}

void reset() {
	reg_rst = 0;
        delay(2);
	reg_rst = 1;
        for(int i=0;i<3;i++) reg_xfer = 0x00;
}

void init() {
	reset();

        delay(200);

        send_cmd(ILI9341_SOFTRESET);

        delay(50);

        send_cmd(ILI9341_DISPLAYOFF);

	send_cmd(ILI9341_POWERCONTROL1);
	send_data(0x23);

	send_cmd(ILI9341_POWERCONTROL2);
	send_data(0x10);

	send_cmd(ILI9341_VCOMCONTROL1); 
	send_data(0x2B); 
	send_data(0x2B); 

	send_cmd(ILI9341_VCOMCONTROL2); 
	send_data(0xC0); 

	send_cmd(ILI9341_MEMCONTROL); 
	send_data(ILI9341_MADCTL_MV | ILI9341_MADCTL_BGR);

	send_cmd(ILI9341_PIXELFORMAT); 
	send_data(0x55);

	send_cmd(ILI9341_FRAMECONTROL); 
	send_data(0x00); 
	send_data(0x1B);
 
	send_cmd(ILI9341_ENTRYMODE); 
	send_data(0x07); 

        send_cmd(ILI9341_SLEEPOUT);

        delay(150);

	send_cmd(ILI9341_DISPLAYON); 

        delay(500);
}

void set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
        send_cmd(ILI9341_COLADDRSET);
        send_data(x1 >> 8);
        send_data(x1);
        send_data(x2 >> 8);
        send_data(x2);

        send_cmd(ILI9341_PAGEADDRSET);
        send_data(y1 >> 8);
        send_data(y1);
        send_data(y2 >> 8);
        send_data(y2);
}

void clear(uint16_t c, int s, int w) {
        set_window(s, 0, s+w-1, HEIGHT-1);
        send_cmd(ILI9341_MEMORYWRITE);
        uint8_t c1 = c >> 8;
        uint8_t c2 = c;

        reg_dc = 1;
        /*for(int i=0; i< HEIGHT*w; i++) {
        	reg_xfer = c1;
        	reg_xfer = c2;
	}*/
        reg_fast_xfer = ((HEIGHT*w) << 16) | c;
}

void clearScreen(uint16_t c) {
  for (int i=0;i<4;i++) clear(c, i*80, 80); // Full screen write sometimnes fails
}

void drawPixel(int16_t x, int16_t y, uint16_t color) {
        if((x < 0) || (y < 0) || (x >= WIDTH) || (y >= HEIGHT)) return;
        
	set_window(x, y, WIDTH-1, HEIGHT-1);	
	send_cmd(ILI9341_MEMORYWRITE);

	send_data(color >> 8);
	send_data(color);
}

void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bc) {
    if((x >= WIDTH)      || // Clip right
       (y >= HEIGHT)     || // Clip bottom
       ((x + 6 - 1) < 0) || // Clip left
       ((y + 8 - 1) < 0))   // Clip top
        return;

    for(int8_t i=0; i<5; i++ ) { // Char bitmap = 5 columns
        uint8_t line = font[c * 5 + i];
        for(int8_t j=0; j<8; j++, line >>= 1) 
            drawPixel(x+i, y+j, line & 1 ? color : bc);
    }
}

void drawText(int16_t x, int16_t y, const char *text, uint16_t c, uint16_t bc) {
    for(int i=0;text[i];i++) drawChar(x + i*6, y,text[i], c, bc);
}

bool sdcard_ccs_mode;

static void sdcard_cs(bool enable)
{
  reg_sdcard_cs = (enable ? 0 : 1);
}

static uint8_t sdcard_xfer(uint8_t value)
{
        reg_sdcard_xfer = value;
        //print("Reading xfer result\n");
	uint8_t ret = reg_sdcard_xfer;
        //print("Result is ");
        //print_hex(ret, 2);
        //print("\n");
	return ret;
}

static const uint8_t sdcard_crc7_table[256] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e,
	0x90, 0x82, 0xb4, 0xa6, 0xd8, 0xca, 0xfc, 0xee,
	0x32, 0x20, 0x16, 0x04, 0x7a, 0x68, 0x5e, 0x4c,
	0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc,
	0x64, 0x76, 0x40, 0x52, 0x2c, 0x3e, 0x08, 0x1a,
	0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a,
	0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28,
	0xc6, 0xd4, 0xe2, 0xf0, 0x8e, 0x9c, 0xaa, 0xb8,
	0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6,
	0x58, 0x4a, 0x7c, 0x6e, 0x10, 0x02, 0x34, 0x26,
	0xfa, 0xe8, 0xde, 0xcc, 0xb2, 0xa0, 0x96, 0x84,
	0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06, 0x14,
	0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2,
	0x3c, 0x2e, 0x18, 0x0a, 0x74, 0x66, 0x50, 0x42,
	0x9e, 0x8c, 0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0,
	0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70,
	0x82, 0x90, 0xa6, 0xb4, 0xca, 0xd8, 0xee, 0xfc,
	0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c,
	0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce,
	0x20, 0x32, 0x04, 0x16, 0x68, 0x7a, 0x4c, 0x5e,
	0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98,
	0x76, 0x64, 0x52, 0x40, 0x3e, 0x2c, 0x1a, 0x08,
	0xd4, 0xc6, 0xf0, 0xe2, 0x9c, 0x8e, 0xb8, 0xaa,
	0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e, 0x28, 0x3a,
	0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34,
	0xda, 0xc8, 0xfe, 0xec, 0x92, 0x80, 0xb6, 0xa4,
	0x78, 0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06,
	0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96,
	0x2e, 0x3c, 0x0a, 0x18, 0x66, 0x74, 0x42, 0x50,
	0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0,
	0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70, 0x62,
	0x8c, 0x9e, 0xa8, 0xba, 0xc4, 0xd6, 0xe0, 0xf2
};

static uint8_t sdcard_crc7(uint8_t crc, uint8_t data)
{
	return sdcard_crc7_table[crc ^ data];
}

static uint16_t sdcard_crc16(uint16_t crc, uint8_t data)
{
	uint16_t x = (crc >> 8) ^ data;
	x ^= x >> 4;
	return (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
}

static uint8_t sdcard_cmd_r1(uint8_t cmd, uint32_t arg)
{
	uint8_t r1;

	sdcard_cs(true);

	sdcard_xfer(0x40 | cmd);
	sdcard_xfer(arg >> 24);
	sdcard_xfer(arg >> 16);
	sdcard_xfer(arg >> 8);
	sdcard_xfer(arg);

	uint8_t crc = 0;
	crc = sdcard_crc7(crc, 0x40 | cmd);
	crc = sdcard_crc7(crc, arg >> 24);
	crc = sdcard_crc7(crc, arg >> 16);
	crc = sdcard_crc7(crc, arg >> 8);
	crc = sdcard_crc7(crc, arg);
	sdcard_xfer(crc | 1);

	do {
		r1 = sdcard_xfer(0xff);
	} while (r1 == 0xff);

	sdcard_cs(false);
	return r1;
}

static uint8_t sdcard_cmd_rw(uint8_t cmd, uint32_t arg)
{
	uint8_t r1;

	sdcard_cs(true);

	sdcard_xfer(0x40 | cmd);
	sdcard_xfer(arg >> 24);
	sdcard_xfer(arg >> 16);
	sdcard_xfer(arg >> 8);
	sdcard_xfer(arg);

	uint8_t crc = 0;
	crc = sdcard_crc7(crc, 0x40 | cmd);
	crc = sdcard_crc7(crc, arg >> 24);
	crc = sdcard_crc7(crc, arg >> 16);
	crc = sdcard_crc7(crc, arg >> 8);
	crc = sdcard_crc7(crc, arg);
	sdcard_xfer(crc | 1);

	do {
		r1 = sdcard_xfer(0xff);
	} while (r1 == 0xff);

	return r1;
}

static uint8_t sdcard_cmd_r37(uint8_t cmd, uint32_t arg, uint32_t *r37)
{
	uint8_t r1;

	sdcard_cs(true);

	sdcard_xfer(0x40 | cmd);
	sdcard_xfer(arg >> 24);
	sdcard_xfer(arg >> 16);
	sdcard_xfer(arg >> 8);
	sdcard_xfer(arg);

	uint8_t crc = 0;
	crc = sdcard_crc7(crc, 0x40 | cmd);
	crc = sdcard_crc7(crc, arg >> 24);
	crc = sdcard_crc7(crc, arg >> 16);
	crc = sdcard_crc7(crc, arg >> 8);
	crc = sdcard_crc7(crc, arg);
	sdcard_xfer(crc | 1);

	do {
		r1 = sdcard_xfer(0xff);
	} while (r1 == 0xff);

	for (int i = 0; i < 4; i++)
		*r37 = (*r37 << 8) | sdcard_xfer(0xff);

	sdcard_cs(false);
	return r1;
}


void printf(char* msg, uint32_t r) {
  print(msg);
  print_hex(r, 8);
  print("\n");
}

void printf2(char* msg, uint8_t r, int r2) {
  print(msg);
  print_hex(r,2);
  print("\n");
  print_hex(r2, 8);
  print("\n");
}

static void sdcard_init()
{
	uint8_t r1;
	uint32_t r37;

	sdcard_cs(false);
        reg_sdcard_prescale = 32;

        reg_sdcard_mode = 0;	

	for (int i = 0; i < 10; i++)
		sdcard_xfer(0xff);

	r1 = sdcard_cmd_r1(0, 0);

	if (r1 != 0x01) {
		printf("Unexpected SD Card CMD0 R1: %02x\n", r1);
		while (1) { }
	}

	r1 = sdcard_cmd_r1(59, 1);

	if (r1 != 0x01) {
		printf("Unexpected SD Card CMD59 R1: %02x\n", r1);
		while (1) { }
	}

	r1 = sdcard_cmd_r37(8, 0x1ab, &r37);
	if (r1 != 0x01 || (r37 & 0xfff) != 0x1ab) {
		printf2("Unexpected SD Card CMD8 R1 / R7: %02x %08x\n", r1, (int)r37);
		while (1) { }
	}

	r1 = sdcard_cmd_r37(58, 0, &r37);

	if (r1 != 0x01) {
		printf("Unexpected SD Card CMD58 R1: %02x\n", r1);
		while (1) { }
	}

	if ((r37 & 0x00300000) == 0) {
		printf("SD Card doesn't support 3.3V! OCR reg: %08x\n", (int)r37);
		while (1) { }
	}

	for (int i = 0;; i++)
	{
		// ACMD41, set HCS
		sdcard_cmd_r1(55, 0);

		r1 = sdcard_cmd_r1(41, 1 << 30);

		if (r1 == 0x00)
			break;

		if (r1 != 0x01 && r1 != 0xff) {
			printf("Unexpected SD Card ACMD41 R1: %02x\n", r1);
			while (1) { }
		}

		if (i == 10000) {
			print("Timeout on SD Card ACMD41.\n");
			while (1) { }
		}
	}

	r1 = sdcard_cmd_r37(58, 0, &r37);

	if (r1 != 0x00) {
		printf("Unexpected SD Card CMD58 R1: %02x\n", r1);
		while (1) { }
	}

	sdcard_ccs_mode = !!(r37 & (1 << 30));

	r1 = sdcard_cmd_r1(16, 512);

	if (r1 != 0x00) {
		printf("Unexpected SD Card CMD16 R1: %02x\n", r1);
		while (1) { }
	}
}

static void sdcard_read(uint8_t *data, uint32_t blockaddr)
{
	if (!sdcard_ccs_mode)
		blockaddr <<= 9;

	uint8_t r1 = sdcard_cmd_rw(17, blockaddr);

	if (r1 != 0x00) {
		printf("Unexpected SD Card CMD17 R1: %02x\n", r1);
		while (1) { }
	}

	while (1) {
		r1 = sdcard_xfer(0xff);
		if (r1 == 0xfe) break;
		if (r1 == 0xff) continue;
		printf("Unexpected SD Card CMD17 data token: %02x\n", r1);
		while (1) { }
	}

	uint16_t crc = 0x0;
	for (int i = 0; i < 512; i++) {
		data[i] = sdcard_xfer(0xff);
		crc = sdcard_crc16(crc, data[i]);
	}

	crc = sdcard_crc16(crc, sdcard_xfer(0xff));
	crc = sdcard_crc16(crc, sdcard_xfer(0xff));

	if (crc != 0) {
		print("CRC Error while reading from SD Card!\n");
		while (1) { }
	}

	sdcard_cs(false);
}

static void sdcard_write(const uint8_t *data, uint32_t blockaddr)
{
	if (!sdcard_ccs_mode)
		blockaddr <<= 9;

	uint8_t r1 = sdcard_cmd_rw(24, blockaddr);

	if (r1 != 0x00) {
		printf("Unexpected SD Card CMD24 R1: %02x\n", r1);
		while (1) { }
	}

	sdcard_xfer(0xff);
	sdcard_xfer(0xfe);

	uint16_t crc = 0x0;
	for (int i = 0; i < 512; i++) {
		crc = sdcard_crc16(crc, data[i]);
		sdcard_xfer(data[i]);
	}

	sdcard_xfer(crc >> 8);
	sdcard_xfer(crc);

	r1 = sdcard_xfer(0xff);
	if ((r1 & 0x0f) != 0x05) {
		printf("Unexpected SD Card CMD24 data response: %02x\n", r1);
		while (1) { }
	}

	while (sdcard_xfer(0xff) != 0xff) {
		/* waiting for sd card */
	}

	sdcard_cs(false);
}

#define MAX_GAMES 4

char games[MAX_GAMES][9];
int num_games;

void main() {
    reg_uart_clkdiv = 139;

    set_irq_mask(0xff);

    // switch to dual IO mode
    reg_spictrl = (reg_spictrl & ~0x007F0000) | 0x00400000;

    init();

    clearScreen(0x6E5D);

    drawText(80,40,"Choose a game :", 0x00A0, 0x6E5D);

    num_games = 0;

    print("Sd card init\n");
    
    sdcard_init();
    print("SD Card Initialized.\n\n");

    print("Master Boot Record:\n\n");

    uint8_t buffer[512];

    sdcard_read(buffer, 0);

    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("MBR is valid.\n\n");

    uint8_t *part = &buffer[446];
    printf("Boot flag: %d\n", part[0]);
    printf("Type code: 0x%x\n", part[4]);

    uint16_t *lba_begin = (uint16_t *) &part[8];

    uint32_t Partition_LBA_Begin = lba_begin[0];

    printf("LBA begin: %ld\n", Partition_LBA_Begin);

    print("\nVolume ID:\n\n");
	
    sdcard_read(buffer, Partition_LBA_Begin);

    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("Volume ID is valid\n\n");

    uint16_t Number_of_Reserved_Sectors = *((uint16_t *) &buffer[0x0e]);
    printf("Number of reserved sectors: %d\n", Number_of_Reserved_Sectors);

    uint8_t Number_of_FATs = buffer[0x10];
    uint32_t Sectors_Per_FAT = *((uint32_t *) &buffer[0x24]);
    uint8_t sectors_per_cluster = buffer[0x0d];
    uint32_t root_dir_first_cluster = *((uint32_t *) &buffer[0x2c]);

    printf("Number of FATs: %d\n", Number_of_FATs);
    printf("Sectors per FAT: %ld\n", Sectors_Per_FAT);
    printf("Sectors per cluster: %d\n", sectors_per_cluster);
    printf("Root dir first cluster: %ld\n", root_dir_first_cluster);
 
    uint32_t fat_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors;
    printf("fat begin lba: %ld\n", fat_begin_lba);

    uint32_t cluster_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors + (Sectors_Per_FAT << 1);
    printf("cluster begin lba: %ld\n", cluster_begin_lba);

    uint32_t root_dir_lba = cluster_begin_lba + ((root_dir_first_cluster - 2) << 3);
    printf("root dir lba: %ld\n", root_dir_lba);

    sdcard_read(buffer, root_dir_lba);

    print("\nRoot directory first sector:\n\n");

    uint8_t filename[13], first_file[13];
    filename[8] = '.';
    filename[12] = 0;
    print("Root directory:\n\n");
    uint16_t first_cluster_lo, first_cluster_hi;
    uint32_t first_cluster, file_size, first_file_cluster = 0;
    uint8_t attrib;

    print("Files:\n");
    for(int i=0; buffer[i];i+=32) {
        print_hex(buffer[i],2);
        print("\n");
        if (buffer[i] != 0xe5) {
            if (buffer[i+11] != 0x0f) {
                for(int j=0;j<8;j++) filename[j] = buffer[i+j];
                for(int j=0;j<3;j++) filename[9+j] = buffer[i+8+j];
                attrib = buffer[i+0x0b];
                first_cluster_hi = *((uint16_t *) &buffer[i+0x14]);
                first_cluster_lo = *((uint16_t *) &buffer[i+0x1a]);
                first_cluster = (first_cluster_hi << 16) + first_cluster_lo;
                file_size = *((uint32_t *) &buffer[i+0x1c]);
                if ((attrib & 0x1f) == 0) {
                    first_file_cluster = first_cluster;
                    for(int j=0;j<13;j++) first_file[j] = filename[j];
                }
                print(filename);
                print("\n");
                if ((attrib & 0x1f) == 0 && num_games < MAX_GAMES) {
                  for(int j=0;j<8;j++) games[num_games][j] = filename[j];
                  games[num_games][8] = 0;
                  num_games++;
                }
            }
        }
    } 

    for(int i=0;i<num_games;i++) 
      drawText(92, 80 + i*20, games[i], 0xD0B7, 0x6E5D);

    drawText(80, 80, "* ",  0xD0B7, 0x6E5D);

    int index = 0, old_index;
    uint8_t buttons = 0, old_buttons;

    while (1) {
      old_buttons = buttons;
      buttons = reg_buttons;
      
      old_index = index;

      if ((buttons & BUTTON_B))  
        if (++index == num_games) index = 0;
  
      if (index != old_index) {
        drawText(80, 80 + (old_index*20), "  ", 0xD0B7, 0x6E5D);
        drawText(80, 80 + (index*20), "* ", 0xD0B7, 0x6E5D);
      }

      delay(5);
    } 
}
