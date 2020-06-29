#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>

#define MAXBUFF 1024
#define MAX 128
#define PERMS 0666

struct QNode{
	char *key;
	struct QNode *next;
};

struct Queue{
	struct QNode *front, *rear;
};

typedef struct Node{
	int numOfPages;
	int numThreads;
	long numOfBytes;
	int s_port;
	char *hoip;
	char *sDir;
	struct Queue *q;
	struct Node1 *l;
	struct pathNodes *p;
}thread_pool;

typedef struct pathNodes{
	char *key;
	struct pathNodes *next;
}paths;

typedef struct Node1{
	char *link;
	struct Node1 *next;
	int flag;
}links;

//trie
typedef struct Node2{
	char letter;
	char *leaf;
	struct Node2 *next_level;
	struct Node2 *same_level;
	struct Node3 *eoString;
}trie;

typedef struct Node3{
	int lineNum;
	char *path;
	struct Node3 *next;
}posting_list;

void insertion(struct Node2 **,int ,char *,char *);
struct Node2 *init(void);
struct Node3 *pl_init(void);
void destroyNodes(struct Node2 **,int );

pthread_mutex_t currentlyIdleMutex;
pthread_mutex_t workReadyMutex;
pthread_mutex_t mtxnonEmpty;
pthread_cond_t currentlyIdleCond;
pthread_cond_t workReadyCond;
pthread_cond_t condnonEmpty;

int currentlyIdle;
int workReady;
int unblocked;
int blocked;

int shutdownTime;
int work_done;

char cwd[MAXBUFF];

//mycrawler.c
struct Node* poolInit(char *,int ,char *,char *,int );
void perror_exit(char *);
char *timeCalc(double );
void dirEmpty(char *);
char *addLinksToFile(struct pathNodes *);

//threads.c--queue
struct QNode* newNode(char *);
struct Queue *createQueue();
void enQueue(struct Queue *,char *);
struct QNode *deQueue(struct Queue *);

//threads.c
void *threadTask(void *);
void contentProcess(char *,struct Node *);
char *getRequest(char *,char *);
void appendFile(char *,char *,struct Node *,int );
void analyseIt(char *,struct Node *);
int exists(struct Node1 *,char *);
char *convert(char *,char *,int );
void hostname_to_ip(char *, char *);

//jobExecutor.c
void jEx(char *,char *,int );
int countLines(char *);
int mSL(char *,int );
int mapping(char *,char ***,int **);
int findEachQuery(char *,char ***);

//connections
long proConn(int *,char **,int ,int );
void execQueries(long *,int ,char **,int ,char **,int *,int ,int );
int digs(long );
char *godFather(long ,int ,int );
char *newPathName(char *,char *);
int str_split(char *,char ***);
void handler(int );
static int resume;
static int endProc;

//searchMode
void search(struct Node2 *,char **,int );
void writeSearchFifo(struct Node3 *,int );