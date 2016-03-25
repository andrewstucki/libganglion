#ifndef __GANGLION_CONSUMER_TEST_H__
#define __GANGLION_CONSUMER_TEST_H__

void test_ganglion_consumer_new_no_context(void **);
void test_ganglion_consumer_new_context(void **);
void test_ganglion_consumer_new_callback_null(void **);
void test_ganglion_consumer_start_already_running(void **);
void test_ganglion_consumer_stop_not_running(void **);
void test_ganglion_consumer_cleanup_running(void **);

#endif // __GANGLION_CONSUMER_TEST_H__
