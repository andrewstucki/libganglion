#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <math.h>

#include "ganglion.h"
#include "_ganglion_internal.h"

//Supervisor functions
static void ganglion_consumer_thread_cleanup(void * args) {
  struct ganglion_thread_status * status = (struct ganglion_thread_status *)args;
  if (GANGLION_DEBUG)
    printf("Finished consuming topic: %s\n", ((struct ganglion_consumer *)status->context)->topic);
  status->status = GANGLION_THREAD_FINISHED;
}

static void * ganglion_consumer_thread(void * args) {
  struct ganglion_thread_status * status = (struct ganglion_thread_status *)args;

  pthread_cleanup_push(ganglion_consumer_thread_cleanup, args);

  ganglion_consumer_start((struct ganglion_consumer *)status->context);
  ganglion_consumer_wait((struct ganglion_consumer *)status->context);

  pthread_cleanup_pop(1);
  pthread_exit(NULL);
}

static void * ganglion_monitor_thread(void * args) {
  struct ganglion_supervisor * self = (struct ganglion_supervisor *) args;
  assert(self != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)self->opaque;
  assert(internal != NULL);

  pthread_attr_t attr;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  struct timespec sleep_time = { 0, 500000000L };
  int i;

  while(internal->status == GANGLION_THREAD_STARTED) {
    for (i = 0; i < internal->consumer_size; i++) {
      if (internal->consumer_statuses[i]->status == GANGLION_THREAD_FINISHED) {
        if (GANGLION_DEBUG)
          printf("Monitor found stopped thread: %d, restarting.\n", i);
        ((struct ganglion_consumer_internal *)internal->consumers[i]->opaque)->status = GANGLION_THREAD_FINISHED;
        internal->consumer_statuses[i]->status = GANGLION_THREAD_STARTED;
        assert(!pthread_create(&internal->consumer_threads[i], &attr, ganglion_consumer_thread, (void *)internal->consumer_statuses[i]));
      }
    }
    nanosleep(&sleep_time, NULL);
  }

  pthread_attr_destroy(&attr);

  pthread_exit(NULL);
}

void ganglion_supervisor_start(struct ganglion_supervisor * self) {
  assert(self != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)self->opaque;
  assert(internal != NULL);

  assert(internal->status != GANGLION_THREAD_STARTED);

  int i;
  pthread_attr_t attr;

  assert((internal->consumer_threads = (pthread_t *)malloc(sizeof(pthread_t) * internal->consumer_size)) != NULL);
  assert((internal->consumer_statuses = (struct ganglion_thread_status **)malloc(sizeof(struct ganglion_thread_status *) * internal->consumer_size)) != NULL);

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  for (i = 0; i < internal->consumer_size; i++) {
    assert((internal->consumer_statuses[i] = (struct ganglion_thread_status *)malloc(sizeof(struct ganglion_thread_status))) != NULL);
    internal->consumer_statuses[i]->thread_id = i;
    internal->consumer_statuses[i]->status = GANGLION_THREAD_STARTED;
    internal->consumer_statuses[i]->context = (void *)internal->consumers[i];
    assert(!pthread_create(&internal->consumer_threads[i], &attr, ganglion_consumer_thread, (void *)internal->consumer_statuses[i]));
  }

  assert(!pthread_create(&internal->monitor_thread, &attr, ganglion_monitor_thread, (void *)self));

  pthread_attr_destroy(&attr);

  internal->status = GANGLION_THREAD_STARTED;
}

void ganglion_supervisor_stop(struct ganglion_supervisor * self) {
  assert(self != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)self->opaque;
  assert(internal != NULL);

  assert(internal->status == GANGLION_THREAD_STARTED);

  int i;

  if (GANGLION_DEBUG)
    printf("Stopping supervisor\n");

  internal->status = GANGLION_THREAD_CANCELED; //will stop the monitor thread
  assert(!pthread_join(internal->monitor_thread, NULL));

  for (i = 0; i < internal->consumer_size; i++) {
    ganglion_consumer_stop(internal->consumers[i]);
    assert(!pthread_join(internal->consumer_threads[i], NULL));
  }

  free(internal->consumer_threads);
  free(internal->consumer_statuses);
}

int ganglion_supervisor_register(struct ganglion_supervisor * self, struct ganglion_consumer * consumer) {
  assert(self != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)self->opaque;
  assert(internal != NULL);

  if (internal->consumers == NULL) {
    assert((internal->consumers = (struct ganglion_consumer **)malloc(sizeof(struct ganglion_consumer *) * ++internal->consumer_size)) != NULL);
  } else {
    assert((internal->consumers = (struct ganglion_consumer **)realloc(internal->consumers, sizeof(struct ganglion_consumer *) * ++internal->consumer_size)) != NULL);
  }
  internal->consumers[internal->consumer_size - 1] = consumer;
  return internal->consumer_size;
}

int ganglion_supervisor_is_started(struct ganglion_supervisor * self) {
  assert(self != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)self->opaque;
  assert(internal != NULL);

  return internal->status == GANGLION_THREAD_STARTED;
}

struct ganglion_supervisor * ganglion_supervisor_new() {
  struct ganglion_supervisor * self = (struct ganglion_supervisor *)malloc(sizeof(struct ganglion_supervisor));
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)malloc(sizeof(struct ganglion_supervisor_internal));

  assert(self != NULL);
  assert(internal != NULL);

  internal->consumer_size = 0;
  internal->consumers = NULL;
  internal->status = GANGLION_THREAD_INITIALIZED;

  self->opaque = (void *)internal;

  return self;
}

void ganglion_supervisor_cleanup(struct ganglion_supervisor * supervisor) {
  assert(supervisor != NULL);
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;
  assert(internal != NULL);

  assert(internal->status != GANGLION_THREAD_STARTED);

  if (internal->consumers != NULL) {
    free(internal->consumers);
  }

  free(supervisor->opaque);
  free(supervisor);
  supervisor = NULL;
}

void ganglion_shutdown() {
  if (GANGLION_DEBUG)
    printf("Shutting down libganglion\n");
  rd_kafka_wait_destroyed(2000);
  int dangling_threads = rd_kafka_thread_cnt();
  if (dangling_threads > 0) {
    printf("Failed to clean up kafka threads, terminating. Threads still in use: %d\n", dangling_threads);
  }
}
