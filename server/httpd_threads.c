#include "myhttpd.h"

void *threadTask(void *ptr) {

	stats *stat=(stats *)ptr;

	while(!shutdownTime){
		// Set yourself as idle and signal to the main thread, when all threads are idle main will start
		pthread_mutex_lock(&currentlyIdleMutex);
		currentlyIdle++;
		pthread_cond_signal(&currentlyIdleCond);
		pthread_mutex_unlock(&currentlyIdleMutex);

		// wait for work from main
		pthread_mutex_lock(&workReadyMutex);
		while (!workReady) {
			pthread_cond_wait(&workReadyCond , &workReadyMutex);
		}
		pthread_mutex_unlock(&workReadyMutex);

		// Check if it's time to finish
		if(shutdownTime)
			break;

		// Do the work
		int fd;
		struct QNode *n = deQueue(stat->q);
		if(n){
			fd=n->key;
			int num;
			char buf[MAXBUFF];
			memset(buf,0,MAXBUFF);
			if( (num=read(fd,buf,MAXBUFF)) < 0)
				perror_exit( "recv" );
			buf[num]='\0';

			char *responce=getResponce(buf,stat);
			unsigned int wroteBytes=0;
			int num1;
			while(wroteBytes < strlen(responce) ){
				num1=write(fd,responce,strlen(responce));
				if(num1<=0)
					perror_exit("write");
				else
					wroteBytes+=num1;
			}
			free(responce);
		}
		// mark yourself as finished and signal to main
		pthread_mutex_lock(&currentlyWorkingMutex);
		currentlyWorking--;
		pthread_cond_signal(&currentlyWorkingCond);
		pthread_mutex_unlock(&currentlyWorkingMutex);
		// Wait for permission to finish
		pthread_mutex_lock(&canFinishMutex);
		while (!canFinish) {
			pthread_cond_wait(&canFinishCond , &canFinishMutex);
		}
		pthread_mutex_unlock(&canFinishMutex);
	}

	pthread_exit(0);
}

///////////////////queue functions///////////////////
struct Queue *createQueue(){

	struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = q->rear = NULL;
	return q;
}


struct QNode* newNode(int k){

	struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->key = k;
	temp->next = NULL;
	return temp; 
}

void enQueue(struct Queue *q, int k){

	// Create a new LL node
	struct QNode *temp = newNode(k);
	// If queue is empty, then new node is front and rear both
	if (!q->rear){
		q->front=q->rear=temp;
       return;
    }
	// Add the new node at the end of queue and change rear
	q->rear->next=temp;
	q->rear=temp;
}

struct QNode *deQueue(struct Queue *q){
	// If queue is empty, return NULL.
	if(!q->front)
		return NULL;
	// Store previous front and move front one node ahead
	struct QNode *temp=q->front;
	q->front=q->front->next;

	// If front becomes NULL, then change rear also as NULL
	if(!q->front)
		q->rear=NULL;
	return temp;
}

///////////////thread work functions////////////////

char *getResponce(char *buf,stats *stat){

	//date and time
	char fulltime[100];
	time_t now = time(0);
	struct tm tm = *gmtime(&now);
	strftime(fulltime, sizeof(fulltime), "%a, %d %b %Y %H:%M:%S %Z", &tm);

	//content
	char *content=getContent(buf,stat->filePath);
	//code
	int code;
	char *name_code;
	if(!strcmp(content,"<html>Trying to access this file but don't think I can make it.</html>")){
		code=403;
		name_code=malloc(10*sizeof(char ));
		strcpy(name_code,"Forbidden");
	}
	else if(!strcmp(content,"<html>Sorry dude, couldn't find this file.</html>")){
		code=404;
		name_code=malloc(10*sizeof(char ));
		strcpy(name_code,"Not Found");
	}
	else{
		code=200;
		name_code=malloc(3*sizeof(char ));
		strcpy(name_code,"OK");
		stat->numOfPages++;
		stat->numOfBytes+=(strlen(content)-13);
	}
	//content length
	int strLength=strlen(content);
	//total string length
	int total_length=200+strlen(content);

	char *str=malloc(total_length*sizeof(char ));
	sprintf(str,"HTTP/1.1 %d %s\n\
Date: %s\n\
Server: myhttpd/1.0.0 (Ubuntu64) \n\
Content-Length: %d \n\
Content-Type: text/html \n\
Connection: Closed \n\n\
%s\n",code,name_code,fulltime,strLength,content);

	free(content);
	free(name_code);
	return str;
}

char *getContent(char *buf,char *path){

	char *page;
	page=strtok(buf," ");
	page=strtok(NULL," ");
	if(!page)
		perror_exit("strtok");
	chdir(getenv("HOME"));
	FILE* fp;
	page++;
	int tl=strlen(path)+strlen(page);
	char *fullpath=malloc((tl+1)*sizeof(char ));
	sprintf(fullpath,"%s%s",path,page);

	char *buffer;
	int length;

	if(access(fullpath,F_OK)==-1){
		length=50;
		buffer=malloc(length*sizeof(char ));
		strcpy(buffer,"<html>Sorry dude, couldn't find this file.</html>");
	}
	else if(access(fullpath, R_OK)!=0){
		length=71;
		buffer=malloc(length*sizeof(char ));
		strcpy(buffer,"<html>Trying to access this file but don't think I can make it.</html>");
	}
	else{
		fp=fopen(fullpath,"r");
		if(!fp){
			perror_exit("file");
		}
		else{
			fseek (fp,0,SEEK_END);
			int length=ftell(fp);
			fseek (fp,0,SEEK_SET);
			buffer=malloc((length+1)*sizeof(char ));
			if(buffer)
				fread(buffer,1,length,fp);
			else
				perror_exit("cont_malloc");
			buffer[length]='\0';
		}
		fclose(fp);
	}

	free(fullpath);
	return buffer;
}