#include "pinItemQueue.h"
#include <stdlib.h>
#include <stdio.h>

void piqCreate(struct piq * q, int size)
{
  q->size = size;
  q->numItems = 0;
  q->head = 0;
  q->tail = 0;
  q->items = malloc(sizeof(struct pinItem) * size);
}

int piqPeek(struct piq * q, struct pinItem ** p)
{
  if (q->numItems > 0) {
    *p = &(q->items[q->tail]);
    return 0;
  } else {
    return -1;
  }
}

int piqInsert(struct piq * q, struct pinItem pi)
{
  if(q->numItems < q->size) {
    //Copy the item locally (no mallocing)
    q->items[q->head] = pi;
    q->head = (q->head + 1)%q->size;
    q->numItems++;
    printf("Head is %d\n", q->head);
    return 0;
  } else {
    //queue is full.
    return -1;
  }
}

int piqPop(struct piq * q, struct pinItem ** p)
{
  if(q->numItems > 0) {
    *p = &(q->items[q->tail]);
    q->tail = (q->tail + 1)%q->size;
    q->numItems--;
    printf("Tail is %d\n", q->tail);
    return 0;
  } else {
    return -1;
  }
}

void piqPrint(struct piq * q)  
{
  for(int i = 0; i < q->head; i++) {
    printf("Pos %d, pin %d\n", i, q->items[i].pin);
  }
}
