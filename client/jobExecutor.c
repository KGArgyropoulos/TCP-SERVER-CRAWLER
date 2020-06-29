#include "mycrawler.h"

void jEx(char *fileName,char *query,int client_socket){

	FILE *f=fopen(fileName,"r");
	if(!f)
		perror_exit("jEx--fopen");
	///////////////////map array///////////////////
	char **map_array=NULL;
	int *SOELine=NULL;
	int numOfDirectories=mapping(fileName,&map_array,&SOELine);
	fclose(f);//close the file,now that we have everything in memory

	/////////////////start workers/////////////////
	/*
	Algorithm for spliting directories to workers
	directories |workers
				|_______
				|
			y	|x
				|

	y workers will take x+1 directories and (workers-y) workers will take x directories
	[y*(x+1)+(workers-y)*x]
	*/
	int numOfWorkers=5; //default
	int x=(numOfDirectories / numOfWorkers);
	int y=(numOfDirectories % numOfWorkers);
	int i,j=0,index=0;
	long *pids=malloc(numOfWorkers*sizeof(long ));

	for(i=0;i<y;i++){
		char **sender=malloc((x+1)*sizeof(char *));
		index=0;
		int from=(x+1)*i;
		for(j=from; j< from+(x+1); j++){
			sender[index]=malloc((SOELine[j]+1)*sizeof(char ));
			strcpy(sender[index],map_array[j]);
			index++;
		}
		pids[i]=proConn(SOELine,sender,index,from);

		for(j=0;j<index;j++)
			free(sender[j]);
		free(sender);
	}
	for(i=y;i<numOfWorkers;i++){
		char **sender=malloc(x*sizeof(char *));
		index=0;
		int from=((x+1)*i - (i-y));
		for(j= from; j< from+x; j++){
			sender[index]=malloc((SOELine[j]+1)*sizeof(char ));
			strcpy(sender[index],map_array[j]);
			index++;
		}
		pids[i]=proConn(SOELine,sender,index,from);

		for(j=0;j<index;j++)
			free(sender[j]);
		free(sender);
	}

	char **input=NULL;
	index=findEachQuery(query,&input);

	execQueries(pids,numOfWorkers,input,index,map_array,SOELine,numOfDirectories,client_socket);

	for(i=0;i<numOfWorkers;i++){
		kill(pids[i],SIGUSR2);
		kill(pids[i],SIGCONT);
		wait(NULL);
	}

	///////////////////////////////////////////////
	int digits;
	for(i=0;i<numOfWorkers;i++){
		digits=digs(pids[i]);
		char *tempName=godFather(pids[i],digits,0);
		if(unlink(tempName)<0){
			perror("Worker can't unlink\n");
		}
		free(tempName);
	}
	free(pids);
	for(i=0;i<numOfDirectories;i++)
		free(map_array[i]);
	free(map_array);
	free(SOELine);

	for(i=0;i<index;i++)
		free(input[i]);
	free(input);
}

int mapping(char *filename,char ***map_arr,int **soeline){

	FILE *f = fopen(filename,"r");
	int i=0,ch=0,index=0;
	int numOfLines=countLines(filename);
	int maxStringLength=mSL(filename,numOfLines);
	char temp[maxStringLength];
	*map_arr=malloc(numOfLines*sizeof(char *));
	*soeline=malloc(numOfLines*sizeof(int));
	while(i<numOfLines){
		ch=fgetc(f);
		if(ch=='\n'){
			temp[index]='\0';
			((*soeline)[i])=strlen(temp);
			((*map_arr)[i])=malloc(((*soeline)[i])*sizeof(char )+1);
			strcpy(((*map_arr)[i]),temp);
			index=0;
			i++;
		}
		else if(index==0 && (ch==' ' || ch=='\t')){
			continue;
		}
		else if(ch=='\t'){
			temp[index++]=' ';
		}
		else{
			temp[index++]=ch;
		}
	}
	fclose(f);
	return numOfLines;
}

int countLines(char *filename){

	FILE *fp = fopen(filename,"r");
	if(!fp)
		printf("Failed to open file, named: %s\n",filename);
	int ch=0,lines=0;
	while(!feof(fp)){
		ch = fgetc(fp);
		if(ch == '\n'){
			lines++;
		}
	}
	
	fclose(fp);
	return lines;
}

int mSL(char *filename,int numOfLines){

	FILE *fp = fopen(filename,"r");	
	int i,ch=0;
	int maxLengthOfLine=0;
	int lengthOfEachLine=0;
	for(i=0;i<numOfLines;i++){
		do{
			ch=fgetc(fp);
			lengthOfEachLine++;
		}while(ch!='\n');

		if(lengthOfEachLine>maxLengthOfLine){
			maxLengthOfLine=lengthOfEachLine;
		}
		lengthOfEachLine=0;
	}

	fclose(fp);
	return maxLengthOfLine;
}

int findEachQuery(char *str,char ***input){

	int wcount=0,counter=0;
	while(counter<strlen(str)){
		if(str[counter++]==' ' || str[counter]=='\0'){
			wcount++;
		}
	}
	*input=malloc((wcount+1)*sizeof(char *));
	char temp[1024];
	counter=0;
	int index=0,i=1;
	((*input)[0])=malloc((strlen("/search")+1)*sizeof(char ));
	strcpy(((*input)[0]),"/search");
	while(i<(wcount+1)){
		if(str[counter]==' ' || str[counter]=='\0' || str[counter]=='\r'){
			((*input)[i])=malloc((index+1)*sizeof(char ));
			temp[index]='\0';
			strcpy(((*input)[i]),temp);
			i++;
			index=0;
			if(str[counter]=='\0' || str[counter]=='\r')
				break;
		}
		else{
			temp[index]=str[counter];
			index++;
		}
		counter++;
	}
	return (wcount+1);
}