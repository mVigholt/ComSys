#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

// Signaling protocol for worker threads
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t fill = PTHREAD_COND_INITIALIZER;
pthread_cond_t destroy = PTHREAD_COND_INITIALIZER;
pthread_mutex_t destroy_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
volatile int destroy_queue = 0;


int job_queue_init(struct job_queue *job_queue, int capacity) {
  assert(job_queue != NULL);

  // Initialize queue
  assert(pthread_mutex_lock(&init_lock) == 0);
  job_queue->size = 0;
  job_queue->capacity = capacity;
  job_queue->head = NULL;
  job_queue->tail = NULL;
  pthread_mutex_init(&job_queue->head_lock, NULL);
  pthread_mutex_init(&job_queue->tail_lock, NULL);
  assert(pthread_mutex_unlock(&init_lock) == 0);

  printf("init()\n");
  return EXIT_SUCCESS;
}

int job_queue_destroy(struct job_queue *job_queue) {
  assert(job_queue != NULL);

  assert(pthread_mutex_lock(&destroy_lock) == 0);
  destroy_queue = 1;
  // Block request if queue is not empty.
  // Wait for it to be empty.
  while(job_queue->size != 0) {
    pthread_cond_wait(&destroy, &destroy_lock);
  };
  assert(pthread_mutex_unlock(&destroy_lock) == 0);

  printf("destroy()\n");
  return EXIT_SUCCESS;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  assert(job_queue != NULL);

  // New node to push at end of queue
  struct job_node* new_node = malloc(sizeof(struct job_node));
  new_node->data = data;
  new_node->next = NULL;

  assert(pthread_mutex_lock(&job_queue->tail_lock) == 0);
  // Block requests when full, wait until availability in queue
  while (job_queue->size == job_queue->capacity) {
    pthread_cond_wait(&fill, &job_queue->tail_lock);
  }

  // Push new node to end of queue and apply new tail
  if (job_queue->head == NULL && job_queue->tail == NULL) {
    job_queue->head = new_node;
    job_queue->tail = new_node;
  } else {
    job_queue->tail->next = new_node;
    job_queue->tail = new_node;
  }
  // Increase size count
  if (job_queue->size != job_queue->capacity) job_queue->size++;
  assert(pthread_mutex_unlock(&job_queue->tail_lock) == 0);

  printf("push(): job size = %i\n", job_queue->size);
  return EXIT_SUCCESS;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  assert(job_queue != NULL);

  assert(pthread_mutex_lock(&job_queue->head_lock) == 0);
  // Block request if queue is empty. 
  // Continue when there are elements in the queue.
  while (job_queue->size == 0) {
    //if queue is set to destroy return -1  
    if (destroy_queue == 1) {
      printf("return -1\n");
      return -1;
    }
    pthread_cond_wait(&fill, &job_queue->head_lock);
  }

  // if (job_queue->head == NULL) {
  //   pthread_mutex_unlock(&job_queue->head_lock);
  //   return EXIT_FAILURE;
  // }
  *(data) = job_queue->head->data;
  struct job_node* tmp = job_queue->head;
  job_queue->head = job_queue->head->next;
  if (job_queue->size != 0) job_queue->size -= 1;

  // If job queue is empty and set to destroy, signal destroy
  if (destroy_queue && job_queue->size == 0) {
    pthread_cond_signal(&destroy);
  }

  free(tmp);
  assert(pthread_mutex_unlock(&job_queue->head_lock) == 0);

  printf("pop()\n");
  return EXIT_SUCCESS;
}
