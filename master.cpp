#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>   //wait() stuff
#include <semaphore.h>
#include <errno.h>

#include "myShm.h"

//count_ptr->count instead of count_ptr.count

int main (int argc, char* argv[]) {
    //Commandline arguments
    int num_of_children = atoi(argv[1]) + 1;
    const char *shmName = argv[2];                                                                  //shared memory name from commandline
    const char *semName = argv[3];                                                                  //semaphore name from commandline
    const int SIZE = 4096;

    //Variable initializations
    int shm_fd, i;
    int *shm_base;	                                                                                //pointer to shared variable count, need mutual exclusion for access
    int *shm_resp = new int[10];                                                                    //pointer to shared array holding responses
    int child_fin = 0;

    //Text block begins here
    printf("Master begins execution\n");

    //Shared memory initialization and Critical Section initialization
    //open shared memory segment with name passed in from commandline
    shm_fd = shm_open(shmName, O_CREAT|O_RDWR, 0666);
    if (shm_fd == -1) {
        printf("observer: Shared memory failed: %s\n", strerror(errno)); exit(1);
    }
    printf("Master creates a shared memory segment named %s\n", shmName);

    //configure size of shared memory segment
    ftruncate(shm_fd, SIZE);

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

    //Initialize shared count
    count_ptr -> count = 0;
    printf("Master initialized index in the shared structure to zero\n");

    //open a named semaphore for mutual exclusion
    sem_t *mutex_sem = sem_open( semName, O_CREAT, 0660, 1);
    if (mutex_sem == SEM_FAILED) {
        printf("observer: sem_open failed: %s\n", strerror(errno)); exit(1);
    }
    printf("Master created a semaphore named %s\n", semName);

    //create n child processes
    printf("Master created %d child processes to execute slave\n", num_of_children - 1);
    printf("Master waits for all child processes to terminate\n\n");
    for(int i = 0; i < num_of_children; i++){
        int child_num = i;
        char child_num_pass[4];
        snprintf(child_num_pass, sizeof(child_num), "%d", child_num);                                           //allow int to be passed into execl as a string

        pid_t pid = fork();                                                                                     //fork() child process
        if(pid == 0){
            //child executes
            execl("./slave", "./slave", child_num_pass, shmName, semName, NULL);
        }
        
        if(pid > 0){
            wait(NULL);
            child_fin++;                                                                                        //keep track of child processes
            if(child_fin == num_of_children){                                                                   //if all children are finished
                //all child processes finished
                printf("\nMaster received termination signals from all %d child processes\n", num_of_children - 1);
                printf("Updated context of shared memory segment after access by child processes: \n");
                (count_ptr->count)--;
                for(int i = 1; i < (num_of_children * 2) - 1; i++){
                    //text block for shared data
                    printf("%s[%d]: %d\n", shmName, count_ptr->count, (resp_ptr->response)[count_ptr->count]);
                    (count_ptr->count)--;
                }
            }
        }
    }


    //Close and free resources attached to shm and semaphores
    //done with semaphore, close it & free up resources allocated to it
    if (sem_close(mutex_sem) == -1) {
        printf("observer: sem_close failed: %s\n", strerror(errno)); exit(1);
    }

    //request to remove the named semaphore, but action won't take place until all references to it have ended
    if (sem_unlink(semName) == -1) {
        printf("observer: sem_unlink failed: %s\n", strerror(errno)); exit(1);
    }
    printf("Master removed the semaphore\n");

    //remove mapped memory segment from the address space
    if (munmap(shm_base,2048) == -1) {
        printf("observer: Unmap 1 failed: %s\n", strerror(errno)); exit(1);
    }
    if (munmap(shm_resp,2048) == -1) {
        printf("observer: Unmap 2 failed: %s\n", strerror(errno)); exit(1);
    }

    //close shared memory segment
    if (close(shm_fd) == -1) {
        printf("observer: Close failed: %s\n", strerror(errno)); exit(1);
    }

    //remove shared memory segment from the file system
    if (shm_unlink(shmName) == -1) {
        printf("observer: Error removing %s: %s\n", shmName, strerror(errno)); exit(1);
    }
    printf("Master closed access to shared memory, removed shared memory segment, and is exiting.\n");

    return 0;
}
