#include <stdint.h>
#include <stdbool.h>
#include <uart/uart.h>
#include <ili9341/ili9341.h>
#include <delay/delay.h>
#include <sdcard/sdcard.h>
#include <flash/flash.h>
#include <button/button.h>

#define reg_uart_clkdiv (*(volatile uint32_t*)0x02000004)
#define reg_buttons (*(volatile uint32_t*)0x03000004)

#define reg_uart_data (*(volatile uint32_t*)0x02000008)

#define USER_IMAGE 0x28000
#define USER_DATA  0x50000

#define MAX_GAMES 4

#define printf sdcard_error

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

char games[MAX_GAMES][9];
uint32_t first_cluster[MAX_GAMES];
uint32_t file_size[MAX_GAMES];

int num_games, game;

uint8_t buffer[512];
uint32_t cluster_begin_lba, first_file_cluster, first_file_size;
uint8_t sectors_per_cluster;
uint32_t fat[128];

void read_files() {
    uint8_t filename[13], first_file[13];
    filename[8] = '.';
    filename[12] = 0;
    
    for(int i=0; buffer[i];i+=32) {
        if (buffer[i] != 0xe5) {
            if (buffer[i+11] != 0x0f) {
                for(int j=0;j<8;j++) filename[j] = buffer[i+j];
                for(int j=0;j<3;j++) filename[9+j] = buffer[i+8+j];
                uint8_t attrib = buffer[i+0x0b];
                uint16_t first_cluster_hi = *((uint16_t *) &buffer[i+0x14]);
                uint16_t first_cluster_lo = *((uint16_t *) &buffer[i+0x1a]);
                uint32_t first_cluster = (first_cluster_hi << 16) + first_cluster_lo;
                uint32_t file_size = *((uint32_t *) &buffer[i+0x1c]);
                if ((attrib & 0x1f) == 0 && filename[9] == 'C') {
                    first_file_cluster = first_cluster;
                    for(int j=0;j<13;j++) first_file[j] = filename[j];
                    first_file_size = file_size;

#ifdef debug
                    //print(filename);
                    //print("\n");
#endif

                }
                if ((attrib & 0x1f) == 0 && num_games < MAX_GAMES) {
                  for(int j=0;j<8;j++) games[num_games][j] = filename[j];
                  games[num_games][8] = 0;
                  num_games++;
                }
            }
        }
    }

}

void copy_file() {
    uint32_t first_lba = cluster_begin_lba + ((first_file_cluster - 2) << 3);
    sdcard_read(buffer, first_lba);

#ifdef debug
    printf("\nFile first sector ", first_lba);

    for (int i = 0; i < 512; i++) {
        print_hex(buffer[i],2);
        if (i & 0x1f == 0x1f) print("\n");
        else if (i & 7 == 7) print(" ");
    }
    print("\n");
#endif

    // Write the file to user data area of flash
    flash_write_enable();
    flash_erase_32kB(USER_DATA);
    flash_wait();
  
    // Assumes file is less than 32kb
    uint32_t n = 0;
    for(uint32_t i =first_file_cluster;i < 128;i = fat[i]) {
        uint32_t lba = cluster_begin_lba + ((i-2) << 3);

#ifdef debug
        printf("Cluster: %ld, lba: %ld\n", i, lba);
#endif

        for(uint32_t j = 0; j<sectors_per_cluster;j++) {
            sdcard_read(buffer, lba+j);
            uint32_t len = ((first_file_size - n) < 256 ? (first_file_size - n) : 256);
            if (len == 0) break;
        
            flash_write_enable();
            flash_write(USER_DATA + n, buffer, len);
            flash_wait();

            // Read back the data and send it to the uart
            flash_read(USER_DATA + n, buffer, len);
            for(int k =0; k<len; k++)
              reg_uart_data = buffer[k];
            
            if (len < 256) break;
            n += 256;
            len = ((first_file_size - n) < 256 ? (first_file_size - n) : 256);
            if (len == 0) break;

            flash_write_enable();
            flash_write(USER_DATA + n, buffer + 256, len);
            flash_wait();

            // Read back the data and send it to the uart
            flash_read(USER_DATA + n, buffer + 256, len);
            for(int k =0; k<len; k++)
              reg_uart_data = buffer[256+k];
  
            n += 256;
        }
    }

}

void main() {
    reg_uart_clkdiv = 139;

    lcd_init();

    lcd_clear_screen(0x6E5D);

    lcd_draw_text(80,40,"Choose a game :", 0x00A0, 0x6E5D);    

    num_games = 0;

    sdcard_init();

    // Read the master boot record
    sdcard_read(buffer, 0);

#ifdef debug
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("MBR is valid.\n\n");
#endif

    uint8_t *part = &buffer[446];

#ifdef debug
    printf("Boot flag: %d\n", part[0]);
    printf("Type code: 0x%x\n", part[4]);
#endif

    uint16_t *lba_begin = (uint16_t *) &part[8];

    uint32_t part_lba_begin = lba_begin[0];

#ifdef debug
    printf("LBA begin: %ld\n", part_lba_begin);
#endif

    // Read the volume id
    sdcard_read(buffer, part_lba_begin);

#ifdef debug
    if (buffer[510] == 0x55 && buffer[511] == 0xAA) print("Volume ID is valid\n\n");
#endif

    uint16_t num_rsvd_sectors = *((uint16_t *) &buffer[0x0e]);

#ifdef debug
    printf("Number of reserved sectors: %d\n", num_rsvd_sectors);
#endif

    uint8_t num_fats = buffer[0x10];
    uint32_t sectors_per_fat = *((uint32_t *) &buffer[0x24]);
    sectors_per_cluster = buffer[0x0d];
    uint32_t root_dir_first_cluster = *((uint32_t *) &buffer[0x2c]);

#ifdef debug
    printf("Number of FATs: %d\n", num_fats);
    printf("Sectors per FAT: %ld\n", sectors_per_fat);
    printf("Sectors per cluster: %d\n", sectors_per_cluster);
    printf("Root dir first cluster: %ld\n", root_dir_first_cluster);
#endif

    uint32_t fat_begin_lba = part_lba_begin + num_rsvd_sectors;

#ifdef debug
    printf("fat begin lba: %ld\n", fat_begin_lba);
#endif

    // Assumes 2 FATs
    cluster_begin_lba = part_lba_begin + num_rsvd_sectors + (sectors_per_fat << 1);

#ifdef debug
    printf("cluster begin lba: %ld\n", cluster_begin_lba);
#endif

    // Assumes 8 sectors per cluster
    uint32_t root_dir_lba = cluster_begin_lba + ((root_dir_first_cluster - 2) << 3);

#ifdef debug
    printf("root dir lba: %ld\n", root_dir_lba);
#endif

    // Read the root directory
    sdcard_read(buffer, root_dir_lba);

    read_files();

    // Configure flash access
    reg_flash_prescale = 0;
    reg_flash_mode = 0;
    reg_flash_cs = 1;

    // power_up
    flash_begin();
    flash_xfer(0xab);
    flash_end();

    // read flash id
    flash_begin();
    flash_xfer(0x9f);

#ifdef debug
    print("flash id:");

    for (int i = 0; i < 3; i++)
        print_hex(flash_xfer(0x00), 2);
	
    print("\n");
#endif

    flash_end();

#ifdef debug
    printf("\nFirst file, first cluster: %ld\n", first_file_cluster);
#endif

    // Read first sector of the FAT
    sdcard_read((uint8_t *) fat, fat_begin_lba);

#ifdef debug
    for (int i = 0; i < 128; i++) {
       for(int j=0; j<4; j++) {
           print_hex(fat[i+ j*8],8);
           print(" ");
       }
       print("\n");
    }
      
    print("\n");
#endif

    copy_file();

    // power_down
    flash_begin();
    flash_xfer(0xb9);
    flash_end();

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
