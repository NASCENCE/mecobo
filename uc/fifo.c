#include "fifo.h"
#include "efm32.h"
#include "em_int.h"
#include "mecobo.h"

void fifoPrintError(int err)
{
  switch(err) {
    case ERR_NO_ERR:
      infop("OK\n");
      break;

    case ERR_INIT_FAILED:
      infop("FIFO init failed. Possible bad malloc OR you did not give a initialized struct fifo\n");
      break;
    case ERR_FULL:
      infop("FIFO full\n");
      break;
    default:
      infop("Unknown error %d", err);
      break;
  }
}

//size is in elementSize (bytes) values.
int fifoInit(struct fifo * ff, uint32_t size, uint32_t elementSize, uint8_t * data) 
{
  if (ff == NULL) {
    return -1;
  }

  //If this is an external RAM chip we do not allocate using it
  if(!data) {
    if (!(ff->data = (uint8_t *)malloc(size * elementSize))) {
      return -1;
    }
  } else {
    ff->data = data;
  }

  ff->mutex = 0;
  ff->size = size;
  ff->elementSize = elementSize;
  ff->numElements = 0;
  ff->head = 0;
  ff->tail = 0;

  /*
  infop("Fifo init with %u elements of %u size. Data at %p\n",
        (unsigned int)ff->size,
        (unsigned int)ff->elementSize,
        ff->data);
    */
  return 0;
}

int fifoInsert(struct fifo * ff, void * element) {
  if (ff == NULL) {
    return -3;
  }

  if (element == NULL) {
    return -3;
  }

  if(ff->numElements == ff->size) {
    return -2;
  }

  //This is not thread safe.
  //Good to insert. 
  //Insert into the head, circular buffer. 
  lock_mutex(ff);
  uint8_t * offset = ff->data + (ff->head * ff->elementSize);
  memcpy(offset, element, ff->elementSize);
  ff->numElements += 1;
  ff->head = (ff->head + 1) % ff->size;  //buffer is circular but won't accept overflow
  unlock_mutex(ff);

  return 0;
}


//retrieve an element from the tail and move it one element 
//up in memory to get the next element
int fifoGet(struct fifo * ff, void ** element) {
  if (ff->numElements <= 0) {
    *element = (void *)NULL;
    return ERR_EMPTY;
  }
   

  lock_mutex(ff);
  *element = ff->data + (ff->tail * ff->elementSize);
  ff->tail = (ff->tail + 1) % ff->size;
  ff->numElements -= 1;
  unlock_mutex(ff);

  //infop("FIFO STATUS numElm: %u tail at: %u head at: %u\n", ff->numElements, ff->tail, ff->head);
  return ERR_NO_ERR;
}

inline int fifoFull(struct fifo * ff)
{
  return ff->numElements == ff->size;
}

void fifoReset(struct fifo * ff) 
{
  ff->numElements = 0;
  ff->head = 0;
  ff->tail = 0;
}


//TODO: Make these things ARM assembly (i.e. real mutexes)
void lock_mutex(struct fifo * ff)
{
  INT_Disable();
  while(ff->mutex != 0);  //wait until mutex is unlocked
  ff->mutex = 1;
}

void unlock_mutex(struct fifo * ff)
{
  ff->mutex = 0;
  INT_Enable();
}



