//Mona Kalaoun 261044639
#ifndef PCB_H
#define PCB_H
#include <stdbool.h>
/*
 * Struct:  PCB 
 * --------------------
 * pid: process(task) id
 * PC: program counter, stores the index of line that the task is executing
 * start: the first line in shell memory that belongs to this task
 * end: the last line in shell memory that belongs to this task
 * job_length_score: for EXEC AGING use only, stores the job length score
 */

#define MAX_PAGES framesize/3

typedef struct
{
    bool priority;
    int pid;
    int PC;
    int start;
    int end;
    int job_length_score;
    int pagetable[MAX_PAGES];
    int num_pages;
    int current_page;
    int next_page;
    int pages_needed;
    char* filename;
    int line_offset;
}PCB;

int generatePID();
PCB * makePCB();
#endif