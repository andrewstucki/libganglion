#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <ganglion.h>
#include "../src/_ganglion_internal.h" //for structure definitions

static void test_consumer_callback(void *context, char *message, int message_size, int partition, long offset) {}

void test_ganglion_consumer_new_no_context(void **state) {
  struct ganglion_consumer * consumer = ganglion_consumer_new("testbroker:1234", 5, "testtopic", "testgroup", NULL, test_consumer_callback);

  assert_non_null(consumer);
  assert_ptr_equal(consumer->context, consumer);
  assert_int_equal(consumer->worker_size, 5);
  assert_string_equal(consumer->topic, "testtopic");
  assert_string_equal(consumer->group, "testgroup");
  assert_int_equal(consumer->status, GANGLION_THREAD_INITIALIZED);
  assert_ptr_equal(consumer->callback, test_consumer_callback);

  assert_non_null(consumer->opaque);
  assert_non_null(consumer->workers);

  ganglion_consumer_cleanup(consumer);
  ganglion_shutdown();
}

void test_ganglion_consumer_new_context(void **state) {
  void *context = (void *)"context";
  struct ganglion_consumer * consumer = ganglion_consumer_new("testbroker:1234", 5, "testtopic", "testgroup", context, test_consumer_callback);

  assert_non_null(consumer);
  assert_ptr_equal(consumer->context, context);
  assert_int_equal(consumer->worker_size, 5);
  assert_string_equal(consumer->topic, "testtopic");
  assert_string_equal(consumer->group, "testgroup");
  assert_int_equal(consumer->status, GANGLION_THREAD_INITIALIZED);
  assert_ptr_equal(consumer->callback, test_consumer_callback);

  assert_non_null(consumer->opaque);
  assert_non_null(consumer->workers);

  ganglion_consumer_cleanup(consumer);
  ganglion_shutdown();
}

// struct ganglion_consumer * ganglion_consumer_new(const char *, int, const char *, const char *, void *, void (*)(void *, char *, int, int, long));
// void ganglion_consumer_cleanup(struct ganglion_consumer *);
// void ganglion_consumer_start(struct ganglion_consumer *);
// void ganglion_consumer_stop(struct ganglion_consumer *);
// void ganglion_consumer_wait(struct ganglion_consumer *);
