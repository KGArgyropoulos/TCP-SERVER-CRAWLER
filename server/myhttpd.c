#include "myhttpd.h"

int main(int argc,char **argv){

	/////////////////get cmd's arguments////////////////
	if(argc!=9){
		printf("Wrong number of command arguments\n");
		return -1;
	}

	int i,serving_port,command_port,numOfThreads,ptr;
	int spfl=0,cpfl=0,notfl=0,ptrfl=0;
	for(i=0;i<argc;i++){
		if(!strcmp(argv[i],"-p")){
			serving_port=atoi(argv[i+1]);
			spfl=1;
		}
		if(!strcmp(argv[i],"-c")){
			command_port=atoi(argv[i+1]);
			cpfl=1;
		}
		if(!strcmp(argv[i],"-t")){
			numOfThreads=atoi(argv[i+1]);
			notfl=1;
		}
		if(!strcmp(argv[i],"-d")){
			ptr=i+1;
			ptrfl=1;
		}
	}
	if(spfl==0 || cpfl==0 || notfl==0 || ptrfl==0){
		printf("Check if the arguments are correct--possible typo\n");
		return -1;
	}
	char *path=malloc((strlen(argv[ptr])+1)*sizeof(char ));
	strcpy(path,argv[ptr]);

	/////////////////initialize threads/////////////////
	currentlyIdleMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	currentlyIdleCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	workReadyMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	workReadyCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	currentlyWorkingCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	currentlyWorkingMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	canFinishMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	canFinishCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;

	pthread_t threads[numOfThreads];

	workReady = 0;
	canFinish = 0;
	currentlyIdle = 0;
	currentlyWorking = numOfThreads;
	shutdownTime=0;

	stats *stat=statsInit(path);
	for (i = 0; i < numOfThreads; i++) {
		pthread_create(&threads[i], NULL, threadTask, (void*)stat);
	}

	clock_t begin = time(NULL);

	/////////////create socket connections//////////////
	int serving_socket=socket(AF_INET,SOCK_STREAM,0);
	int command_socket=socket(AF_INET,SOCK_STREAM,0);
	if( (serving_socket < 0) || (command_socket < 0) )
		perror_exit( "Socket" );

	if (setsockopt(serving_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		perror_exit("setsockopt(SO_REUSEADDR) failed");

	if (setsockopt(command_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		perror_exit("setsockopt(SO_REUSEADDR) failed");	

	struct sockaddr_in serving_in,command_in;
	serving_in.sin_family=AF_INET;
	serving_in.sin_port=htons(serving_port);
	serving_in.sin_addr.s_addr=INADDR_ANY;

	command_in.sin_family=AF_INET;
	command_in.sin_port=htons(command_port);
	command_in.sin_addr.s_addr=INADDR_ANY;

	if (bind(serving_socket, (struct sockaddr *) &serving_in, sizeof(serving_in)) < 0)
		perror_exit( "bind" );
	if (listen(serving_socket, MAX) < 0)
		perror_exit( "listen" );

	if (bind(command_socket, (struct sockaddr *) &command_in, sizeof(command_in)) < 0)
		perror_exit( "bind" );
	if (listen(command_socket, MAX) < 0)
		perror_exit( "listen" );

	int client_socket=0;
	int *fds=malloc(2*sizeof(int));
	fds[0]=serving_socket;
	fds[1]=command_socket;

	while(!shutdownTime){
		int fd = findPort(fds);
		if(fd == -1 )
			perror_exit("fd");

		///////////////communicate with client//////////////
		if((client_socket=accept(fd,(struct sockaddr*) NULL,NULL)) < 0)
			perror_exit( "Connect" );

		if( fd == serving_socket){
			enQueue(stat->q, client_socket);
			// Make sure all of them are ready
			pthread_mutex_lock(&currentlyIdleMutex);
			while (currentlyIdle != numOfThreads) {
				pthread_cond_wait(&currentlyIdleCond, &currentlyIdleMutex);
			}
			pthread_mutex_unlock(&currentlyIdleMutex);
			// All threads are now blocked
			// Prevent them from finishing before authorized.
			canFinish = 0;
			currentlyWorking = numOfThreads;

			///////////Signal to the threads to start///////////
			pthread_mutex_lock(&workReadyMutex);
			workReady = 1;
			pthread_cond_broadcast(&workReadyCond );
			pthread_mutex_unlock(&workReadyMutex);

			//////////////Wait for them to finish///////////////
			pthread_mutex_lock(&currentlyWorkingMutex);
			while (currentlyWorking != 0) {
				pthread_cond_wait(&currentlyWorkingCond, &currentlyWorkingMutex);
			}
			pthread_mutex_unlock(&currentlyWorkingMutex);

			//////////Prevent them from starting again//////////
			currentlyIdle=0;
			workReady = 0;

			////////////////Allow them to finish////////////////
			pthread_mutex_lock(&canFinishMutex);
			canFinish = 1;
			pthread_cond_broadcast(&canFinishCond);
			pthread_mutex_unlock(&canFinishMutex);
		}
		else{
			int num;
			char buf[MAXBUFF];
			memset(buf,0,MAXBUFF);
			if( (num=recv(client_socket,buf,MAXBUFF,0)) < 0)
				perror_exit( "recv" );
			buf[num]='\0';

			if(!strncmp(buf,"STATS",5)){
				clock_t end = clock();
				double time_spent = (double)(time(NULL) - begin);
				char *time_str = timeCalc(time_spent);
				char *answer=malloc(200*sizeof(char ));
				sprintf(answer,"Server up for %s, served %d pages, %ld bytes\n",time_str,stat->numOfPages,stat->numOfBytes);
				if( (send(client_socket,answer,strlen(answer),0)) == -1 )
					perror_exit( "send" );
				free(time_str);
				free(answer);
			}
			else if(!strncmp(buf,"SHUTDOWN",8)){
				shutdownTime=1;
			}
			else{
				perror_exit("bad client's request");
			}
		}
		close(client_socket);
	}
	close(serving_socket);
	close(command_socket);
	//Signal to the threads to start in order to finish
	pthread_mutex_lock(&workReadyMutex);
	workReady = 1;
	pthread_cond_broadcast(&workReadyCond );
	pthread_mutex_unlock(&workReadyMutex);

	for(i=0;i<numOfThreads;i++)
		pthread_join(threads[i],0);

	////////////////////////////////////////////////////

	freeStats(stat);
	free(fds);
	free(path);
	return 0;
}

void perror_exit(char *message) {
	perror(message);
	exit(EXIT_FAILURE);
}

int findPort(int *fds) {
	fd_set readfds;
	int maxfd, fd;
	int i;
	int status;

	FD_ZERO(&readfds);
	maxfd = -1;
	for(i=0;i<2;i++){
		FD_SET(fds[i], &readfds);
		if(fds[i]>maxfd)
			maxfd=fds[i];
	}
	status = select(maxfd+1,&readfds,NULL,NULL,NULL);
	if(status<0){
		perror_exit("select");
		return -1;
	}
	fd = -1;
	for (i = 0; i < 2; i++) {
		if(FD_ISSET(fds[i], &readfds)){
			fd=fds[i];
			break;
		}
	}
	return fd;
}

stats* statsInit(char *path){

	stats *temp=malloc(sizeof(stats));
	temp->numOfPages=0;
	temp->numOfBytes=0;
	temp->filePath=malloc((strlen(path)+1)*sizeof(char ));
	strcpy(temp->filePath,path);
	temp->q = createQueue();

	return temp;
}

void freeStats(stats *stat){

	free(stat->filePath);
	free(stat->q);
	free(stat);
}

char *timeCalc(double x){

	char *temp=malloc(10*sizeof(char ));

	int hours = (int)(x/3600);
	int mintemp = (int)((int)x%3600);
	int minutes = (int)(mintemp/60);
	double seconds = (mintemp % 60); 

	sprintf(temp,"%d:%d:%.2f",hours,minutes,seconds);
	return temp;
}