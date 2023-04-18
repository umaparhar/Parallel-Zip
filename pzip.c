#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/sysinfo.h>
 
 
#define NUM_SYS_THREADS get_nprocs()
int NUM_THREADS;
 
struct arg_struct{
    int start_index;
    int end_index;
    char *chars;
    int *ints;
    char *f;
    int num;
}args;
 
void *processFile(void *a){
    struct arg_struct *args = (struct arg_struct*)a;
    int i = args->start_index;
    int j;
    char c;
    int val;
    char prevChar;
    int prevCount;
    int currCount = 0;
    int firstChar = 0;
    int arr_index = 0;
    int size = args->end_index - args->start_index + 1;
 
    while(i <= args->end_index){
        if(args->f[i] == '\0'){
            i++;
            continue;
        }
        currCount = 0;
        if(firstChar == 0 && args->num != 1){
            if(args->f[i] == prevChar){
                currCount += prevCount;
            }
            else{
                prevChar = '\0';
                prevCount = 0;
            }
        }
        firstChar = 1;
        c = args->f[i];
        currCount++;
        j = i + 1;
        while(c == args->f[j] && j <= args->end_index){
            j++;
            currCount++;
        }
        i = j;
        if((args->num > NUM_THREADS && i >= size)){
            prevChar = c;
            prevCount = currCount;
            break;
        }
        val = currCount;
        args->chars[arr_index] = c;
        args->ints[arr_index] = val;
        arr_index++;
    }
    return 0;      
}
 
 
int main(int argc, char *argv[]){
    if(argc <= 1){
        printf("pzip: file1 [file2 ...]\n");
        exit(1);
    }
 
    char* fchar_arr[argc - 1];
    int* fint_arr[argc - 1];
    int i = 1;
    int validFiles = 0;
    while(i < argc){
        NUM_THREADS = NUM_SYS_THREADS;
        int file = open(argv[i], O_RDONLY);
        struct stat s;
        int size;
 
        int status = fstat(file, &s);
        if(status != 0){
            i++;
	    continue;
        }
        size = s.st_size;
        if(size == 0){
            return 0;
        }
 
        if (size < 100)
        {
            NUM_THREADS = 1;
        }
       
        fchar_arr[validFiles] = malloc(sizeof(char)*size);
	fint_arr[validFiles] = malloc(sizeof(int)*size);
        validFiles++;
	char *f = (char *)mmap(0, size, PROT_READ, MAP_PRIVATE, file, 0);  
 
        pthread_t threads[NUM_THREADS];
        struct arg_struct args_arr[NUM_THREADS];
 
        int fourth = 0;
        if (NUM_THREADS != 1)
        {
            fourth = size / NUM_THREADS - 1;
 
        }
 
        for (int r = 0; r < NUM_THREADS; r++)
        {
            if (r == 0) {
                args_arr[0].start_index = 0;
            } else {
                args_arr[r].start_index = args_arr[r - 1].end_index + 1;
            }
 
            if (r == NUM_THREADS - 1)
            {
                args_arr[r].end_index = size - 1;
            } else {
                args_arr[r].end_index = args_arr[r].start_index + fourth;
            }
 
            args_arr[r].chars = (char *) malloc(sizeof(char)*(size/NUM_THREADS));
	    args_arr[r].ints = (int *) malloc(sizeof(int)*(size/NUM_THREADS));
	    args_arr[r].f = f;
            args_arr[r].num = r + 1;
 
        }
       
        for (int i = 0; i < NUM_THREADS; i++)
        {
            pthread_create(&threads[i], NULL, &processFile, &args_arr[i]);
        }
 
        for (int i = 0; i < NUM_THREADS; i++)
        {
	 
       	    pthread_join(threads[i], NULL);
        }
   
        int file_arr_counter = 0;
        for(int k = 0; k < NUM_THREADS; k++){ //going through each threads arg struct
            int j = 0;
            while(args_arr[k].chars[j] != '\0'){
                char curr = args_arr[k].chars[j];
                int currCount = args_arr[k].ints[j];
                if(args_arr[k].chars[j+1] == '\0' && k != NUM_THREADS - 1){ //on last element of first 3 arrays
                    if(args_arr[k+1].chars[0] == curr){
                        args_arr[k+1].ints[0] += currCount;
                        j++;
                        continue;
                    }
                }
                fchar_arr[validFiles - 1][file_arr_counter] = curr;
                fint_arr[validFiles - 1][file_arr_counter] = currCount;
                file_arr_counter++;
                j++;
            }
        }
   	i++;
    }
 
    for(int f = 0; f < validFiles; f++){
        int j = 0;
        while(fchar_arr[f][j] != '\0'){
	    char curr = fchar_arr[f][j];
	    int currCount = fint_arr[f][j];
	    if(f != validFiles - 1 && fchar_arr[f][j+1] == '\0'){
	        if(fchar_arr[f+1][0] == curr){
		    fint_arr[f+1][0] += currCount;
                    j++;
                    continue;
                }
            }
            fwrite(&currCount, sizeof(currCount), 1, stdout);
            fwrite(&curr, sizeof(curr), 1, stdout);
            j++;
        }
    }   
    
    return 0;

}
