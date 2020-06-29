#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>

#define MAXBUFF 1024
#define MAX 128

struct QNode{
	int key;
	struct QNode *next;
};

struct Queue{
	struct QNode *front, *rear;
};

typedef struct Node{
	int numOfPages;
	long numOfBytes;
	char *filePath;
	struct Queue *q;
}stats;

pthread_mutex_t currentlyIdleMutex;
pthread_cond_t  currentlyIdleCond;
int currentlyIdle;

pthread_mutex_t workReadyMutex;
pthread_cond_t  workReadyCond;
int workReady;

pthread_cond_t  currentlyWorkingCond;
pthread_mutex_t currentlyWorkingMutex;
int currentlyWorking;

pthread_mutex_t canFinishMutex;
pthread_cond_t  canFinishCond;
int canFinish;

int shutdownTime;

//myhttpd.c
struct Node* statsInit(char *);
void freeStats(struct Node *);
int findPort(int *);
void perror_exit(char *);
char *timeCalc(double );

//threads.c--queue
struct QNode* newNode(int );
struct Queue *createQueue();
void enQueue(struct Queue *,int );
struct QNode *deQueue(struct Queue *);

//threads.c
void *threadTask(void *);
char *getResponce(char *,struct Node *);
char *getContent(char *,char *);