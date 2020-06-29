#include "mycrawler.h"

int main(int argc,char **argv){

	getcwd(cwd, sizeof(cwd));

	/////////////////get cmd's arguments////////////////
	if(argc!=12){
		printf("Wrong number of command arguments\n");
		return -1;
	}

	int i,port,command_port,numOfThreads;
	int ptr_host,ptr_sdir,ptr_sturl;
	int fl_hst=0,fl_port=0,fl_cmd=0,fl_not=0,fl_dir=0;

	for(i=0;i<argc;i++){
		if(!strcmp(argv[i],"-h")){
			ptr_host=i+1;
			fl_hst=1;
		}
		if(!strcmp(argv[i],"-p")){
			port=atoi(argv[i+1]);
			fl_port=1;
		}
		if(!strcmp(argv[i],"-c")){
			command_port=atoi(argv[i+1]);
			fl_cmd=1;
		}
		if(!strcmp(argv[i],"-t")){
			numOfThreads=atoi(argv[i+1]);
			fl_not=1;
		}
		if(!strcmp(argv[i],"-d")){
			ptr_sdir=i+1;
			ptr_sturl=i+2;
			fl_dir=1;
		}
	}
	if(fl_dir==0 || fl_not==0 || fl_cmd==0 || fl_port==0 || fl_hst==0){
		printf("Check if the arguments are correct--possible typo\n");
		return -1;
	}

	char *host_or_ip=malloc((strlen(argv[ptr_host])+1)*sizeof(char ));
	char *save_dir=malloc((strlen(argv[ptr_sdir])+1)*sizeof(char ));
	char *starting_URL=malloc((strlen(argv[ptr_sturl])+1)*sizeof(char ));
	strcpy(host_or_ip,argv[ptr_host]);
	strcpy(save_dir,argv[ptr_sdir]);
	strcpy(starting_URL,argv[ptr_sturl]);

	/////////////////initialize threads/////////////////
	currentlyIdleMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	currentlyIdleCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	workReadyMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	workReadyCond = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	mtxnonEmpty = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	condnonEmpty = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
	workReady=0;
	currentlyIdle=0;
	unblocked=0;
	blocked=0;

	shutdownTime=0;
	work_done=0;

	pthread_t *threads=malloc(numOfThreads*sizeof(pthread_t ));
	thread_pool *tpool=poolInit(starting_URL,port,host_or_ip,save_dir,numOfThreads);
	for (i = 0; i < numOfThreads; i++) {
		pthread_create(&threads[i], NULL, threadTask, (void*)tpool);
	}

	// Make sure all of them are ready
	pthread_mutex_lock(&currentlyIdleMutex);
	while (currentlyIdle != numOfThreads) {
		pthread_cond_wait(&currentlyIdleCond, &currentlyIdleMutex);
	}
	pthread_mutex_unlock(&currentlyIdleMutex);

	///////////Signal to the threads to start///////////
	pthread_mutex_lock(&workReadyMutex);
	workReady = 1;
	pthread_cond_broadcast(&workReadyCond );
	pthread_mutex_unlock(&workReadyMutex);

	clock_t begin = time(NULL);
	//////////////create socket connection//////////////
	int command_socket=socket(AF_INET,SOCK_STREAM,0);
	if(command_socket < 0)
		perror_exit( "Socket" );

	if (setsockopt(command_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
		perror_exit("setsockopt(SO_REUSEADDR) failed");	

	struct sockaddr_in command_in;

	command_in.sin_family=AF_INET;
	command_in.sin_port=htons(command_port);
	command_in.sin_addr.s_addr=INADDR_ANY;

	if (bind(command_socket, (struct sockaddr *) &command_in, sizeof(command_in)) < 0)
		perror_exit( "bind" );
	if (listen(command_socket, MAX) < 0)
		perror_exit( "listen" );

	int client_socket=0;

	///////////////communicate with client//////////////
	socklen_t addrlen=sizeof(command_in);
	while(!shutdownTime){
		if((client_socket=accept(command_socket,NULL,NULL)) < 0)
			perror_exit( "accept" );

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
			sprintf(answer,"Crawler up for %s, downloaded %d pages, %ld bytes\n",time_str,tpool->numOfPages,tpool->numOfBytes);
			if( (send(client_socket,answer,strlen(answer),0) ) == -1 )
				perror_exit( "send" );
			free(time_str);
			free(answer);
		}
		else if(!strncmp(buf,"SHUTDOWN",8)){
			shutdownTime=1;
		}
		else if(!strncmp(buf,"SEARCH",6)){
			if(work_done!=numOfThreads){
				char *answer=malloc(21*sizeof(char ));
				strcpy(answer,"Crawling in progress");
				if( (send(client_socket,answer,strlen(answer),0) ) == -1 )
					perror_exit( "send" );			
				free(answer);
			}
			else{
				char *fileToOpen=addLinksToFile(tpool->p);
				char *searchString=malloc(strlen(buf)*sizeof(char ));
				strcpy(searchString,buf);
				searchString+=7;
				jEx(fileToOpen,searchString,client_socket);
				free(fileToOpen);
			}
		}
		else{
			perror_exit("bad client's request");
		}

		close(client_socket);
	}
	close(command_socket);

	for(i=0;i<numOfThreads;i++)
		pthread_cancel(threads[i]);

	////////////////////////////////////////////////////

	free(tpool->hoip);
	free(host_or_ip);
	free(save_dir);
	free(starting_URL);

	return 0;
}

thread_pool* poolInit(char *starting_URL,int port,char *host_or_ip,char *save_dir,int numOfThreads){

	thread_pool *temp=malloc(sizeof(thread_pool));
	temp->numOfPages=0;
	temp->numOfBytes=0;
	temp->numThreads=numOfThreads;
	temp->s_port=port;
	temp->hoip=malloc((strlen(host_or_ip)+1)*sizeof(char ));
	strcpy(temp->hoip,host_or_ip);
	temp->sDir=malloc((strlen(save_dir)+1)*sizeof(char ));
	strcpy(temp->sDir,save_dir);
	dirEmpty(temp->sDir);//if directory contains anything--it will be deleted

	temp->q = createQueue();
	enQueue(temp->q,starting_URL);

	links *temp1=(links *)malloc(sizeof(links ));
	temp1->link = malloc((strlen(starting_URL)+1)*sizeof(char ));
	strcpy(temp1->link,starting_URL);
	temp1->next=NULL;
	temp1->flag=1;
	temp->l=temp1;

	return temp;
}

char *addLinksToFile(paths *list){

	paths *temp=list;
	chdir(getenv("HOME"));
	chdir(getenv(cwd));
	char *temp1=malloc((strlen(cwd)+11)*sizeof(char ));
	sprintf(temp1,"%s/paths.txt",cwd);

	FILE *fp=fopen(temp1,"w");
	if(!fp)
		perror_exit("path file");

	while(temp){
		fprintf(fp, "%s\n", temp->key);
		temp=temp->next;
	}
	fclose(fp);

	return temp1;
}

void dirEmpty(char *dirname){

	chdir(getenv("HOME"));

	int n=0;
	struct dirent *d;
	DIR *dir = opendir(dirname);
	if(!dir) //Not a directory or doesn't exist
		perror_exit("save directory");
	while(d=readdir(dir)){
		if(++n > 2)
			break;
	}
	closedir(dir);
	if(n<=2) //Directory Empty
		return;

	//else delete everything inside it
	char *temp=malloc((14+strlen(dirname))*sizeof(char ));
	sprintf(temp,"exec rm -r %s/*",dirname);
	system(temp);
	free(temp);
}

void perror_exit(char *message) {
	perror(message);
	exit(EXIT_FAILURE);
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