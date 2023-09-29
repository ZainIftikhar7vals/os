#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_READER_THREADS 100
#define NUM_WRITER_THREADS 100
#define X 5

int g_waiting_readers = 0;
int resource_counter = 0;        
int sample_file = 0;

pthread_mutex_t resource_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t read_phase_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t write_phase_cond = PTHREAD_COND_INITIALIZER;


void *reader_thread_func(void *thread_argument) { /* thread main */
  int id = *((int*)thread_argument);
  int local_num_readers = 0;
  int local_waiting_readers = 0;

  /* Insert into buffer */
  pthread_mutex_lock (&resource_mutex);
    g_waiting_readers++;
    while(resource_counter == -1) {
      pthread_cond_wait (&read_phase_cond, &resource_mutex);
    }
    local_waiting_readers = --g_waiting_readers;
    local_num_readers = ++resource_counter;
  pthread_mutex_unlock (&resource_mutex);

  for(int i=0; i < X; i++) {
    sleep(1);
    printf("Reader Thread ID: %d, Number of Readers Present: %d ,Waiting Readers: %d, File Data %d \n", id, local_num_readers, local_waiting_readers, sample_file);
  }

  pthread_mutex_lock (&resource_mutex);
    resource_counter--;

    if(resource_counter == 0){
      pthread_cond_signal (&write_phase_cond);
    }
  pthread_mutex_unlock (&resource_mutex);
  return 0;
}

void *writer_thread_func(void *thread_argument) { /* thread main */
  int id = *((int*)thread_argument);

  pthread_mutex_lock (&resource_mutex);  
    while(resource_counter != 0) {
      pthread_cond_wait (&write_phase_cond, &resource_mutex);
    }
    resource_counter = -1;
  pthread_mutex_unlock (&resource_mutex);

  for(int i=0; i < X; i++) {
    printf("Writer Thread ID: %d, Number of Readers Present from perpective of writer: %d, Waiting Readers: %d\n", id, resource_counter, g_waiting_readers);
    sleep(1);
    sample_file = 1;
  }
  
  pthread_mutex_lock (&resource_mutex); 
    resource_counter = 0;

    pthread_cond_broadcast (&read_phase_cond);
    pthread_cond_signal (&write_phase_cond);
  pthread_mutex_unlock (&resource_mutex);
  return 0;
}

int main(void) {
  int i;

  int reader_thread_num[NUM_READER_THREADS];
  int writer_thread_num[NUM_WRITER_THREADS];
  pthread_t reader_tids[NUM_READER_THREADS];
  pthread_t writer_tids[NUM_WRITER_THREADS];

  for(i = 0; i < NUM_WRITER_THREADS; i++) {
    writer_thread_num[i] = i;
    pthread_create(&writer_tids[i], NULL, writer_thread_func, &writer_thread_num[i]);
  }

  for(i = 0; i < NUM_READER_THREADS; i++) {
    reader_thread_num[i] = i;
    pthread_create(&reader_tids[i], NULL, reader_thread_func, &reader_thread_num[i]);
  }

  for(i = 0; i < NUM_READER_THREADS; i++) {
    pthread_join(reader_tids[i], NULL);
  }

  for(i = 0; i < NUM_WRITER_THREADS; i++) {
    pthread_join(writer_tids[i], NULL);
  }

  return 0;
}
