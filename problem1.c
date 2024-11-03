#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <sys/shm.h>		// This is necessary for using shared memory constructs
#include <semaphore.h>		// This is necessary for using semaphore
#include <fcntl.h>			// This is necessary for using semaphore
#include <pthread.h>        // This is necessary for Pthread          
#include <string.h>
#include <math.h>
#include "utils.h"
#define VAR_ACCESS_SEMAPHORE "/var_access_semaphore"

long int global_var = 0;

// this struct is used to pass parameters to the thread function
typedef struct {
    int thread_id;
    sem_t* semaphores[9];
    long int *shared_var;
} thread_params;

/**
* This function should be implemented by yourself. It must be invoked
* in the child process after the input parameter has been obtained.
* @parms: The input parameter from the terminal.
*/
void multi_threads_run(long int input_param);

// The function define the thread function, which is used to modify the shared variable.
void* thread_function(void* args);


int main(int argc, char **argv)
{
	int shmid, status;
	long int local_var = 0;
	long int *shared_var_p, *shared_var_c;

	if (argc < 2) { 
		printf("Please enter a nine-digit decimal number and the number of operations as input parameters.\nUsage: ./main <input_param> <num_of_operations>\n");
		exit(-1);
	}
	
	// write the number of operations to gobal variable
	global_var = strtol(argv[2], NULL, 10);

   	/*
		Creating semaphores. Mutex semaphore is used to acheive mutual
		exclusion while processes access (and read or modify) the global
		variable, local variable, and the shared memory.
	*/ 

	// Checks if the semaphore exists, if it exists we unlink him from the process.
	sem_unlink(VAR_ACCESS_SEMAPHORE);
	
	// Create the semaphore. sem_init() also creates a semaphore. Learn the difference on your own.
	sem_t *var_access_semaphore = sem_open(VAR_ACCESS_SEMAPHORE, O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);

	// Check for error while opening the semaphore
	if (var_access_semaphore != SEM_FAILED){
		printf("Successfully created new semaphore!\n");
	}	
	else if (errno == EEXIST) {   // Semaphore already exists
		printf("Semaphore appears to exist already!\n");
		var_access_semaphore = sem_open(VAR_ACCESS_SEMAPHORE, 0);
	}
	else {  // An other error occured
		assert(var_access_semaphore != SEM_FAILED);
		exit(-1);
	}

	/*  
	    Creating shared memory. 
        The operating system keeps track of the set of shared memory
	    segments. In order to acquire shared memory, we must first
	    request the shared memory from the OS using the shmget()
      	system call. The second parameter specifies the number of
	    bytes of memory requested. shmget() returns a shared memory
	    identifier (SHMID) which is an integer. Refer to the online
	    man pages for details on the other two parameters of shmget()
	*/
	// create shared memory to store a long int and an int
	shmid = shmget(IPC_PRIVATE, sizeof(long int), 0666 | IPC_CREAT);

	/* 
	    After forking, the parent and child must "attach" the shared
	    memory to its local data segment. This is done by the shmat()
	    system call. shmat() takes the SHMID of the shared memory
	    segment as input parameter and returns the address at which
	    the segment has been attached. Thus shmat() returns a char
	    pointer.
	*/

	if (fork() == 0) { // Child Process
        
		printf("Child Process: Child PID is %jd\n", (intmax_t) getpid());
		
		/*  shmat() returns a long int pointer which is typecast here
		    to long int and the address is stored in the long int pointer shared_var_c. */
        shared_var_c = (long int *) shmat(shmid, 0, 0);

		while (1) // Loop to check if the variables have been updated.
		{
			// Get the semaphore
			sem_wait(var_access_semaphore);
			printf("Child Process: Got the variable access semaphore.\n");

			if ( (global_var != 0) || (local_var != 0) || (shared_var_c[0] != 0) )
			{
				printf("Child Process: Read the global variable with value of %ld.\n", global_var);
				printf("Child Process: Read the local variable with value of %ld.\n", local_var);
				printf("Child Process: Read the shared variable with value of %ld.\n", shared_var_c[0]);

                // Release the semaphore
                sem_post(var_access_semaphore);
                printf("Child Process: Released the variable access semaphore.\n");
                
				break;
			}

			// Release the semaphore
			sem_post(var_access_semaphore);
			printf("Child Process: Released the variable access semaphore.\n");
		}

        /**
         * After you have fixed the issue in Problem 1-Q1, 
         * uncomment the following multi_threads_run function 
         * for Problem 1-Q2. Please note that you should also
         * add an input parameter for invoking this function, 
         * which can be obtained from one of the three variables,
         * i.e., global_var, local_var, shared_var_c[0].
         */
		multi_threads_run(shared_var_c[0]);

		/* each process should "detach" itself from the 
		   shared memory after it is used */

		shmdt(shared_var_c);

		exit(0);
	}
	else { // Parent Process

		printf("Parent Process: Parent PID is %jd\n", (intmax_t) getpid());

		/*  shmat() returns a long int pointer which is typecast here
		    to long int and the address is stored in the long int pointer shared_var_p.
		    Thus the memory location shared_var_p[0] of the parent
		    is the same as the memory locations shared_var_c[0] of
		    the child, since the memory is shared.
		*/
		shared_var_p = (long int *) shmat(shmid, 0, 0);

		// Get the semaphore first
		sem_wait(var_access_semaphore);
		printf("Parent Process: Got the variable access semaphore.\n");

		global_var = strtol(argv[1], NULL, 10);
		local_var = strtol(argv[1], NULL, 10);
		shared_var_p[0] = strtol(argv[1], NULL, 10);

		// Release the semaphore
		sem_post(var_access_semaphore);
		printf("Parent Process: Released the variable access semaphore.\n");
        
		wait(&status);

		/* each process should "detach" itself from the 
		   shared memory after it is used */

		shmdt(shared_var_p);

		/* Child has exited, so parent process should delete
		   the cretaed shared memory. Unlike attach and detach,
		   which is to be done for each process separately,
		   deleting the shared memory has to be done by only
		   one process after making sure that noone else
		   will be using it 
		 */

		shmctl(shmid, IPC_RMID, 0);

        // Close and delete semaphore. 
        sem_close(var_access_semaphore);
        sem_unlink(VAR_ACCESS_SEMAPHORE);

		exit(0);
	}

	exit(0);
}

void* thread_function(void* args) {
    thread_params* params = (thread_params*)args;
    int thread_id = params->thread_id;
	int num_of_operations = global_var;
	
	//printf("num_of_operations: %d\n", num_of_operations);
    int first_digit = thread_id;
    int second_digit = (thread_id + 1) % 9;

    for (int i = 0; i < num_of_operations; ++i) { 
        // lock 2 semaphores
        sem_wait(params->semaphores[first_digit]);
        sem_wait(params->semaphores[second_digit]);
		//printf("Thread %d: Started\n", thread_id+1);

        // read the two digits
		int digit1 = params->shared_var[first_digit];
		int digit2 = params->shared_var[second_digit];
		
		int prev_digit1 = digit1;
		int prev_digit2 = digit2;

		// calculate increment
		int increment = (thread_id + 1) & (digit1 + digit2);

		// update the two digits
		digit1 = (digit1 + increment) % 10;
		digit2 = (digit2 + increment) % 10;

		// write the two digits back
		params->shared_var[first_digit] = digit1;
		params->shared_var[second_digit] = digit2;

        printf("Thread %d: Modified digits[%d] and digits[%d] from %d and %d to %d and %d\n", thread_id+1, first_digit+1, second_digit+1, prev_digit1, prev_digit2, digit1, digit2);

        // unlock 2 semaphores
        sem_post(params->semaphores[first_digit]);
        sem_post(params->semaphores[second_digit]);
    }

    pthread_exit(NULL);
}
/**
* This function should be implemented by yourself. It must be invoked
* in the child process after the input parameter has been obtained.
* @parms: The input parameter from terminal.
*/
void multi_threads_run(long int input_param)
{
	pthread_t threads[9];
    sem_t* semaphores[9];
    thread_params params[9];
    long int shared_var[9];

    // initialize shared_var
    for (int i = 0; i < 9; ++i) {
        shared_var[i] = (input_param / (long int)pow(10, 9 - 1 - i)) % 10;
		printf("shared_var[%d]: %ld\n", i, shared_var[i]);
    }

    // create semaphores
    for (int i = 0; i < 9; ++i) {
        char sem_name[20];
		sprintf(sem_name, "/sem_%d", i); // create a unique name for the semaphore
		sem_unlink(sem_name);            //guarantees that the semaphore is destroyed when the program exits
		semaphores[i] = sem_open(sem_name, O_CREAT, 0644, 1);
		if (semaphores[i] == SEM_FAILED) {
			perror("sem_open failed");
			exit(EXIT_FAILURE);
		}
    }

    // create threads
    for (int i = 0; i < 9; ++i) {
        params[i].thread_id = i;
        params[i].shared_var = shared_var;
        memcpy(params[i].semaphores, semaphores, sizeof(semaphores));// pass the semaphores to the thread function
        pthread_create(&threads[i], NULL, thread_function, (void*)&params[i]);
    }

    // wait for threads to finish
    for (int i = 0; i < 9; ++i) {
        pthread_join(threads[i], NULL);
    }
	
	for (int i = 0; i < 9; ++i) {
        printf("Final shared_var[%d]: %ld\n", i+1, shared_var[i]);
    }

    // output the final result
    long int result = 0;
    for (int i = 0; i < 9; ++i) {
        result = result * 10 + shared_var[i];
    }
	// call the function to save the result to a file
    saveResult("p1_result.txt", result);
    printf("Final result saved to p1_result.txt\n");
    printf("Final result: %ld\n", result);

    // close and unlink semaphores
    for (int i = 0; i < 9; ++i) {
        sem_close(semaphores[i]);
        char sem_name[20];
        sprintf(sem_name, "sem_%d", i);
        sem_unlink(sem_name);
    }

}