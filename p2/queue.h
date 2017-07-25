/*
 *  FILE   : queue.h
 *  AUTHOR : Jeffrey Hunter, o.blanco
 *  WEB    : http://www.iDevelopment.info
 *  NOTES  : Define queue record structure and
 *           all forward declarations.
 */

#include <stdio.h>
#include <stdlib.h>

#define Error(Str)        FatalError(Str)
#define FatalError(Str)   fprintf(stderr, "%s\n", Str), exit(1)

typedef int ElementType;

#ifndef _Queue_h
#define _Queue_h

  struct QueueRecord;
  typedef struct QueueRecord *Queue;

  int         IsEmpty(Queue Q);
  int         IsFull(Queue Q);
  Queue       CreateQueue(int MaxElements);
  void        DisposeQueue(Queue Q);
  void        MakeEmpty(Queue Q);
  int        Enqueue(ElementType X, Queue Q, pthread_mutex_t* mutex, int wait_time, int favourite);
  ElementType Front(Queue Q);
  void        Dequeue(Queue Q);
  ElementType FrontAndDequeue(Queue Q, int favourite);
  void        Delete(ElementType X, Queue Q);

#endif  /* _Queue_h */
