#ifndef DICTIONARY_H
#define DICTIONARY_H

#define MAX_WORD_LENGTH 50

typedef struct _word {
	struct _word* next;
	char* word;
} Word;

typedef struct _dictionary {
	Word* first;
} Dictionary;

/**
 * Put word in the dictonnary if is not already inside.
 * @param dico : the dictonnary
 * @param word : the word that you want insert. It should not be higher than max_word_len of the dictionary
 * @return 1 if success, 0 if the word is already inside and -1 otherwise
 */
int put(Dictionary* dico, char* word);

/**
 * Check if a word is contains in the dictionnary
 * @param dico : the dictonnary
 * @param word : the word that you want check
 * @return 1 if it is present, 0 otherwise
 */
int get(Dictionary* dico, char* word);

int write_dictionary(Dictionary * d, int fd);

#endif
