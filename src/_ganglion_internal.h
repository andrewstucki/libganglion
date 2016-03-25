#ifndef ___GANGLION_INTERNAL_H__
#define ___GANGLION_INTERNAL_H__

#include <rdkafka.h>

struct ganglion_kafka_internal {
  rd_kafka_t *consumer;
  rd_kafka_conf_t *config;
  rd_kafka_topic_conf_t *topic_config;
  rd_kafka_topic_partition_list_t *topics;
};

struct ganglion_kafka_internal_message {
  char * payload;
  int length;
  int partition;
  long offset;
  struct ganglion_consumer * consumer;
};

struct ganglion_kafka_producer_internal {
  rd_kafka_t *producer;
  rd_kafka_conf_t *config;
  rd_kafka_topic_conf_t *topic_config;

  pthread_t worker;
  volatile sig_atomic_t status;
};

#ifdef UNIT_TESTING

//FREE
# ifdef free
#   undef free
# endif

  extern void _test_free(void *ptr, const char* file, const int line);
# define free(ptr) _test_free(ptr, __FILE__, __LINE__)

//CALLOC
# ifdef calloc
#   undef calloc
# endif

  extern void* _test_calloc(const size_t nmemb, const size_t size, const char* file, const int line);
# define calloc(nmemb, size) _test_calloc(nmemb, size, __FILE__, __LINE__)

//MALLOC
# ifdef malloc
#   undef malloc
# endif

  extern void* _test_malloc(const size_t size, const char* file, const int line);
# define malloc(size) _test_malloc(size, __FILE__, __LINE__)

//REALLOC
# ifdef realloc
#   undef realloc
# endif

  extern void* _test_realloc(void * ptr, const size_t size, const char* file, const int line);
# define realloc(ptr, size) _test_realloc(ptr, size, __FILE__, __LINE__)

//ASSERT
  extern void mock_assert(const int result, const char* const expression, const char * const file, const int line);
# undef assert
# define assert(expression) mock_assert((int)(expression), #expression, __FILE__, __LINE__);

# endif

#endif // ___GANGLION_INTERNAL_H__
