#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

int job_queue_init(struct job_queue *job_queue, int capacity) {
  //assert(0);
  if (capacity > 0){
    job_queue->capacity = capacity;
    job_queue->front = 0;
    job_queue->size = 0; 
    job_queue->destroyed = 0; 
    job_queue->queue = malloc(capacity * sizeof(void*));
    pthread_mutex_init(&job_queue->lock, NULL); 
    pthread_cond_init(&job_queue->isEmpty, NULL);
    pthread_cond_init(&job_queue->isNotEmpty, NULL);
    pthread_cond_init(&job_queue->isNotFull, NULL);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_destroy(struct job_queue *job_queue) {
  //assert(0);
  if ((job_queue != NULL) && (job_queue->destroyed == 0)) {
    pthread_mutex_lock(&job_queue->lock);
    while (job_queue->size > 0) {
      pthread_cond_wait(&job_queue->isEmpty, &job_queue->lock);
    }
    free(job_queue->queue);
    pthread_mutex_unlock(&job_queue->lock);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_push(struct job_queue* job_queue, void* data) {
  //assert(0);
  if ((job_queue != NULL) && (job_queue->destroyed == 0)) {
    pthread_mutex_lock(&job_queue->lock);
    while (job_queue->size == job_queue->capacity) {
      pthread_cond_wait(&job_queue->isNotFull, &job_queue->lock);  
    }
    int next_element = ((job_queue->front + 1) + (job_queue->size - 1)) % job_queue->capacity;
    job_queue->queue[next_element] = data;
    job_queue->size++;
    pthread_cond_signal(&job_queue->isNotEmpty);
    pthread_mutex_unlock(&job_queue->lock);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int job_queue_pop(struct job_queue *job_queue, void** data) {
  //assert(0);
  if ((job_queue != NULL) && (job_queue->destroyed == 0)) {
    pthread_mutex_lock(&job_queue->lock);
    while (job_queue->size <= 0) {
      pthread_cond_wait(&job_queue->isNotEmpty, &job_queue->lock);  
    }
    *data = job_queue->queue[1];
    job_queue->front = (job_queue->front + 1) % job_queue->capacity;
    job_queue->size--;
    if (job_queue->size > 0) {
      pthread_cond_signal(&job_queue->isNotFull);
    } else {
      pthread_cond_signal(&job_queue->isEmpty);
    }
    pthread_mutex_unlock(&job_queue->lock);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}