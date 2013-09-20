#ifndef __LL_H__
#define __LL_H__

struct llItem {
  struct llItem * next;
  int data;
};

void llInsert(struct llItem ** head, int data);
void llRemove(struct llItem ** head, int data);
void llClear(struct llItem ** head);

#endif
