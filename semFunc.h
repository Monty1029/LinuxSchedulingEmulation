/*
Functions modified from programming examples in Chapter 14 of "Beginning Linux Programming 4th Edition"
Modified by: 	Irusha Vidanamadura 	100935300
		Monty Dhanani 		100926543
*/
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef __SEMFUNCTIONS__
#define __SEMFUNCTIONS__


union semun {
    int val;                    /* value for SETVAL */
    struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
    unsigned short int *array;  /* array for GETALL, SETALL */
    struct seminfo *__buf;      /* buffer for IPC_INFO */
};

int set_semvalue(int sem_id,int sem_num, int init_sem_val);
int del_semvalue(int sem_id);
int semaphore_wait(int sem_id,int sem_num);
int semaphore_signal(int sem_id,int sem_num);



#endif /*__SEMFUNCTIONS__*/

