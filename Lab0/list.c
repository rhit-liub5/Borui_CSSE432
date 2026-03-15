/* File: list.c 
 * Author: 
   
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct List_node_s {
	int length;
	char *word;
    struct List_node_s *next;
} List_node;

List_node *create_node(const char *word)
{
    List_node *node = (List_node *)malloc(sizeof(List_node));

    node->length = strlen(word);
    node->word = (char *)malloc(strlen(word) + 1);

    strcpy(node->word, word);
    node->next = NULL;
    return node;
}

void add_word(List_node **head, const char *word)
{
    List_node *new_node = create_node(word);

    if (*head == NULL || new_node->length < (*head)->length)
    {
        new_node->next = *head;
        *head = new_node;
        return;
    }

    List_node *curr = *head;
    while (curr->next != NULL && curr->next->length <= new_node->length)
    {
        curr = curr->next;
    }

    new_node->next = curr->next;
    curr->next = new_node;
}

void print_list(List_node *head)
{
    List_node *curr = head;
    while (curr != NULL)
    {
        printf("%s\n", curr->word);
        curr = curr->next;
    }
}

void free_list(List_node *head)
{
    List_node *curr = head;
    while (curr != NULL)
    {
        List_node *temp = curr;
        curr = curr->next;
        free(temp->word);
        free(temp);
    }
}

int main(int argc, char *argv[])
{
    List_node *head = NULL;

    for (int i = 1; i < argc; i++)
    {
        add_word(&head, argv[i]);
    }

    print_list(head);
    free_list(head);

    return 0;
}
