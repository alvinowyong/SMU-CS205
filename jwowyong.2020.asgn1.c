#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSGSIZE 32
#define N 16
#define M 5

int find_highest_ready_index(int priority[], bool ready[]) {
    int lowest_running = N;
    int lowest_running_index = -1;
    for (int i = 0; i < N; i++) {
        if (priority[i] < lowest_running && ready[i] == 1) {
            lowest_running = priority[i];
            lowest_running_index = i;
        }
    }
    return lowest_running_index;
}

int find_lowest(int priority[], bool running[]) {
    int biggest_running = -1;
    for (int i = 0; i < N; i++) {
        if (priority[i] > biggest_running && running[i] == 1) {
            biggest_running = priority[i];
        }
    }
    return biggest_running;
}

int find_lowest_index(int priority[], bool running[]) {
    int biggest_running = -1;
    int biggest_running_index = -1;
    for (int i = 0; i < N; i++) {
        if (priority[i] > biggest_running && running[i] == 1) {
            biggest_running = priority[i];
            biggest_running_index = i;
        }
    }
    return biggest_running_index;
}

int find_highest(int priority[], bool running[]) {
    int lowest_running = N;
    for (int i = 0; i < N; i++) {
        if (priority[i] < lowest_running && running[i] == 1) {
            lowest_running = priority[i];
        }
    }
    return lowest_running;
}

int find_highest_index(int priority[], bool running[]) {
    int lowest_running = N;
    int lowest_running_index = -1;
    for (int i = 0; i < N; i++) {
        if (priority[i] < lowest_running && running[i] == 1) {
            lowest_running = priority[i];
            lowest_running_index = i;
        }
    }
    return lowest_running_index;
}

void parent_read(int p[]) {
    // Flags for processes
    bool terminated[N] = {0};
    bool ready[N] = {0};
    bool running[N] = {0};
    bool stopped[N] = {0};
    int priority[N] = {0};
    int status;

    // Variables for tracking processes
    int num_done = 0;
    int n_process = 0;
    int num_running = 0;
    int lowest = INT_MAX;
    int lowest_index = N;
    int highest = INT_MIN;
    int highest_index = -1;

    // Pipe configurations
    int nread;
    pid_t processes[N];
    char buf[MSGSIZE];

    // Close read link - no reading necessary for parent
    close(p[1]);

    while (1) {
        char *args[M];
        nread = read(p[0], buf, MSGSIZE);
        switch (nread) {

        // Case -1 - read call if return -1 then pipe is empty because of fcntl
        case -1:
            // Case "-1" means pipe is empty and errono
            // Set EAGAIN
            if (errno == EAGAIN) {
                if (n_process > N) {
                    // Escape all processes if number of processes exceed 16
                    printf("Total number of processess exceeded 16.\nProgramme exiting after 5 seconds...\n");
                    sleep(10);
                    exit(0);
                } else {
                    // Loop through all processes and wait for completion
                    for (int i = 0; i < n_process; i++) {
                        pid_t pid = processes[i];

                        // If process completed - pid returned
                        if (waitpid(pid, &status, WNOHANG) == pid) {
                            // Send termination signal
                            kill(pid, SIGTERM);

                            // Set "RUNNING" flag to false
                            // Set "TERMINATED" flag to true
                            running[i] = 0;
                            terminated[i] = 1;

                            // Update variables for tracking statuses
                            num_done++;
                            num_running--;

                            // TODO: Minus 2 from n_process
                            // If there are processes that has yet to run till termination
                            if (num_done < n_process) {
                                // Search for the next ready process and resume
                                int highest_ready = find_highest_ready_index(priority, ready);
                                kill(processes[highest_ready], SIGCONT);

                                // Set flags for resumed process
                                ready[highest_ready] = 0;
                                running[highest_ready] = 1;

                                // Update priority of running tasks
                                lowest = find_lowest(priority, running);
                                highest = find_highest(priority, running);
                                lowest_index = find_lowest_index(priority, running);
                                highest_index = find_highest_index(priority, running);
                            }
                        }
                    }
                }
                sleep(1);
                break;
            } else {
                perror("read");
                exit(4);
            }

        // Case 0 - all Bytes read and EOF (end of conversation)
        case 0:
            printf("Terminating all processes and exiting.\n\n");
            for (int i = 0; i < n_process; i++) {
                kill(processes[i], SIGTERM);
            }
            // Close read link and exit
            close(p[0]);
            exit(0);

        // Case Default - Pipe is not empty
        default:
            printf("\r");
            char *p = strtok(buf, " ");

            char *cmd = p;
            int arg_cnt = 0;
            while (p = strtok(NULL, " ")) {
                args[arg_cnt] = p;
                arg_cnt++;
            }

            // Matchcase for cmd
            if (strstr(cmd, "run") != NULL) {
                pid_t pid = fork();

                if (pid == 0) {
                    char file[] = "./";
                    strcat(file, args[1]);
                    char *arg_array[] = {file, NULL, NULL, NULL, NULL};
                    for (int i = 1; i < arg_cnt - 1; i++) {
                        arg_array[i] = args[i + 1];
                    }
                    execvp(arg_array[0], arg_array);
                } else if (pid > 0) {
                    // Check if there's already 3 processess running
                    if (num_running == 3) {
                        // Check if new process has higher priority
                        if (atoi(args[0]) < lowest) {
                            processes[n_process] = pid;
                            running[n_process] = 1;
                            priority[n_process] = atoi(args[0]);
                            n_process++;

                            // Find and pause the lowest process
                            kill(processes[lowest_index], SIGSTOP);
                            running[lowest_index] = 0;
                            ready[lowest_index] = 1;

                            // Update priority of running tasks
                            lowest = find_lowest(priority, running);
                            highest = find_highest(priority, running);
                            lowest_index = find_lowest_index(priority, running);
                            highest_index = find_highest_index(priority, running);
                        } else {
                            // Stop new process immediately and set states
                            kill(pid, SIGSTOP);

                            // Set flags for new process
                            processes[n_process] = pid;
                            ready[n_process] = 1;
                            priority[n_process] = atoi(args[0]);

                            // Update variables for tracking
                            n_process++;
                        }
                    } else {
                        // If there's less than 3 processess running
                        // Set flags for new process
                        processes[n_process] = pid;
                        running[n_process] = 1;
                        stopped[n_process] = 0;
                        terminated[n_process] = 0;
                        priority[n_process] = atoi(args[0]);

                        // Update priority of running tasks
                        lowest = find_lowest(priority, running);
                        highest = find_highest(priority, running);
                        lowest_index = find_lowest_index(priority, running);
                        highest_index = find_highest_index(priority, running);

                        // Update variables for tracking
                        n_process++;
                        num_running++;
                    }

                    printf("PID %d created\n", pid);
                }
            } else if (strstr(cmd, "stop") != NULL) {
                // check if pid exists and stop if found
                pid_t pid = atoi(args[0]);
                bool found = 0;
                for (int i = 0; i < n_process; i++) {
                    if (processes[i] == pid) {
                        found = 1;

                        // Catch for terminated processes
                        if (terminated[i]) {
                            printf("Process had already been terminated\n");
                            break;
                        }

                        // "STOP" the process and set "STOPPED" flags to true
                        kill(pid, SIGSTOP);
                        ready[i] = 0;
                        running[i] = 0;
                        stopped[i] = 1;

                        if (running[i] == 1) {
                            // Search for index of ready process with highest priority
                            int highest_ready = find_highest_ready_index(priority, ready);

                            if (highest_ready == -1) {
                                // No other processes is in "READY" state
                                // Decrement "running" counter
                                num_running--;
                            } else {
                                // Continue highest ready process
                                kill(processes[highest_ready], SIGCONT);
                                ready[highest_ready] = 0;
                                running[highest_ready] = 1;
                            }

                            // Set "RUNNING" flag to false before updating priority
                            running[i] = 0;

                            // Update priority of running tasks
                            lowest = find_lowest(priority, running);
                            highest = find_highest(priority, running);
                            lowest_index = find_lowest_index(priority, running);
                            highest_index = find_highest_index(priority, running);
                        }

                        printf("Stopping %d\n", pid);
                        break;
                    }
                }
                if (!found) {
                    printf("PID not found\n");
                }
            } else if (strstr(cmd, "kill") != NULL) {
                // check if pid exists and kill if found
                pid_t pid = atoi(args[0]);
                bool found = 0;
                for (int i = 0; i < n_process; i++) {
                    if (processes[i] == pid) {
                        found = 1;

                        // Catch for terminated processes
                        if (terminated[i]) {
                            printf("Process had already been terminated\n");
                            break;
                        }

                        kill(pid, SIGTERM);
                        ready[i] = 0;
                        stopped[i] = 0;
                        terminated[i] = 1;

                        // Check if process is currently running
                        if (running[i] == 1) {
                            // Run highest priority
                            int highest_ready = find_highest_ready_index(priority, ready);
                            kill(processes[highest_ready], SIGCONT);
                            ready[highest_ready] = 0;
                            running[highest_ready] = 1;

                            // printf("Updating priority queue\n");
                            running[i] = 0;

                            // Update priority of running tasks
                            lowest = find_lowest(priority, running);
                            highest = find_highest(priority, running);
                            lowest_index = find_lowest_index(priority, running);
                            highest_index = find_highest_index(priority, running);
                        } else {
                            num_running--;
                        }

                        num_done++;
                        printf("Terminated %d\n", pid);

                        // Additional waitpid call to prevent "run till termination" cleanup method for running
                        waitpid(pid, &status, WNOHANG);
                        break;
                    }
                }
                if (!found) {
                    printf("PID not found\n");
                }
            } else if (strstr(cmd, "resume") != NULL) {
                pid_t pid = atoi(args[0]);
                bool found = 0;

                // Loop and search for process using 'pid'
                for (int i = 0; i < n_process; i++) {
                    if (processes[i] == pid) {
                        found = 1;

                        // Catch for terminated processes
                        if (terminated[i]) {
                            printf("Terminated process cannot be resumed\n");
                            break;
                        }

                        if (priority[i] < lowest) {
                            // If 3 processes are already running
                            if (num_running == 3) {
                                // Kill the lowest priority process
                                kill(processes[lowest_index], SIGSTOP);
                                running[lowest_index] = 0;
                                ready[lowest_index] = 1;
                            }

                            // Resume the specified process
                            kill(pid, SIGCONT);
                            stopped[i] = 0;
                            running[i] = 1;

                            // Update priority of running tasks
                            lowest = find_lowest(priority, running);
                            highest = find_highest(priority, running);
                            lowest_index = find_lowest_index(priority, running);
                            highest_index = find_highest_index(priority, running);

                            printf("Resuming %d\n", pid);
                        } else {
                            stopped[i] = 0;
                            ready[i] = 1;
                            printf("Resuming %d - process does not have priority to run immediately.\n", pid);
                        }
                        
                        break;
                    }
                }
                if (!found) {
                    printf("PID not found\n");
                }
            } else if (strstr(cmd, "list") != NULL) {
                // loop through all child processes, display status
                if (n_process == 0) {
                    printf("No process have been admitted yet.\n");
                }
                for (int i = 0; i < n_process; i++) {
                    if (running[i]) {
                        printf("%d, %d, %d\n", processes[i], 0, priority[i]);
                    } else if (ready[i]) {
                        printf("%d, %d, %d\n", processes[i], 1, priority[i]);
                    } else if (stopped[i]) {
                        printf("%d, %d, %d\n", processes[i], 2, priority[i]);
                    } else if (terminated[i]) {
                        printf("%d, %d, %d\n", processes[i], 3, priority[i]);
                    } else {
                        printf("Unable to determine state of process with pid: %d\n", processes[i]);
                    }
                }
            } else {
                printf("Invalid command\n");
                continue;
            }
        }
    }
}

void child_write(int p[]) {
    // Close read link - no reading necessary for child
    close(p[0]);

    while (1) {
        // Prompt user for input
        char *input = NULL;
        input = readline("CS205>");

        // "exit" command
        if (strstr(input, "exit") != NULL) {
            break;
        }

        // Catch error writing to pipe
        if (write(p[1], input, MSGSIZE) == -1) {
            printf("Error happened while writing to pipe\n");
            return;
        };

        // Sleep readline for 1 second
        sleep(1);
    }

    exit(0);
}

int main() {
    int p[2];

    // Check for error for pipe
    if (pipe(p) < 0)
        exit(1);

    // Check for error for fcntl
    if (fcntl(p[0], F_SETFL, O_NONBLOCK) < 0) {
        exit(2);
    }

    // fork() and execute functions
    switch (fork()) {
    // error
    case -1:
        exit(3);

    // 0 for child process
    case 0:
        child_write(p);
        break;

    default:
        parent_read(p);
        break;
    }

    return 0;
}