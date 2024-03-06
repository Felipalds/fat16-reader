#include <stdio.h>
#include <stdlib.h>

const char *FILENAME = "./data/test.img";

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
  int size;
} __attribute__((packed)) root_directory_file;

unsigned short *read_fat_file(root_directory_file *file, int fat_table_position,
                              FILE *file_pointer) {
  unsigned short file_first_cluster;
  unsigned short *file_clusters =
      (unsigned short *)malloc(1 * sizeof(unsigned short));
  fseek(file_pointer, fat_table_position + (file->low_16_bits) * 2, SEEK_SET);
  if (file_clusters == NULL) {
    printf("Error allocating");
  }

  file_clusters[0] = file->low_16_bits;

  short k = 1;

  while (
      fread(&file_first_cluster, sizeof(file_first_cluster), 1, file_pointer) &&
      file_first_cluster != 0xFFFF && file_first_cluster != 0x0) {
    file_clusters[k] = file_first_cluster;
    file_clusters = (unsigned short *)realloc(file_clusters,
                                              (k + 1) * sizeof(unsigned short));
    if (file_clusters == NULL) {
      printf("Error reallocating file");
    }
    fseek(file_pointer, fat_table_position + file_clusters[k] * 2, SEEK_SET);
    k++;
  }

  for (int c = 0; c < k; c++) {
    printf("%hu -> ", file_clusters[c]);
  }
  printf("\n");
  return file_clusters;
}

void read_file_data(unsigned short *fat_pos, FILE *file_pointer,
                    int first_data_sector, int sectors_per_cluster,
                    int bytes_per_sector, int size) {
  int cluster_size = bytes_per_sector * sectors_per_cluster;
  printf("first data sector %hd\n", first_data_sector);
  int c = 0;
  printf("file size: %d\n", size);
  while (1) {
    if (fat_pos[c] == 0) {
      break;
    }
    int current_data_sector =
        (((fat_pos[c] - 2) * sectors_per_cluster) + first_data_sector) *
        bytes_per_sector;

    fseek(file_pointer, current_data_sector, SEEK_SET);
    char *just_testing = malloc(cluster_size);
    fread(just_testing, cluster_size, 1, file_pointer);

    for (int c = 0; c < cluster_size; c++) {
      printf("%c", just_testing[c]);
    }
  }
}

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
  printf("Root dir location: %d \n", root_location);

  printf("================================\n");
  printf("Bytes per sector: %hu\n", boot_record.bytes_per_sector);
  printf("Sectors per cluster: %hu\n", boot_record.sectors_per_cluster);
  printf("Reserved sectors: %hu\n", boot_record.reserved_sector_count);
  printf("Root entry count: %hu\n", boot_record.root_entry_count);
  printf("Table size: %hu\n", boot_record.table_size_16);

  int fat_table_position =
      boot_record.reserved_sector_count * boot_record.bytes_per_sector;
  int root_dir_sectors = ((boot_record.root_entry_count * 32) +
                          (boot_record.bytes_per_sector - 1)) /
                         boot_record.bytes_per_sector;

  int first_data_sector =
      boot_record.reserved_sector_count +
      (boot_record.table_count * boot_record.table_size_16) + root_dir_sectors;

  int data_sectors = boot_record.total_sectors_16 -
                     (boot_record.reserved_sector_count +
                      (boot_record.table_count * boot_record.table_size_16) +
                      root_dir_sectors);

  fseek(file_pointer, root_location, SEEK_SET);

  root_directory_file rf;
  root_directory_file first_file;
  int k = 0;

  for (int c = 0; c < boot_record.root_entry_count; c++) {
    fread(&rf, sizeof(rf), 1, file_pointer);
    if (!(rf.file_type == 0x0F) && !(rf.file_type == 0)) {
      printf("%d.", k);
      rf.file_type == 0x20 ? printf("(FIL)") : printf("(DIR)");
      printf("%s\n", rf.file_name);

      if (k == 3) {
        first_file = rf;
      }
      k++;
    }
  }
  unsigned short *fat_table_positions =
      read_fat_file(&first_file, fat_table_position, file_pointer);

  printf("asdfasdfsdaf %d", first_file.size);

  read_file_data(fat_table_positions, file_pointer, first_data_sector,
                 boot_record.sectors_per_cluster, boot_record.bytes_per_sector,
                 first_file.size);

  fclose(file_pointer);
}
