#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

// Signaling protocol for threads
pthread_cond_t push_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t pop_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
volatile int size;
volatile int destroy_queue = 0;


int job_queue_init(struct job_queue *job_queue, int capacity) {
  assert(job_queue != NULL);

  // Initialize queue
  assert(pthread_mutex_lock(&m) == 0);
  job_queue->capacity = capacity;
  job_queue->head = job_queue->tail = NULL;
  assert(pthread_mutex_unlock(&m) == 0);

  return EXIT_SUCCESS;
}

int job_queue_destroy(struct job_queue *job_queue) {
  assert(job_queue != NULL);

  assert(pthread_mutex_lock(&m) == 0);
  // Set destroy
  destroy_queue = 1;
  // Signal waiting pop() requests
  assert(pthread_cond_signal(&pop_cond) == 0);
  assert(pthread_mutex_unlock(&m) == 0);

  return EXIT_SUCCESS;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  assert(job_queue != NULL);

  assert(pthread_mutex_lock(&m) == 0);
  // Block requests when full, signal pop() and wait until availability in queue
  while (size == job_queue->capacity) {
    assert(pthread_cond_signal(&pop_cond) == 0);
    assert(pthread_cond_wait(&push_cond, &m) == 0);
  }

  // New node
  struct job_node* new_node = malloc(sizeof(struct job_node));
  new_node->data = data;
  new_node->next = NULL;

  // Enqueue new node
  if (job_queue->tail == NULL || job_queue->head == NULL) {
    job_queue->head = new_node;
    job_queue->tail = new_node;
  } else {
    job_queue->tail->next = new_node;
    job_queue->tail = new_node;
  }

  // Increase size count
  assert(size < job_queue->capacity);
  size++;
  
  assert(pthread_cond_signal(&pop_cond) == 0);
  assert(pthread_mutex_unlock(&m) == 0);

  return EXIT_SUCCESS;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  assert(job_queue != NULL);

  assert(pthread_mutex_lock(&m) == 0);
  // Block request if queue is empty. 
  while (job_queue->head == NULL) {
    // If queue is set to destroy return -1, and signal waiting pop() requests
    if (destroy_queue) {
      assert(pthread_cond_signal(&pop_cond) == 0);
      assert(pthread_mutex_unlock(&m) == 0);
      return -1;
    }
    assert(pthread_cond_wait(&pop_cond, &m) == 0);
  }

  // Dequeue head
  assert(job_queue->head != NULL);
  *(data) = job_queue->head->data;
  struct job_node* tmp = job_queue->head;
  job_queue->head = job_queue->head->next;

  // Decrease size count
  assert(size > 0);
  size -= 1;
  
  // If job-queue is empty, signal push()
  if (!destroy_queue && job_queue->head == NULL) {
    assert(pthread_cond_signal(&push_cond) == 0);
  }

  free(tmp);
  assert(pthread_mutex_unlock(&m) == 0);

  return EXIT_SUCCESS;
}
