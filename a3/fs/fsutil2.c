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
  //ex: file with 3 DB in sectors 3,4,5 or 3,4,7 is not fragmented
  //ex: file with 2 DB in sectors 4 and 10 is fragmented
  //Fragmentable file : file that has more than 1 DB
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
        //offset_t length = f->inode->data.length;
        //printf("data.length: %d\n", length);
        //size_t sectors1 = bytes_to_sectors(length);
        //printf("sectors1: %ld\n", sectors1);
        //int fs = f_size/512;
        //printf("fsize: %d\n", fs);
        //Loop through the sectors 
        for (int j = 1; j < (f_size / 512); j++){  //starting at 1 because index 0 indicates the first sector. Need to start comparing the first 2 sectors
          //printf("sector[j]: %d\n", sectors[j]);
          if (sectors[j]-sectors[j-1]>3){ //sector[j] gives the sector number so calculate the distance between the current and next sectors
            //printf("sector[j]: %d\n", sectors[j]);
            fragmented++;
            break;
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

int defragment() {
  // TODO
  //Reduce nbr of fragmented files to 0 without data loss
  //int fragmented = 0; //count for number of fragmented files
  //int fragmentable = 0; //count for number of fragmentable files
  //float degree = 0; //variable storing fragmentation degree

  //struct containing essential info to store a file
  struct file_store {
    char *filename;
    int size;
    char *contents;
  }; 

  struct file_store *files = NULL; //initialize files array
  int nbr_files = 0;  //initialize nbr of files counter

  struct dir *dir;
  char name[NAME_MAX + 1]; //stores file names
  
  //First loop: Store files + their data into an array
  dir = dir_open_root(); 
  while (dir_readdir(dir, name)){ 
    struct file *f = filesys_open(name); 
    if (f != NULL){
      int f_size = file_length(f); //calculate file size

      if (f_size > 0){ 
        char *buffer = malloc(f_size); //buffer for file contents
        fsutil_read(name, buffer, f_size);

        file_close(f);

        files = realloc(files, (nbr_files + 1) * sizeof(struct file_store));
        files[nbr_files].filename = strdup(name);
        files[nbr_files].contents = buffer;

        nbr_files++;
        free(buffer);
      }
    }
    //file_close(f);
  }
  dir_close(dir);

  dir = dir_open_root();
  while (dir_readdir(dir, name)){ 
    fsutil_rm(name);
  }
  dir_close(dir);

  dir = dir_open_root();
  for (int i = 0; i<nbr_files; i++){
    fsutil_create(files[i].filename, files[i].size);
    fsutil_write(files[i].filename, files[i].contents, files[i].size);
    //free(files[i].filename);
    //free(files[i].contents);
  }

  free(files);
  dir_close(dir);
    //file_close(f);



  //degree = (float) fragmented/fragmentable;


  //printf("Num fragmentable files: %d\n", fragmentable);
  //printf("Num fragmented files: %d\n", fragmented);
  //printf("Fragmentation pct: %.6f\n", degree);
  return 0;
}

void recover(int flag) {
  if (flag == 0) { // recover deleted inodes

    // TODO
  } else if (flag == 1) { // recover all non-empty sectors

    // TODO
  } else if (flag == 2) { // data past end of file.

    // TODO
  }
}