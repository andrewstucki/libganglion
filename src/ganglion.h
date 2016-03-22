#ifndef __GANGLION_H__
#define __GANGLION_H__

#include <signal.h>
#include <pthread.h>

// Structs for threads (either workers or consumers)
enum ganglion_thread_finished_status {
  GANGLION_THREAD_INITIALIZED,
  GANGLION_THREAD_STARTED,
  GANGLION_THREAD_FINISHED,
  GANGLION_THREAD_CANCELED,
  GANGLION_THREAD_ERROR
};

enum ganglion_message_status {
  GANGLION_MSG_EOF,
  GANGLION_MSG_ERROR,
  GANGLION_MSG_UNKNOWN,
  GANGLION_MSG_OK
};

struct ganglion_thread_status {
  long thread_id;
  volatile sig_atomic_t status;
  void * context;
};

// Structs for consumer
struct ganglion_consumer {
  int worker_size;
  const char * topic;
  const char * group;
  void (* callback)(void *, char *, int, long);

  struct ganglion_thread_status ** worker_statuses;
  pthread_t * workers;

  volatile sig_atomic_t status;

  void * context;
  void * opaque;
};

// Structs for supervisor
struct ganglion_supervisor {
  int consumer_size;
  struct ganglion_consumer ** consumers;
  struct ganglion_thread_status ** consumer_statuses;
  pthread_t * consumer_threads;

  pthread_t monitor_thread;
  volatile sig_atomic_t status;
};

struct ganglion_consumer * ganglion_consumer_new(const char *, int, const char *, const char *, void *, void (*)(void *, char *, int, long));
void ganglion_consumer_cleanup(struct ganglion_consumer *);
void ganglion_consumer_start(struct ganglion_consumer *);
void ganglion_consumer_stop(struct ganglion_consumer *);
void ganglion_consumer_wait(struct ganglion_consumer *);

struct ganglion_supervisor * ganglion_supervisor_new();
void ganglion_supervisor_cleanup(struct ganglion_supervisor *);
void ganglion_supervisor_start(struct ganglion_supervisor *);
void ganglion_supervisor_stop(struct ganglion_supervisor *);
int ganglion_supervisor_register(struct ganglion_supervisor *, struct ganglion_consumer *);
int ganglion_supervisor_is_started(struct ganglion_supervisor *);

#endif // __GANGLION_H__
