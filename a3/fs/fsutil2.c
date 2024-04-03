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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int copy_in(char *fname) {
  // TODO
  //real HD -> shell HD
  //lack of free space -> "Warning: could only write %d out of %ld bytes (reached end of file)"
  //%d: nbr of bytes you could write %ld:total file size of the file
  size_t b;
  FILE* file = fopen(fname, "rb");

  if (file == NULL) {
    printf("fdn\n");
    return 0;
  }

  //Find real HD file's size 
  fseek(file, 0, SEEK_END); //point to the end of the file
  unsigned int size = ftell(file); //calculate the size of the file
  printf("size of OG file: %d\n", size);
  fseek(file, 0, SEEK_SET); //point to start of file for reading

  fsutil_create(fname, size); //create file on shell HD using same name and size as OG
  
  char* buffer = malloc((size+1)*sizeof(char)); 
  memset(buffer, 0 , size+1);

  
  //while ((b=fread(buffer, 1, size+1, file)) > 0){
    //fsutil_write(fname, buffer, b);
  //}
  while (!feof(file)) {
    fread(buffer, sizeof(buffer), sizeof(buffer), file);
  }

  fsutil_write(fname, buffer, size+1);

  fclose(file);
  free(buffer);
  fsutil_seek(fname, 0); //reset file's offset 
  return 0;
}

int copy_out(char *fname) {
  //code from lab 
  //struct file *file_s = get_file_by_fname(fname);
  //block_sector_t *in = get_inode_data_sectors(file_s->inode);
  //printf("inode: %ls\n", in);
  //offset_t offset = file_tell(file_s); //store file's current offset 

  int size = fsutil_size(fname); //retrieve size of the file
  char* buffer = malloc((size+1)*sizeof(char)); 
  memset(buffer, 0 , size+1);
  fsutil_seek(fname, 0); //make sure offset points to the start of the file
  fsutil_read(fname, buffer, size);

  FILE* file = fopen(fname, "w");

  if (file == NULL) {
    printf("fdn\n");
    return 0;
  }
  fputs(buffer, file);
  fclose(file);
  free(buffer);
 // fsutil_seek(fname, offset); //reset file's offset 
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
    //Recover deleted files
    //Scan all free sectors on the drive and check if any contain inodes which were previously deleted from root but still exist on disk
    //If found, restore the inodes so files can be accessed normally
    //Filename should be recovered0-%d where d is the sector nbr containing deleted inode (use dir_add)
    // TODO

    //if I'm checking all the sectors and using buffer_cache_read , how do I know if the sector has an inode or not?
    //check if inode->data.magic is equal to the inode magic constant
    //try storing it using inode_open, then check if it has the magic number
    
    // when we recover the inode, we need to go through each data sector and mark it in the bitmap
    //Read through buffer_cache_read. allocate *char buffer pass in and sector number. Loop until nbr of bits in freemap
    //After having sector number of free sector, use inode_open. Check with magic number (same as INODE_MAGIC)
    //bitmap mark after recovering the file
  
    for (block_sector_t sector = 0; sector < num_free_sectors(); sector++){
      struct inode_disk *inode_buff = malloc(BLOCK_SECTOR_SIZE);
      buffer_cache_read(sector, inode_buff);

      if (inode_buff->magic == INODE_MAGIC){
        struct dir *dir = dir_open_root(); ;
        char name[NAME_MAX + 1]; //stores file names
        snprintf(name, sizeof(name), "recovered0-%d", sector);
        bitmap_mark(free_map, sector);
        dir_add(dir, name, sector, false);
        dir_close(dir);
      }
      free(inode_buff);
    }


  } else if (flag == 1) { // recover all non-empty sectors
    // TODO
  } else if (flag == 2) { // data past end of file.
    //The bytes to sectors function to get number of sectors
    //Then get inode data sectors to get the array of sectors of the inode
    //And use the number of sectors -1 for last sector
    //As the index
    //buffer cacher read
    //bytes_to_sectors with fsutil_size passed to get length of array 
    // TODO
  }
}