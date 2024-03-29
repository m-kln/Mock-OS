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

int access_counter = 0;

int process_initialize(char *filename){
    FILE* fp;
    int error_code = 0;
    fp = fopen(filename, "rt");
    if(fp == NULL){
		return FILE_DOES_NOT_EXIST;
    }
    int maxpages = countPages(filename);
    //printf("maxpages: %d\n", maxpages);
    PCB* newPCB = makePCB();
    //load the first 2 pages 
    newPCB->current_page = 0;
    //printf("frame at 2 in init: %d\n", newPCB->pagetable[2]);
    
    for (int p = 0; p< 2; p++){
        if (!feof(fp)){
            error_code = load_page(fp, filename, newPCB);
            newPCB->current_page++;
        }
    }

    //printf("frame at 2 after load first two: %d\n", newPCB->pagetable[2]);
    newPCB->current_page = 0;

    newPCB->filename = filename;
    //newPCB->file = fp;
    newPCB->PC = find_PC(newPCB);
    newPCB->pages_needed = maxpages;
    //newPCB->current_page = 0;
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
    int frame = pcb->pagetable[pcb->current_page]; 
    pc = varmemsize + (frame - 1)*3;
    return pc;
}

void handle_page_fault(PCB *pcb){
    //add pcb to tail of ready queue
    //printf("current page: %d, frame: %d\n", pcb->current_page, pcb->pagetable[pcb->current_page]);
    FILE *fp = fopen(pcb->filename, "rt");
    char line[100];
    for (int i = 0; i< pcb->current_page * 3; i++){
        if (fgets(line, sizeof(line), fp) == NULL){
            break;
        }
    }
    //fseek(fp, pcb->current_page * 3 * size , SEEK_SET);
   // printf("im loading new page\n");
    //pcb->lru_timer[pcb->current_page]++;
    load_page(fp, pcb->filename, pcb);
    //printf("i finished loading\n");
    pcb->PC = find_PC(pcb);
    //printf("new PC: %d\n", pcb->PC);
    fclose(fp);
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = pcb;
    ready_queue_add_to_tail(node);

    //fclose(fp);
}

bool execute_process(QueueNode *node, int quanta){
    char *line = NULL;
    PCB *pcb = node->pcb;
    int instructions_left = quanta;
    for(int i=0; i<quanta; i++){
        pcb->start = varmemsize + (pcb->pagetable[pcb->current_page]-1)*3;
	    pcb->end = pcb->start + 2;
        pcb->lru_timer[pcb->current_page]++; //= access_counter++;
        instructions_left--;
        //pcb->instr = instructions_left;
        //printf("PC in ex loop: %d, quanta: %d, current page: %d\n", pcb->PC, quanta, pcb->current_page);
        //printf("frame at 2: %d\n", pcb->pagetable[2]);
        line = mem_get_value_at_line(pcb->PC++);
        //printf("PC after line: %d\n", pcb->PC);
        //printf("line: %s\n", line);
        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        //printf("PCB end: %d\n", pcb->end);
        if(pcb->PC > pcb->end){
            printf("process: %d\n", pcb->pid);
            printf("page before: %d\n", pcb->current_page);
            //pcb->current_page++;
            pcb->current_page++;    
            printf("page after: %d\n", pcb->current_page);
            printf("frame at current: %d\n", pcb->pagetable[pcb->current_page]);
            pcb->end = find_PC(pcb) + 2;

            if (pcb->current_page + 1 > pcb->pages_needed){
                //printf("terminate\n");
                //printf("parse 2: \n");
                if (strcmp(line, "none") != 0) parseInput(line);
                terminate_process(node);
                in_background = false;
                return true;
            }  

            if (pcb->current_page + 1 <= pcb->pages_needed && pcb->pagetable[pcb->current_page] == -1){
                //printf("parse 1: \n");
                if (strcmp(line, "none") != 0) parseInput(line);
                //pcb->lru_timer[pcb->current_page]++;
                //printf("i need u\n");
                handle_page_fault(pcb);
                return true;
            }
            //pcb->end = find_PC(pcb) + 2;
            //printf("page needed: %d\n", pcb->pages_needed);

        }
       // printf("normal parsing\n");
        //printf("parse 3: \n");
        if (strcmp(line, "none") != 0) parseInput(line);
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
        bool result = execute_process(cur, quanta);
        //printf("%d\n", result);
        if(!result) {
            //printf("instr: %d\n", cur->pcb->instr);
            ready_queue_add_to_tail(cur);
            //cur->pcb->lru_timer[cur->pcb->current_page]++;
            printf("current page: %d, access: %d\n",cur->pcb->current_page, cur->pcb->lru_timer[cur->pcb->current_page]);
            //print_ready_queue();
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

