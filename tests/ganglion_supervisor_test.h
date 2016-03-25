#ifndef __GANGLION_SUPERVISOR_TEST_H__
#define __GANGLION_SUPERVISOR_TEST_H__

void test_ganglion_supervisor_new(void **);
void test_ganglion_supervisor_cleanup_running(void **);
void test_ganglion_supervisor_start_already_running(void **);
void test_ganglion_supervisor_stop_not_running(void **);
void test_ganglion_supervisor_register(void **);
void test_ganglion_supervisor_is_started(void **);

#endif // __GANGLION_SUPERVISOR_TEST_H__
