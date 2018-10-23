#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>
#include <ili9341/ili9341.h>
#include <delay/delay.h>
#include <sdcard/sdcard.h>

#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)
#define reg_leds (*(volatile uint32_t*)0x03000000)
#define reg_buttons (*(volatile uint32_t*)0x03000004)

#define reg_flash_prescale (*(volatile uint32_t*)0x08000000)
#define reg_flash_cs (*(volatile uint32_t*)0x08000004)
#define reg_flash_xfer (*(volatile uint32_t*)0x08000008)
#define reg_flash_mode (*(volatile uint32_t*)0x0800000C)

#define BUTTON_UP 0x04
#define BUTTON_DOWN 0x10
#define BUTTON_LEFT 0x80
#define BUTTON_RIGHT 0x08
#define BUTTON_B 0x20
#define BUTTON_A 0x40

#define USER_IMAGE 0x28000
#define USER_DATA 0x50000

void sdcard_error(char* msg, uint32_t r) {
  print(msg);
  print(" : ");
  print_hex(r, 8);
  print("\n");
}

void sdcard_error2(char* msg, uint32_t r1, uint32_t r2) {
  print(msg);
  print(" : ");
  print_hex(r1, 8);
  print(" , ");
  print_hex(r2, 8);
  print("\n");
}

void flash_begin() {
  reg_flash_cs = 0;
}

void flash_end() {
  reg_flash_cs = 1;
}

uint8_t flash_xfer(uint8_t d) {
  reg_flash_xfer = d;
  return reg_flash_xfer;
}

void flash_write_enable() {
	flash_begin();
	flash_xfer(0x06);
	flash_end();
}

void flash_bulk_erase() {
	flash_begin();
	flash_xfer(0xc7);
	flash_end();
}

void flash_erase_64kB(int addr) {
	flash_begin();
	flash_xfer(0xd8);
	flash_xfer(addr >> 16);
	flash_xfer(addr >> 8);
	flash_xfer(addr);
	flash_end();
}

void flash_erase_32kB(int addr) {
	flash_begin();
	flash_xfer(0x52);
	flash_xfer(addr >> 16);
	flash_xfer(addr >> 8);
	flash_xfer(addr);
	flash_end();
}

void flash_write(int addr, uint8_t *data, int n) {
	flash_begin();
	flash_xfer(0x02);
	flash_xfer(addr >> 16);
	flash_xfer(addr >> 8);
	flash_xfer(addr);
	while (n--)
		flash_xfer(*(data++));
	flash_end();
}

void flash_read(int addr, uint8_t *data, int n) {
	flash_begin();
	flash_xfer(0x03);
	flash_xfer(addr >> 16);
	flash_xfer(addr >> 8);
	flash_xfer(addr);
	while (n--)
		*(data++) = flash_xfer(0);
	flash_end();
}

void flash_wait() {
	while (1)
	{
		flash_begin();
		flash_xfer(0x05);
		int status = flash_xfer(0);
		flash_end();

		if ((status & 0x01) == 0)
			break;

		delay(1);
	}
}

#define MAX_GAMES 4

#define printf sdcard_error

char games[MAX_GAMES][9];
int num_games;

void main() {
    reg_uart_clkdiv = 139;

    lcd_init();

    lcd_clear_screen(0x6E5D);

    lcd_draw_text(80,40,"Choose a game :", 0x00A0, 0x6E5D);    

    num_games = 0;

    // Read the flash id

    reg_flash_prescale = 4;
    reg_flash_mode = 0;
    reg_flash_cs = 1;

    // power_up
    flash_begin();
    flash_xfer(0xab);
    flash_end();

    // read flash id
    flash_begin();
    flash_xfer(0x9f);
    print("flash id:");

    for (int i = 0; i < 20; i++)
        print_hex(flash_xfer(0x00), 2);
	
    print("\n");
    flash_end();

    sdcard_init();
    //print("SD Card Initialized.\n\n");

    //print("Master Boot Record:\n\n");

    uint8_t buffer[512];

    sdcard_read(buffer, 0);

    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("MBR is valid.\n\n");

    uint8_t *part = &buffer[446];
    //printf("Boot flag: %d\n", part[0]);
    //printf("Type code: 0x%x\n", part[4]);

    uint16_t *lba_begin = (uint16_t *) &part[8];

    uint32_t Partition_LBA_Begin = lba_begin[0];

    //printf("LBA begin: %ld\n", Partition_LBA_Begin);

    //print("\nVolume ID:\n\n");

    sdcard_read(buffer, Partition_LBA_Begin);

    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("Volume ID is valid\n\n");

    uint16_t Number_of_Reserved_Sectors = *((uint16_t *) &buffer[0x0e]);
    //printf("Number of reserved sectors: %d\n", Number_of_Reserved_Sectors);

    uint8_t Number_of_FATs = buffer[0x10];
    uint32_t Sectors_Per_FAT = *((uint32_t *) &buffer[0x24]);
    uint8_t sectors_per_cluster = buffer[0x0d];
    uint32_t root_dir_first_cluster = *((uint32_t *) &buffer[0x2c]);

    //printf("Number of FATs: %d\n", Number_of_FATs);
    //printf("Sectors per FAT: %ld\n", Sectors_Per_FAT);
    //printf("Sectors per cluster: %d\n", sectors_per_cluster);
    //printf("Root dir first cluster: %ld\n", root_dir_first_cluster);

    uint32_t fat_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors;
    //printf("fat begin lba: %ld\n", fat_begin_lba);

    // Assumes 2 FATs
    uint32_t cluster_begin_lba = Partition_LBA_Begin + Number_of_Reserved_Sectors + (Sectors_Per_FAT << 1);
    //printf("cluster begin lba: %ld\n", cluster_begin_lba);

    // Assumes 8 sectors per cluster
    uint32_t root_dir_lba = cluster_begin_lba + ((root_dir_first_cluster - 2) << 3);
    //printf("root dir lba: %ld\n", root_dir_lba);

    sdcard_read(buffer, root_dir_lba);

    //print("\nRoot directory first sector:\n\n");

    uint8_t filename[13], first_file[13];
    filename[8] = '.';
    filename[12] = 0;
    //print("Root directory:\n\n");
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
      lcd_draw_text(92, 80 + i*20, games[i], 0xD0B7, 0x6E5D);

    lcd_draw_text(80, 80, "* ",  0xD0B7, 0x6E5D);

    int index = 0, old_index;
    uint8_t buttons = 0, old_buttons;

    while (1) {
      old_buttons = buttons;
      buttons = reg_buttons;

      old_index = index;

      if ((buttons & BUTTON_B))
        if (++index == num_games) index = 0;

      if (index != old_index) {
        lcd_draw_text(80, 80 + (old_index*20), "  ", 0xD0B7, 0x6E5D);
        lcd_draw_text(80, 80 + (index*20), "* ", 0xD0B7, 0x6E5D);
      }

      delay(5);
    } 
}
