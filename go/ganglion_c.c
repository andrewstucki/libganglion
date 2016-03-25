#include "_cgo_export.h"

void cgo_producer_report_callback_wrapper(void * context, const char *topic, char *message, int message_len) {
	producerReportCallbackWrapper(context, (char *)topic, message, message_len);
}

void cgo_consumer_message_callback_wrapper(void * context, char *message, int message_len, int partition, long offset) {
	consumerMessageCallbackWrapper(context, message, message_len, partition, offset);
}
