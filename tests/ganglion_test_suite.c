#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "ganglion_consumer_test.h"
#include "ganglion_producer_test.h"
#include "ganglion_supervisor_test.h"

int main(void) {
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_ganglion_consumer_new_no_context),
    cmocka_unit_test(test_ganglion_consumer_new_context),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
