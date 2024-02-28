#include <stdio.h>
#include <stdlib.h>

const char *FILENAME = "./data/fat16_4sectorpercluster.img";

typedef struct fat_BS {
  unsigned char bootjmp[3];
  unsigned char oem_name[8];
  unsigned short bytes_per_sector;
  unsigned char sectors_per_cluster;
  unsigned short reserved_sector_count;
  unsigned char table_count;
  unsigned short root_entry_count;
  unsigned short total_sectors_16;
  unsigned char media_type;
  unsigned short table_size_16;
  unsigned short sectors_per_track;
  unsigned short head_side_count;
  unsigned int hidden_sector_count;
  unsigned int total_sectors_32;

  // this will be cast to it's specific type once the driver actually knows what
  // type of FAT this is.
  unsigned char extended_section[54];

} __attribute__((packed)) fat_BS_t;

typedef struct root_directory_file {
  unsigned char file_name[11];
  unsigned char file_type;
  unsigned char windows_NT;
  unsigned char creation_time;
  unsigned char file_time[2];
  unsigned char date_file[2];
  unsigned char last_access_time[2];
  unsigned char high_16_bits[2];
  unsigned char last_modification_time[2];
  unsigned char last_modification_date[2];
  unsigned short low_16_bits;
  unsigned char size[4];

} __attribute__((packed)) root_directory_file;

int main() {

  fat_BS_t boot_record;

  FILE *file_pointer;
  file_pointer = fopen(FILENAME, "rb");
  if (file_pointer == NULL) {
    printf("Error openning the file!");
    return 1;
  }

  fread(&boot_record, sizeof(fat_BS_t), 1, file_pointer);
  int root_location =
      boot_record.bytes_per_sector * boot_record.reserved_sector_count +
      boot_record.bytes_per_sector * boot_record.table_size_16 *
          boot_record.table_count;

  printf("================================\n");
  printf("Bytes per sector: %hu\n", boot_record.bytes_per_sector);
  printf("Sectors per cluster: %hu\n", boot_record.sectors_per_cluster);
  printf("Reserved sectors: %hu\n", boot_record.reserved_sector_count);
  printf("Number of root entries: %hu\n", boot_record.root_entry_count);

  int fat_table_position =
      boot_record.reserved_sector_count * boot_record.table_count;

  fseek(file_pointer, root_location, SEEK_SET);

  root_directory_file rf;
  int k = 0;
  for (int c = 0; c < boot_record.root_entry_count; c++) {
    fread(&rf, sizeof(rf), 1, file_pointer);
    if (!(rf.file_type == 0x0F) && !(rf.file_type == 0)) {
      printf("%d.", k++);
      rf.file_type == 0x20 ? printf("(FIL)") : printf("(DIR)");
      printf("%s\n", rf.file_name);

      // reading file 01
      if (k == 1) {
        int file_position = rf.low_16_bits * 2 + fat_table_position;
        unsigned short current_cluster = 1;
        fseek(file_pointer, file_position, SEEK_SET);
        unsigned short *file_cluster = malloc(current_cluster);
        unsigned short current_file_cluster;
        fread(&current_file_cluster, 2, 1, file_pointer);
        if (current_file_cluster == 0xFFFF) {
          break;
        } else {
          file_cluster = realloc(file_cluster, ++current_cluster);
        }
      }
    }
  }

  fclose(file_pointer);
}
