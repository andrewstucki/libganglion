#include <iostream>
#include "../ganglion.h"

void handle_message(void * consumer, char * payload, int length, int partition, long offset) {
  printf("Received message with offset %ld from partition %d\n", offset, partition);
}

void handle_report(void * producer, const char * topic, char * payload, int length) {
  printf("Successfully sent message to topic: %s\n", topic);
}

int main(void) {
  GanglionSupervisor *supervisor = new GanglionSupervisor();
  GanglionConsumer *consumer_one = new GanglionConsumer("localhost:9092", 400, "analytics", "libganglion.cpp.analytics", handle_message);
  GanglionConsumer *consumer_two = new GanglionConsumer("localhost:9092", 400, "stream", "libganglion.cpp.analytics", handle_message);

  std::cout << "Adding Consumers" << std::endl;

  supervisor->add(consumer_one);
  supervisor->add(consumer_two);

  std::cout << "Starting Supervisor" << std::endl;

  supervisor->start();

  GanglionProducer *producer = new GanglionProducer("localhost:9092", "example", "none", 1000, 100, handle_report);

  std::string str = "hello";
  std::vector<char> data(str.begin(), str.end());
  producer->publish("analytics", data);
  producer->publish("stream", data);

  std::cin.ignore();

  if (supervisor->is_started()) {
    supervisor->stop();
  }

  delete consumer_one;
  delete consumer_two;
  delete supervisor;

  return 0;
}
