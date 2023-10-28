#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

int job_queue_init(struct job_queue *job_queue, int capacity) {
  if (capacity > 0){
    job_queue->capacity = capacity;
    job_queue->front = 0;
    job_queue->size = 0; 
    job_queue->kill = 0;
    job_queue->waitingPop = 0;
    job_queue->waitingPush = 0;
    job_queue->queue = malloc(sizeof(void*) * capacity);
    pthread_mutex_init(&job_queue->lock, NULL); 
    pthread_cond_init(&job_queue->signalDestroy, NULL);
    pthread_cond_init(&job_queue->signalPop, NULL);
    pthread_cond_init(&job_queue->signalPush, NULL);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_destroy(struct job_queue* job_queue) {
  if (job_queue != NULL) {
    pthread_mutex_lock(&job_queue->lock);
    job_queue->kill = 1;
    while ((job_queue->size > 0 ) || (job_queue->waitingPop > 0)){
      if (job_queue->waitingPop > 0) {
        pthread_cond_broadcast(&job_queue->signalPop);
      }
      pthread_cond_wait(&job_queue->signalDestroy, &job_queue->lock);
    }
    pthread_mutex_unlock(&job_queue->lock);
    free(job_queue->queue);
    pthread_mutex_destroy(&job_queue->lock);
    pthread_cond_destroy(&job_queue->signalDestroy);
    pthread_cond_destroy(&job_queue->signalPop);
    pthread_cond_destroy(&job_queue->signalPush);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_push(struct job_queue* job_queue, void* data) {
  if ((job_queue != NULL)){
    pthread_mutex_lock(&job_queue->lock);
    job_queue->waitingPush ++;
    while (job_queue->size == job_queue->capacity) {
      if (job_queue->waitingPop > 0) {
        pthread_cond_broadcast(&job_queue->signalPop);
      }
      pthread_cond_wait(&job_queue->signalPush, &job_queue->lock);  
    }
    job_queue->waitingPush --;

    int next_element = ((job_queue->front + 1) + (job_queue->size - 1)) % job_queue->capacity;
    job_queue->queue[next_element] = data;
    job_queue->size ++;
    if (job_queue->waitingPop > 0) {
      pthread_cond_broadcast(&job_queue->signalPop);
    } else if (job_queue->waitingPush > 0) {
      pthread_cond_broadcast(&job_queue->signalPush);
    }
    pthread_mutex_unlock(&job_queue->lock);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_pop(struct job_queue* job_queue, void** data) {
  if ((job_queue != NULL)){
    pthread_mutex_lock(&job_queue->lock);
    job_queue->waitingPop ++;
    while ((job_queue->size == 0) && (job_queue->kill == 0)) {
      if (job_queue->waitingPush > 0) {
        pthread_cond_broadcast(&job_queue->signalPush);
      }
      pthread_cond_wait(&job_queue->signalPop, &job_queue->lock);
    }
    job_queue->waitingPop --;

    int emptyQueue = 1;
    if (job_queue->size > 0) {
      *data = job_queue->queue[job_queue->front];
      job_queue->front = (job_queue->front + 1) % job_queue->capacity;
      job_queue->size --;
      emptyQueue = 0;
    }
      
    if (job_queue->waitingPush > 0) {
      pthread_cond_broadcast(&job_queue->signalPush);
    } else if (job_queue->waitingPop > 0) {
      pthread_cond_broadcast(&job_queue->signalPop);
    } else {
      pthread_cond_broadcast(&job_queue->signalDestroy);
    }
    pthread_mutex_unlock(&job_queue->lock);

    if (emptyQueue == 0) {
      return EXIT_SUCCESS;
    } else {
      return -1;
    }
  }  
  return EXIT_FAILURE;
}