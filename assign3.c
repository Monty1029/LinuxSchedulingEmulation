/*
SYSC 4001 - Assignment 3
Linux Scheduling Emulation
Authors:	Irusha Vidanamadura 	100935300
		Monty Dhanani 		100926543
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <malloc.h>
#include <sys/types.h>
#include <linux/sched.h>
#include <sys/time.h>
#include "assign3.h"



/*	Function Prototypes 	*/
void producer_thread_function(void *arg);
void consumer_thread_function(void *arg);
void execute_process(process_info *runningProcess);
//void remove_process(process_info *removeProcess);
int max_value(int a, int b);
int min_value(int a, int b);


/*	Global Variables 	*/
static int max_priority;
static int min_priority;
unsigned int processes_completed = 0;
bool finished = FALSE;


bool min_to_start = FALSE;

/*	Mutexes for each Producer Consumer buffer */
pthread_mutex_t producer_consumer_mutex [NUM_CONSUMER_THREADS];

/*	Array for the 4 producer-consumer buffers	*/
process_info producer_consumer_buffer [QUEUE_LENGTH*NUM_CONSUMER_THREADS];

/*	Mutexes for each CPU		*/
pthread_mutex_t consumer_state_mutex[NUM_CONSUMER_THREADS];


/*	Array for consumer cpu state	*/
cpu_info consumer_state [NUM_CONSUMER_THREADS];



int main(){

	int response;
	int sem_num = 0;
	pthread_t consumer_thread[NUM_CONSUMER_THREADS];
	void *thread_result = 0;
	pthread_attr_t thread_attr;
	int temp_index;


	struct sched_param scheduling_value;




	/*	RR Scheduling policy thread attribute creation 	*/
	max_priority = sched_get_priority_max(SCHED_RR);
	min_priority = sched_get_priority_min(SCHED_RR);

	response = pthread_attr_init(&thread_attr);
	printf("Create: %d\n",response);
	if (response != 0) {
		perror("Attribute creation failed\n");
		exit(EXIT_FAILURE);
	}
	response = pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
	printf("Policy Set %d\n",response);
	if (response != 0) {
		perror("Setting schedpolicy failed");
		exit(EXIT_FAILURE);
	}
	response = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	printf("Set attribute %d\n",response);
	if (response != 0) {
		perror("Setting detached attribute failed\n");
		exit(EXIT_FAILURE);
	}
	/***************************************/


	/*	Initializing thread and consumer_state mutexes 	*/
	for(temp_index = 0; temp_index < NUM_CONSUMER_THREADS; temp_index++){

		response = pthread_mutex_init(&(producer_consumer_mutex[temp_index]), NULL);
		if (response != 0) {
			perror("Mutex initialization failed");
			exit(EXIT_FAILURE);
		}
		response = pthread_mutex_init(&(consumer_state_mutex[temp_index]), NULL);
		if (response != 0) {
			perror("Mutex initialization failed");
			exit(EXIT_FAILURE);
		}
	}
	/***************************************/


	/* 	Creating the producer		*/
	response = pthread_create(&(consumer_thread[0]), NULL, (void*) &producer_thread_function,NULL);
	if (response != 0) {
		perror("Thread creation failed\n");
		exit(EXIT_FAILURE);
	}

	scheduling_value.sched_priority = min_priority;
	response = pthread_attr_setschedparam(&thread_attr, &scheduling_value);
	if (response != 0) {
		perror("Setting scheduling priority failed\n");
		exit(EXIT_FAILURE);
	}
	/***************************************/




	/* 	Creating the 4 consumers	*/
	for(temp_index = 0; temp_index < NUM_CONSUMER_THREADS; temp_index++) {

		response = pthread_create(&consumer_thread[temp_index], NULL, (void*) &consumer_thread_function, (void *)&temp_index);
		if (response != 0) {
			perror("Consumer Thread creation failed\n");
			exit(EXIT_FAILURE);
		}

		scheduling_value.sched_priority = temp_index+1;
		response = pthread_attr_setschedparam(&thread_attr, &scheduling_value);
		if (response != 0) {
			perror("Setting Consumer scheduling priority failed\n");
			exit(EXIT_FAILURE);
		}
		usleep(1);
	}
	/***************************************/


	/*
	printf("max priorityls: %d\n", max_priority);
	printf("min priority: %d\n", min_priority);

	pthread_attr_destroy(&thread_attr);
	while(!thread_finished) {
		printf("Waiting for thread to say it's finished...\n");
		usleep(1);
	}
	*/
	while(!finished);
	printf("Other thread finished, bye!\n");
	exit(EXIT_SUCCESS);

}




/*
 *	Name:producer_thread_function
 *	Input Arguments:
 *		None
 *	Function: Creates process info and stores it in
 *
 */
void producer_thread_function(void *arg) {
	int temp_index;
	int temp_random_buffer;
	int buffer_index = 0;

	process_info *process_creation = malloc(sizeof(process_info));

	for (temp_index=0; temp_index<NUM_PROCESSES; temp_index++) {
		printf("%d\n",temp_index);
		/*	Generation of new Process info	*/
		process_creation-> pid = temp_index+1;

		if (temp_index < 4) {
			process_creation->policy = SCHED_FIFO;
			process_creation->static_prio = rand()%98 + 1;
		}
		else if (temp_index >= 4 && temp_index < 8) {
			process_creation->policy = SCHED_RR;
			process_creation->static_prio = rand() % 98+1;
			if (temp_index == 5) {
				min_to_start = TRUE;
			}
		}
		else {
			process_creation->policy = SCHED_OTHER;
			process_creation->static_prio = rand() % 40 + 100;
		}
		process_creation->time_slice = 1;
		process_creation->sum_exec_runtime = (rand() % 50 + 5);
		process_creation->completed = FALSE;

		/***************************************/
		printf("\tPRODUCER: Process %d\tPriority %d\tSched:%d\tExec T:%d\n",
				process_creation-> pid,
				process_creation->static_prio,
				process_creation->policy,
				process_creation->sum_exec_runtime
				);

		/*	Storing of new Process info	*/
		pthread_mutex_lock(&(producer_consumer_mutex[temp_index%NUM_CONSUMER_THREADS]));
		printf("\tPRODUCER: Process %d info Stored to position %d !\n",process_creation-> pid,(temp_index*QUEUE_LENGTH)%28 +buffer_index);
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].pid= process_creation->pid;
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].policy= process_creation->policy;
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].static_prio= process_creation->static_prio;
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].time_slice= process_creation->time_slice;
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].sum_exec_runtime= process_creation->sum_exec_runtime;
		producer_consumer_buffer[(temp_index*QUEUE_LENGTH)%28+buffer_index].completed= process_creation->completed;
		pthread_mutex_unlock(&(producer_consumer_mutex[temp_index%NUM_CONSUMER_THREADS]));
		pthread_mutex_destroy(&(producer_consumer_mutex[temp_index%NUM_CONSUMER_THREADS]));
		/***************************************/
		if (temp_index%NUM_CONSUMER_THREADS == 0) {
			buffer_index++;
		}

	}
	while(!finished);
	pthread_exit(NULL);

}





void consumer_thread_function(void *arg) {

	int my_thread_id = *(int *)arg;

	unsigned int temp_index = 0;
	unsigned int current_priority = 139;
	unsigned int highest_priority = 139;

	int selected_process_index = -1;

	unsigned int ready_queue_0_size = 0;
	unsigned int ready_queue_1_size = 0;
	unsigned int ready_queue_2_size = 0;

	process_info selected_process;

	bool process_execution = FALSE;


	while (!min_to_start);

	while(!finished){
		process_execution = FALSE;

		/*	Selecting the process with the highest priority		*/
		pthread_mutex_lock(&(producer_consumer_mutex[my_thread_id]));
		for (temp_index= 0; temp_index < QUEUE_LENGTH; temp_index++) {
			current_priority = producer_consumer_buffer[my_thread_id*QUEUE_LENGTH+temp_index].static_prio;
			if (current_priority < highest_priority &&
					current_priority !=0 &&
					producer_consumer_buffer[my_thread_id*QUEUE_LENGTH+temp_index].serviced == FALSE) {
				highest_priority = current_priority;
				selected_process_index = temp_index;
			}
		}

		selected_process.pid  		= producer_consumer_buffer[selected_process_index].pid;
		selected_process.policy  	= producer_consumer_buffer[selected_process_index].policy;
		selected_process.static_prio  	= producer_consumer_buffer[selected_process_index].static_prio;
		selected_process.time_slice 	= producer_consumer_buffer[selected_process_index].time_slice;
		selected_process.sum_exec_runtime  = producer_consumer_buffer[selected_process_index].sum_exec_runtime;
		selected_process.completed  	= producer_consumer_buffer[selected_process_index].completed;
		selected_process.time_slice  	= producer_consumer_buffer[selected_process_index].time_slice;
		producer_consumer_buffer[selected_process_index].serviced = TRUE;

		pthread_mutex_unlock(&(producer_consumer_mutex[my_thread_id]));
		pthread_mutex_destroy(&(producer_consumer_mutex[my_thread_id]));
		/***************************************/



		/*	Storing the selected process into appropriate ready queue		*/
		pthread_mutex_lock(&(consumer_state_mutex[my_thread_id]));
		if(highest_priority <= READY_QUEUE_0_MAX_PRIORITY && ready_queue_0_size<QUEUE_LENGTH &&	selected_process.completed == FALSE ){
			printf("CONSUMER %d: Process %d selected added to ready queue 0\r\n",my_thread_id, selected_process.pid);

			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].pid = selected_process.pid;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].policy = selected_process.policy;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].static_prio = selected_process.static_prio;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].sum_exec_runtime = selected_process.sum_exec_runtime;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].completed = selected_process.completed;
			consumer_state[my_thread_id].rq0[ready_queue_0_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			ready_queue_0_size++;

		}else if(highest_priority <= READY_QUEUE_1_MAX_PRIORITY && ready_queue_1_size<QUEUE_LENGTH  && selected_process.completed == FALSE){
			printf("CONSUMER %d: Process %d selected added to ready queue 1\r\n",my_thread_id, selected_process.pid);
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].pid = selected_process.pid;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].policy = selected_process.policy;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].static_prio = selected_process.static_prio;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].sum_exec_runtime = selected_process.sum_exec_runtime;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].completed = selected_process.completed;
			consumer_state[my_thread_id].rq1[ready_queue_1_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			ready_queue_1_size++;
		}
		else if(highest_priority < READY_QUEUE_2_MAX_PRIORITY && ready_queue_2_size<QUEUE_LENGTH  && selected_process.completed == FALSE){
			printf("CONSUMER %d: Process %d selected added to ready queue 2\r\n",my_thread_id, selected_process.pid);
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].pid = selected_process.pid;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].policy = selected_process.policy;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].static_prio = selected_process.static_prio;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].sum_exec_runtime = selected_process.sum_exec_runtime;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].completed = selected_process.completed;
			consumer_state[my_thread_id].rq2[ready_queue_2_size%QUEUE_LENGTH].time_slice = selected_process.time_slice;
			ready_queue_2_size++;
		}
		pthread_mutex_unlock(&(consumer_state_mutex[my_thread_id]));
		pthread_mutex_destroy(&(consumer_state_mutex[my_thread_id]));

		/***************************************/

		/*	Executing the first process in the appropriate ready queue*/

		for(temp_index = 0; temp_index < QUEUE_LENGTH*QUEUE_TYPES; temp_index++ ){

			pthread_mutex_lock(&(consumer_state_mutex[my_thread_id]));

			if( temp_index <QUEUE_LENGTH &&	consumer_state[my_thread_id].rq0[temp_index%QUEUE_LENGTH].pid !=  0 && consumer_state[my_thread_id].rq0[temp_index%QUEUE_LENGTH].completed != TRUE){

				printf("CONSUMER %d: Process %d selected for execution with policy %d and expected executed time of %d \r\n",
						my_thread_id,
						(int) (consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].pid,
						(int) (consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].policy,
						(int) (consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].sum_exec_runtime);
				(consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].last_CPU = my_thread_id;
				execute_process( &consumer_state[my_thread_id].rq0[temp_index%QUEUE_LENGTH]  );
				consumer_state[my_thread_id].rq0[temp_index%QUEUE_LENGTH].completed = TRUE;

			}else if( temp_index < QUEUE_LENGTH*2 && consumer_state[my_thread_id].rq1[temp_index%QUEUE_LENGTH].pid !=  0 &&	consumer_state[my_thread_id].rq1[temp_index%QUEUE_LENGTH].completed != TRUE ){
				printf("CONSUMER %d: Process %d selected for execution with policy %d and expected executed time of %d \r\n",
						my_thread_id,
						(int) (consumer_state[my_thread_id].rq1)[temp_index%QUEUE_LENGTH].pid,
						(int) (consumer_state[my_thread_id].rq1)[temp_index%QUEUE_LENGTH].policy,
						(int) (consumer_state[my_thread_id].rq1)[temp_index%QUEUE_LENGTH].sum_exec_runtime);

				(consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].last_CPU = my_thread_id;
				execute_process(  &consumer_state[my_thread_id].rq1[temp_index%QUEUE_LENGTH]  );


			}else if(temp_index < QUEUE_LENGTH*3 && consumer_state[my_thread_id].rq2[temp_index%QUEUE_LENGTH].pid !=  0 && consumer_state[my_thread_id].rq2[temp_index%QUEUE_LENGTH].completed != TRUE ){
				printf("CONSUMER %d: Process %d selected for execution with policy %d and expected executed time of %d \r\n",
						my_thread_id,
						(int) (consumer_state[my_thread_id].rq2)[temp_index%QUEUE_LENGTH].pid,
						(int) (consumer_state[my_thread_id].rq2)[temp_index%QUEUE_LENGTH].policy,
						(int) (consumer_state[my_thread_id].rq2)[temp_index%QUEUE_LENGTH].sum_exec_runtime);

				(consumer_state[my_thread_id].rq0)[temp_index%QUEUE_LENGTH].last_CPU = my_thread_id;
				execute_process(  &consumer_state[my_thread_id].rq2[temp_index%QUEUE_LENGTH]  );

			}
			usleep(1);
			pthread_mutex_unlock(&(consumer_state_mutex[my_thread_id]));
			pthread_mutex_destroy(&(consumer_state_mutex[my_thread_id]));
		}
	}

	pthread_exit(NULL);
}



void execute_process(process_info *runningProcess){
	unsigned long int time_clicks;
	struct timeval diff_t;
	unsigned int accumulated_execution_runtime;
	unsigned int iterations;
	printf("\t\tExecuting #%d!!!\n",runningProcess->pid);

	runningProcess->sleep_avg += runningProcess->last_time_click;
	runningProcess->dynamic_priority = max_value(100, min_value(runningProcess->previous_priority - (unsigned int) runningProcess->sleep_avg + 5, 139));
	runningProcess->previous_priority = runningProcess->dynamic_priority;
	
	//Executes program in FIFO fashion
	if (runningProcess->policy == SCHED_FIFO && runningProcess->completed != TRUE) {
		printf("\t\tCONSUMER: %d PROCESS %d: Scheduling policy: FIFO\n",runningProcess->last_CPU,runningProcess->pid);
		printf("\t\tCONSUMER: %d PROCESS %d: Expected execution runtime: %d \n",runningProcess->last_CPU, runningProcess->pid,runningProcess->sum_exec_runtime);

		//Record start time
		gettimeofday(&(runningProcess->start_t), NULL);
		usleep(runningProcess->sum_exec_runtime);
		gettimeofday(&(runningProcess->stop_t), NULL);
		runningProcess->completed = TRUE;
		runningProcess->serviced = TRUE;
		processes_completed++;

	}
	//Executes program in ROUND ROBIN fashion
	else if (runningProcess->policy == SCHED_RR && runningProcess->completed != TRUE) {
		printf("\t\tPROCESS %d: Scheduling policy: Round Robin\n",runningProcess->pid);
		printf("\t\tPROCESS %d: Static Priority: %d \n", runningProcess->pid,runningProcess->static_prio);
		printf("\t\tPROCESS %d: Accumulated execution runtime: %d \n", runningProcess->pid, runningProcess->acc_exec_runtime);
		printf("\t\tPROCESS %d: Expected execution runtime: %d \n", runningProcess->pid, runningProcess->sum_exec_runtime);
		printf("\t\tPROCESS %d: Iteration number: %d \n", runningProcess->pid, runningProcess->iterations);

		usleep(runningProcess->time_slice);
		runningProcess->acc_exec_runtime += runningProcess->time_slice;
		runningProcess->iterations+=1;
		if (runningProcess->acc_exec_runtime >= runningProcess->sum_exec_runtime) {
			gettimeofday(&(runningProcess->stop_t), NULL);
			runningProcess->completed = TRUE;
			processes_completed++;
		}

	}
	//Executes program in OTHER/NORMAL (COMPLETELY FAIR SHARE) fashion
	else if(runningProcess->policy == SCHED_OTHER && runningProcess->completed != TRUE) {
		printf("\t\tPROCESS %d: Scheduling policy: Completely Fair Share\n", runningProcess->pid);
		printf("\t\tPROCESS %d: Static Priority: %d \n", runningProcess->pid, runningProcess->static_prio);
		printf("\t\tPROCESS %d: Accumulated execution runtime: %d \n", runningProcess->pid, runningProcess->acc_exec_runtime);
		printf("\t\tPROCESS %d: Expected execution runtime: %d \n", runningProcess->pid, runningProcess->sum_exec_runtime);
		printf("\t\tPROCESS %d: Iteration number: %d \n", runningProcess->pid, runningProcess->iterations);
		printf("\t\tPROCESS %d: Sleep average: %li \n", runningProcess->pid, runningProcess->sleep_avg);
		printf("\t\tPROCESS %d: Last Time Click: %li \n", runningProcess->pid, runningProcess->last_time_click);
		printf("\t\tPROCESS %d: Dynamic Priority: %d \n", runningProcess->pid, runningProcess->dynamic_priority);
		printf("\t\tPROCESS %d: Previous Priority: %d \n", runningProcess->pid, runningProcess->previous_priority);
		printf("\t\tPROCESS %d: Service time per iteration: %d \n", runningProcess->pid, runningProcess->serv_time_it);

		runningProcess->iterations++;
		if (runningProcess->static_prio < 120) {
			usleep((140-runningProcess->static_prio)*20);
		}
		else{
			usleep((140-runningProcess->static_prio)*5);
		}

		runningProcess->acc_exec_runtime += runningProcess->time_slice;
		accumulated_execution_runtime = runningProcess->acc_exec_runtime;
		runningProcess->iterations = runningProcess->iterations;
		if (runningProcess->acc_exec_runtime >= runningProcess->sum_exec_runtime) {
			//Record end time
			gettimeofday(&(runningProcess->stop_t), NULL);
			runningProcess->completed = TRUE;
			processes_completed++;
			runningProcess->sleep_avg = (accumulated_execution_runtime/runningProcess->iterations);
			if (runningProcess->sleep_avg >	10){
				runningProcess->sleep_avg = 10;
			}
		}

	}

	if (processes_completed == 30) {
		finished = TRUE;
	}

	if(runningProcess->completed){
		timersub(&(runningProcess->stop_t), &(runningProcess->start_t), &diff_t);
		time_clicks = (long int) diff_t.tv_usec/100.0;
		runningProcess->sleep_avg -= time_clicks;
		runningProcess->last_time_click = time_clicks;
		printf("CONSUMER %d PROCESS %d: Time elapsed: %f\n", runningProcess->last_CPU,runningProcess->pid, (long int)diff_t.tv_sec*1000.0+((long int)diff_t.tv_usec/1000.0));
		runningProcess->serv_time_it = rand() % 1000;
		printf("******NUMBER OF PROCESSES FINISHED: %d\n",processes_completed);
	}
}

/*	Returns the max and min values for comparison	*/
int max_value(int a, int b){
	if(a<b){
		return b;
	}
	return a;
}

int min_value(int a, int b){
	if(a>b){
		return b;
	}
	return a;
}

