extern crate ganglion;

use ganglion::{GanglionSupervisor, GanglionProducer, GanglionConsumer, GanglionConsumerHandler};
use std::thread;
use std::time::Duration;

#[derive(Clone)]
struct ConsumerHandler {}
impl GanglionConsumerHandler for ConsumerHandler {
    fn handle_message(&mut self, _: i32, _: i64, topic: String, _: Vec<u8>) {
        println!("Received message for topic: '{}'", topic);
    }
}

fn main() {
    let mut producer = GanglionProducer::new("localhost:9092", "rust_ganglion", "none", 1000, 100);

    let mut supervisor = GanglionSupervisor::new();
    println!("Making consumers");

    let consumer_one = GanglionConsumer::new("localhost:9092", 100, "analytics", "rust_ganglion.analytics", ConsumerHandler{});
    let consumer_two = GanglionConsumer::new("localhost:9092", 100, "stream", "rust_ganglion.analytics", ConsumerHandler{});

    println!("Registering consumers");

    supervisor.register(consumer_one);
    supervisor.register(consumer_two);

    println!("Starting consumers");

    supervisor.start();

    thread::sleep(Duration::from_millis(3000));

    println!("Sending messages");

    producer.publish("analytics", b"yay!!!");
    producer.publish("stream", b"yay!!!");

    thread::sleep(Duration::from_millis(1000));

    supervisor.stop();
}
