/*
Functions modified from programming examples in Chapter 14 of "Beginning Linux Programming 4th Edition"
Modified by: 	Irusha Vidanamadura 	100935300
		Monty Dhanani 		100926543
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>
#include "semFunc.h"

int set_semvalue(int sem_id,int sem_num, int init_sem_val);
int del_semvalue(int sem_id);
int semaphore_wait(int sem_id,int sem_num);
int semaphore_signal(int sem_id,int sem_num);


/* Sets the initial value of the specified semaphore in the specified set. */

int set_semvalue(int sem_id,int sem_num, int init_sem_val)
{
    union semun sem_union;

    sem_union.val = init_sem_val;
    if (semctl(sem_id, sem_num, SETVAL, sem_union) == -1) return(-1);
    return(1);
}

/* Deletes the semaphore set. */

int del_semvalue(int sem_id)
{
    union semun sem_union;
    
    if (semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
        return -1;
    return 1;
}


/* Imposes the wait command on the specified semaphore. */
int semaphore_wait(int sem_id, int sem_num)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = sem_num;
    sem_b.sem_op = -1; /* P() */
    sem_b.sem_flg = 0;
    if (semop(sem_id, &sem_b, 1) == -1) {
        return(-1);
    }
    return(1);
}


/* Imposes the signal command on the specified semaphore. */
int semaphore_signal(int sem_id,int sem_num)
{
    struct sembuf sem_b;
    
    sem_b.sem_num = sem_num;
    sem_b.sem_op = 1; /* V() */
    sem_b.sem_flg = 0;
    if (semop(sem_id, &sem_b, 1) == -1) {
        return(-1);
    }
    return(1);
}

