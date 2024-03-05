#include<stdlib.h>
#include<string.h>
#include<stdio.h>
#include<stdbool.h>

#define VAR_STORE_SIZE 100
#define FRAME_STORE_SIZE 900
#define FRAME_SIZE 3
#define SHELL_MEM_LENGTH VAR_STORE_SIZE + FRAME_STORE_SIZE
const int TOTAL_FRAMES = FRAME_STORE_SIZE / FRAME_SIZE;
//const int FRAME_MEM_SIZE = SHELL_MEM_LENGTH - VAR_MEM_SIZE;

struct memory_struct{
	char *var; //name of var
	char *value; //its value
};

//where the frame store and the variable store are
//const int FRAME_STORE_SIZE = 2;
//const int FRAME_SIZE = 3;

//int THRESHOLD = FRAME_STORE_SIZE * FRAME_SIZE;

struct memory_struct shellmemory[SHELL_MEM_LENGTH]; //first 100 lines for var store and the rest for frame store

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

// Set key value pair
void mem_set_value(char *var_in, char *value_in) {
	int i;
	for (i=0; i<VAR_STORE_SIZE; i++){ 
		if (strcmp(shellmemory[i].var, var_in) == 0){ //if var already exists, update the value
			shellmemory[i].value = strdup(value_in); 
			//strdup duplicates a string. Takes str as input, allocates mem to hold a copy of that str, copies input str into alloc mem
			//and returns a ptr to newly alloc mem containing dup string
			return; //exit loop immediately, stop iteration
		} 
	}

	//Value does not exist, need to find a free spot.
	for (i=0; i<VAR_STORE_SIZE; i++){
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
int load_file(FILE* fp, int* pStart, int* pEnd, char* filename)
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
}



char * mem_get_value_at_line(int index){
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