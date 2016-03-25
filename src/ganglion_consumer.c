#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <syslog.h>
#include <string.h>

#include "ganglion.h"
#include "_ganglion_internal.h"

static enum ganglion_message_status ganglion_kafka_internal_check_message(rd_kafka_message_t *message) {
  if (message->err && (message->err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION || message->err == RD_KAFKA_RESP_ERR__UNKNOWN_TOPIC)) {
    return GANGLION_MSG_ERROR;
  } else if (message->err && message->err == RD_KAFKA_RESP_ERR__PARTITION_EOF) {
    return GANGLION_MSG_EOF;
  } else if (message->err) {
    if (GANGLION_DEBUG)
      printf("librdkafka error code: %d\n", message->err);
    return GANGLION_MSG_UNKNOWN;
  }
  return GANGLION_MSG_OK;
}

static void * ganglion_worker_thread(void * args) {
  struct ganglion_kafka_internal_message * consumer_message = (struct ganglion_kafka_internal_message *)args;

  assert(consumer_message->consumer->callback != NULL);

  if (GANGLION_DEBUG)
    printf("Invoking callback for topic: %s\n", consumer_message->consumer->topic);

  (consumer_message->consumer->callback)(consumer_message->consumer->context, consumer_message->payload, consumer_message->length, consumer_message->partition, consumer_message->offset);

  free(consumer_message->payload);
  free(consumer_message);

  pthread_exit(NULL);
}

static struct ganglion_kafka_internal * ganglion_kafka_internal_new() {
  struct ganglion_kafka_internal * self = (struct ganglion_kafka_internal *)malloc(sizeof(struct ganglion_kafka_internal));
  assert(self != NULL);

  self->config = rd_kafka_conf_new();
  self->topic_config = rd_kafka_topic_conf_new();

#ifdef UNIT_TESTING
  rd_kafka_conf_set_log_cb(self->config, NULL);
#endif

  assert(rd_kafka_topic_conf_set(self->topic_config, "offset.store.method", "broker", NULL, 0) == RD_KAFKA_CONF_OK);
  assert(rd_kafka_topic_conf_set(self->topic_config, "enable.auto.commit", "false", NULL, 0) == RD_KAFKA_CONF_OK);
  rd_kafka_conf_set_default_topic_conf(self->config, self->topic_config);

  return self;
}

static void ganglion_kafka_internal_cleanup(struct ganglion_kafka_internal * kafka) {
  rd_kafka_topic_partition_list_destroy(kafka->topics);
  rd_kafka_destroy(kafka->consumer);
  free(kafka);
  kafka = NULL;
}

static void ganglion_kafka_internal_set_group(struct ganglion_kafka_internal * self, const char * group) {
  assert(rd_kafka_conf_set(self->config, "group.id", group, NULL, 0) == RD_KAFKA_CONF_OK);
}

static void ganglion_kafka_internal_initialize(struct ganglion_kafka_internal * self, const char *brokers, const char *topic) {
  assert(self->consumer = rd_kafka_new(RD_KAFKA_CONSUMER, self->config, NULL, 0));
  rd_kafka_set_log_level(self->consumer, LOG_DEBUG);
  assert(rd_kafka_brokers_add(self->consumer, brokers) != 0);
  rd_kafka_poll_set_consumer(self->consumer);
  self->topics = rd_kafka_topic_partition_list_new(1);
  rd_kafka_topic_partition_list_add(self->topics, topic, -1);
}

static void ganglion_kafka_internal_consume(struct ganglion_kafka_internal * self, struct ganglion_consumer *consumer) {
  int i, j;
  if (GANGLION_DEBUG)
    printf("Beginning consumption of topic: %s\n", consumer->topic);
  pthread_attr_t attr;
  rd_kafka_topic_partition_list_t *topic_partition_list;
  rd_kafka_topic_partition_t *topic_partition;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  assert((topic_partition_list = rd_kafka_topic_partition_list_new(12)) != NULL);
  assert(!rd_kafka_subscribe(self->consumer, self->topics));

  while (consumer->status != GANGLION_THREAD_CANCELED && consumer->status != GANGLION_THREAD_ERROR) {
    if (GANGLION_DEBUG)
      printf("Main consumer loop for topic: %s\n", consumer->topic);
    for (i = 0; i < consumer->worker_size; i++) {
      rd_kafka_message_t *message;
      consumer->workers[i] = NULL;
      if ((message = rd_kafka_consumer_poll(self->consumer, 1000)) != NULL) {
        enum ganglion_message_status message_check = ganglion_kafka_internal_check_message(message);
        if (message_check == GANGLION_MSG_ERROR) {
          consumer->status = GANGLION_THREAD_ERROR;
          goto done;
        } else if (message_check == GANGLION_MSG_EOF || message_check == GANGLION_MSG_UNKNOWN) { //a non-abortable error
          goto done;
        }

        if (GANGLION_DEBUG)
          printf("Topic: %s, received message\n", consumer->topic);

        struct ganglion_kafka_internal_message * consumer_message = (struct ganglion_kafka_internal_message *)malloc(sizeof(struct ganglion_kafka_internal_message));

        char * payload = (char *)malloc(message->len + 1);
        memcpy(payload, message->payload, message->len);

        consumer_message->payload = payload;
        consumer_message->length = message->len;
        consumer_message->partition = message->partition;
        consumer_message->offset = message->offset;
        consumer_message->consumer = consumer;

        assert(!pthread_create(&consumer->workers[i], &attr, ganglion_worker_thread, (void *)consumer_message));

      done:
        rd_kafka_message_destroy(message);

        for(j = 0; j < topic_partition_list->cnt; j++) {
          if (!strcmp(topic_partition_list->elems[j].topic, consumer->topic) && (topic_partition_list->elems[j].partition == message->partition)) {
            topic_partition = &topic_partition_list->elems[j];
            break;
          }
        }

        if (topic_partition == NULL) {
          topic_partition = rd_kafka_topic_partition_list_add(topic_partition_list, consumer->topic, message->partition);
          topic_partition->offset = message->offset;
        } else {
          if (topic_partition->offset < message->offset) {
            topic_partition->offset = message->offset;
          }
        }
      } else {
        break;
      }
    }
    for (i = 0; i < consumer->worker_size; i++) {
      if (consumer->workers[i] != NULL) {
        assert(!pthread_join(consumer->workers[i], NULL));
      } else {
        break;
      }
    }
    // commit offsets
    if (topic_partition_list->cnt > 0)
      assert(!rd_kafka_commit(self->consumer, topic_partition_list, 0));
  }

  pthread_attr_destroy(&attr);
  assert(!rd_kafka_consumer_close(self->consumer));
  // printf("freeing partition list\n");
  rd_kafka_topic_partition_list_destroy(topic_partition_list);
}

//Consumer functions
struct ganglion_consumer * ganglion_consumer_new(const char * brokers, int workers, const char * topic, const char * group, void * context, void (* callback)(void *, char *, int, int, long)) {
  struct ganglion_consumer * self = (struct ganglion_consumer *)malloc(sizeof(struct ganglion_consumer));
  assert(self != NULL);

  if (context == NULL) {
    self->context = self;
  } else {
    self->context = context;
  }

  assert((self->workers = (pthread_t *)malloc(sizeof(pthread_t) * workers)) != NULL);

  self->worker_size = workers;
  self->topic = topic;
  self->group = group;
  self->status = GANGLION_THREAD_INITIALIZED;
  self->callback = callback;

  struct ganglion_kafka_internal * kafka = ganglion_kafka_internal_new();
  ganglion_kafka_internal_set_group(kafka, group);
  ganglion_kafka_internal_initialize(kafka, brokers, topic);

  self->opaque = (void *)kafka;

  return self;
}

void ganglion_consumer_cleanup(struct ganglion_consumer *consumer) {
  assert(consumer->status != GANGLION_THREAD_STARTED);

  ganglion_kafka_internal_cleanup((struct ganglion_kafka_internal *)consumer->opaque);

  free(consumer->workers);
  free(consumer);
  consumer = NULL;
}

void ganglion_consumer_start(struct ganglion_consumer *self) {
  assert(self->status != GANGLION_THREAD_STARTED);
  self->status = GANGLION_THREAD_STARTED;
  ganglion_kafka_internal_consume((struct ganglion_kafka_internal *)self->opaque, self);
}

void ganglion_consumer_wait(struct ganglion_consumer *self) {
  for (int i = 0; i < self->worker_size; i++) {
    if (self->workers[i] != NULL) {
      pthread_join(self->workers[i], NULL);
    }
  }
}

void ganglion_consumer_stop(struct ganglion_consumer *self) {
  assert(self->status == GANGLION_THREAD_STARTED);
  self->status = GANGLION_THREAD_CANCELED; //stops loop;
}
