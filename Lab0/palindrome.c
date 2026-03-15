/* File: palindrome.c 
   Author: Borui Liu
   
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> //allows to use "bool" as a boolean type
#include <ctype.h>
#include <string.h>

    /*Optional functions, uncomment the next two lines
     * if you want to create these functions after main: */
    // bool readLine(char** line, size_t* size, size_t* length);

    /*
     * NOTE that I used char** for the line above... this is a pointer to
     * a char pointer.  I used this because of the availability of
     * a newer function getline which takes 3 arguments (you should look it
     * up) and the first argument is a char**.  You can create a char*, say
     * called var, and to make it a char** just use &var when calling this
     * function.  If this is too confusing, you can use fgets instead.  Feel
     * free to change the function prototypes as you need them.
     * Also, note the use of size_t as a type.  You can look this up, but
     * essentially, this is just a special type of int to track sizes of
     * things like strings...
     */

    bool isPalindrome(const char *line, size_t len)
{

  char clean[100];
  char reverse[100];

  int i, j = 0;

  for (i = 0; line[i] != '\0'; i++)
  {

    if (isalpha(line[i]))
    {
      clean[j] = tolower(line[i]);
      j++;
    }
  }

  clean[j] = '\0';

  for (i = 0; i < j; i++)
  {
    reverse[i] = clean[j - 1 - i];
  }

  reverse[j] = '\0';

  if (strcmp(clean, reverse) == 0)
    return true;
  else
    return false;
};

int main(int argc, char *argv[]){
    //your code here
    char input[100];

    while (true){
        printf("Enter a string (or 'exit' to quit): ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break; // EOF or error
        }

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, ".") == 0)
        {
          break;
        }

        if (isPalindrome(input, strlen(input))) {
            printf("'%s' is a palindrome.\n", input);
        } else {
            printf("'%s' is not a palindrome.\n", input);
        }
    }
}
