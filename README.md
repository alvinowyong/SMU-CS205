# SMU-CS205: Operating System Concepts with Android
A non-blocking, priority based process manager

### Compile and Run

Repository contains files which have already been compiled.
If you wish to recompile or if permission are not brought over...

```bash
$ gcc jwowyong.2020.asgn1.c -o jwowyong.2020.asgn1 -lreadline
```

To run process manager...
```bash
$ ./jwowyong.2020.asgn1
```

### List of Commands
1. run [priority] ./prog [output] [duration] [priority]: Running an executable program with optional input arguments
```bash
$ run 4 ./jwowyong.2020.asgn1 x101 101 4
PID 567169 created
```
2. stop [PID]: Put a process with the specified PID in the stopped state. If a running process is stopped, then dispatch highest priority ready process to run state
```bash
$ stop 567169
Stopping 567169
```
3. kill [PID]: Terminate a process with the specified PID. If a running process is killed, then dispatch highest priority ready process to run state
```bash
$ kill 567169
Terminated 567169
```
4. resume [PID]: Resume the stopped process with the specified PID to ready state. If the number of running processes is going to exceed three, then only the top three priority processes are allowed in run state
```bash
$ resume 567169
Resuming 567169
```
5. list : List all the processes by: PID, state, priority. Use the following mapping to represent the states – 0: running, 1: ready, 2: stopped, 3: terminated
```list
$ list
567169, 0, 4
567177, 0, 5
```
7. exit : Terminate all child processes if they have not yet been terminated, and exit from parent process
```list
$ resume 567169
CS205>exit
Terminating all processes and exiting.
```
