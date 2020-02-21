#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>

#define ROW_LENGTH 1024

typedef struct{
	long type;
	char row[ROW_LENGTH];
	char exit;
} msg;


void reader(int* pipefd, char* path){

	char buffer[ROW_LENGTH];
	FILE* file;	

	if(!(file = fopen(path,"r"))){
		perror("fopen");
		exit(1);
	}



	while((fgets(buffer,ROW_LENGTH,file))){

		 if (write(pipefd[1], buffer, ROW_LENGTH) == -1){
		 	perror("pipe-write");
		 	exit(1);
		 }
	}

	buffer[0] = '\0';

	if (write(pipefd[1], buffer, ROW_LENGTH) == -1){
		 	perror("pipe-write");
		 	exit(1);
	}

	fclose(file);

	exit(0);

}

void writer(int queue){

	msg m;

	while(1){

		msgrcv(queue, &m, sizeof(msg)-sizeof(long), 0, IPC_NOWAIT);
		if(m.exit==1) break;
		fprintf(stderr, "%s", m.row);
	}


	exit(0);

}



int main(int argc, char** argv){

	int pipefd[2];
	char buffer[ROW_LENGTH];
	int queue;
	msg m;

	if(argc<=2 || argc>=4){
		printf("Numero di parametri non corretto.");
		exit(1);
	}

	if(pipe(pipefd)==-1){
		perror("pipe");
		exit(1);
	}

	if( (queue = msgget(IPC_PRIVATE, IPC_CREAT|IPC_EXCL|0600)) == -1){
		perror("msgget");
		exit(1);
	}


	if(fork()==0) reader(pipefd, argv[2]);
		else if(fork()==0) writer(queue);


	while(m.row[0]!='\0'){
		if(read(pipefd[0],m.row,ROW_LENGTH)==-1){
			perror("read");
			exit(1);
		}
		if(strstr(m.row,argv[1])){
			m.exit = 0;
			msgsnd(queue, &m, sizeof(msg)-sizeof(long), 0);
		}
	}

	m.exit = 1;
	msgsnd(queue, &m, sizeof(msg)-sizeof(long), 0);

	wait(NULL);

	msgctl(queue, IPC_RMID, NULL);

	exit(0);
}