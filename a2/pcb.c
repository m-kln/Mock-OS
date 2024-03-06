//Mona Kalaoun 261044639
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1; 

int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
   // newPCB->PC = varmemsize; //PC: indicates address of next instruction
    //newPCB->start  = start;
    //newPCB->end = end;
    //newPCB->job_length_score = 1+newPCB->end-newPCB->start;
    newPCB->priority = false;
    for (int i = 0; i < MAX_PAGES; i++){
       newPCB->pagetable[i] = -1; //initialize the page table values
    }
    newPCB->num_pages = 0;
    newPCB->current_page = 0;
    newPCB->line_offset = 0;
    return newPCB;
}