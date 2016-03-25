#include <stdlib.h>
#include <assert.h>

#include <rdkafka.h>

#include "ganglion.h"
#include "_ganglion_internal.h"

static void report_callback_wrapper(rd_kafka_t *kafka, const rd_kafka_message_t *message, void *context) {
  struct ganglion_producer * self = (struct ganglion_producer *)context;

  assert(self);
  struct ganglion_producer_internal * internal = (struct ganglion_producer_internal *)self->opaque;
  assert(internal);

  //TODO: error handling, maybe something like:
  //if (message->err) (internal->error_callback)(internal->context, rd_kafka_topic_name(message->rkt), (char *)message->err, message->len);
  (internal->report_callback)(internal->context, rd_kafka_topic_name(message->rkt), (char *)message->payload, message->len);
}

static void * ganglion_producer_poller_thread(void * args) {
  struct ganglion_producer_internal * producer = (struct ganglion_producer_internal *)args;

  while(producer->status != GANGLION_THREAD_CANCELED) {
    rd_kafka_poll(producer->producer, 1000);
  }

  pthread_exit(NULL);
}

static struct ganglion_producer_internal * ganglion_producer_internal_new(struct ganglion_producer * producer, const char * brokers, void * context, void (*report_callback)(void *, const char *, char *, int)) {
  struct ganglion_producer_internal * self = (struct ganglion_producer_internal *)malloc(sizeof(struct ganglion_producer_internal));
  assert(self != NULL);

  if (context == NULL) {
    self->context = producer;
  } else {
    self->context = context;
  }
  // more stuff here
  self->report_callback = report_callback;

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  self->config = rd_kafka_conf_new();
  self->topic_config = rd_kafka_topic_conf_new();
  self->status = GANGLION_THREAD_INITIALIZED;

#ifdef UNIT_TESTING
  rd_kafka_conf_set_log_cb(self->config, NULL);
#endif

  if (self->report_callback != NULL) {
    assert(rd_kafka_topic_conf_set(self->topic_config, "produce.offset.report", "true", NULL, 0) == RD_KAFKA_CONF_OK);
    rd_kafka_conf_set_dr_msg_cb(self->config, report_callback_wrapper);
    rd_kafka_conf_set_opaque(self->config, self->context);
  }

  if (producer->queue_length > 0) {
    char batch_size[15];
    sprintf(batch_size, "%d", producer->queue_length);
    assert(rd_kafka_conf_set(self->config, "batch.num.messages", batch_size, NULL, 0) == RD_KAFKA_CONF_OK);
  }

  if (producer->queue_flush_rate > 0) {
    char flush_timeout[15];
    sprintf(flush_timeout, "%d", producer->queue_flush_rate);
    assert(rd_kafka_conf_set(self->config, "queue.buffering.max.ms", flush_timeout, NULL, 0) == RD_KAFKA_CONF_OK);
  }

  assert(self->producer = rd_kafka_new(RD_KAFKA_PRODUCER, self->config, NULL, 0));
  assert(rd_kafka_brokers_add(self->producer, brokers) != 0);

  assert(!pthread_create(&self->worker, &attr, ganglion_producer_poller_thread, (void *)self));

  self->status = GANGLION_THREAD_STARTED;
  pthread_attr_destroy(&attr);

  return self;
}

static void ganglion_producer_internal_cleanup(struct ganglion_producer_internal * internal) {
  internal->status = GANGLION_THREAD_CANCELED;
  assert(!pthread_join(internal->worker, NULL));

  while (rd_kafka_outq_len(internal->producer) > 0)
    rd_kafka_poll(internal->producer, 100);

  rd_kafka_topic_conf_destroy(internal->topic_config);
  rd_kafka_destroy(internal->producer);
  free(internal);
  internal = NULL;
}

struct ganglion_producer * ganglion_producer_new(const char *brokers, const char *id, const char *compression, int queue_length, int queue_flush_rate, void * context, void (*report_callback)(void *, const char *, char *, int)) {
  struct ganglion_producer * self = (struct ganglion_producer *)malloc(sizeof(struct ganglion_producer));
  assert(self != NULL);

  self->id = id;
  self->queue_length = queue_length;
  self->queue_flush_rate = queue_flush_rate;

  struct ganglion_producer_internal * internal = ganglion_producer_internal_new(self, brokers, context, report_callback);
  self->opaque = (void *)internal;

  return self;
}

void ganglion_producer_cleanup(struct ganglion_producer * producer) {
  assert(producer);
  struct ganglion_producer_internal * internal = (struct ganglion_producer_internal *)producer->opaque;
  assert(internal);

  if (GANGLION_DEBUG)
    printf("Cleaning up producer\n");
  ganglion_producer_internal_cleanup(internal);
  free(producer);
  producer = NULL;
}

void ganglion_producer_publish(struct ganglion_producer * self, char * topic, char * payload, long long length) {
  assert(self);
  struct ganglion_producer_internal * internal = (struct ganglion_producer_internal *)self->opaque;
  assert(internal);

  rd_kafka_topic_t * kafka_topic = rd_kafka_topic_new(internal->producer, topic, rd_kafka_topic_conf_dup(internal->topic_config));
  assert(rd_kafka_produce(kafka_topic, RD_KAFKA_PARTITION_UA, RD_KAFKA_MSG_F_COPY, payload, length, NULL, 0, (void *)self) != -1);
  rd_kafka_topic_destroy(kafka_topic);
}
