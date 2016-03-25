extern crate libc;
use libc::{c_void, c_char, c_int, c_long, c_longlong};

pub struct GanglionConsumer {
    internal: *mut CGanglionConsumer
}

pub struct GanglionSupervisor {
    internal: *mut CGanglionSupervisor
}

pub struct GanglionProducer {
    internal: *mut CGanglionProducer
}

extern "C" {
    fn ganglion_consumer_new(brokers: *const c_char, workers: c_int, topic: *const c_char, group: *const c_char, context: *mut c_void, callback: Option<unsafe extern "C" fn(context: *mut c_void, message: *mut c_char, message_size: c_int, partition: c_int, offset: c_long)>) -> *mut CGanglionConsumer;
    fn ganglion_consumer_cleanup(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_start(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_stop(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_wait(consumer: *mut CGanglionConsumer);

    fn ganglion_supervisor_new() -> *mut CGanglionSupervisor;
    fn ganglion_supervisor_cleanup(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_start(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_stop(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_register(supervisor: *mut CGanglionSupervisor, consumer: *mut CGanglionConsumer) -> c_int;
    fn ganglion_supervisor_is_started(supervisor: *mut CGanglionSupervisor) -> c_int;

    fn ganglion_producer_new(brokers: *const c_char, id: *const c_char, compression: *const c_char, queue_length: c_int, flush_timeout: c_int, context: *mut c_void, report_callback: Option<unsafe extern "C" fn(context: *mut c_void, topic: *const c_char, message: *mut c_char, message_size: c_int)>) -> *mut CGanglionProducer;
    fn ganglion_producer_cleanup(producer: *mut CGanglionProducer);
    fn ganglion_producer_publish(producer: *mut CGanglionProducer, topic: *mut c_char, message: *mut c_char, message_size: c_longlong);

    fn ganglion_shutdown();
}
