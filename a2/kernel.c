//Mona Kalaoun 261044639
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"

bool active = false;
bool debug = false;
bool in_background = false;

int process_initialize(char *filename){
    FILE* fp;
    int* start = (int*)malloc(sizeof(int));
    int* end = (int*)malloc(sizeof(int));
    int error_code = 0;
    fp = fopen(filename, "rt");
    if(fp == NULL){
		return FILE_DOES_NOT_EXIST;
    }
    int maxpages = countPages(filename);
    PCB* newPCB = makePCB();
    for (int p = 0; p< maxpages; p++){
        error_code = load_page(fp, filename, newPCB);
        newPCB->current_page++;
    }
    newPCB->PC = find_PC(newPCB);
    //printf("PC: %d\n", newPCB->PC);
    if(error_code != 0){
        fclose(fp);
        return FILE_ERROR;
    }

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    ready_queue_add_to_tail(node);

    fclose(fp);
    
    return 0;
}

int find_PC(PCB *pcb){
    int pc;
    //printf("current page: %d, frame: %d\n", pcb->current_page, pcb->pagetable[pcb->current_page]);
    int frame = pcb->pagetable[0];
    pc = varmemsize + (frame - 1)*3;
    return pc;
}

void handle_page_fault(PCB *pcb){
    //add pcb to tail of ready queue
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = pcb;
    ready_queue_add_to_tail(node);
}

bool execute_process(QueueNode *node, int quanta){ //quanta: nbr of instr a process will run before switching to another process and running those instr
   // printf("quanta: %d\n", quanta);
    char *line = NULL;
    PCB *pcb = node->pcb; //the arrow -> allows access to members of a struct through a ptr
    for(int i=0; i<quanta; i++){
       // printf("PC: %d\n", pcb->PC);
        line = mem_get_value_at_line(pcb->PC++);
        //printf("line: %s\n", line);
        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        if(pcb->PC>pcb->end){
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }   
    }
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        if(is_ready_empty()) {
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if(!execute_process(cur, 1)) {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg){
    //printf("im in kernel inside RR\n");
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        if(is_ready_empty()){
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        if(!execute_process(cur, quanta)) {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char* policy){ //, bool mt){
    //printf("im in kernel inside schedule at start\n");
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return SCHEDULING_ERROR;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(strcmp("FCFS",policy)==0){
        scheduler_FCFS();
    }else if(strcmp("SJF",policy)==0){
        scheduler_SJF();
    }else if(strcmp("RR",policy)==0){
        //printf("im in kernel inside schedule at RR\n");
        arg[0] = 2;
        scheduler_RR((void *) arg);
    }else if(strcmp("AGING",policy)==0){
        scheduler_AGING();
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        scheduler_RR((void *) arg);
    }
    return 0;
}

