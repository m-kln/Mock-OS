//Mona Kalaoun 261044639
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>
#include "pcb.h"
#include "shellmemory.h"

#define FRAME_SIZE 3
#define SHELL_MEM_LENGTH varmemsize + framesize
const int TOTAL_FRAMES = framesize / FRAME_SIZE;

struct memory_struct{
	char *var; //name of var
	char *value; //its value
};

struct memory_struct shellmemory[SHELL_MEM_LENGTH]; //first part for var store and the rest for frame store

//function to alloc a new frame
	//mem init
	//call mem set value to allocate a frame

// Helper functions
int match(char *model, char *var) {
	int i, len=strlen(var), matchCount=0;
	for(i=0;i<len;i++)
		if (*(model+i) == *(var+i)) matchCount++;
	if (matchCount == len)
		return 1;
	else
		return 0;
}

char *extract(char *model) {
	char token='=';    // look for this to find value
	char value[1000];  // stores the extract value
	int i,j, len=strlen(model);
	for(i=0;i<len && *(model+i)!=token;i++); // loop till we get there
	// extract the value
	for(i=i+1,j=0;i<len;i++,j++) value[j]=*(model+i);
	value[j]='\0';
	return strdup(value);
}


// Shell memory functions

void mem_init(){ //sets all vars and values to none 
	int i;
	for (i=0; i<SHELL_MEM_LENGTH; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

void resetvarmem(){ //sets all vars and values to none 
	int i;
	for (i=0; i<varmemsize; i++){		
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<varmemsize; i++){ 
		if (strcmp(shellmemory[i].var, var_in) == 0){ //if var already exists, update the value
			shellmemory[i].value = strdup(value_in); 
			//strdup duplicates a string. Takes str as input, allocates mem to hold a copy of that str, copies input str into alloc mem
			//and returns a ptr to newly alloc mem containing dup string
			return; //exit loop immediately, stop iteration
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<varmemsize; i++){
		if (strcmp(shellmemory[i].var, "none") == 0){
			shellmemory[i].var = strdup(var_in);
			shellmemory[i].value = strdup(value_in);
			return;
		} 
	}

	return;

}

//get value based on input key
char *mem_get_value(char *var_in) {
	int i;
	for (i=0; i<SHELL_MEM_LENGTH; i++){
		if (strcmp(shellmemory[i].var, var_in) == 0){
			return strdup(shellmemory[i].value); //returning using strdup to ensure str is modifiable since it is a copy 
		} 
	}
	return NULL;

}

//prints all vars and their values stored in mem
void printShellMemory(){
	int count_empty = 0;
	for (int i = 0; i < SHELL_MEM_LENGTH; i++){
		if(strcmp(shellmemory[i].var,"none") == 0){
			count_empty++;
		}
		else{
			printf("\nline %d: key: %s\t\tvalue: %s\n", i, shellmemory[i].var, shellmemory[i].value);
		}
    }
	printf("\n\t%d lines in total, %d lines in use, %d lines free\n\n", SHELL_MEM_LENGTH, SHELL_MEM_LENGTH-count_empty, count_empty);
}


/*
 * Function:  addFileToMem 
 * 	Added in A2
 * --------------------
 * Load the source code of the file fp into the shell memory:
 * 		Loading format - var stores fileID, value stores a line
 *		Note that the first 100 lines are for set command, the rests are for run and exec command
 *
 *  pStart: This function will store the first line of the loaded file 
 * 			in shell memory in here. Stores starting index of file at which we have 
 * 			a contiguous block of empty elements that can fit the file
 *	pEnd: This function will store the last line of the loaded file 
 			in shell memory in here
 *  fileID: Input that need to provide when calling the function, 
 			stores the ID of the file (var)
 * 
 * returns: error code, 21: no space left
 * 
 * Stores the entirety of the program into shell mem, assuming there is no partition between frame and var store
 */

/*int load_file(FILE* fp, int* pStart, int* pEnd, char* filename)
{
	char *line; //each line read from the file 
    size_t i; //size_t : unsigned int (good for index)
    int error_code = 0;
	bool hasSpaceLeft = false;
	bool flag = true; //set to true to allow while loop to execute at least once
	i=101; //first 100 elements of shell mem is for variables and the frame store would start after
	size_t candidate;
	while(flag){ //goal: find starting index in mem for loading the file
		printf("restarting loop");
		flag = false;
		for (i; i < SHELL_MEM_LENGTH; i++){
			if(strcmp(shellmemory[i].var,"none") == 0){
				*pStart = (int)i; //as soon as an empty slot is found, set the start index to current i
				hasSpaceLeft = true;
				break;
			}
		}
		candidate = i; //remember location of empty slot
		printf("candidate in loop: %d\n", candidate);
		//finds a non empty slot and sets flag to true (ignore)
		for(i; i < SHELL_MEM_LENGTH; i++){
			if(strcmp(shellmemory[i].var,"none") != 0){
				flag = true;
				break;
			}
		}
	}
	//^ after while loop, nothing set in memory yet
	printf("candidate after loop: %d\n", candidate);
	i = candidate;
	printf("i after loop: %d\n", i);
	printShellMemory();
	//shell memory is full
	if(hasSpaceLeft == 0){
		error_code = 21;
		return error_code;
	}
    
	//load each line of file in memory 
    for (size_t j = i; j < SHELL_MEM_LENGTH; j++){
		printf("j at loop: %d\n", j);
        if(feof(fp))
        {
			printf("j at eof: %d\n", j);
            *pEnd = (int)j-1;
            break;
        }else{
			line = calloc(1, SHELL_MEM_LENGTH); //calloc is good for when u have elements that start with default values
			printf("line after calloc: %s\n", line);
			if (fgets(line, SHELL_MEM_LENGTH, fp) == NULL) //fgets reads until either mem-length-1 or \n or EOF
			{
				continue;
			}
			printf("line after fgets: %s\n", line);
			shellmemory[j].var = strdup(filename); //not using strndup cuz files are usually null terminated
            shellmemory[j].value = strndup(line, strlen(line)); 
			printShellMemory();
			//strndup duplicates a specified nbr of chars from start of a string
			//strlen does not include null terminator. Good to use here since we want to avoid unneeded chars 
			free(line);
        }
    }

	//no space left to load the entire file into shell memory
	if(!feof(fp)){
		error_code = 21;
		//clean up the file in memory
		for(int j = 1; i <= SHELL_MEM_LENGTH; i ++){
			shellmemory[j].var = "none";
			shellmemory[j].value = "none";
    	}
		return error_code;
	}
	printShellMemory();
    return error_code;
}*/

int countLines(char* filename){
	FILE* f = fopen(filename, "r");
	int total_lines = 0;
	char temp[framesize];
	while (!feof(f)){
		fgets(temp,framesize,f);
		total_lines++;
	}
	fclose(f);
	return total_lines;
}

int countPages(char* filename) {
	int total_lines = countLines(filename);
	int total_pages = total_lines/3;
	if (total_lines%3 != 0) total_pages++;

	return total_pages;
}

int find_free_frame() {
	int index = -1;
	int start_index;
	for (int i = varmemsize; i < SHELL_MEM_LENGTH; i += 3){
		if(strcmp(shellmemory[i].var,"none") == 0){
			index = i;
			break;
		}
	}

	//eviction
	if (index == -1){
		int victim = rand() % TOTAL_FRAMES;
		index = varmemsize + victim*3;
		printf("Page fault! Victim page contents:\n");
		for (int i = index; i < index + 3; i++){
			char *l = mem_get_value_at_line(i);
			if (strcmp(l, "none") != 0) printf("%s", l);
		}
		printf("\nEnd of victim page contents.\n");
		//printf("frame index start: %d\n", index);
		//printf("frame index end: %d\n", index+2);
		mem_free_lines_frame(index, index+2);
	}
	//printf("frame index: %d\n", index);
	return index;
}

int load_page(FILE *fp, char *filename, PCB *pcb) {
	int error_code = 0;
	int pages_needed = countPages(filename);
	int total_lines = countLines(filename);
	int lines_read = 0;
	int frame_index = 0;
	int initial_frame = 0;
	int fstart = 0;
    // Load the lines from the file into shellmemory
	frame_index = find_free_frame();
	//printf("frame index: %d\n", frame_index);
		
	initial_frame = frame_index;

	for (int i = 0; i < 3; i++) {
		char line[100];
		if (fgets(line, sizeof(line), fp)) {
			// Copy the line into the shellmemory array
			shellmemory[frame_index].var = strdup(filename); // Store filename
			shellmemory[frame_index].value = strndup(line, strlen(line)); // Store line
			//printShellMemory();
		} else {
			if (feof(fp)){
				break;
			}
			return 21;
		}
		frame_index++;
	}

	int frame_number = ((initial_frame - varmemsize) / 3) + 1;
	pcb->pagetable[pcb->next_page] = frame_number; 
	pcb->num_pages++;
	//printf("frame: %d, current_page: %d, number of pages: %d\n", frame_number, pcb->current_page, pcb->num_pages);
	//pcb->current_page++;

    // Calculate the ending position in frame store
	//pcb->line_offset = initial_frame - fstart;
	//printf("line offset: %d\n", pcb->line_offset);
	//pcb->start = fstart;
	//pcb->start = initial_frame;
	pcb->start = varmemsize + (pcb->pagetable[0]-1)*3;
	pcb->end = pcb->start + 2;
    //pcb->end = frame_index - 1;
	//printShellMemory();

	//printf("pcb start: %d, pcb end: %d\n", pcb->start, pcb->end);

    return 0; // Success
}


char *mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}

//frees mem allocated for lines between start and end indices
void mem_free_lines_between(int start, int end){
	printf("start: %d, end: %d\n", start, end);
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
		printf("in loop 1");
		if(shellmemory[i].var != NULL){
			printf("in loop 2");
			//free(shellmemory[i].var);
		}	
		printf("in loop 3");
		if(shellmemory[i].value != NULL){
			printf("in loop 4");
			//free(shellmemory[i].value);
		}	
		printf("in loop 5");
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
		printf("in loop 6");
	}
	printf("in loop 7");
}

void mem_free_lines_frame(int start, int end){
	//printf("start: %d, end: %d\n", start, end);
	for (int i=start; i<=end; i++){
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}