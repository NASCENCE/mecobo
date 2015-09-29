#ifndef __FIFO_H__
#define __FIFO_H__

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct fifo {
  uint32_t mutex;
  uint32_t size;
  uint32_t elementSize;
  uint32_t numElements;
  uint32_t head;
  uint32_t tail;
  uint8_t * data;
};

#define ERR_NO_ERR 0
#define ERR_INIT_FAILED -1
#define ERR_FULL -2
#define ERR_EMPTY -3
void fifoPrintError(int error);
int fifoInit(struct fifo * ff, uint32_t size, uint32_t elementSize, uint8_t * data);
int fifoInsert(struct fifo * ff, void * elm);
int fifoGet(struct fifo * ff, void ** elm);
int fifoFull(struct fifo * ff);
void fifoReset(struct fifo * ff);
void lock_mutex(struct fifo * ff);
void unlock_mutex(struct fifo * ff);
#endif
