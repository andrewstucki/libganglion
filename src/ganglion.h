#ifndef __GANGLION_H__
#define __GANGLION_H__

#ifdef __cplusplus
extern "C" {
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

#ifdef __cplusplus
}

#include <string>
#include <vector>
#include <iostream>

class GanglionConsumer {
  friend class GanglionSupervisor;
  ganglion_consumer * internal;
  char *c_brokers;
  char *c_topic;
  char *c_group;
public:
  GanglionConsumer(std::string brokers, int workers, std::string topic, std::string group, void (* callback)(void *, char *, int, int, long)) {
    c_brokers = new char[brokers.length() + 1];
    std::strcpy(c_brokers, brokers.c_str());

    c_topic = new char[topic.length() + 1];
    std::strcpy(c_topic, topic.c_str());

    c_group = new char[group.length() + 1];
    std::strcpy(c_group, group.c_str());

    internal = ganglion_consumer_new(c_brokers, workers, c_topic, c_group, NULL, callback);
  }

  ~GanglionConsumer(void) {
    ganglion_consumer_cleanup(internal);
    delete [] c_brokers;
    delete [] c_topic;
    delete [] c_group;
  }

  void start(void) {
    ganglion_consumer_start(internal);
  }

  void stop(void) {
    ganglion_consumer_stop(internal);
  }

  void wait(void) {
    ganglion_consumer_wait(internal);
  }
};

class GanglionSupervisor {
  ganglion_supervisor * internal;
public:

  GanglionSupervisor(void) {
    internal = ganglion_supervisor_new();
  }

  ~GanglionSupervisor(void) {
    ganglion_supervisor_cleanup(internal);
  }

  void start(void) {
    ganglion_supervisor_start(internal);
  }

  void stop(void) {
    ganglion_supervisor_stop(internal);
  }

  int add(GanglionConsumer *consumer) {
    return ganglion_supervisor_register(internal, consumer->internal);
  }

  bool is_started(void) {
    return ganglion_supervisor_is_started(internal);
  }
};

class GanglionProducer {
  ganglion_producer * internal;
  char *c_brokers;
  char *c_id;
  char *c_compression;
public:
  GanglionProducer(std::string brokers, std::string id, std::string compression, int queue_length, int queue_flush_rate, void (*report_callback)(void *, const char *, char *, int)) {
    c_brokers = new char[brokers.length() + 1];
    std::strcpy(c_brokers, brokers.c_str());

    c_id = new char[id.length() + 1];
    std::strcpy(c_id, id.c_str());

    c_compression = new char[compression.length() + 1];
    std::strcpy(c_compression, compression.c_str());

    internal = ganglion_producer_new(c_brokers, c_id, c_compression, queue_length, queue_flush_rate, NULL, report_callback);
  }

  ~GanglionProducer(void) {
    ganglion_producer_cleanup(internal);
    delete [] c_brokers;
    delete [] c_id;
    delete [] c_compression;
  }

  void publish(std::string topic, std::vector<char> message) {
    ganglion_producer_publish(internal, (char *)topic.c_str(), (char *)(&message[0]), message.size());
  }
};
#endif

#endif // __GANGLION_H__
