/* myShm.h */
/* Header file to be used with master.c and slave.c
*/

#include <semaphore.h>

/*
struct CLASS {
    int index;              //index to next available response slot 
    int response[10];       //each child writes its child number & lucky number here
*/

struct CLASS {
	sem_t mutex_sem; /* enforce mutual exclusion for accessing count */
	int count;		 /*shared variable, need mutual exclusion*/
    int response[10];
};