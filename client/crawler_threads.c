#include "mycrawler.h"

void *threadTask(void *ptr){

	thread_pool *pool=(thread_pool *)ptr;
	struct QNode *n;
	// Set yourself as idle and signal to the main thread, when all threads are idle main will start
	pthread_mutex_lock(&currentlyIdleMutex);
	currentlyIdle++;
	pthread_cond_signal(&currentlyIdleCond);
	pthread_mutex_unlock(&currentlyIdleMutex);

	// wait for signal from main
	pthread_mutex_lock(&workReadyMutex);
	while (!workReady) {
		pthread_cond_wait(&workReadyCond , &workReadyMutex);
	}
	pthread_mutex_unlock(&workReadyMutex);

	while(blocked<pool->numThreads){
		n=deQueue(pool->q);
		if(n){
			char *temp=malloc((strlen(n->key)+1)*sizeof(char ));
			strcpy(temp,n->key);
			if(!exists(pool->l,temp)){
				contentProcess(temp,pool);
				pthread_mutex_lock(&mtxnonEmpty);
				unblocked=1;
				pthread_cond_signal(&condnonEmpty);
				pthread_mutex_unlock(&mtxnonEmpty);
			}
			free(temp);
		}
		else{
			unblocked=0;
			pthread_mutex_lock(&mtxnonEmpty);
			blocked++;
			while(!unblocked)
				pthread_cond_wait(&condnonEmpty,&mtxnonEmpty);
			pthread_mutex_unlock(&mtxnonEmpty);
		}
	}
	work_done++;
	pthread_exit(0);
}

///////////////////queue functions///////////////////
struct Queue *createQueue(){

	struct Queue *q = (struct Queue*)malloc(sizeof(struct Queue));
	q->front = NULL;
	q->rear = NULL;
	return q;
}

struct QNode* newNode(char *link){

	struct QNode *temp = (struct QNode*)malloc(sizeof(struct QNode));
	temp->key=malloc((strlen(link)+1)*sizeof(char ));
	strcpy(temp->key,link);
	temp->next = NULL;
	return temp; 
}

void enQueue(struct Queue *q,char *link){

	struct QNode *temp = newNode(link);
	if(!(q->rear)){
		q->front=temp;
		q->rear=temp;
		return;
	}
	q->rear->next=temp;
	q->rear=temp;
}

struct QNode *deQueue(struct Queue *q){

	if(!(q->front))
		return NULL;

	struct QNode *temp;
	temp=q->front;
	q->front=q->front->next;

	if(!(q->front))
		q->rear=NULL;

	return temp;
}

///////////////thread work functions////////////////

void contentProcess(char *url,thread_pool *pool){

	pool->numOfPages++;
	int sock;
	struct sockaddr_in serv_addr;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
		perror_exit("socket");

	//memset(&serv_addr, '0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(pool->s_port);
	char ip[100];
	hostname_to_ip(pool->hoip,ip);
	serv_addr.sin_addr.s_addr=inet_addr(ip);

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
		perror_exit("connect");

	char *request=getRequest(url,pool->hoip);
	if( (send(sock,request,strlen(request),0)) == -1 )
		perror_exit("send");

	char buffer[MAXBUFF];
	int times=1,len;
	//memset(buffer,0,MAXBUFF);
	while(1){
		memset(buffer,0,MAXBUFF);
		if((len=recv(sock,buffer,MAXBUFF,0)) < 0)
			perror_exit("read");
		else if(len==0)
			break;
		else{
			buffer[len]='\0';
			appendFile(buffer,url,pool,times);
			if(times==1)
				times=0;
		}
	}
	close(sock);
	analyseIt(url,pool);

	free(request);
}

void analyseIt(char *url,thread_pool *pool){

	chdir(getenv("HOME"));
	url+=(12+strlen(pool->hoip));
	int tl=((int)strlen(pool->sDir)+(int)strlen(url));
	char *str_file=malloc((tl+1)*sizeof(char ));
	sprintf(str_file,"%s%s",pool->sDir,url);
	FILE *fp;
	if(!(fp=fopen(str_file,"r")))
		perror_exit("fopen");

	int ch,count=0;
	char word[MAXBUFF];	//supposing no string is greater that 1024 bytes
	memset(word,0,MAXBUFF);
	int linkIsComing=0;
	while(!feof(fp)){
		ch = fgetc(fp);
		if(ch == '\n' || ch == ' '){
			word[count]='\0';
			count=0;
			if(linkIsComing){
				char *finalStr=convert(word,pool->hoip,pool->s_port);
				enQueue(pool->q,finalStr);
				linkIsComing=0;
				free(finalStr);
			}
			else if(!strcmp(word,"<a")){
				linkIsComing=1;
			}
		}
		else{
			word[count]=ch;
			count++;
		}
	}
	fclose(fp);
	free(str_file);
}

int exists(links *linklist,char *str){

	while(linklist->next){
		if(!strcmp(linklist->link,str)){
			return 1;
		}
		linklist=linklist->next;
	}

	if(linklist->flag==1){	//first time--starting URL
		linklist->flag=0;
		return 0;
	}

	if(!strcmp(linklist->link,str)){
		return 1;
	}

	//doesn't exist-add it
	links *temp=(links *)malloc(sizeof(links ));
	temp->link=malloc((strlen(str)+1)*sizeof(char ));
	strcpy(temp->link,str);
	temp->next=NULL;
	temp->flag=0;
	linklist->next=temp;

	return 0;
}

char *convert(char *link,char *host,int port){

	char *temp;
	temp=strtok(link,"/");
	temp=strtok(NULL,">");

	char *str=malloc(1024*sizeof(char ));
	sprintf(str,"http://%s:%d%s",host,port,temp);

	return str;
	
}

void appendFile(char *buffer,char *url,thread_pool *pool,int flag){

	chdir(getenv("HOME"));

	url+=(12+strlen(pool->hoip));
	char *temp=malloc((strlen(url)+1)*sizeof(char ));
	strcpy(temp,url);
	char *site;
	site=strtok(temp,"/");
	site=strtok(site,"/");
	if(!site)
		return;

	int tl=((int)strlen(site)+(int)strlen(pool->sDir));
	char *tempdir=malloc((tl+2)*sizeof(char ));
	sprintf(tempdir,"%s/%s",pool->sDir,site);

	//tempdir is the path to site's directory
	DIR *dir;
	if(!(dir=opendir(tempdir))){	//directory doesn't exist
		mkdir(tempdir, 0777);
		paths *temPath=(paths *)malloc(sizeof(paths ));
		temPath->key=malloc((strlen(tempdir)+1)*sizeof(char ));
		strcpy(temPath->key,tempdir);
		temPath->next=pool->p;
		pool->p=temPath;
	}

	tl=((int)strlen(url)+(int)strlen(pool->sDir));
	char *tempfile=malloc((tl+1)*sizeof(char ));
	sprintf(tempfile,"%s%s",pool->sDir,url);

	FILE *fp;
	if(!(fp=fopen(tempfile,"a")))
		perror_exit("fopen");

	if(flag){	//write only the content to the file--not the http responce
		int i=0;
		while(i<(strlen(buffer)-1)){
			if(buffer[i]=='\n' && buffer[i+1]=='\n')
				break;
			else
				i++;
		}
		buffer+=(i+1);
	}

	fprintf(fp,"%s",buffer);

	pool->numOfBytes+=strlen(buffer);

	free(temp);
	free(tempdir);
	free(tempfile);

	fclose(fp);
	closedir(dir);
}

char *getRequest(char *url,char *host){

	char *msg=malloc((strlen(url)+100)*sizeof(char ));

	url+=(12+strlen(host));
	sprintf(msg,"GET  %s HTTP/1.1\r\nHost: %s\r\n",url,host);

	return msg;
}

void hostname_to_ip(char *hostname,char *ip){

	struct hostent *he;
	struct in_addr **addr_list;
	int i;

	if(!(he=gethostbyname(hostname))) 
		perror_exit("gethostbyname");

	addr_list= (struct in_addr **) he->h_addr_list;

	for(i=0 ; addr_list[i]!= NULL; i++){
		strcpy(ip , inet_ntoa(*addr_list[i]) );
		return;
	}
}