//Mona Kalaoun 261044639
#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H
#include "pcb.h"
void mem_init();
void resetvarmem();
char *mem_get_value(char *var);
void mem_set_value(char *var, char *value);
int load_file(FILE* fp, int* pStart, int* pEnd, char* fileID);
int countLines(char *filename);
int countPages(char *filename);
int find_free_frame();
int load_page(FILE* fp, char* fileID, PCB *pcb);
char *mem_get_value_at_line(int index);
void mem_free_lines_between(int start, int end);
void mem_free_lines_frame(int start, int end);
void printShellMemory();
#endif