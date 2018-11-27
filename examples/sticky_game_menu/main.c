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
#define reg_warm_boot (*(volatile uint32_t*)0x09000000)

#define reg_uart_data (*(volatile uint32_t*)0x02000008)

#define USER_IMAGE 0x28000
#define USER_DATA  0x50000
#define TOP_IMAGE  0x78000

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
uint32_t first_clusters[MAX_GAMES];
uint32_t file_sizes[MAX_GAMES];

int num_games;

uint8_t buffer[512];
uint32_t cluster_begin_lba;
uint8_t sectors_per_cluster;
uint32_t fat[128];
uint32_t fat_begin_lba;

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
                if ((attrib & 0x1f) == 0 && num_games < MAX_GAMES) {
                  for(int j=0;j<8;j++) games[num_games][j] = filename[j];
                  print(filename);
                  print(" ");
                  print_hex(first_cluster, 8);
                  print(" ");
                  print_hex(file_size, 8);
                  print("\n");
                  games[num_games][8] = 0;
                  first_clusters[num_games] = first_cluster;
                  file_sizes[num_games] = file_size;
                  num_games++;
                }
            }
        }
    }

}

void copy_file(uint32_t start, uint32_t first_cluster, uint32_t file_size) {
    uint32_t first_lba = cluster_begin_lba + ((first_cluster - 2) << 3);

#ifdef debug
    sdcard_read(buffer, first_lba);

    printf("\nFile first sector ", first_lba);

    for (int i = 0; i < 512; i++) {
        print_hex(buffer[i],2);
        if (i & 0x1f == 0x1f) print("\n");
        else if (i & 7 == 7) print(" ");
    }
    print("\n");
#endif

    uint32_t fat_sector = first_cluster >> 7;
    printf("First cluster ", first_cluster);
    printf("Fat Sector ", fat_sector);

    sdcard_read((uint8_t *) fat, fat_begin_lba + fat_sector);

    uint32_t n = 0;
    // Assumes no more han 8 clusters
    for(uint32_t i = first_cluster;i < 1024;i = fat[i & 0x7f]) {

        if ((i >> 7) != fat_sector) {
          fat_sector = i >> 7;
          printf("Reading fat sector ", fat_sector);
          sdcard_read((uint8_t *) fat, fat_begin_lba + fat_sector);
            
          for (int i = 0; i < 32; i++) {
            for(int j=0; j<4; j++) {
              print_hex(fat[i*4+ j],8);
              print(" ");
            }
            print("\n");
          }

          print("\n");
        }
        
        uint32_t lba = cluster_begin_lba + ((i -2) << 3);

#ifdef debug
        printf("Cluster: %ld, lba: %ld\n", i, lba);
#endif

        for(uint32_t j = 0; j<sectors_per_cluster;j++) {
            printf("Reading sector", lba+j);
            sdcard_read(buffer, lba+j);
            uint32_t len = ((file_size - n) < 256 ? (file_size - n) : 256);
            if (len == 0) break;
       
            if ((n & 0x7fff) == 0) {
                // Erase data in 32kb chunks
                flash_write_enable();
                flash_erase_32kB(start + n);
                flash_wait();
            } 

            flash_write_enable();
            flash_write(start + n, buffer, len);
            flash_wait();

            // Read back the data
            flash_read(start + n, buffer, len);
            //for(int k =0; k<len; k++)
            //  reg_uart_data = buffer[k];
            
            if (len < 256) break;
            n += 256;
            len = ((file_size - n) < 256 ? (file_size - n) : 256);
            if (len == 0) break;

            flash_write_enable();
            flash_write(start + n, buffer + 256, len);
            flash_wait();

            // Read back the data
            flash_read(start + n, buffer + 256, len);
            //for(int k =0; k<len; k++)
            //  reg_uart_data = buffer[256+k];
  
            n += 256;
        }
        if (n >= file_size) break;
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

    fat_begin_lba = part_lba_begin + num_rsvd_sectors;

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

    for (int i = 0; i < 32; i++) {
       for(int j=0; j<4; j++) {
           print_hex(fat[i*4+ j],8);
           print(" ");
       }
       print("\n");
    }
      
    print("\n");

    for(int i=0;i<num_games;i++)
      lcd_draw_text(92, 80 + i*20, games[i], 0xD0B7, 0x6E5D);

    lcd_draw_text(80, 80, "* ",  0xD0B7, 0x6E5D);

    int index = 0, old_index;
    uint8_t buttons = 0, old_buttons;

    while (1) {
      old_buttons = buttons;
      buttons = reg_buttons;

      old_index = index;

      if ((buttons & BUTTON_B) && !(old_buttons & BUTTON_B)) {
        if (++index == num_games) index = 0;
      } else if ((buttons & BUTTON_A) && !(old_buttons & BUTTON_A)) {

        printf("Copy file size ", file_sizes[index]);
        print("\n");
        
        copy_file(TOP_IMAGE, first_clusters[index], 135100);
        
        if (file_sizes[index] > 0x28000) {
          // Assumes file is in contiguous clusters
          copy_file(USER_DATA, first_clusters[index] + 40, file_sizes[index] - 0x28000);
        }

        // power_down
        flash_begin();
        flash_xfer(0xb9);
        flash_end();

        lcd_clear_screen(0x00);

        reg_warm_boot = 0; // Reboot

        break;
      }

      if (index != old_index) {
        lcd_draw_text(80, 80 + (old_index*20), "  ", 0xD0B7, 0x6E5D);
        lcd_draw_text(80, 80 + (index*20), "* ", 0xD0B7, 0x6E5D);
      }

      delay(5);
    } 
}
