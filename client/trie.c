#include "mycrawler.h"

void insertion(trie **root,int lNum,char *word,char *path){

	if((*root)->next_level==NULL){//first time
		trie *newNode=init();
		newNode->letter=word[0];
		(*root)->next_level=newNode;
		int i;
		for(i=1;i<strlen(word);i++){
			trie *nextNode=init();
			nextNode->letter=word[i];
			newNode->next_level=nextNode;
			newNode=newNode->next_level;
		}
		newNode->leaf=malloc((strlen(word)+1)*sizeof(char));
		strcpy(newNode->leaf,word);
		posting_list *pl=pl_init();
		pl->lineNum=lNum;
		pl->path=malloc((strlen(path)+1)*sizeof(char));
		strcpy(pl->path,path);
		newNode->eoString=pl;
		return;
	}

	trie *temp=(*root)->next_level;
	int i=0;
	while(i<strlen(word)){
		if(temp->letter==word[i]){
			i++;
			if(temp->next_level!=NULL && i<strlen(word)){
				temp=temp->next_level;
			}
			else{
				for(i;i<strlen(word);i++){
					trie *newNode=init();
					newNode->letter=word[i];
					temp->next_level=newNode;
					temp=temp->next_level;
				}
			}
		}
		else{
			if(temp->same_level!=NULL){
				temp=temp->same_level;
			}
			else{
				trie *newNode=init();
				newNode->letter=word[i];
				temp->same_level=newNode;
				temp=temp->same_level;
				i++;
				for(i;i<strlen(word);i++){
					trie *nextNode=init();
					nextNode->letter=word[i];
					temp->next_level=nextNode;
					temp=temp->next_level;
				}
			}
		}
	}

	if(temp->eoString){
		posting_list *pl=pl_init();
		pl->lineNum=lNum;
		pl->path=malloc((strlen(path)+1)*sizeof(char));
		strcpy(pl->path,path);
		pl->next=temp->eoString;
		temp->eoString=pl;
	}
	else{
		temp->leaf=malloc((strlen(word)+1)*sizeof(char));
		strcpy(temp->leaf,word);
		posting_list *pl=pl_init();
		pl->lineNum=lNum;
		pl->path=malloc((strlen(path)+1)*sizeof(char));
		strcpy(pl->path,path);
		temp->eoString=pl;
	}
}

trie *init(void){

	trie *newNode=malloc(sizeof(trie));
	newNode->next_level=NULL;
	newNode->same_level=NULL;
	newNode->leaf=NULL;
	newNode->eoString=NULL;

	return newNode;
}

posting_list *pl_init(void){

	posting_list *pl=malloc(sizeof(posting_list));
	pl->next=NULL;

	return pl;
}

void destroyNodes(trie **root,int flag){

	trie *temp;
	if(flag==0){
		temp=(*root)->next_level;
	}
	else{
		temp=(*root)->same_level;
	}


	if(temp->next_level){
		destroyNodes(&temp,0);
		if(temp->same_level){
			destroyNodes(&temp,1);
			if(temp->eoString){
				free(temp->leaf);
				posting_list *pl=temp->eoString;
				while(pl->next){
					posting_list *temp1=pl;
					pl=pl->next;
					free(temp1->path);
					free(temp1);
				}
				free(pl->path);
				free(pl);
			}
			free(temp);
			return;
		}
	}
	else if(temp->same_level){
		destroyNodes(&temp,1);
	}

	if(temp->eoString){
		free(temp->leaf);
		posting_list *pl=temp->eoString;
		while(pl->next){
			posting_list *temp1=pl;
			pl=pl->next;
			free(temp1->path);
			free(temp1);
		}
		free(pl->path);
		free(pl);
	}
	free(temp);
}