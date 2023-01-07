#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#define ARG_NUM 4
#define NULL_T_SIZE 1
#define MAX_WORD_LEN 50

/**
 * Author: Ethan Pinto
 * Student Number: s4642286
 * Course Code: CSSE2310
 */

/* The arguments provided in the command line. */
typedef struct {
    int index;
    char *data;
} ArgType;

/**
 * This function creates the structs for the command line arguments.
 * Returns an array of pointers pointing to each struct.
 * Throughout the program, each argument is represented as follows:
 * argStructs[0] = specifier, argStructs[1] = single letter
 * argStructs[2] = letters, argStructs[3] = dictionary file
 */
ArgType **create_structs(void) {
    ArgType **argStructs = (ArgType **) malloc(sizeof(ArgType *) * ARG_NUM);

    // Fill array with pointers to structs containing info about each argument.
    argStructs[0] = (ArgType *) malloc(sizeof(ArgType)); // specifier
    argStructs[1] = (ArgType *) malloc(sizeof(ArgType)); // single letter
    argStructs[2] = (ArgType *) malloc(sizeof(ArgType)); // letters
    argStructs[3] = (ArgType *) malloc(sizeof(ArgType)); // dictionary file

    // Set the value of each argument to be NULL initially.
    argStructs[0]->data = NULL;
    argStructs[1]->data = NULL;
    argStructs[2]->data = NULL;
    argStructs[3]->data = NULL;
    
    return argStructs;
}

/**
 * Takes in an array consisting of the argument structs
 * and frees the memory allocated for each.
 * Returns nothing.
 */
void free_structs(ArgType **argStructs) {
    for (int i = 0; i < ARG_NUM; i++) {
        // Free data in each argument struct.
        if (argStructs[i]->data) { 
            free(argStructs[i]->data);
            argStructs[i]->data = NULL;
        }
        free(argStructs[i]);
    }
    free(argStructs);
}

/**
 * Handles all the cases that arise through errors in command line
 * argument inputs. It is one of the main exit pathways for the program.
 * It takes in an exit code and the array of argument structs.
 * Returns nothing.
 */
void err_check(int exitcode, ArgType **argStructs) {
    switch (exitcode) {
        case 1: // Invalid command line inputs.
            fprintf(stderr, "Usage: unjumble [-alpha|-len|-longest]" 
                    " [-include letter] letters [dictionary]\n");
            free_structs(argStructs);
            exit(1);

        case 2: // Dictionary file cannot be opened or does not exist.
            fprintf(stderr, "unjumble: file \"%s\" can not be opened\n",
                    argStructs[3]->data);
            free_structs(argStructs);
            exit(2);

        case 3: // Less than 3 characters in letters argument.
            fprintf(stderr, "unjumble: must supply at least three letters\n");
            free_structs(argStructs);       
            exit(3);

        case 4: // Letters are not all alphabetical.
            fprintf(stderr, "unjumble: can only unjumble alphabetic "
                    "characters\n");
            free_structs(argStructs);           
            exit(4);

        default:
            return;
    }
}

/**
 * Checks if an input string (letterTest) is composed only of letters. 
 * Returns 1 if all characters are letters
 * Returns -1 if some characters are not letters.
 */
int alpha_check(char *letterTest) {
    int letterCount = 0;

    for (int i = 0; i < strlen(letterTest); i++) {
        // Check if each character is a letter.
        if (isalpha(letterTest[i]) != 0) {
            letterCount += 1;
        }
    } 
    if (letterCount == strlen(letterTest)) {
        return 1;
    } else {
        return -1;
    }
}

/**
 * Checks each argument in argv to find the optional and required arguments.
 * It returns an array with length argc, and the value at each index
 * is a 0 (optional argument) or 1 (required argument).
 */
int *find_optional(int argc, char **argv, ArgType **argStructs) {
    int *indexList = (int *) malloc(sizeof(int) * argc);
    // Index 0 is the program name, so it will be assigned a value of 0.
    indexList[0] = 0;

    for (int i = 1; i < argc; i++) {
        // Try to open argument as if it were a file.
        FILE *dictFile = fopen(argv[i], "r");
        if (argv[i][0] == '-') {
            indexList[i] = 0;
        } else if (strlen(argv[i]) == 1) {
            // Check if argument is the single letter after '-include'
            if (argStructs[1]->data) {
                if (argStructs[1]->index == i) {
                    indexList[i] = 0;
                }
            }
        } else if (dictFile != NULL) {
            fclose(dictFile);
            indexList[i] = 0;
        } else {
            // Argument could be the letters argument.
            indexList[i] = 1;
        }
    }
    return indexList;
}

/**
 * Identifies the dictionary input, check if is a valid file, and stores
 * the value if it is. Returns an exitcode 2 if the file is invalid,
 * returns an exitcode 1 if there are arguments after the filename, 
 * and returns 0 if no errors were encountered.
 */
int is_dict(int argc, char **argv, ArgType **argStructs) {
    argStructs[3]->data = (char *) malloc(strlen("/usr/share/dict/words")
            + NULL_T_SIZE);

    // Check if the letters argument is the last argument.
    if (argStructs[2]->index == argc - 1) {
        // No dictionary file was input. Set dictionary to default values.
        strcpy(argStructs[3]->data, "/usr/share/dict/words");

    } else if (argStructs[2]->index < argc) {
        FILE *dictFile = fopen(argv[argStructs[2]->index + 1], "r");
        argStructs[3]->index = argStructs[2]->index + 1;
        argStructs[3]->data = (char *)realloc(argStructs[3]->data,
                strlen(argv[argStructs[3]->index])
                + NULL_T_SIZE);
        strcpy(argStructs[3]->data, argv[argStructs[3]->index]);

        if (argStructs[3]->index != argc - 1) {
            // Dictionary argument is not last argument.
            return 1;
        } else if (dictFile == NULL) { 
            return 2;
        } else {
            fclose(dictFile);
        }
    }
    return 0;
}

/**
 * Identifies and stores the letters argument (if present). 
 * Returns 0 if no errors occurred, 1 if there is a usage error,
 * 3 if letters doesn't have more than 3 characters, and 4 if not all
 * letters are alphabetical.
 */
int is_letters(int argc, char **argv, ArgType **argStructs) {
    int letterNum = 0;
    int *indexList = find_optional(argc, argv, argStructs);
   
    // Calculate number of possible letters arguments.
    for (int i = 0; i < argc; i++) {
        if (indexList[i] == 1) {
            letterNum++;
        }
    } 
    // Check if letters argument is not present, if so, return 1.
    if (letterNum == 0) {
        free(indexList);
        return 1;
    } 

    argStructs[2]->data = (char *) malloc(sizeof(char));
    // Iterate through arguments to find the letters argument.
    for (int i = 0; i < argc; i++) {
        if (indexList[i] == 1) {
            // Check properties of argument (type of characters and length)
            if (strlen(argv[i]) < 3) {
                free(indexList);
                return 3;
            } else if (alpha_check(argv[i]) == -1) {
                free(indexList);
                return 4;
            } else {
                // The letters argument has been found.
                argStructs[2]->index = i;
                argStructs[2]->data = (char *) realloc(argStructs[2]->data, 
                        strlen(argv[i]) + NULL_T_SIZE);
                strcpy(argStructs[2]->data, argv[i]);
                
                free(indexList);
                return 0;
            }
        }   
    }
    return 0;
}

/**
 * Finds the index and type of specifier in the command line arguments
 * if there is one present. Also identifies if -include is present as
 * well as if it is followed by a valid single letter.
 * Returns 0 if no errors occurred, and 1 for a usage error.
 */
int is_spec(int argc, char **argv, ArgType **argStructs) {
    int specNum = 0, incNum = 0;
    argStructs[0]->data = (char *) malloc(sizeof(char) + NULL_T_SIZE);
    argStructs[1]->data = (char *) malloc(sizeof(char) + NULL_T_SIZE);
   
    for (int i = 1; i < argc; i++) {
        // Search each argument for specifiers (-alpha, -len and -longest)
        if ((strcmp(argv[i], "-alpha") == 0) || (strcmp(argv[i], "-len") == 0)
                || (strcmp(argv[i], "-longest") == 0)) {
            specNum++;
            argStructs[0]->index = i;
            argStructs[0]->data = (char *) realloc(argStructs[0]->data,
                    strlen(argv[i]) + NULL_T_SIZE);
            strcpy(argStructs[0]->data, argv[i]);

        } else if (strcmp(argv[i], "-include") == 0) {
            incNum++;
            // Check if the argument following '-include' is a single letter.
            if (strlen(argv[i + 1]) == 1 && isalpha(*argv[i + 1]) != 0) {
                // Get the index and value of the single letter.
                argStructs[1]->index = i + 1;
                strcpy(argStructs[1]->data, argv[i + 1]);
            } else {
                return 1;
            }
        } else if (argv[i][0] == '-') {
            // There are other arguments that start with '-' but are invalid.
            return 1;
        }
    }
    if (specNum > 1 || incNum > 1) {
        // More than one specifier is present in the command line.
        return 1;
    } else if (specNum == 0 && incNum == 0) {
        // Free memory and clear argument data.
        free(argStructs[0]->data);
        free(argStructs[1]->data);
        argStructs[0]->data = NULL;
        argStructs[1]->data = NULL;
    } else if (incNum == 0) {
        free(argStructs[1]->data);
        argStructs[1]->data = NULL;
    } else if (specNum == 0) {
        free(argStructs[0]->data);
        argStructs[0]->data = NULL;
    }
    return 0;
}

/* SORTING FUNCTIONS */

/**
 * Counts the number of words in a dictionary file.
 * Returns an integer corresponding to the word count.
 */ 
int count_words(ArgType **argStructs) {
    FILE *dictFile = fopen(argStructs[3]->data, "r");
    int wordCount = 0;
    char temp[MAX_WORD_LEN];
    
    // Calculate number of words in dictionary file.
    while (fgets(temp, MAX_WORD_LEN, dictFile) != NULL) {
        wordCount++;
    }
    fclose(dictFile);
    return wordCount;
}

/**
 * Reads a dictionary file and returns an array of all the words
 * contained in the dictionary file.
 * Note: If no dictionary file was input into the command line, the
 * default dictionary file is read.
 */ 
char **get_words(ArgType **argStructs) {
    FILE *dictFile = fopen(argStructs[3]->data, "r");
    int wordCount = count_words(argStructs);

    // Create an array to hold all words provided in dictionary file.
    char **wordList = (char **) malloc(sizeof(char *) * wordCount);
    // Create a temporary array to store each word.
    char word[MAX_WORD_LEN];

    // Allocate memory to each pointer and store words in array.
    for (int i = 0; i < wordCount; i++) {
        fgets(word, MAX_WORD_LEN, dictFile);
        word[strlen(word) - 1] = '\0';
        wordList[i] = (char *) malloc(strlen(word) + NULL_T_SIZE);
        strcpy(wordList[i], word);
    }
    fclose(dictFile);
    return wordList;
}

/**
 * Takes in the argStructs array and a pointer to a string and compares
 * each letter in the string to each letter in the letters argument.
 * Returns 1 if all letters in the word match a letter in the letters argument
 * and returns -1 if some letters do not match.
 */ 
int has_all_letters(ArgType **argStructs, char *word) {
    int matchingLetters = 0;

    char *lettersCopy = (char *)malloc(strlen(argStructs[2]->data) +
            NULL_T_SIZE);
    strcpy(lettersCopy, argStructs[2]->data);

    // Iterate through the word and letters argument and match letters.
    for (int j = 0; j < strlen(word); j++) {
        char letter1 = (char)toupper((int)word[j]);
        for (int i = 0; i < strlen(lettersCopy); i++) {
            if (lettersCopy[i] != '-') {
                char letter2 = (char)toupper((int)lettersCopy[i]);
                if (letter1 == letter2) {
                    matchingLetters++;
                    // Remove letter as it has already been used.
                    lettersCopy[i] = '-'; 
                    break;
                }
            }
        }
    }
    free(lettersCopy);

    if (strlen(word) == matchingLetters) {
        return 1;
    } else {
        return -1;
    }
}

/**
 * This function takes in two char *, letter and word, and returns 1
 * if the word contains the specified letter. It returns -1 otherwise.
 */ 
int has_single_letter(char *letter, char *word) {
    char singleLetter = (char)toupper((int)letter[0]);
    for (int j = 0; j < strlen(word); j++) {
        char wordLetter = (char)toupper((int)word[j]);
        if (singleLetter == wordLetter) {
            return 1;
        }
    }
    return -1;
}

/**
 * Takes in array of string pointers which point to dictionary words.
 * Sorts them based on if they can be made with the letters provided
 * in the letters argument.
 * Returns an array of sorted words.
 */ 
char **sort_normal(ArgType **argStructs, int *numWords) {
    int numberWords = 0;
    // Get the list of dictionary words.
    char **wordList = get_words(argStructs);
    int wordCount = count_words(argStructs);

    // Sort through dictionary words and filter in matching words.
    for (int i = 0; i < wordCount; i++) {
        if (strlen(wordList[i]) > strlen(argStructs[2]->data) ||
                strlen(wordList[i]) < 3 ||
                has_all_letters(argStructs, wordList[i]) == -1) { 
            free(wordList[i]);
            wordList[i] = NULL;
        }
        // Remove all words that don't contain the single letter (if present)
        if (argStructs[1]->data && wordList[i]) {
            if (has_single_letter(argStructs[1]->data, wordList[i]) == -1) {
                free(wordList[i]);
                wordList[i] = NULL;
            }
        }
    }
    
    char **sortedWords = (char **) malloc(sizeof(char *) * wordCount);
    // Transfer all words into a new array containing only words.
    int wordIndex = 0;
    for (int k = 0; k < wordCount; k++) {
        if (wordList[k]) {
            sortedWords[wordIndex] = (char *) malloc(strlen(wordList[k]) +
                    NULL_T_SIZE);
            strcpy(sortedWords[wordIndex], wordList[k]);
            free(wordList[k]);
            numberWords++;
            wordIndex++;
        }
    }
    // Free the list of words provided by the get_words function.
    free(wordList);
    *numWords = numberWords;
    return sortedWords;
}

/**
 * The comparison function for the "-alpha" argument. It is used to sort
 * the array of dictionary words in lexicographical order.
 * Returns 1 if word1 should go after word2.
 * Returns -1 if word1 should go before word2
 * Returns 0 if the words are the same alphabetically.
 */
int cmp_alpha(const void *word1, const void *word2) {
    int result = strcasecmp(*(const char **)word1, *(const char **)word2);
    // Check the return value of strcasecmp function.
    if (result == 0) {
        int compareValue = strcmp(*(const char **)word1,
                *(const char **)word2);
        if (compareValue > 0) {
            return 1;
        } else if (compareValue < 0) {
            return -1;
        } else {
            return 0;
        }
    } 
    return result;
}

/**
 * The comparison function for the "-len" argument. It is used to sort
 * the array of dictionary words in descending length.
 * Returns 1 if word1 is shorter than word2.
 * Returns -1 if word1 is longest than word2.
 * Returns result of cmp_alpha if the words are the same length.
 */ 
int cmp_len(const void *word1, const void *word2) {
    // Compare the length of each word.
    if (strlen(*(const char **)word1) > strlen(*(const char **)word2)) {
        return -1; // word 1 is longer than word 2.
    } else if (strlen(*(const char **)word1) < strlen(*(const char **)word2)) {
        return 1; // word 1 is shorter than word 2.
    } 
    return cmp_alpha(word1, word2);
}

/**
 * Finds the largest word/s in the dictionary using the cmp_len function
 * and removes any words that are shorter than this word length.
 * Returns nothing.
 */ 
void cmp_longest(char **sortedWords, int wordCount) {
    qsort(sortedWords, wordCount, sizeof(const char *), cmp_len);
    int maxLength = 0;
    
    // Find the length of the longest word in the sorted words list.
    for (int i = 0; i < wordCount; i++) {
        if (sortedWords[i]) {
            maxLength = strlen(sortedWords[i]);
            break;
        }
    }
    // Remove all words that have a length smaller than maxLength.
    for (int j = 0; j < wordCount; j++) {
        if (sortedWords[j]) {
            if (strlen(sortedWords[j]) < maxLength) {
                free(sortedWords[j]);
                sortedWords[j] = NULL;
            }
        }
    }
}

/**
 * The entry point to the unjumble program. This function
 * handles the logical flow of the program and oversees
 * the outcomes of the program.
 */ 
int main(int argc, char **argv) {
    // Create the structs for the command line arguments.
    ArgType **argStructs = create_structs();

    // Check number of inputs.
    if (argc < 2 || argc > 6) {
        err_check(1, argStructs);
    }

    // Check for errors encountered while finding and storing required values.
    err_check(is_spec(argc, argv, argStructs), argStructs);
    err_check(is_letters(argc, argv, argStructs), argStructs);
    err_check(is_dict(argc, argv, argStructs), argStructs); 

    // Calculate number of words to be printed.
    int actualWordCount = 0;
    char **sortedWords = sort_normal(argStructs, &actualWordCount);

    if (actualWordCount == 0) {
        exit(10);
    }

    // Sort the dictionary words according to the specifier provided.
    if (argStructs[0]->data) {
        if (strcmp(argStructs[0]->data, "-alpha") == 0) {
            qsort(sortedWords, actualWordCount, sizeof(const char *),
                    cmp_alpha);

        } else if (strcmp(argStructs[0]->data, "-len") == 0) {
            qsort(sortedWords, actualWordCount, sizeof(const char *), cmp_len);

        } else if (strcmp(argStructs[0]->data, "-longest") == 0) {
            cmp_longest(sortedWords, actualWordCount);
        }
    }
    // Print words to standard output.
    for (int i = 0; i < actualWordCount; i++) {
        if (sortedWords[i]) {
            printf("%s\n", sortedWords[i]);
            free(sortedWords[i]);
        }
    }
    free(sortedWords);
    free_structs(argStructs);
    exit(0);
    return 0;
}
