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
  return 0;
}

int copy_out(char *fname) {
  //code from lab 
  int size = fsutil_size(fname); //retrieve size of the file
  char* buffer = malloc((size+1)*sizeof(char)); 
  memset(buffer, 0 , size+1);
  fsutil_seek(fname, 0); //make sure offset points to the start of the file
  fsutil_read(fname, buffer, size);

  FILE* file = fopen(fname, "w");
  if (file == NULL) {
    printf("fdn\n");
    return -1;
  }
  fputs(buffer, file);
  fclose(file);
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