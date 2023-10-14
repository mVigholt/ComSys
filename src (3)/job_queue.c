#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"




int job_queue_init(struct job_queue *job_queue, int capacity) {
  if (capacity <= 0){
    return -1;
  }
  
  // Alloker hukommelse
  job_queue-> array =( void**) malloc(capacity* sizeof(void*));

   if (job_queue->array == NULL) {
      return -1;
    }
  //Initialiser jobkøen struktur
  job_queue->capacity = capacity; //tildeler capacity til queue-strukturen
  job_queue->front = job_queue->size = 0; // Denne linje initialiserer front og size til 0
  job_queue->rear = 0;// Initialiser rear

  pthread_mutex_init(&job_queue->mutex, NULL);
  pthread_cond_init(&job_queue->cond, NULL);


  return 0;

}

int isFull (struct job_queue * job_queue){
  return ( job_queue->size == job_queue->capacity);
}
int isEmpty (struct  job_queue * job_queue){
  return (job_queue->size == 0);
}


int job_queue_destroy(struct job_queue *job_queue) {

  if (job_queue== NULL){
    return -1;
  }
  free (job_queue->array);
  job_queue->capacity=0;
  job_queue-> front = job_queue->size=0;
  job_queue->rear=0;
  job_queue->death=1;

  pthread_mutex_destroy(&job_queue->mutex);
  pthread_cond_destroy(&job_queue->cond);

  return 0;

}



int job_queue_push(struct job_queue *job_queue, void *data) {
  pthread_mutex_lock(&job_queue->mutex);

  if (isFull(job_queue)) {
    pthread_cond_wait(&job_queue->cond, &job_queue->mutex);// Wait for data if the queue is empty
    
  }
  

  job_queue->array[job_queue->rear] = data;//1) pointer piger til arrya. 2) ponter til næster position, hvor det skal være.
  job_queue->rear= (job_queue->rear +1) %job_queue->capacity;//incremant +1
  job_queue->size++; //size of ellement
  
  //printf("Push: Data added successfully.\n");
  pthread_cond_signal(&job_queue->cond);//weake up agin
  pthread_mutex_unlock(&job_queue->mutex);// Unlock the mutex
  

  return 0;
}




int job_queue_pop(struct job_queue *job_queue, void **data) {

  pthread_mutex_lock(&job_queue->mutex);// Wait for data if the queue is empty

    while (isEmpty(job_queue)){
      pthread_cond_wait(&job_queue->cond, &job_queue->mutex);
    }
  
  
  *data = job_queue->array[job_queue->front];
  job_queue->front =(job_queue->front +1) %job_queue->capacity;
  job_queue->size = job_queue->size -1;
  // Unlock the mutex
  pthread_mutex_unlock(&job_queue->mutex);


  return 0;
  
}

