#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "../src/ganglion.h"

#define BUFFER_SIZE 1024

struct ganglion_producer * producer;

struct ganglion_supervisor * supervisor;
struct ganglion_consumer * consumer_one;
struct ganglion_consumer * consumer_two;

void handle_shutdown(int signo) {
  ganglion_producer_cleanup(producer);

  if (ganglion_supervisor_is_started(supervisor)) {
    ganglion_supervisor_stop(supervisor);

    ganglion_consumer_cleanup(consumer_two);
    ganglion_consumer_cleanup(consumer_one);
    ganglion_supervisor_cleanup(supervisor);
  }

  ganglion_shutdown();

  exit(0);
}

void handle_message(void * consumer, char * payload, int length, int partition, long offset) {
  char * message = (char *)malloc(length + 1);
  memcpy(message, payload, length);
  message[length] = '\0';

  struct ganglion_consumer * self = (struct ganglion_consumer *)consumer;
  printf("Topic %s received message %s with offset %ld from partition %d\n", self->topic, message, offset, partition);

  free(message);
}

void handle_report(void * producer, const char * topic, char * payload, int length) {
  char * message = (char *)malloc(length + 1);
  memcpy(message, payload, length);
  message[length] = '\0';

  printf("Successfully sent message %s to topic: %s\n", message, topic);

  free(message);
}

int main (int argc, char **argv) {
  char buffer[BUFFER_SIZE];

  signal(SIGINT, handle_shutdown);

  supervisor = ganglion_supervisor_new();

  producer = ganglion_producer_new("localhost:9092", "example", "none", GANGLION_PRODUCER_ASYNC, 1000, 100, NULL, handle_report);
  consumer_one = ganglion_consumer_new("localhost:9092", 400, "analytics", "libganglion.analytics", NULL, handle_message);
  consumer_two = ganglion_consumer_new("localhost:9092", 10, "stream", "libganglion.stream", NULL, handle_message);

  ganglion_supervisor_register(supervisor, consumer_one);
  ganglion_supervisor_register(supervisor, consumer_two);

  ganglion_supervisor_start(supervisor);

  while(1) {
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strlen(buffer)-1] = '\0'; //remove \n

    ganglion_producer_publish(producer, "stream", buffer, strlen(buffer));
  }
  return 0;
}
