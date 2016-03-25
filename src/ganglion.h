#ifndef __GANGLION_H__
#define __GANGLION_H__

#include <signal.h>
#include <pthread.h>

#ifndef GANGLION_DEBUG
#define GANGLION_DEBUG 0
#endif

// Structs for consumer
struct ganglion_consumer {
  int worker_size;
  const char * topic;
  const char * group;

  void *opaque;
};

// Structs for supervisor
struct ganglion_supervisor {
  void *opaque;
};

// Structs for producer
struct ganglion_producer {
  const char * id;
  const char * compression;

  int queue_length; // batch.num.messages
  int queue_flush_rate; // queue.buffering.max.ms

  void *opaque;
};

struct ganglion_consumer * ganglion_consumer_new(const char *, int, const char *, const char *, void *, void (*)(void *, char *, int, int, long));
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

struct ganglion_producer * ganglion_producer_new(const char *, const char *, const char *, int, int, void *, void (*)(void *, const char *, char *, int));
void ganglion_producer_cleanup(struct ganglion_producer *);
void ganglion_producer_publish(struct ganglion_producer *, char *, char *, long long);

void ganglion_shutdown();

#endif // __GANGLION_H__
