#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#include "../src/ganglion.h"

struct ganglion_supervisor * supervisor;
struct ganglion_consumer * consumer_one;
struct ganglion_consumer * consumer_two;

static volatile int running = 1;

void handle_shutdown(int signo) {
  if (ganglion_supervisor_is_started(supervisor)) {
    ganglion_supervisor_stop(supervisor);

    ganglion_consumer_cleanup(consumer_two);
    ganglion_consumer_cleanup(consumer_one);
    ganglion_supervisor_cleanup(supervisor);
  }
  running = 0;
}

void handle_message(void * consumer, char * payload, int partition, long offset) {
  struct ganglion_consumer * self = (struct ganglion_consumer *)consumer;
  printf("Topic %s received message %s with offset %ld from partition %d\n", self->topic, payload, offset, partition);
}

int main (int argc, char *argv[])
{
  signal(SIGINT, handle_shutdown);

  supervisor = ganglion_supervisor_new();
  consumer_one = ganglion_consumer_new("localhost:9092", 400, "analytics", "libganglion.analytics", NULL, handle_message);
  consumer_two = ganglion_consumer_new("localhost:9092", 10, "stream", "libganglion.stream", NULL, handle_message);

  ganglion_supervisor_register(supervisor, consumer_one);
  ganglion_supervisor_register(supervisor, consumer_two);

  ganglion_supervisor_start(supervisor);

  while(running) {
    sleep(1);
  }
  return 0;
}
