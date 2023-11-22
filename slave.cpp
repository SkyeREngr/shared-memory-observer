#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <semaphore.h>
#include <errno.h>

#include "myShm.h"

int main (int argc, char* argv[]) {
    //Commandline arguments
    int childNum = atoi(argv[1]);                                                                   //child number received from parent processes
    const char *shmName = argv[2];                                                                  //shared memory name from commandline
    const char *semName = argv[3];                                                                  //semaphore name from commandline
    const int SIZE = 4096;

    //Variable initializations
    int shm_fd;
    char buffer[4];
    int *shm_base;	                                                                            //pointer to shared variable count, need mutual exclusion for access
    int *shm_resp = new int[10];                                                                    //pointer to shared array holding responses

    //Shared memory initialization and Critical Section initialization
    //open shared memory segment with name passed in from commandline
    shm_fd = shm_open(shmName, O_CREAT|O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("observer: Shared memory failed: %s\n", strerror(errno)); exit(1);
    }

    //map shared memory segment in the address space of the process
    shm_base = (int *) mmap(0,2048,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if (shm_base == MAP_FAILED) {
        printf("observer: Map failed: %s\n", strerror(errno)); exit(1);
    }

    //map shared memory segment in the address space of the process (2)
    shm_resp = (int *) mmap(0,2048,PROT_READ|PROT_WRITE,MAP_SHARED,shm_fd,0);
    if (shm_resp == MAP_FAILED) {
        printf("observer: Map failed: %s\n", strerror(errno)); exit(1);
    }

    //structure shared memory segment to struct CLASS
    struct CLASS *count_ptr = (struct CLASS *) shm_base;
    struct CLASS *resp_ptr = (struct CLASS *) shm_resp;

    //create a named semaphore for mutual exclusion
    sem_t *mutex_sem = sem_open( semName, O_CREAT, 0660, 1);
    if (mutex_sem == SEM_FAILED) {
        printf("observer: sem_open failed: %s\n", strerror(errno)); exit(1);
    }


    //Critical Section manipulation
    //lock mutex_sem for critical section
    if (sem_wait(mutex_sem) == -1) {
        printf("observer: sem_wait failed: %s/n", strerror(errno)); exit(1);
    }
    
    if(childNum == 0){
        (resp_ptr -> response)[count_ptr -> count] = 0;
        (count_ptr -> count)++;
    }
    else{
        printf("I am child %d, received shared memory name %s and semaphore name %s\n", childNum, shmName, semName);

        //Variable manipulation
        //get lucky number from user
        printf("Child number %d, What is my lucky number?\n", childNum);
        fgets(buffer, 4, stdin);

        //write childNum to slot y and increment y to y+1
        (resp_ptr -> response)[count_ptr -> count] = childNum;
        (count_ptr -> count)++;

        //write lucky num to slot y+1 and increment y to y+2
        (resp_ptr -> response)[count_ptr -> count] = atoi(buffer);
        (count_ptr -> count)++;
        printf("I have written my child number to slot %d and my lucky number to slot %d, and updated index to %d\n", (count_ptr -> count) - 2, (count_ptr -> count) - 1, (count_ptr -> count));

        printf("Child %d closed access to shared memory and terminates.\n", childNum);
    }

    //exit critical section, release mutex_sem
    if (sem_post(mutex_sem) == -1) {
        printf("observer: sem_post failed: %s\n", strerror(errno)); exit(1);
    }
}