#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "ganglion_consumer_test.h"
#include "ganglion_producer_test.h"
#include "ganglion_supervisor_test.h"

int main(void) {
  const struct CMUnitTest consumer_tests[] = {
    cmocka_unit_test(test_ganglion_consumer_new_no_context),
    cmocka_unit_test(test_ganglion_consumer_new_context),
    cmocka_unit_test(test_ganglion_consumer_new_callback_null),
    cmocka_unit_test(test_ganglion_consumer_start_already_running),
    cmocka_unit_test(test_ganglion_consumer_stop_not_running),
    cmocka_unit_test(test_ganglion_consumer_cleanup_running),
  };

  const struct CMUnitTest producer_tests[] = {
    cmocka_unit_test(test_ganglion_producer_new_no_context),
    cmocka_unit_test(test_ganglion_producer_new_context),
  };

  const struct CMUnitTest supervisor_tests[] = {
    cmocka_unit_test(test_ganglion_supervisor_new),
    cmocka_unit_test(test_ganglion_supervisor_cleanup_running),
    cmocka_unit_test(test_ganglion_supervisor_start_already_running),
    cmocka_unit_test(test_ganglion_supervisor_stop_not_running),
    cmocka_unit_test(test_ganglion_supervisor_register),
    cmocka_unit_test(test_ganglion_supervisor_is_started),
  };

  printf("\n\033[36mRunning Consumer Tests\033[0m\n\n");
  int consumer_tests_return = cmocka_run_group_tests(consumer_tests, NULL, NULL);
  if (consumer_tests_return != 0) return consumer_tests_return;

  printf("\n\033[36mRunning Producer Tests\033[0m\n\n");

  int producer_tests_return = cmocka_run_group_tests(producer_tests, NULL, NULL);
  if (producer_tests_return != 0) return producer_tests_return;

  printf("\n\033[36mRunning Supervisor Tests\033[0m\n\n");

  return cmocka_run_group_tests(supervisor_tests, NULL, NULL);
}
