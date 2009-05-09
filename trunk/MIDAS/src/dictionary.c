#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../includes/dictionary.h"
#include "../includes/log.h"


int put(Dictionary* dico, char* word){
	int ret;
	Word* tmp;

	if(dico==NULL || word==NULL){
		log_message_error("Adding a word to the dictionary",PROGRAM,"Invalid dictionary or word");
		return -1;
	}

	Word* new_word = calloc(1,sizeof(Word));
	if(new_word==NULL){
		log_message_error("Allocating new Word cell",SYSTEM,"calloc");
		return -1;
	}
	new_word->word = calloc(MAX_WORD_LENGTH, sizeof(char));
	if(new_word->word==NULL){
		log_message_error("Allocating new word",SYSTEM,"calloc");
		return -1;
	}
	new_word->next = NULL;
	strncpy(new_word->word,word,MAX_WORD_LENGTH);

	if(dico->first == NULL){
		dico->first = new_word ;
		return 1;
	}

	tmp = dico->first;
	if((ret=strncmp(tmp->word,new_word->word,MAX_WORD_LENGTH))>0){
		new_word->next = tmp;
		dico->first = new_word;
		return 1;
	}else if(ret==0){
		return 0;
	}
	while(tmp->next!=NULL){
		ret= strncmp(tmp->next->word,new_word->word,MAX_WORD_LENGTH);
		if(ret < 0){
			tmp = tmp->next;
			continue;
		}else if(ret > 0){
			break;
		}else if(ret==0){
			return 0;
		}
	}
	new_word->next = tmp->next;
	tmp->next = new_word;
	return 1;
}

int get(Dictionary * dico, char * word){
	Word* tmp;

	if(dico==NULL || word==NULL){
		log_message_error("Adding a word to the dictionary",PROGRAM,"Invalid dictionary or word");
		return -1;
	}

	tmp = dico->first;
	if(tmp==NULL) {
		return 0;
	}
	do {
		if(strncmp(tmp->word,word,MAX_WORD_LENGTH) == 0){
			return 1;
		}
		tmp=tmp->next;
	} while(tmp != NULL);
	return 0;
}

int write_dictionary(Dictionary* dictionary, int fd) {
	int ret;
	Word* tmp = dictionary->first;
	char buffer[MAX_WORD_LENGTH+1];

	if(tmp==NULL) {
		ret = write(fd, "\n", 1);
		if(ret==-1) {
			log_message_error("Writing the dictionary, sending void list", SYSTEM, "write");
			return 0;
		}
	} else {
		while(tmp!=NULL){
			sprintf(buffer,"%s\n",tmp->word);
			ret = write(fd, buffer, strlen(buffer));
			if(ret==-1) {
				log_message_error("Writing the dictionary, sending word", SYSTEM, "write");
				return 0;
			}
			tmp = tmp->next;
		}
	}
	ret = write(fd, "\n", 1);
	if(ret==-1) {
		log_message_error("Writing the dictionary, closing list", SYSTEM, "write");
		return 0;
	}
	return 1;
}

