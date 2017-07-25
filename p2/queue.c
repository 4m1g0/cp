/*
 *  FILE   : queue.c
 *  AUTHOR : Jeffrey Hunter, o.blanco
 *  WEB    : http://www.iDevelopment.info
 *  NOTES  : Implement all functions required
 *           for a Queue data structure.
 */

#include "queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define MinQueueSize (1)
#define MAX_NSEC 1000000000

struct chair {
    ElementType element;
    pthread_cond_t *cond;
    int favourite;
    struct timespec time; // unused
};

struct QueueRecord {
  int Capacity;
  int Front;
  int Rear;
  int Size;
  struct chair *Array;
};

int IsEmpty(Queue Q) {
  return Q->Size == 0;
}

int IsFull(Queue Q) {
  return Q->Size == Q->Capacity;
}

Queue CreateQueue(int MaxElements) {
  Queue Q;

  if (MaxElements < MinQueueSize) {
    Error("CreateQueue Error: Queue size is too small.");
  }

  Q = malloc (sizeof(struct QueueRecord));
  if (Q == NULL) {
    FatalError("CreateQueue Error: Unable to allocate more memory.");
  }

  Q->Array = malloc( sizeof(struct chair) * MaxElements );
  if (Q->Array == NULL) {
    FatalError("CreateQueue Error: Unable to allocate more memory.");
  }
  
  int i;
  for (i=0; i < MaxElements; i++){
    Q->Array[i].cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
    pthread_cond_init(Q->Array[i].cond, NULL);
  }

  Q->Capacity = MaxElements;
  MakeEmpty(Q);

  return Q;
}

void MakeEmpty(Queue Q) {

  Q->Size = 0;
  Q->Front = 0;
  Q->Rear = 0;

}

void DisposeQueue(Queue Q) {
  if (Q != NULL) {
    free(Q->Array);
    free(Q);
  }
}

static int Succ(int Value, Queue Q) {
  if (++Value == Q->Capacity) {
    Value = 0;
  }

  return Value;
}

static int Pred(int Value, Queue Q) {
  if (--Value < 0) {
    Value = Q->Capacity-1;
  }
  return Value;
}

int Enqueue(ElementType X, Queue Q, pthread_mutex_t* mutex, int wait_time, int favourite) {
  struct timespec now;
  
  if (IsFull(Q)) {
    Error("Enqueue Error: The queue is full.");
  } else {
    Q->Size++;
    Q->Array[Q->Rear].element = X;
    Q->Array[Q->Rear].favourite = favourite;

    clock_gettime(CLOCK_REALTIME, &now); 
    Q->Array[Q->Rear].time = now;
    
    printf("encolamos a: %d, de la pos %d, nievo size: %d", X, pos, size);
    
    Q->Rear = Succ(Q->Rear, Q); // tengo que incrementar antes de dormir por que se va a soltar el mutex
    if (1/*wait_time < 0*/)
    {
        pthread_cond_wait(Q->Array[Pred(Q->Rear, Q)].cond, mutex);
        return 0;
    }
    
    if (now.tv_nsec + wait_time > MAX_NSEC)
    {
      now.tv_nsec = now.tv_nsec + wait_time - MAX_NSEC;
      now.tv_sec++;
    } else
      now.tv_nsec += wait_time;
    
    int error = pthread_cond_timedwait(Q->Array[Pred(Q->Rear, Q)].cond, mutex, &now);
    printf("Nos hemos cansado de esperar? %d\n", error);
    return error;
  }

}

ElementType Front(Queue Q) {

  if (!IsEmpty(Q)) {
    return Q->Array[Q->Front].element;
  }
  Error("Front Error: The queue is empty.");

  /* Return value to avoid warnings from the compiler */
  return 0;

}

void Dequeue(Queue Q) {

  if (IsEmpty(Q)) {
    Error("Dequeue Error: The queue is empty.");
  } else {
    pthread_cond_signal(Q->Array[Q->Front].cond);
    Q->Size--;
    Q->Front = Succ(Q->Front, Q);
  }

}

ElementType FrontAndDequeue(Queue Q, int favourite) {

  ElementType X = 0;
  int i, pos;

  if (IsEmpty(Q))
    Error("FrontAndDequeue Error: The queue is empty.");

  pos = Q->Front;
  if (favourite != -1)
  {
    for (i = 0; i < Q->Size; i++)
    {
      if (Q->Array[pos].favourite == favourite || Q->Array[pos].favourite == -1)
        break;
      
      if (i == Q->Size-1) // ningún cliente quiere estar contigo
        return -1;
      
      pos = Succ(pos, Q);
    }
  }
  Q->Size--;
  pthread_cond_signal(Q->Array[pos].cond);
  X = Q->Array[pos].element;
  
  printf("Desencolamos a: %d, de la pos %d, nievo size: %d", X, pos, size);
  
  if (pos != Q->Front)
  {
      struct chair seat = Q->Array[pos];
      while (pos != Q->Front) // hemos cogido a alguien de el medio hay que mover los otros.
      {
        Q->Array[pos] = Q->Array[Pred(pos, Q)];
        pos = Pred(pos, Q);
      }
      Q->Array[pos] = seat; // colocamos el hueco de nuevo en algun lado
  }
  Q->Front = Succ(Q->Front, Q);
  return X;
}

void Delete(ElementType X, Queue Q)
{
  int i;
  int pos = Q->Front;
  for (i = 0; i < Q->Size; i++)
  {
    printf("comprobamos %d, %d\n", pos, Q->Array[pos].element);
    if (Q->Array[pos].element == X)
    {
      struct chair seat = Q->Array[pos];
      while (pos != Q->Front)
      {
        Q->Array[pos] = Q->Array[Pred(pos, Q)];
        pos = Pred(pos, Q);
      } 
      Q->Array[pos] = seat;
      Q->Front = Succ(Q->Front, Q);
      return;
    }
    
    pos = Succ(pos, Q);
  }
  printf("error %d", X);
  Error("Delete Error: Not found.");
}
