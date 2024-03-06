//Mona Kalaoun 261044639
#ifndef KERNEL
#define KERNEL
#include "pcb.h"
int process_initialize(char *filename);
int find_PC(PCB *pcb);
int schedule_by_policy(char* policy); //, bool mt);
int shell_process_initialize();
void ready_queue_destory();
#endif