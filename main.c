#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  unsigned short current_cluster = file->low_16_bits;
  unsigned short *file_clusters =
      (unsigned short *)malloc(1 * sizeof(unsigned short));

  if (file_clusters == NULL) {
    printf("Error allocating");
  }

  short cluster_pos = 0;
  file_clusters[cluster_pos] = current_cluster;

  while (1) {
    int cluster_location = fat_table_position + current_cluster * 2;
    fseek(file_pointer, cluster_location, SEEK_SET);
    fread(&current_cluster, sizeof(current_cluster), 1, file_pointer);

    if (current_cluster == 0xFFFF) {
      break;
    }

    cluster_pos++;
    file_clusters = (unsigned short *)realloc(
        file_clusters, (cluster_pos + 1) * sizeof(unsigned short));
    file_clusters[cluster_pos] = current_cluster;
    // printf("%hu - ", current_cluster);

    if (file_clusters == NULL) {
      printf("Error reallocating file");
    }
  }

  // for (int c = 0; c < cluster_pos; c++) {
  //   printf("%hu -> ", file_clusters[c]);
  // }
  // printf("\n");
  return file_clusters;
}

void read_file_data(unsigned short *fat_pos, FILE *file_pointer,
                    int first_data_sector, int sectors_per_cluster,
                    int bytes_per_sector, int size) {
  int cluster_size = bytes_per_sector * sectors_per_cluster;
  int file_size = size;
  // printf("first data sector %d\n", first_data_sector);
  int c = 0;
  while (1) {
    if (fat_pos[c] == 0) {
      break;
    }
    int current_data_sector =
        (((fat_pos[c] - 2) * sectors_per_cluster) + first_data_sector) *
        bytes_per_sector;

    fseek(file_pointer, current_data_sector, SEEK_SET);
    char *current_file = malloc(cluster_size);
    fread(current_file, cluster_size, 1, file_pointer);

    for (int c = 0; c < cluster_size; c++) {
      printf("%c", current_file[c]);
      file_size = file_size - 1;
      if (file_size == 0) {
        break;
      }
    }
    c++;
  }
  printf("\n");
}

int main(int argc, char *argv[]) {

  if (argc == 1) {
    printf("Filename not provided. Please run ./main ./PATH/TO/FILE.img and "
           "try again.\n");
    exit(1);
  }

  const char *FILENAME = argv[1];

  if (strlen(FILENAME) == 0 || FILENAME == NULL) {
    printf("Filename not provided. Please run ./main ./PATH/TO/FILE.img and "
           "try again.\n");
    exit(1);
  }
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

  system("clear");
  printf("\n========= FAT 16 READER =========\n");
  printf("     By: Luiz Felipe F. Rosa\n");
  printf("=================================\n");
  printf("Root dir location: %d \n", root_location);
  printf("Bytes per sector: %hu\n", boot_record.bytes_per_sector);
  printf("Sectors per cluster: %hu\n", boot_record.sectors_per_cluster);
  printf("Reserved sectors: %hu\n", boot_record.reserved_sector_count);
  printf("Root entry count: %hu\n", boot_record.root_entry_count);
  printf("Table size: %hu\n", boot_record.table_size_16);
  printf("Fat numbers: %hu\n", boot_record.table_count);

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
  root_directory_file *files = malloc(1 * sizeof(root_directory_file));
  int k = 0;

  for (int c = 0; c < boot_record.root_entry_count; c++) {
    fread(&rf, sizeof(rf), 1, file_pointer);
    if (!(rf.file_type == 0x0F) && !(rf.file_type == 0) &&
        !(rf.file_name[0] == 0xE5)) {
      files[k] = rf;
      k++;
      files = realloc(files, (k + 1) * sizeof(root_directory_file));
    }
  }

  int option;
  while (1) {
    printf("===================\n");
    for (int c = 0; c < k; c++) {
      printf("%d. ", c);
      files[c].file_type == 0x20 ? printf("(FIL)") : printf("(DIR)");
      printf("%s %d bytes | cluster %hd\n", files[c].file_name, files[c].size,
             files[c].low_16_bits);
    }

    printf("Which one do you want to read? (-1 to exit) \n");
    scanf("%d", &option);
    system("clear");
    if (option < 0)
      break;

    if (option >= k) {
      printf("Type a valid option!");
      continue;
    }

    unsigned short *fat_table_positions =
        read_fat_file(&files[option], fat_table_position, file_pointer);

    read_file_data(fat_table_positions, file_pointer, first_data_sector,
                   boot_record.sectors_per_cluster,
                   boot_record.bytes_per_sector, files[option].size);
  }

  fclose(file_pointer);
}
