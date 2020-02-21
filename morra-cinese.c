#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>

#define SASSO 'S'
#define CARTA 'C'
#define FORBICI 'F'

#define S_G1 0
#define S_G2 1
#define S_GIUDICE 2
#define S_TABELLONE 3

int WAIT(int sem_des, int num_semaforo){
	struct sembuf operazioni[1] = {{num_semaforo,-1,0}};
	return semop(sem_des, operazioni, 1);
}

int SIGNAL(int sem_des, int num_semaforo){
	struct sembuf operazioni[1] = {{num_semaforo,+1,0}};
	return semop(sem_des, operazioni, 1);
}

typedef struct
{
	char mossa1;
	char mossa2;
	char vincitore;
	int p_giocate;
	int fine_partita;
	
} partita;


void giocatore1(int shm_id, int sem_id){

	partita *p;
	
	srand(123039091);

	if( (p = (partita *) shmat(shm_id,NULL,0)) == (partita *) -1){
		perror("shmat");
		exit(1);
	}
	while(1){

		WAIT(sem_id,S_G1);

		if(p->fine_partita) break;

		int k = rand()%3;

		if(k==0){
			p->mossa1 = SASSO;
			fprintf(stderr, "P1: mossa 'sasso'\n");
		}
		else if(k==1){ 
			p->mossa1 = CARTA;
			fprintf(stderr, "P1: mossa 'carta'\n");
		}
		else{ 
			p->mossa1 = FORBICI;
			fprintf(stderr, "P1: mossa 'forbici'\n");
		}

		SIGNAL(sem_id,S_GIUDICE);
	}

	exit(0);
}

void giocatore2(int shm_id, int sem_id){

	partita *p;


	srand(129319);
	
	if( (p = (partita *) shmat(shm_id,NULL,0)) == (partita *) -1){
		perror("shmat");
		exit(1);
	}

	while(1){

		WAIT(sem_id,S_G2);

		if(p->fine_partita) break;

		int k = rand()%3;

		if(k==0){
			p->mossa2 = SASSO;
			fprintf(stderr, "P2: mossa 'sasso'\n");
		}
		else if(k==1){ 
			p->mossa2 = CARTA;
			fprintf(stderr, "P2: mossa 'carta'\n");
		}
		else{ 
			p->mossa2 = FORBICI;
			fprintf(stderr, "P2: mossa 'forbici'\n");
		}

		SIGNAL(sem_id,S_GIUDICE);
	}

	exit(0);
}

void giudice(int shm_id, int sem_id, int num_partite){

	partita *p;
	int i=0;

	if( (p = (partita *) shmat(shm_id,NULL,0)) == (partita *) -1){
		perror("shmat");
		exit(1);
	}

	p->p_giocate = 0;

	while(i<num_partite){
		WAIT(sem_id, S_GIUDICE);
		WAIT(sem_id, S_GIUDICE);
		p->p_giocate++;
		if(p->mossa1 == p->mossa2){
			fprintf(stderr, "G: Partita patta e quindi da ripetere\n");
			p->vincitore = 0;
			SIGNAL(sem_id,S_TABELLONE);
		}
		else{
			if(p->mossa1 == SASSO && p->mossa2 == FORBICI){ 
				p->vincitore = 1;	
				fprintf(stderr, "G: Partita Vinta da P1\n");
			}
			else if(p->mossa1 == FORBICI && p->mossa2 == CARTA){ 
				p->vincitore = 1;
				fprintf(stderr, "G: Partita Vinta da P1\n");
			}
			else if(p->mossa1 == CARTA && p->mossa2 == SASSO){ 
				p->vincitore = 1;
				fprintf(stderr, "G: Partita Vinta da P1\n");
			}
			else{ 
				p->vincitore = 2;
				fprintf(stderr, "G: Partita Vinta da P2\n");
			}
			SIGNAL(sem_id,S_TABELLONE);
			i++;
		}
	}
	p->fine_partita = 1;
	SIGNAL(sem_id,S_TABELLONE);
}

void tabellone(int shm_id, int sem_id){

	int win_p1=0;
	int win_p2=0;
	partita* p;

	if( (p = (partita *) shmat(shm_id,NULL,0)) == (partita *) -1){
		perror("shmat");
		exit(1);
	}

	while(1){
		WAIT(sem_id,S_TABELLONE);

		if(p->vincitore==1) win_p1++;
		else if(p->vincitore==2) win_p2++;

		if(p->fine_partita)
			break;

		fprintf(stderr, "T: Classifica temporanea: P1 = %d P2 = %d\n",win_p1,win_p2);
		SIGNAL(sem_id,S_G1);
		SIGNAL(sem_id,S_G2);
	}

	fprintf(stderr, "Classifica finale: P1 = %d P2 = %d\n",win_p1,win_p2);
	if(win_p1>win_p2) fprintf(stderr, "P1 ha vinto il torneo\n");
	else if(win_p1<win_p2) fprintf(stderr, "P2 ha vinto il torneo\n");
	else fprintf(stderr, "Torneo finito in pareggio\n");
	exit(0);
}


int main(int argc, char** argv ){

	int shm_id, sem_id;
	int partite_totali;

	if((shm_id = shmget(IPC_PRIVATE, sizeof(partita), IPC_CREAT|IPC_EXCL|0600))==-1){
		perror("shmget");
		exit(1);
	} 

	if((sem_id = semget(IPC_PRIVATE,4,IPC_CREAT|IPC_EXCL|0600)) == -1){
		perror("semget");
		exit(1);
	}

	partite_totali = atoi(argv[1]);

	semctl(sem_id, S_G1, SETVAL, 1);
	semctl(sem_id,S_G2,SETVAL,1);
	semctl(sem_id,S_GIUDICE,SETVAL,0);
	semctl(sem_id,S_TABELLONE,SETVAL,0);


	if(fork()==0) giocatore1(shm_id,sem_id);
	else if(fork()==0) giocatore2(shm_id,sem_id);
	else if(fork()==0) giudice(shm_id,sem_id,partite_totali);
	else tabellone(shm_id,sem_id);
	
	
	exit(0);
}