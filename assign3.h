/*
SYSC 4001 - Assignment 3
Linux Scheduling Emulation
Authors:	Irusha Vidanamadura 	100935300
		Monty Dhanani 		100926543
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/msg.h>

#define NUM_CONSUMER_THREADS 4
#define NUM_PROCESSES 20
#define QUEUE_TYPES 3
#define QUEUE_LENGTH 7

#define READY_QUEUE_0 0
#define READY_QUEUE_1 1
#define READY_QUEUE_2 2

#define READY_QUEUE_0_MAX_PRIORITY 99
#define READY_QUEUE_1_MAX_PRIORITY 130
#define READY_QUEUE_2_MAX_PRIORITY 140

#define MAX_SLEEP_AVG 10

#define TRUE 1
#define FALSE 0


//Structure that holds information on the process that is to be executed.
typedef struct task_info{
	bool completed; //Checks if the process has finished executing
	unsigned int pid;
	unsigned int static_prio;
	unsigned int policy; //0 is OTHER/NORMAL (COMPLETELY FAIR SHARE), 1 is FIFO, 2 is ROUND ROBIN
	unsigned int time_slice;
	unsigned int sum_exec_runtime; //Expected execution runtime
	unsigned int acc_exec_runtime; //Accumulated execution runtime
	unsigned int iterations; //Number of iterations the process has done
	unsigned int dynamic_priority;
	unsigned int previous_priority;
	unsigned int serv_time_it; //Service time per iteration
	unsigned int last_CPU; //Last CPU that executed the process
	unsigned int serviced;
	unsigned long int sleep_avg; //Average sleeping time
	unsigned long int last_time_click; //Time clicks on previous run
	struct timeval stop_t; //Start time of execution of process
	struct timeval start_t; //End time of execution of process
}process_info;

//Structure that holds information about the queues in each CPU (queue of processes, and queue size)
typedef struct cpu_thread_info{

	unsigned int rq0_size;
	unsigned int rq1_size;
	unsigned int rq2_size;
	process_info rq0[QUEUE_LENGTH];
	process_info rq1[QUEUE_LENGTH];
	process_info rq2[QUEUE_LENGTH];
}cpu_info;

