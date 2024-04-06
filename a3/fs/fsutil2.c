#include "fsutil2.h"
#include "bitmap.h"
#include "cache.h"
#include "debug.h"
#include "directory.h"
#include "file.h"
#include "filesys.h"
#include "free-map.h"
#include "fsutil.h"
#include "inode.h"
#include "off_t.h"
#include "partition.h"
#include "../interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int copy_in(char *fname) {
  //real HD -> shell HD
  //lack of free space -> "Warning: could only write %d out of %ld bytes (reached end of file)"
  //%d: nbr of bytes you could write %ld:total file size of the file
  FILE* file = fopen(fname, "rb");

  if (file == NULL) {
    return FILE_DOES_NOT_EXIST;
  }

  //Find real HD file's size 
  fseek(file, 0, SEEK_END); //point to the end of the file
  unsigned int size = ftell(file); //calculate the size of the file
  fseek(file, 0, SEEK_SET); //point to start of file for reading

  size_t file_sectors = size / BLOCK_SECTOR_SIZE; //calculate nbr of sectores needed by the file
  if (size % BLOCK_SECTOR_SIZE != 0) file_sectors++;

  size_t free_sectors = num_free_sectors();

  unsigned int write = 0; //nbr of sectors to write for large file

  if (file_sectors > free_sectors){ //large file case (sectors needed more than available)
    //calculation of required sectors for writing are inspired by a comment under Ed post #770
    if (free_sectors <= 123){
      write = free_sectors * 512;
    } else if (free_sectors > 123 && free_sectors <= (123+128)){
      write = (free_sectors - 2) * 512;
    } else {
      size_t additional = ((free_sectors - 3) - 1 - 128) / 128;
      write = (free_sectors - (additional + 3)) * 512;
    }
    if (!fsutil_create(fname, 1)){ //create file on shell HD using same name and size as OG
      fclose(file);
      return FILE_CREATION_ERROR;
    } 

    char* buffer = malloc((write+1)*sizeof(char)); 
    memset(buffer, 0 , write+1);

    while (!feof(file)) {
      if(fread(buffer, sizeof(buffer), sizeof(buffer), file)){
        continue;
      }else {
        fclose(file);
        return FILE_READ_ERROR;
      }
    }

    if (!fsutil_write(fname, buffer, write+1)){
      fclose(file);
      return FILE_WRITE_ERROR;
    }
    free(buffer);

    printf("Warning: could only write %d out of %d bytes (reached end of file)\n", write, size);
  } else {
    if (!fsutil_create(fname, size)){ //create file on shell HD using same name and size as OG
      fclose(file);
      return FILE_CREATION_ERROR;
    } 

    char* buffer = malloc((size+1)*sizeof(char)); 
    memset(buffer, 0 , size+1);

    while (!feof(file)) {
      if(fread(buffer, sizeof(buffer), sizeof(buffer), file)){
        continue;
      }else {
        fclose(file);
        return FILE_READ_ERROR;
      }
    }

    if (!fsutil_write(fname, buffer, size+1)){
      fclose(file);
      return FILE_WRITE_ERROR;
    }
    free(buffer);
  }
  fclose(file);
  fsutil_seek(fname, 0); //reset file's offset 
  return 0;
}

int copy_out(char *fname) {
  //code from lab 
  int size = fsutil_size(fname); //retrieve size of the file
  char* buffer = malloc((size+1)*sizeof(char)); 
  memset(buffer, 0 , size+1);
  fsutil_seek(fname, 0); //make sure offset points to the start of the file
  if (!fsutil_read(fname, buffer, size)) {
    return FILE_READ_ERROR;
  }

  FILE* file = fopen(fname, "w");

  if (file == NULL) {
    return FILE_DOES_NOT_EXIST;
  }

  fputs(buffer, file);
  fclose(file);
  free(buffer);
  return 0;
}

void find_file(char *pattern) {
  //go through all files in root dir and print the name of each file whose contents contain the pattern
  //similar structure as fsutil_ls()
  struct dir *dir;
  char name[NAME_MAX + 1]; //stores file names

  dir = dir_open_root(); 
  while (dir_readdir(dir, name)){
    struct file *f = filesys_open(name); //open each file in root dir
    if (f != NULL){
      int f_size = file_length(f); //calculate file size
      if (f_size > 0){
        char *buffer = malloc(f_size + 1);

        fsutil_read(name, buffer, f_size); //buffer contains contents of the file
        if (strstr(buffer, pattern) != NULL){ //search for the pattern in the buffer
          printf("%s\n", name); //if found, print the name of the file
        }
        free(buffer);
      }
    }
    file_close(f);
  }
  dir_close(dir);
  return;
}

void fragmentation_degree() {
  //Print out the degree of fragmentation of fs
  //degree = number of fragmented filed / number of fragmentable files
  //fragmented file: contains at least 2 consecutive data blocks in sectors that are more than 3 away from each other
  //Fragmentable file : file that has more than 1 direct block

  int fragmented = 0; //count for number of fragmented files
  int fragmentable = 0; //count for number of fragmentable files
  float degree = 0; //variable storing fragmentation degree

  //structure similar to fsutil_ls()
  struct dir *dir;
  char name[NAME_MAX + 1]; //stores file names

  dir = dir_open_root(); 
  while (dir_readdir(dir, name)){ 
    struct file *f = filesys_open(name); //open each file in root dir
    if (f != NULL){
      int f_size = file_length(f); //calculate file size

      if (f_size > 512){ //if file is greater than 512 bytes (max sector size), it is fragmentable
        fragmentable++;

        block_sector_t *sectors = get_inode_data_sectors(f->inode); //get the sectors of data blocks associated to the inode of the file
        
        //Calculate nbr of sectors needed by a file
        offset_t length = f->inode->data.length;
        size_t nbr_sectors = bytes_to_sectors(length);
        
        //Loop through the sectors to check for fragmented files
        //j starts at 1 because first comparison is between sector 1 and sector 0 (if start at 0 -> current sector compares with itself)
        for (int j = 1; j < nbr_sectors; j++){  
          //sector[?] gives the sector number so calculate the distance between the current and next sectors
          if (sectors[j]-sectors[j-1]>3){  //if distance is more than 3
            fragmented++; //file is fragmented
            break; //no need to continue the loop at this point
          }
        }
        free(sectors);
      }
    }
    file_close(f);
  }
  dir_close(dir);

  degree = (float) fragmented/fragmentable;


  printf("Num fragmentable files: %d\n", fragmentable);
  printf("Num fragmented files: %d\n", fragmented);
  printf("Fragmentation pct: %.6f\n", degree);
}


//Helper function to count the number of files in root dir
int file_count() {
  //same code as fsutil_ls but with a counter
  struct dir *dir;
  char name[NAME_MAX + 1];
  int count = 0;
  dir = dir_open_root();
  if (dir == NULL)
    return 1;
  while (dir_readdir(dir, name))
    count++;
  dir_close(dir);
  return count;
}


int defragment() {
  //Reduce nbr of fragmented files to 0 without data loss

  //struct containing essential info to store a file
  struct file_store {
    char *filename;
    int size;
    char *contents;
  }; 

  int total_files = file_count(); //get the total number of files in root dir

  struct file_store files[(total_files + 1)*sizeof(struct file_store)]; //initialize files array
  int index = 0;  //initialize index counter for files array

  struct dir *dir;
  char name[NAME_MAX + 1]; //stores file names
  
  //Loop #1: Store files + their data into an array
  dir = dir_open_root(); 
  while (dir_readdir(dir, name)){ 
    struct file *f = filesys_open(name); 
    if (f != NULL){
      int f_size = file_length(f); //calculate file size

      if (f_size > 0){ 
        char *buffer = malloc(f_size); //buffer for file contents
        fsutil_read(name, buffer, f_size); //read file contents into buffer
        file_close(f);

        //store all the file's info 
        files[index].size = f_size;
        files[index].filename = strdup(name);
        files[index].contents = buffer;
        
        index++; //next file in the array
      }
    }
  }
  dir_close(dir);

  //Loop #2: remove all the files from root dir
  dir = dir_open_root();
  while (dir_readdir(dir, name)){ 
    fsutil_rm(name);
  }
  dir_close(dir);

  //Loop #3: recreate all the files and rewrite their contents
  dir = dir_open_root();
  for (int i = 0; i<total_files; i++){
    fsutil_create(files[i].filename, files[i].size);
    fsutil_write(files[i].filename, files[i].contents, files[i].size);
  }

  dir_close(dir);

  return 0;
}

void recover(int flag) {
  if (flag == 0) { // recover deleted inodes
    size_t total_bits = bitmap_size(free_map); //get total nbr of bits in the free map

    //Iterate through each bit in the bitmap to scan free sectors
    for (size_t bit = 0; bit < total_bits; bit++){
      if (!bitmap_test(free_map, bit)){ //if current bit is free (not set)
        struct inode_disk *inode_buff = malloc(BLOCK_SECTOR_SIZE); //need the disk format of inode
        buffer_cache_read(bit, inode_buff); //read contents of the sector represented by the current bit

        //restore inodes
        if (inode_buff->magic == INODE_MAGIC){ //checks if buffer contains a valid inode for recovery 
          struct dir *dir = dir_open_root(); ;
          char name[NAME_MAX + 1];
          snprintf(name, sizeof(name), "recovered0-%ld", bit);  //format filename
          bitmap_mark(free_map, bit); //mark current sector in bitmap as used
          dir_add(dir, name, bit, false); //add recovered file in the root dir
          dir_close(dir);
        }
        free(inode_buff);
      }
    }

  } else if (flag == 1) { // recover all non-empty sectors
  
    //Iterate through each sector starting from sector 4
    for (size_t sector = 4; sector < num_free_sectors(); sector++){
      char *buffer = malloc(BLOCK_SECTOR_SIZE); //buffer containing sector data
      buffer_cache_read(sector, buffer); 

      bool is_nonzero = false; //check if data is non-zero
      //check every byte in the buffer
      for (size_t i = 0; i<BLOCK_SECTOR_SIZE; i++){
        if (buffer[i] != 0) { //non-zero byte
          is_nonzero = true;
          break;
        }
      }

      //Count total nbr of non-zero bytes which will be used for the size of the file
      int size = 0;
      for (size_t i = 0; i<BLOCK_SECTOR_SIZE; i++){
        if (buffer[i] != 0) {
          size++;
        }
      }

      //if sector is non-empty
      if(is_nonzero){ 
        char name[NAME_MAX + 1];
        snprintf(name, sizeof(name), "recovered1-%ld.txt", sector);  //format filename
        //Create file in real filesystem
        FILE *file = fopen(name, "wb"); //write in binary mode
        if (file != NULL){
          fwrite(buffer, 1, size, file); //write the data stored in buffer to the file
          fclose(file);
        }
      }
      free(buffer);
    }
  } else if (flag == 2) { // data past end of file.
    struct dir *dir;
    char name[NAME_MAX + 1]; //stores file names
    dir = dir_open_root(); 
    while (dir_readdir(dir, name)){ 
      struct file *f = filesys_open(name); 
      if (f != NULL){
        //Calculate nbr of sectors needed by a file
        offset_t length = f->inode->data.length;
        size_t nbr_sectors = bytes_to_sectors(length);

        block_sector_t *sectors = get_inode_data_sectors(f->inode); //get array of sectors of the inode
        if (sectors != NULL){
          //Hidden data is in the last sector
          block_sector_t last_sector = sectors[nbr_sectors - 1]; //find last sector
          char *buffer = malloc(BLOCK_SECTOR_SIZE); //buffer containing sector data
          buffer_cache_read(last_sector, buffer); //read last sector data into buffer

          bool hidden = false; //boolean to find hidden data
          //Calculate start index of the last sector
          //nbr_sectors - 1: nbr of fully occupied sectors (last one contains hidden)
          //(nbr_sectors - 1) * 512: total nbr of fully occupied bytes
          //length - (nbr_sectors - 1) * 512: nbr of bytes left in the last sector that might contain hidden data aka the start index
          size_t start = length - (nbr_sectors - 1) * 512; 
          //iterate through the remaining bytes of the last sector to find potential hidden data
          for (size_t i = start; i < BLOCK_SECTOR_SIZE; i++){ 
            if (buffer[i] != 0){ 
              hidden = true;
              break;
            }
          }

          //Count total nbr of non-zero bytes in hidden data section which will be used for the size of the file
          int size = 0;
          for (size_t i = start; i<BLOCK_SECTOR_SIZE; i++){
            if (buffer[i] != 0) {
              size++;
            }
          }

          if (hidden){
            char filename[FILENAME_MAX];
            snprintf(filename, sizeof(filename), "recovered2-%s.txt", name);  //format filename
            //Create file in real filesystem
            FILE *file = fopen(filename, "wb"); //write in binary mode
            if (file != NULL){
              //&buffer[start+1] ensures that we are only writing the hidden data 
              fwrite(&buffer[start+1], 1, size, file); //write the data stored in buffer to the file
              fclose(file);
            }
          }
          free(buffer);
          free(sectors);
        }
        file_close(f);
      }
    }
    dir_close(dir);
  }
}