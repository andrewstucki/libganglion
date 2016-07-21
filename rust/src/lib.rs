#![allow(dead_code)]
#![allow(unused_must_use)]

extern crate libc;
use libc::{c_void, c_char, c_int, c_long, c_longlong};
use std::ffi::CString;
use std::sync::mpsc;
use std::mem;
use std::slice;
use std::thread;
use std::sync::Arc;
use std::cell::UnsafeCell;

pub trait GanglionConsumerHandler {
    fn handle_message(&mut self, partition: i32, offset: i64, topic: String, message: Vec<u8>);
}

enum ConsumerHandlerMessage {
    Receive(i32, i64, Vec<u8>, mpsc::SyncSender<()>),
    Close,
}

struct GanglionConsumerHandlerWrapper<T> {
    pub inner: UnsafeCell<T>,
}

struct ConsumerSender(*mut CGanglionConsumer);
unsafe impl Send for ConsumerSender {}
unsafe impl Sync for ConsumerSender {}
impl Clone for ConsumerSender {
    fn clone(&self) -> ConsumerSender {
        ConsumerSender(self.0)
    }
}

unsafe impl<T> Sync for GanglionConsumerHandlerWrapper<T> {}
impl<T> GanglionConsumerHandlerWrapper<T> {
    fn new(inner: T) -> GanglionConsumerHandlerWrapper<T> {
        GanglionConsumerHandlerWrapper {
            inner: UnsafeCell::new(inner)
        }
    }

    fn get(&self) -> *mut T {
        self.inner.get()
    }
}

extern fn consumer_callback(context: *mut c_void, message: *mut c_char, message_size: c_int, partition: c_int, offset: c_long) {
    let message: Vec<u8> = unsafe { slice::from_raw_parts(mem::transmute(message), message_size as usize).iter().cloned().collect() };
    let sender: &mut mpsc::Sender<ConsumerHandlerMessage> = unsafe { mem::transmute(context) };
    println!("Consumer Received Message");
    let (finish_sender, finished) = mpsc::sync_channel(0);
    println!("Sending");
    sender.send(ConsumerHandlerMessage::Receive(partition, offset, message, finish_sender));
    println!("Sent");
    finished.recv();
    println!("Consumer Finished Receiving Message");
}

pub struct GanglionConsumer(*mut CGanglionConsumer, String, mpsc::Sender<ConsumerHandlerMessage>, mpsc::Receiver<()>);
impl GanglionConsumer {
    pub fn new<T: GanglionConsumerHandler + Clone + Send + Sync + 'static>(brokers: &str, workers: i32, topic: &str, group: &str, handler: T) -> GanglionConsumer {
        let (sender, receiver) = mpsc::channel();
        let (signaler, signaled) = mpsc::sync_channel(0);
        let (quit_sender, quit_receiver) = mpsc::sync_channel::<()>(0);
        let threaded_handler = Arc::new(GanglionConsumerHandlerWrapper::new(handler));
        let threaded_topic = topic.to_owned().clone();

        let c_brokers = CString::new(brokers).unwrap();
        let c_topic = CString::new(topic).unwrap();
        let c_group = CString::new(group).unwrap();

        let thread_sender = sender.clone();
        thread::spawn(move || { //test
            let consumer = unsafe { ganglion_consumer_new(c_brokers.as_ptr(), workers, c_topic.as_ptr(), c_group.as_ptr(), mem::transmute(&thread_sender), consumer_callback) };
            signaler.send(ConsumerSender(consumer));
            loop {
                match receiver.recv() {
                    Ok(ConsumerHandlerMessage::Receive(partition, offset, message, rendezvous)) => {
                        println!("Callback for handler");
                        let rendezvous_sender = rendezvous.clone();
                        let worker_partition = partition.clone();
                        let worker_offset = offset.clone();
                        let worker_message = message.clone();
                        let worker_topic = threaded_topic.clone();
                        let worker_handler = threaded_handler.clone();
                        thread::spawn(move || {
                            unsafe { (*worker_handler.get()).handle_message(worker_partition, worker_offset, worker_topic, worker_message) };
                            rendezvous_sender.send(());
                        });
                    },
                    Ok(ConsumerHandlerMessage::Close) => {
                        println!("Closing");
                        break;
                    },
                    Err(e) => {
                        println!("Error {}", e);
                        break;
                    }
                }
            }
            quit_sender.send(()).unwrap();
            receiver.recv().unwrap(); // one more to avoid the segfaulting consumer
            quit_sender.send(()).unwrap();
        });

        let consumer = signaled.recv().unwrap();
        GanglionConsumer(consumer.0, topic.to_owned(), sender, quit_receiver)
    }
}

impl Drop for GanglionConsumer {
    fn drop(&mut self) {
        match self.2.send(ConsumerHandlerMessage::Close) {
            Err(_) => {},
            Ok(_) => {
                self.3.recv().unwrap();
                unsafe { ganglion_consumer_cleanup(self.0) };
                self.2.send(ConsumerHandlerMessage::Close).unwrap();
                self.3.recv().unwrap();
            }
        }
    }
}

pub struct GanglionSupervisor(*mut CGanglionSupervisor, Vec<GanglionConsumer>);

impl GanglionSupervisor {
    pub fn new() -> GanglionSupervisor {
        GanglionSupervisor(unsafe { ganglion_supervisor_new() }, vec!())
    }

    pub fn start(&mut self) {
        unsafe { ganglion_supervisor_start(self.0) };
    }

    pub fn stop(&mut self) {
        unsafe { ganglion_supervisor_stop(self.0) };
    }

    pub fn register(&mut self, consumer: GanglionConsumer) -> i32 {
        println!("Registering");
        let added = unsafe { ganglion_supervisor_register(self.0, consumer.0) };
        self.1.push(consumer);
        println!("Registered, {}", added);
        added
    }

    pub fn is_started(&self) -> bool {
        unsafe { ganglion_supervisor_is_started(self.0) }
    }
}

impl Drop for GanglionSupervisor {
    fn drop(&mut self) {
        unsafe { ganglion_supervisor_cleanup(self.0) };
    }
}

enum ProducerMessage {
    Publish(String, Vec<u8>),
    Report(String, Vec<u8>),
    Close,
}

pub struct GanglionProducer(mpsc::Sender<ProducerMessage>, mpsc::Receiver<()>);

extern fn reporter_callback(context: *mut c_void, topic: *const c_char, message: *mut c_char, message_size: c_int) {
    let topic = unsafe { CString::from_raw(mem::transmute(topic)).into_string().unwrap() };
    let message: Vec<u8> = unsafe { slice::from_raw_parts(mem::transmute(message), message_size as usize).iter().cloned().collect() };
    let sender: &mut mpsc::Sender<ProducerMessage> = unsafe { mem::transmute(context) };
    println!("Send");
    sender.send(ProducerMessage::Report(topic, message));
    println!("Finished");
}

impl GanglionProducer {
    pub fn new(brokers: &str, id: &str, compression: &str, queue_length: i32, flush_rate: i32) -> GanglionProducer {
        let c_brokers = CString::new(brokers).unwrap();
        let c_id = CString::new(id).unwrap();
        let c_compression = CString::new(compression).unwrap();

        let (sender, receiver) = mpsc::channel::<ProducerMessage>();
        let (quit_sender, quit_receiver) = mpsc::sync_channel(0);


        let record_sender = sender.clone();
        thread::spawn(move || {
            let producer = unsafe { ganglion_producer_new(c_brokers.as_ptr(), c_id.as_ptr(), c_compression.as_ptr(), queue_length, flush_rate, mem::transmute(&record_sender), reporter_callback) };
            loop {
                match receiver.recv() {
                    Ok(ProducerMessage::Report(topic, _)) => {
                        println!("Callback for message on {}", topic);
                    },
                    Ok(ProducerMessage::Publish(topic, message)) => {
                        unsafe { ganglion_producer_publish(producer, mem::transmute(CString::new(topic).unwrap().as_ptr()), mem::transmute(message.as_ptr()), message.len() as c_longlong) };
                    },
                    Ok(ProducerMessage::Close) | Err(_) => {
                        break;
                    }
                }
            }
            unsafe { ganglion_producer_cleanup(producer) };
            quit_sender.send(()).unwrap();
        });

        GanglionProducer(sender, quit_receiver)
    }

    pub fn publish(&mut self, topic: &str, message: &[u8]) {
        self.0.send(ProducerMessage::Publish(topic.to_owned(), message.iter().cloned().collect())).unwrap();
    }
}

impl Drop for GanglionProducer {
    fn drop(&mut self) {
        match self.0.send(ProducerMessage::Close) {
            Err(_) => {},
            Ok(_) => {
                self.1.recv().unwrap();
            }
        }
    }
}

#[repr(C)]
struct CGanglionConsumer {
    worker_size: c_int,
    topic: *const c_char,
    group: *const c_char,

    opaque: *mut c_void
}

#[repr(C)]
struct CGanglionSupervisor {
    opaque: *mut c_void
}

#[repr(C)]
struct CGanglionProducer {
    id: *const c_char,
    compression: *const c_char,
    queue_length: c_int,
    queue_flush_rate: c_int,

    opaque: *mut c_void
}

#[link(name = "ganglion")]
extern "C" {
    fn ganglion_consumer_new(brokers: *const c_char, workers: c_int, topic: *const c_char, group: *const c_char, context: *mut c_void, callback: extern "C" fn(context: *mut c_void, message: *mut c_char, message_size: c_int, partition: c_int, offset: c_long)) -> *mut CGanglionConsumer;
    fn ganglion_consumer_cleanup(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_start(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_stop(consumer: *mut CGanglionConsumer);
    fn ganglion_consumer_wait(consumer: *mut CGanglionConsumer);

    fn ganglion_supervisor_new() -> *mut CGanglionSupervisor;
    fn ganglion_supervisor_cleanup(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_start(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_stop(supervisor: *mut CGanglionSupervisor);
    fn ganglion_supervisor_register(supervisor: *mut CGanglionSupervisor, consumer: *mut CGanglionConsumer) -> c_int;
    fn ganglion_supervisor_is_started(supervisor: *mut CGanglionSupervisor) -> bool;

    fn ganglion_producer_new(brokers: *const c_char, id: *const c_char, compression: *const c_char, queue_length: c_int, flush_timeout: c_int, context: *mut c_void, report_callback: extern "C" fn(context: *mut c_void, topic: *const c_char, message: *mut c_char, message_size: c_int)) -> *mut CGanglionProducer;
    fn ganglion_producer_cleanup(producer: *mut CGanglionProducer);
    fn ganglion_producer_publish(producer: *mut CGanglionProducer, topic: *mut c_char, message: *mut c_char, message_size: c_longlong);

    fn ganglion_shutdown();
}
