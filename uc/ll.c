#include "ll.h"
#include <stdlib.h>
#include <stdio.h>

void llInsert(struct llItem ** head, int data)
{
  printf("Inserting element %d\n", data);
  struct llItem * new = malloc(sizeof(struct llItem));
  new->data = data;
  new->next = (*head);
  *head = new;
}

void llRemove(struct llItem ** head, int data)
{
  printf("Removing %d\n", data);
  struct llItem * curr = *head;
  struct llItem * prev = *head;

  while(curr != NULL) {
    if(curr->data == data) {
      
      if(curr == *head) {
        //only element in list, special.
        *head = NULL;
      } else {
        prev->next = curr->next;
      }

      free(curr);
      printf("Removed element\n");
      return;
    }
    prev = curr;
    curr = curr->next;
  }
}

void llClear(struct llItem ** head) {
  printf("Clearing linked list\n");

  struct llItem * c = *head;
  struct llItem * tmp;

  while(c != NULL) {
    tmp = c->next;
    free(c);
    c = tmp;
  }

  *head = NULL;
  printf("linked list cleared\n");
}
