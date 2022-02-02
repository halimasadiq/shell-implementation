#define _GNU_SOURCE_
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>

char *readShell();
void loop();
void myexit();
char **findArgs(char *cmd);
void run(char **args, char *cmd);
void mycd(char **args);
void waitfg(pid_t pid);
void myKill(int jobID);
int mybg(int jobID);
int myfg(int jobID);

int tArgc = 0;
int bgOn = 0;
char cc[300];
//===========================================================
//Functions for jobs: initialize struct, clear job list, remove a job, add a job
//STRUCT FOR JOBS
struct jobs{

	int jobID;
	pid_t pid;
	int status;
	char cmd[256];
};

volatile sig_atomic_t spid;
struct jobs j[100];
int jobNum = 0;

void removeJob(struct jobs *j){
	j -> jobID = 0;
	j -> pid = 0;
	j -> status = 0;
	j -> cmd[0] = '\0';
}

int maxID(struct jobs *j){
	int max = 0;
	for(int i = 0; i < jobNum; i++){
		if(j[i].jobID > max){
			max = j[i].jobID;
		}
	}
	return max;
}
//---------------------------------------------------------

void addjob(struct jobs *j, pid_t pid, int status, char *cmd){


		if(j[jobNum].pid == 0){
		j[jobNum].jobID = jobNum + 1;
		if(jobNum > 50){
			jobNum = 1;
		}
		j[jobNum].pid = pid;
		j[jobNum].status = status;
		strcpy(j[jobNum].cmd, cmd);
		}

	jobNum++;
}

//------------------------------------------------------------
void deleteJob(struct jobs *j, pid_t pid){

	for(int i = 0; i < jobNum; i++){
		if(j[i].pid == pid){
			removeJob(&j[i]);
		}
	}
	jobNum--;
}
//-----------------------------------------------------------
pid_t fgJobs(struct jobs *j){

	for(int i = 0; i < jobNum; i++){
		if(j[i].status == 1){
			return j[i].pid;
		}
	}
	return 0;

}
//------------------------------------------------------------
int jobID_pid(pid_t pid){
	if (pid < 1){
		return 0;
	}
	for(int i = 0; i  < jobNum; i++){
		if(j[i].pid == pid){
			return j[i].jobID;
		}
	}
	return 0;
}

//============================================================
void printJobs(struct jobs *j){


	if(jobNum == 0){
		;
	}
	else{
		char *state;
		char *bg;
		for(int i = 0; i < 100; i++){
			if(j[i].jobID == 0){;}

			if(j[i].status == 1){
				state = "Running";
				printf("[%d] %d %s %s\n",j[i].jobID, j[i].pid, state, strtok(j[i].cmd,"\n"));	
			}
			if(j[i].status == 2){
				state = "Running";
				if(strstr(j[i].cmd,"&")){
					printf("[%d] %d %s %s\n",j[i].jobID, j[i].pid, state, strtok(j[i].cmd,"\n"));
				}
				else{
				bg = "&";
				printf("[%d] %d %s %s %s\n",j[i].jobID, j[i].pid, state, strtok(j[i].cmd,"\n"), bg);
				}

			}
			if(j[i].status == 3){
				char *temp = strtok(j[i].cmd,"\n");
				if(temp[strlen(temp)-1] =='&'){
					temp[strlen(temp)-1] = '\0';
				}
				state = "Stopped";
				printf("[%d] %d %s %s\n",j[i].jobID, j[i].pid, state, temp);
			}
		}
	}

}

//============================================================
//SIGNAL HANDLERS

//------------------------------------------------


void sigint_handler(int sig){
	pid_t pid = fgJobs(j);
	if(pid != 0){
		kill(-pid,sig);
	}
	return;
}
//---------------------------------------------
void sigtstp_handler(int sig){
	pid_t pid = fgJobs(j);
	if(pid != 0){
		kill(-pid,sig);
	}
	return;
}
//----------------------------------------------
void sigchld_handler(int sig){

	int status;
	int olderrno = errno;
	sigset_t mask_all, prev_all;
	pid_t pid;

	sigfillset(&mask_all);
	int jobid;

	while((pid = waitpid(-1,&status, WNOHANG | WUNTRACED)) > 0){
		jobid  = jobID_pid(pid);
		sigprocmask(SIG_BLOCK, &mask_all, &prev_all);

		if(WIFEXITED(status)){

			for(int i = 0; i < jobNum; i++){
				if(j[i].jobID == jobid && j[i].pid == pid && j[i].status == 2){
					deleteJob(j,pid);
			//		printf("[%d] %d terminated with exit status: %d\n",jobid,pid,WEXITSTATUS(status));
				}
				else if(j[i].jobID == jobid && j[i].pid == pid){
				deleteJob(j,pid);
				}
			}
		}

		else if (WIFSIGNALED(status)){
			deleteJob(j,pid);
			printf("[%d] %d terminated by signal %d\n",jobid,pid,WTERMSIG(status));
		}


		else if (WIFSTOPPED(status)){
			for(int i = 0; i < jobNum; i++){
				if(j[i].jobID == jobid){
					j[i].status = 3;
				}
			}
		}


		sigprocmask(SIG_SETMASK,&prev_all,NULL);
	}
	//if(errno != ECHILD)
		//perror("waitpid error");
	errno = olderrno;

}


//=============================================================

int main(int argc, char **argv){
	loop();
}//end main

//=============================================================
void loop(){

	char *cmd;
	char **args;
	char cmdCopy[256];



	signal(SIGINT, sigint_handler);
	signal(SIGTSTP, sigtstp_handler);
	signal(SIGCHLD, sigchld_handler);

	while(1){
		printf("> ");
		cmd = readShell();
		strcpy(cmdCopy,cmd);
		args = findArgs(cmd);
		if(cmd[0] == '\n'){;}
		else{
		run(args,cmdCopy);
		}

		free(args);
		free(cmd);
	}

}//end loop
//=============================================================
char *readShell(){

	char *cmd = NULL;
	size_t bufsize = 0;

	if(getline(&cmd, &bufsize, stdin) == -1){
		if(feof(stdin)){
			myexit();
		}
		else{
			perror("getline");
			exit(EXIT_FAILURE);
		}
	}
	return cmd;
}//end readShell
//=============================================================
char **findArgs(char *cmd){

	//printf("entered from args loop: %s\n",cmd);
	int bufsize = 64, position = 0;
	char **tokens = malloc(bufsize *sizeof(char*));
	char *token;
	
//	strcpy(cc," ");


	if(!tokens){
		fprintf(stderr, "Error");
		exit(EXIT_FAILURE);
	}

	token = strtok(cmd, " \t\r\n");
	while(token != NULL){
		tokens[position] = token;
		if(position == 0){
			strcpy(cc,token);
		}
		else{
		strcat(cc," ");
		strcat(cc,token);
		}
//		strcat(cc," ");
		position++;

		if(position >= bufsize){
			bufsize += 64;
			tokens = realloc(tokens, bufsize * sizeof(char*));
			if(!tokens){
				fprintf(stderr, "Error");
				exit(EXIT_FAILURE);
			}
		}
		token = strtok(NULL, " \t\r\n\a");
	}
	tokens[position] = NULL;

	tArgc = position;

	return tokens;
}//end findArgs
//=============================================================
void run(char **args, char *c){

	pid_t pid;
	sigset_t mask_all,mask_one, prev_one;

	char *dest = malloc(256);
	strcpy(dest,"/usr/bin/");
	char *dest2 = malloc(256);
	strcpy(dest2, "/bin/");


	if((strcmp(args[tArgc-1], "&") == 0) || strstr(args[tArgc-1],"&")){
		if(strcmp(args[tArgc-1],"&")==0){
			args[tArgc-1] = NULL;
		}
		else if(strstr(args[tArgc-1],"&")){

			int len = strlen(args[tArgc-1]);
			args[tArgc-1][len-1] = '\0';

		}
		
//		printf("run in bg with final arg: %s\n",args[tArgc-1]);
		bgOn = 1;
	}

	if(strcmp(args[0],"exit") == 0){
		myexit();
	}
	else if(strcmp(args[0], "cd") == 0){
		mycd(args);
	}
	else if(strcmp(args[0], "jobs") == 0){
		printJobs(j);
	}
	else if(strcmp(args[0], "kill") == 0){
		if(!strstr(args[1],"%")){
			printf("Error: Enter job ID as %%JobID\n");
		}
		else{
		char *wo = args[1] + 1;
		long int i = strtol(wo,NULL, 10);
		//toKilljid = (int)i;
		//printf("in kill loop with jid: %ld\n", i);
		myKill(i);
		}
	}

	else if(strcmp(args[0], "bg") == 0){
		if(!strstr(args[1],"%")){
			printf("Error: Enter job ID as %%JobID\n");
		}
		else{
		char *wo = args[1] + 1;
		long int i = strtol(wo,NULL, 10);
		//toKilljid = (int)i;
		//printf("in kill loop with jid: %ld\n", i);
		bgOn = mybg(i);
		}
	}


	else if(strcmp(args[0], "fg") == 0){
		if(!strstr(args[1],"%")){
			printf("Error: Enter job ID as %%JobID\n");
		}
		else{
		char *wo = args[1] + 1;
		long int i = strtol(wo,NULL, 10);
		//toKilljid = (int)i;
		//printf("in kill loop with jid: %ld\n", i);
		bgOn = myfg(i);
		}
	}

	else{
		sigfillset(&mask_all);
		sigemptyset(&mask_one);
		sigaddset(&mask_one,SIGCHLD);

		sigprocmask(SIG_BLOCK, &mask_one, &prev_one);

		pid = fork();
		
		if(pid == 0){
			//printf("Before: %d\n",getpgid(pid));
			setpgid(pid,0);
			sigprocmask(SIG_SETMASK,&prev_one,NULL);
	//		printf("After: %d\n",getpgid(pid));
	//		printf("ParentID: %d\n",getppid());
			if(access(args[0],F_OK) == 0){
				//if(!strstr(args[0], "/")){printf("%s: command not found\n",strtok(c, "\n"));}
				if(execv(args[0],args) == -1){
				perror(args[0]);
	//				printf("%s: Command not found.\n",args[0]);
					exit(EXIT_FAILURE);
				}
				exit(EXIT_SUCCESS);
			}
			else{
				if(access(strcat(dest,args[0]),F_OK)== 0){
					if(execv(dest,args) == -1){
					perror(args[0]);
					exit(EXIT_FAILURE);
					}
					exit(EXIT_SUCCESS);
				}
				else if(access(strcat(dest2,args[0]),F_OK) == 0){
					if(execv(dest2,args) == -1){
						perror(args[0]);
						exit(EXIT_FAILURE);
					}
				exit(1);
				}

				else{
					char *ret;
					ret = strstr(args[0], "/");
					if(ret == NULL){
					printf("%s: command not found\n",args[0]);
					}
					else{
					printf("%s: No such file or directory\n",args[0]);
					}
					exit(1);
				}
			}//end else
		}//end if
		else{

			if(bgOn == 1){
			//	sigprocmask(SIG_BLOCK,&mask_all,NULL);
				addjob(j,pid,2,cc);
				for(int i = 0; i < jobNum; i++){
					if(j[i].pid == pid){
						printf("[%d] %d\n", j[i].jobID, j[i].pid);
					}
				}
				sigprocmask(SIG_SETMASK,&prev_one,NULL);
			}
			if(bgOn == 0){
			//	sigprocmask(SIG_BLOCK,&mask_all,NULL);
				addjob(j,pid,1,cc);
				//printf("in bg on == 0 loop\n");
				sigprocmask(SIG_SETMASK,&prev_one,NULL);
				waitfg(pid);
			}
		}
	}//end outer else
	free(dest);
	free(dest2);
	bgOn = 0;
	
}//end run



//===============================================================
void waitfg(pid_t pid){

	while(1){
		if(pid != fgJobs(j)){
			break;
		}
		else{
			sleep(1);
		}
	}
	return;
}
//===============================================================
void mycd(char **args){

//	printf("in cd function\n");

		if(args[1] == NULL){
			if(setenv("PWD",getenv("HOME"),1) == -1){
				perror("setenv");
			}
			chdir(getenv("PWD"));
		}


		else{
			char *path = realpath(args[1],NULL);
			if(path == NULL){
				printf("in path null\n");
				perror(args[1]);
			}
			else{
			//printf("pwd: %s\n",getenv("PWD"));
			//printf("path: %s\n",path);

				if(setenv("PWD",path,1) == -1){
					perror("setenv");
				}
			//printf("pwd1: %s\n",getenv("PWD"));
			chdir(getenv("PWD"));
			free(path);
			}
		}
	//}//end if
	return;
}//end mycd
//===============================================================
void myexit(){
//	printf("in exit loop\n");

	for(int i = 0; i< jobNum; i++){
		if(j[i].status == 3){
			kill(-(j[i].pid),SIGHUP);
			kill(-(j[i].pid),SIGCONT);
			j[i].status = 2;
		}
	}
	for(int i = 0; i <jobNum; i++){
		if(j[i].status == 1 || j[i].status == 2){
			kill(-(j[i].pid),SIGHUP);
		}

	}
	exit(EXIT_SUCCESS);



}//end myexit

//=================================================================
void myKill(int jid){

	int killed = 0;

	for(int i = 0; i < jobNum; i++){
		if(j[i].jobID == jid){
			kill((j[i].pid),SIGTERM);
			killed = 1;
		}
	}
	if(killed == 0){
		printf("Invalid job ID: %d\n",jid);
		
	}
}

//===================================================================
int mybg(int jid){

	int found = 0;
	for(int i = 0; i <jobNum; i++){

		if(j[i].jobID == jid){
			found = 1;
			j[i].status = 2;
			kill(-(j[i].pid),SIGCONT);
		}
	}
	if(found == 0){
		printf("Invalid job ID\n");
	}
	return 1;

}


//===================================================================
int myfg(int jid){

	int found = 0;
	for(int i = 0; i <jobNum; i++){

		if(j[i].jobID == jid){
			found = 1;
			j[i].status = 1;
			kill(-(j[i].pid),SIGCONT);
			waitfg(j[i].pid);
		}
	}
	if(found == 0){
		printf("Invalid job ID\n");
	}
	return 0;

}

