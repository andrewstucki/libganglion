#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include <rdkafka.h>

#include "ganglion.h"

//Supervisor functions
void ganglion_consumer_thread_cleanup(void * args) {
  struct ganglion_thread_status * status = (struct ganglion_thread_status *)args;
  printf("Finished consuming topic: %s\n", ((struct ganglion_consumer *)status->context)->topic);
  status->status = GANGLION_THREAD_FINISHED;
}

void * ganglion_consumer_thread(void * args) {
  struct ganglion_thread_status * status = (struct ganglion_thread_status *)args;

  pthread_cleanup_push(ganglion_consumer_thread_cleanup, args);

  ganglion_consumer_start((struct ganglion_consumer *)status->context);
  ganglion_consumer_wait((struct ganglion_consumer *)status->context);

  pthread_cleanup_pop(1);
  pthread_exit(NULL);
}

void * ganglion_monitor_thread(void * args) {
  struct ganglion_supervisor * self = (struct ganglion_supervisor *) args;
  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  struct timespec sleep_time = { 0, 500000000L };
  int i;

  while(self->status == GANGLION_THREAD_STARTED) {
    for (i = 0; i < self->consumer_size; i++) {
      if (self->consumer_statuses[i]->status == GANGLION_THREAD_FINISHED) {
        printf("Monitor found stopped thread: %d, restarting.\n", i);
        self->consumers[i]->status = GANGLION_THREAD_FINISHED;
        self->consumer_statuses[i]->status = GANGLION_THREAD_STARTED;
        assert(!pthread_create(&self->consumer_threads[i], &attr, ganglion_consumer_thread, (void *)self->consumer_statuses[i]));
      }
    }
    nanosleep(&sleep_time, NULL);
  }

  pthread_attr_destroy(&attr);

  pthread_exit(NULL);
}

void ganglion_supervisor_start(struct ganglion_supervisor * self) {
  assert(self->status != GANGLION_THREAD_STARTED);

  int i;
  pthread_attr_t attr;

  assert((self->consumer_threads = (pthread_t *)malloc(sizeof(pthread_t) * self->consumer_size)) != NULL);
  assert((self->consumer_statuses = (struct ganglion_thread_status **)malloc(sizeof(struct ganglion_thread_status *) * self->consumer_size)) != NULL);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (i = 0; i < self->consumer_size; i++) {
    assert((self->consumer_statuses[i] = (struct ganglion_thread_status *)malloc(sizeof(struct ganglion_thread_status))) != NULL);
    self->consumer_statuses[i]->thread_id = i;
    self->consumer_statuses[i]->status = GANGLION_THREAD_STARTED;
    self->consumer_statuses[i]->context = (void *)self->consumers[i];
    assert(!pthread_create(&self->consumer_threads[i], &attr, ganglion_consumer_thread, (void *)self->consumer_statuses[i]));
  }

  assert(!pthread_create(&self->monitor_thread, &attr, ganglion_monitor_thread, (void *)self));

  pthread_attr_destroy(&attr);

  self->status = GANGLION_THREAD_STARTED;
}

void ganglion_supervisor_stop(struct ganglion_supervisor * self) {
  assert(self->status == GANGLION_THREAD_STARTED);

  int i;

  printf("Stopping supervisor\n");

  self->status = GANGLION_THREAD_CANCELED; //will stop the monitor thread
  assert(!pthread_join(self->monitor_thread, NULL));

  for (i = 0; i < self->consumer_size; i++) {
    ganglion_consumer_stop(self->consumers[i]);
    assert(!pthread_join(self->consumer_threads[i], NULL));
  }

  free(self->consumer_threads);
  free(self->consumer_statuses);
}

int ganglion_supervisor_register(struct ganglion_supervisor * self, struct ganglion_consumer * consumer) {
  if (self->consumers == NULL) {
    assert((self->consumers = (struct ganglion_consumer **)malloc(sizeof(struct ganglion_consumer *) * ++self->consumer_size)) != NULL);
  } else {
    assert((self->consumers = (struct ganglion_consumer **)realloc(self->consumers, sizeof(struct ganglion_consumer *) * ++self->consumer_size)) != NULL);
  }
  self->consumers[self->consumer_size - 1] = consumer;
  return self->consumer_size;
}

int ganglion_supervisor_is_started(struct ganglion_supervisor * self) {
  return self->status == GANGLION_THREAD_STARTED;
}

struct ganglion_supervisor * ganglion_supervisor_new() {
  struct ganglion_supervisor * self = (struct ganglion_supervisor *)malloc(sizeof(struct ganglion_supervisor));

  assert(self != NULL);

  self->consumer_size = 0;
  self->consumers = NULL;
  self->status = GANGLION_THREAD_INITIALIZED;

  return self;
}

void ganglion_supervisor_cleanup(struct ganglion_supervisor * supervisor) {
  assert(supervisor->status != GANGLION_THREAD_STARTED);

  free(supervisor->consumers);
  free(supervisor);
  supervisor = NULL;

  rd_kafka_wait_destroyed(2000);
  int dangling_threads = rd_kafka_thread_cnt();
  if (dangling_threads > 0) {
    printf("Failed to clean up librdkafka, terminating, threads still in use: %d\n", dangling_threads);
  }
}
