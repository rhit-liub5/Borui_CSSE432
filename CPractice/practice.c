#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void funciton1(int n){
    printf("In function1, n=%d\n", n);
    n += 30;
}

void function2(int* n){
    printf("In function2, *n = %d\n", *n);
    *n += 100;
}

typedef struct {
    int length;
    char* word;
} Word_tracker;

int main(int argc, char *argv[]) {
    // int a = 10;
    // int* p =&a;
    // int b[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    // int* bPtr = b;

    // printf("b[0] = %d, b[1] = %d, bPtr = %d, bPtr[1] = %d, *[bPtr + 1] = %d\n", b[0], b[1], bPtr, bPtr[1], *(bPtr + 1));
    // printf("a = %d, *p = %d, p=%p, &a = %p\n", a, *p, p, &a);

    // funciton1(a);
    // funciton1(*p);
    // printf("a = %d, *p = %d, p=%p, &a = %p\n", a, *p, p, &a);

    // function2(p);
    // function2(&a);

    if (argc < 2){
        printf("Usage: ./program filename\n");
        return 1;
    }

    char* filename = argv[1];
    FILE* fp = fopen(filename, "r");
    if(fp == NULL){
        perror("file not found\n");
        return 1;
    }
    char curWord[100];
    Word_tracker* longestWord = (Word_tracker*) malloc(sizeof(Word_tracker));
    longestWord->length = 0;
    longestWord->word = NULL;

    while(fscanf(fp, "%s", curWord) == 1){
        int curLength = strlen(curWord);
        if(curLength > longestWord->length){
            if(longestWord->word != NULL){
                free(longestWord->word);
                longestWord->length = curLength;
                longestWord->word = (char*) malloc(curLength * sizeof(char) + 1);
                strncpy(longestWord->word, curWord, curLength);
                longestWord->word[curLength] = '\0';
            }
        }
    }
    fclose(fp);
    printf("Longest word: %s, length: %d\n", longestWord->word, longestWord->length);
    return 0;
}