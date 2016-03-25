#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <ganglion.h>
#include "../src/_ganglion_internal.h"

#include "ganglion_test_helpers.h"

void test_ganglion_supervisor_new(void **state) {
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();

  assert_non_null(supervisor);
  assert_non_null(supervisor->opaque);

  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;

  assert_int_equal(internal->consumer_size, 0);
  assert_null(internal->consumers);
  assert_int_equal(internal->status, GANGLION_THREAD_INITIALIZED);

  ganglion_supervisor_cleanup(supervisor);
  ganglion_shutdown();
}

void test_ganglion_supervisor_cleanup_running(void **state) {
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;
  internal->status = GANGLION_THREAD_STARTED;

  expect_assert_failure(ganglion_supervisor_cleanup(supervisor));

  internal->status = GANGLION_THREAD_INITIALIZED;
  ganglion_supervisor_cleanup(supervisor);
  ganglion_shutdown();
}

void test_ganglion_supervisor_start_already_running(void **state) {
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;
  internal->status = GANGLION_THREAD_STARTED;

  expect_assert_failure(ganglion_supervisor_start(supervisor));

  internal->status = GANGLION_THREAD_INITIALIZED;
  ganglion_supervisor_cleanup(supervisor);
  ganglion_shutdown();
}

void test_ganglion_supervisor_stop_not_running(void **state) {
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();

  expect_assert_failure(ganglion_supervisor_stop(supervisor));

  ganglion_supervisor_cleanup(supervisor);
  ganglion_shutdown();
}

void test_ganglion_supervisor_register(void **state) {
  int consumer_size;
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();
  struct ganglion_consumer * consumer_one = ganglion_consumer_new("testbroker:1234", 5, "testtopic", "testgroup", NULL, test_consumer_noop_callback);
  struct ganglion_consumer * consumer_two = ganglion_consumer_new("testbroker:1234", 5, "testtopic", "testgroup", NULL, test_consumer_noop_callback);

  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;

  assert_int_equal(internal->consumer_size, 0);
  assert_null(internal->consumers);

  consumer_size = ganglion_supervisor_register(supervisor, consumer_one);
  assert_int_equal(consumer_size, 1);
  assert_int_equal(internal->consumer_size, 1);
  assert_non_null(internal->consumers);
  assert_ptr_equal(internal->consumers[0], consumer_one);

  consumer_size = ganglion_supervisor_register(supervisor, consumer_two);
  assert_int_equal(consumer_size, 2);
  assert_int_equal(internal->consumer_size, 2);
  assert_non_null(internal->consumers);
  assert_ptr_equal(internal->consumers[1], consumer_two);

  ganglion_supervisor_cleanup(supervisor);
  ganglion_consumer_cleanup(consumer_one);
  ganglion_consumer_cleanup(consumer_two);
  ganglion_shutdown();
}

void test_ganglion_supervisor_is_started(void **state) {
  struct ganglion_supervisor * supervisor = ganglion_supervisor_new();
  struct ganglion_supervisor_internal * internal = (struct ganglion_supervisor_internal *)supervisor->opaque;
  assert_false(ganglion_supervisor_is_started(supervisor));

  internal->status = GANGLION_THREAD_STARTED;
  assert_true(ganglion_supervisor_is_started(supervisor));

  internal->status = GANGLION_THREAD_INITIALIZED;
  ganglion_supervisor_cleanup(supervisor);
  ganglion_shutdown();
}

// struct ganglion_supervisor * ganglion_supervisor_new();
// void ganglion_supervisor_cleanup(struct ganglion_supervisor *);
// void ganglion_supervisor_start(struct ganglion_supervisor *);
// void ganglion_supervisor_stop(struct ganglion_supervisor *);
// int ganglion_supervisor_register(struct ganglion_supervisor *, struct ganglion_consumer *);
// int ganglion_supervisor_is_started(struct ganglion_supervisor *);
