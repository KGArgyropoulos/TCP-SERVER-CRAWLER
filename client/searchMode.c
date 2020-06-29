#include "mycrawler.h"

void search(trie *root,char **command,int size){

	int writefd;
	int digits=digs(getpid());
	char *wrName=godFather(getpid(),digits,0);
	if((writefd = open(wrName, O_WRONLY | O_APPEND) ) < 0){
		perror("worker: can't open write fifo");
		exit(1);
	}
	int i,j;
	for(j=1;j<size;j++){
		trie *temp=root->next_level;
		char *word=malloc((strlen(command[j])+1)*sizeof(char));
		strcpy(word,command[j]);
		i=0;
		while(i<strlen(word)){
			if(temp->letter==word[i]){
				i++;
				if(i==strlen(word)){
					//found
					if(temp->eoString){
						posting_list *pl=temp->eoString;
						writeSearchFifo(pl,writefd);
						while(pl->next){
							pl=pl->next;
							writeSearchFifo(pl,writefd);
						}
					}
				}
				else if(temp->next_level){
					temp=temp->next_level;
				}
				else{
					posting_list *pl=NULL;
					break;
				}
			}
			else{
				if(temp->same_level){
					temp=temp->same_level;
				}
				else{
					posting_list *pl=NULL;
					break;
				}
			}
		}
		free(word);
	}
	close(writefd);
	free(wrName);
}

void writeSearchFifo(posting_list *pl,int writefd){

	chdir(getenv("HOME"));
	chdir(pl->path);
	FILE *fp = fopen(pl->path,"r");
	if(!fp){
		perror("here it is");
		exit(1);
	}
	char **map=NULL;
	int *soeline=NULL;
	int nol=mapping(pl->path,&map,&soeline);
	///////////////appropriate form////////////////
	int i,length;
	int digits=digs(pl->lineNum);
	int total_length=(int)strlen(pl->path)+digits+soeline[pl->lineNum]+4;//3 '\n' and 1 '\0'
	char finalString[total_length];
	sprintf(finalString,"%s\n%d\n%s\n",pl->path,pl->lineNum,map[pl->lineNum]);
	///////////////////////////////////////////////
	if(write(writefd,finalString,total_length) != total_length){
		perror("worker: can't write fifo");
		exit(1);
	}
	///////////////////////////////////////////////
	for(i=0;i<nol;i++)
		free(map[i]);
	free(map);
	free(soeline);
	fclose(fp);
}