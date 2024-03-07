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

void resetvarmem(){ //sets all vars and values in var store to none 
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

//returns total number of lines in a file
int countLines(char* filename){
	FILE* f = fopen(filename, "r");
	int total_lines = 0;
	char temp[framesize];
	while (!feof(f)){
		fgets(temp,framesize,f);
		total_lines++; //increment until EOF
	}
	fclose(f);
	return total_lines;
}

//returns total number of pages needed to store the contents of a file
int countPages(char* filename) {
	int total_lines = countLines(filename);
	int total_pages = total_lines/3; //each page is 3 lines
	if (total_lines%3 != 0) total_pages++; //if nbr of lines is not a multiple of 3, increase page count

	return total_pages;
}

//finds a free spot in frame store
int find_free_frame() {
	int index = -1;
	int start_index;
	for (int i = varmemsize; i < SHELL_MEM_LENGTH; i += 3){ //framestore starts from index varmemsize
		if(strcmp(shellmemory[i].var,"none") == 0){
			index = i;
			break;
		}
	}

	//eviction
	if (index == -1){
		int victim = rand() % TOTAL_FRAMES; //random replacement
		index = varmemsize + victim*3; //determine starting index/line of the victim frame
		printf("Page fault! Victim page contents:\n");
		for (int i = index; i < index + 3; i++){
			char *l = mem_get_value_at_line(i); //print all commands of victim page
			if (strcmp(l, "none") != 0) printf("%s", l);
		}
		printf("\nEnd of victim page contents.\n");
		
		mem_free_lines_frame(index, index+2); //reset vars and values to none corresponding to the evicted page
	}
	return index;
}

//function loading one page at a time
int load_page(FILE *fp, char *filename, PCB *pcb) {
	int error_code = 0;
	int pages_needed = countPages(filename);
	int total_lines = countLines(filename);
	int lines_read = 0;
	int frame_index = 0;
	int initial_frame = 0;
	int fstart = 0;

	frame_index = find_free_frame(); //find a free slot
		
	initial_frame = frame_index; //save the first index of the slot

	for (int i = 0; i < 3; i++) { //max 3 lines
		char line[100];
		if (fgets(line, sizeof(line), fp)) {
			// Copy the line into the shellmemory array
			shellmemory[frame_index].var = strdup(filename); // Store filename
			shellmemory[frame_index].value = strndup(line, strlen(line)); // Store line
		} else {
			if (feof(fp)){
				break;
			}
			return 21; //error code
		}
		frame_index++;
	}

	int frame_number = ((initial_frame - varmemsize) / 3) + 1; //adding 1 so that frame numbers start with the number 1,2,3,..
	pcb->pagetable[pcb->next_page] = frame_number; //saving the current frame in the current page
	pcb->num_pages++;

	pcb->start = varmemsize + (pcb->pagetable[0]-1)*3;
    pcb->end = frame_index - 1;

    return 0; // Success
}


char *mem_get_value_at_line(int index){
	if(index<0 || index > SHELL_MEM_LENGTH) return NULL; 
	return shellmemory[index].value;
}

//frees mem allocated for lines between start and end indices
void mem_free_lines_between(int start, int end){
	for (int i=start; i<=end && i<SHELL_MEM_LENGTH; i++){
		if(shellmemory[i].var != NULL){
			free(shellmemory[i].var);
		}	
		if(shellmemory[i].value != NULL){
			free(shellmemory[i].value);
		}	
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}

//function resetting values in a frame to none
void mem_free_lines_frame(int start, int end){
	for (int i=start; i<=end; i++){
		shellmemory[i].var = "none";
		shellmemory[i].value = "none";
	}
}