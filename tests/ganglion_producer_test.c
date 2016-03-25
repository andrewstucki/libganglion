#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>

#include <cmocka.h>

#include <ganglion.h>
#include "../src/_ganglion_internal.h"

#include "ganglion_test_helpers.h"

void test_ganglion_producer_new_no_context(void **state) {
  struct ganglion_producer * producer = ganglion_producer_new("testbroker:1234", "gangliontest", "snappy", 1000, 100, NULL, test_producer_noop_callback);

  assert_non_null(producer);
  assert_string_equal(producer->id, "gangliontest");
  assert_int_equal(producer->queue_length, 1000);
  assert_int_equal(producer->queue_flush_rate, 100);
  assert_non_null(producer->opaque);

  struct ganglion_producer_internal * internal = (struct ganglion_producer_internal *)producer->opaque;
  assert_ptr_equal(internal->report_callback, test_producer_noop_callback);
  assert_ptr_equal(internal->context, producer);

  ganglion_producer_cleanup(producer);
  ganglion_shutdown();
}

void test_ganglion_producer_new_context(void **state) {
  void *context = (void *)"context";
  struct ganglion_producer * producer = ganglion_producer_new("testbroker:1234", "gangliontest", "snappy", 1000, 100, context, test_producer_noop_callback);

  assert_non_null(producer);
  assert_string_equal(producer->id, "gangliontest");
  assert_int_equal(producer->queue_length, 1000);
  assert_int_equal(producer->queue_flush_rate, 100);
  assert_non_null(producer->opaque);

  struct ganglion_producer_internal * internal = (struct ganglion_producer_internal *)producer->opaque;
  assert_ptr_equal(internal->report_callback, test_producer_noop_callback);
  assert_ptr_equal(internal->context, context);

  ganglion_producer_cleanup(producer);
  ganglion_shutdown();
}


// struct ganglion_producer * ganglion_producer_new(const char *, const char *, const char *, enum ganglion_producer_mode, int, int, void *, void (*)(void *, const char *, char *, int));
// void ganglion_producer_cleanup(struct ganglion_producer *);
// void ganglion_producer_publish(struct ganglion_producer *, char *, char *, long long);
