#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

int job_queue_init(struct job_queue *job_queue, int capacity) {
  assert(job_queue != NULL);
  
  //init tmp node
  // struct job_node* tmp = malloc(sizeof(struct job_node));
  // assert(tmp != NULL);
  // tmp->next = NULL;

  //init queue
  job_queue->size = 0;
  job_queue->capacity = capacity;
  job_queue->head = NULL;
  job_queue->tail = NULL;
  pthread_mutex_init(&job_queue->head_lock, NULL);
  pthread_mutex_init(&job_queue->tail_lock, NULL);

  printf("init()\n");
  return EXIT_SUCCESS;
}

int job_queue_destroy(struct job_queue *job_queue) {
  assert(job_queue != NULL);

  pthread_mutex_lock(&job_queue->destroy_lock);
  job_queue->destroy = 1;
  pthread_mutex_unlock(&job_queue->destroy_lock);

  while(job_queue->size != 0) {
    //wait until it becomes zero
  }

  free(job_queue);
  return EXIT_SUCCESS;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  //err if queue is detroyed
  assert(job_queue != NULL);

  //block requests when full
  // while (job_queue->size == job_queue->capacity) {
  //   // block request (wait or spin to handle the waiting requests)
  // }

  //enqueue new node at end
  struct job_node* new_node = malloc(sizeof(struct job_node));
  new_node->data = data;
  new_node->next = NULL;

  pthread_mutex_lock(&job_queue->tail_lock);
  if (job_queue->head == NULL && job_queue->tail == NULL) {
    job_queue->head = new_node;
    job_queue->tail = new_node;
  } else {
    job_queue->tail->next = new_node;
    job_queue->tail = new_node;
  }
  job_queue->size++;
  pthread_mutex_unlock(&job_queue->tail_lock);

  printf("push(): job size = %i\n", job_queue->size);
  return EXIT_SUCCESS;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  assert(job_queue != NULL);

  //block requests when empty
  // while(job_queue->size == 0) {
  //   if (job_queue->destroy) {
  //     return -1;
  //   }
  //   //spin or wait until queue is not empty
  // }

  pthread_mutex_lock(&job_queue->head_lock);
  
  //struct job_node* new_head = tmp->next;
  if (job_queue->head == NULL) {
    pthread_mutex_unlock(&job_queue->head_lock);
    return EXIT_FAILURE;
  }
  *(data) = job_queue->head->data;
  struct job_node* tmp = job_queue->head;
  job_queue->head = job_queue->head->next;
  job_queue->size -= 1;
  pthread_mutex_unlock(&job_queue->head_lock);

  free(tmp);

  printf("pop()\n");
  return EXIT_SUCCESS;
}
