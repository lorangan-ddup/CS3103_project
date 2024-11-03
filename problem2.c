#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>

#include "utils.h"

#define SHM_SIZE 1048576 // 1MB buffer size for shared memory
#define MAX_FILES 100 // Maximum number of text files
#define VAR_WRITE_SEMAPHORE "/write_semaphore"
#define VAR_READ_SEMAPHORE "/read_semaphore"

// Global variables to store file paths
char *file_paths[MAX_FILES];
int file_count = 0;

int countLongWords(char *text);

/**
 * @brief This function recursively traverse the source directory.
 * 
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name);

int main(int argc, char **argv) {
	int process_id; // Process identifier 

	int shmid;
    char *shared_mem;
    sem_t *write_semaphore, *read_semaphore;
    // The source directory. 
    // It can contain the absolute path or relative path to the directory.
	char *dir_name = argv[1];

	if (argc < 2) {
		printf("Main process: Please enter a source directory name.\nUsage: ./main <dir_name>\n");
		exit(-1);
	}

	traverseDir(dir_name);

    /////////////////////////////////////////////////
    // You can add some code here to prepare before fork.

	// Initialize semaphore for mutual exclusion
    sem_unlink(VAR_WRITE_SEMAPHORE);
    sem_unlink(VAR_READ_SEMAPHORE);
    write_semaphore = sem_open(VAR_WRITE_SEMAPHORE, O_CREAT | O_EXCL, 0644, 1);
    read_semaphore = sem_open(VAR_READ_SEMAPHORE, O_CREAT | O_EXCL, 0644, 0);
    if (write_semaphore == SEM_FAILED || read_semaphore == SEM_FAILED) {
        perror("sem_open failed");
        exit(EXIT_FAILURE);
    }
    // Create shared memory segment
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(EXIT_FAILURE);
    }
    /////////////////////////////////////////////////


	switch (process_id = fork()) {

	default:
		/*
			Parent Process
		*/
		printf("Parent process: My ID is %jd\n", (intmax_t) getpid());
        
        /////////////////////////////////////////////////
        // Implement your code for parent process here.
		shared_mem = (char *)shmat(shmid, NULL, 0);
		if (shared_mem == (char *) -1) {
			perror("shmat failed");
			exit(EXIT_FAILURE);
		}

		
		for (int i = 0; i < file_count; ++i) {
			sem_wait(write_semaphore); // wait for child to read content

			FILE *file = fopen(file_paths[i], "r");
			printf("Parent process: Reading file %s\n", file_paths[i]);
			if (!file) {
				perror("File open failed");
				continue;
			}
			
			int read_size;
			int end_of_file = 0;

			while ((read_size = fread(shared_mem, 1, SHM_SIZE, file)) > 0) {
				if (read_size < SHM_SIZE) {
					end_of_file = 1; // mark end of file
				}
				
				// Clear the rest of the buffer
				memset(shared_mem + read_size, 0, SHM_SIZE - read_size);

				printf("Parent process: Written part of file %s to shared memory.\n", file_paths[i]);

				sem_post(read_semaphore); 
				sem_wait(write_semaphore); 
			}

			// Check if the file has been read completely
			if (end_of_file) {
				strcpy(shared_mem, "EOF");
				sem_post(read_semaphore); // notify child that file has been read
			}

			fclose(file);
		}

		

		// Ensure the child process has finished reading the last file
		sem_wait(write_semaphore); 

		wait(NULL); // wait for child process to finish

		// Detach and delete shared memory
		shmdt(shared_mem);
		shmctl(shmid, IPC_RMID, NULL);

		sem_close(write_semaphore);
		sem_close(read_semaphore);
		sem_unlink(VAR_WRITE_SEMAPHORE);
		sem_unlink(VAR_READ_SEMAPHORE);
        /////////////////////////////////////////////////


		printf("Parent process: Finished.\n");
		break;

	case 0:
		/*
			Child Process
		*/

		printf("Child process: My ID is %jd\n", (intmax_t) getpid());

        /////////////////////////////////////////////////
        // Implement your code for child process here.
		shared_mem = (char *)shmat(shmid, NULL, 0);
		if (shared_mem == (char *) -1) {
			perror("shmat failed");
			exit(EXIT_FAILURE);
		}

		int total_word_count = 0;
		
		for (int i = 0; i < file_count; ++i) {
			while (1) {
				sem_wait(read_semaphore); // wait for parent to write content

				if (strcmp(shared_mem, "EOF") == 0) {
					sem_post(write_semaphore); // notify parent that file has been read
					break;
				}

				// check if file is math.txt
				if (strstr(file_paths[i], "math")) {
					//define a pipe to pass long word count to parent
					int pipe_fd[2];
					if (pipe(pipe_fd) == -1) {
						perror("pipe failed");
						exit(EXIT_FAILURE);
					}
					if (fork() == 0) { // in child process
						
						close(pipe_fd[0]); 
						int long_word_count = countLongWords(shared_mem);
						write(pipe_fd[1], &long_word_count, sizeof(int)); 
						close(pipe_fd[1]);
						exit(0); 
					} else {
						int long_word_count;
						read(pipe_fd[0], &long_word_count, sizeof(int));
						total_word_count += long_word_count;
					}
				}else{
					int word_count = wordCount(shared_mem); // 统计当前块的单词数
					total_word_count += word_count;
					printf("Child process: Counted %d words in file %s\n", word_count, file_paths[i]);
				}
				sem_post(write_semaphore); // 通知父进程可继续写入共享内存
			}
		}

		// Write total word count to result file
		saveResult("p2_result.txt", total_word_count);

		// Detach shared memory
		shmdt(shared_mem);
        /////////////////////////////////////////////////


		printf("Child process: Finished.\n");
		exit(0);

	case -1:
		/*
		Error occurred.
		*/
		printf("Fork failed!\n");
		exit(-1);
	}

	exit(0);
}
/**
 * calculate the number of words in math.txt file
 */
int countLongWords(char *text) {
    int count = 0;
    char buffer[SHM_SIZE];  // 创建本地缓冲区
    strncpy(buffer, text, SHM_SIZE);  // 复制共享内存内容到本地缓冲区
    buffer[SHM_SIZE - 1] = '\0';  // 确保字符串以 null 结尾

    char *token = strtok(buffer, " \n");  // 在本地缓冲区上操作
    while (token != NULL) {
        if (strlen(token) > 5) {
            count++;
        }
        token = strtok(NULL, " \n");
    }
    return count;
}

/**
 * @brief This function recursively traverse the source directory.
 * 
 * @param dir_name : The source directory name.
 */
void traverseDir(char *dir_name){
   
    // Implement your code here to find out
    // all textfiles in the source directory.
	DIR *dir = opendir(dir_name);
    struct dirent *entry;

    if (!dir) {
        perror("opendir failed");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            // Skip current and parent directories
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            // Construct new path
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
            traverseDir(path);
        } else if (entry->d_type == DT_REG) {
            // Check if the file has .txt extension
            if (strstr(entry->d_name, ".txt")) {
                if (file_count < MAX_FILES) {
                    char *path = malloc(1024);
                    snprintf(path, 1024, "%s/%s", dir_name, entry->d_name);
                    file_paths[file_count++] = path;
                    printf("Found text file: %s\n", path);
                } else {
                    printf("Reached maximum file count (%d). Skipping extra files.\n", MAX_FILES);
                    break;
                }
            }
        }
    }
    closedir(dir);
}