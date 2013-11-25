#ifndef __PINITEMQ_H__
#define __PINITEMQ_H__

#include "pinItem.h"

struct piq {
  int size;
  int numItems;
  int head;
  int tail;
  struct pinItem * items;
};

void piqCreate(struct piq * q, int size);
void piqDestroy(struct piq * q);
int  piqInsert(struct piq * q, struct pinItem pi);
int  piqPop(struct piq * q, struct pinItem ** p);
int  piqPeek(struct piq * q, struct pinItem ** p);
void piqPrint(struct piq * q);

#endif
