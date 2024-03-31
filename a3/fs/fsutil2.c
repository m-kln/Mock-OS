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
  // TODO
  return;
}

void fragmentation_degree() {
  // TODO
}

int defragment() {
  // TODO
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