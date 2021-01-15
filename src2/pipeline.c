#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void pipeline(char *argv1[], char *argv2[], pid_t pids[]){
	int fds[2];
	pipe(fds); // 0, read; 1, write
	pid_t pid = fork(); 
	
	if(pid < 0){
		perror("fork");
		exit(-1);
	} else if(!pid){ // write端
		close(fds[0]);
		dup2(fds[1], 1);
		execvp(argv1[0], argv1);
	} 
	pids[0] = pid;

	pid = fork();
	if(pid < 0){
		perror("fork");
		exit(-1);
	} else if(!pid){ // read端
		close(fds[1]);
		dup2(fds[0], 0);
		execvp(argv2[0], argv2);
	} 
	pids[1] = pid;

	close(fds[1]); 
	close(fds[0]); 
} 


// 1. exevcp 对文件描述符的影响
