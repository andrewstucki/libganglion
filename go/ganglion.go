package ganglion

/*
#cgo CFLAGS: -I/Users/andrewstucki/Code/libganglion/src
#cgo LDFLAGS: /Users/andrewstucki/Code/libganglion/build/libganglion.a -lz -lpthread
#include <ganglion.h>

extern void cgo_producer_report_callback_wrapper(void * context, const char *topic, char *message, int message_len);
extern void cgo_consumer_message_callback_wrapper(void * context, char *message, int message_len, int partition, long offset);
*/
import "C"

// #cgo pkg-config: libganglion

import (
	"runtime"
	"unsafe"
)

//////////////////////////////////
//////////// Consumer ////////////
//////////////////////////////////

// ConsumerHandler docs here
type ConsumerHandler interface {
	HandleMessage([]byte, int, int)
}

// Consumer docs here
type Consumer struct {
	base *C.struct_ganglion_consumer
}

func consumerFinalizer(consumer *Consumer) {
	C.ganglion_consumer_cleanup(consumer.base)
}

//export consumerMessageCallbackWrapper
func consumerMessageCallbackWrapper(cContext unsafe.Pointer, cMessage *C.char, cMessageSize C.int, cPartition C.int, cOffset C.long) {
	handler := *(*ConsumerHandler)(cContext)
	message := C.GoBytes(unsafe.Pointer(cMessage), cMessageSize)
	handler.HandleMessage(message, int(cPartition), int(cOffset))
}

// CreateConsumer docs here
func CreateConsumer(brokers string, workers int, topic, group string, handler ConsumerHandler) *Consumer {
	if handler == nil {
		panic("Ganglion consumers cannot have nil handlers!")
	}
	consumer := &Consumer{}
	consumer.base = C.ganglion_consumer_new(C.CString(brokers), C.int(workers), C.CString(topic), C.CString(group), unsafe.Pointer(&handler), (*[0]byte)(C.cgo_consumer_message_callback_wrapper))

	runtime.SetFinalizer(consumer, consumerFinalizer)
	return consumer
}

// Start docs here
func (c *Consumer) Start() {
	C.ganglion_consumer_start(c.base)
}

// Stop docs here
func (c *Consumer) Stop() {
	C.ganglion_consumer_stop(c.base)
}

// Wait docs here
func (c *Consumer) Wait() {
	C.ganglion_consumer_wait(c.base)
}

////////////////////////////////////
//////////// Supervisor ////////////
////////////////////////////////////

// Supervisor docs here
type Supervisor struct {
	base *C.struct_ganglion_supervisor
}

func supervisorFinalizer(supervisor *Supervisor) {
	C.ganglion_supervisor_cleanup(supervisor.base)
}

// CreateSupervisor docs here
func CreateSupervisor() *Supervisor {
	supervisor := &Supervisor{}
	supervisor.base = C.ganglion_supervisor_new()

	runtime.SetFinalizer(supervisor, supervisorFinalizer)
	return supervisor
}

// Start docs here
func (s *Supervisor) Start() {
	C.ganglion_supervisor_start(s.base)
}

// Stop docs here
func (s *Supervisor) Stop() {
	C.ganglion_supervisor_stop(s.base)
}

// Register docs here
func (s *Supervisor) Register(consumer *Consumer) {
	C.ganglion_supervisor_register(s.base, consumer.base)
}

// IsStarted docs here
func (s *Supervisor) IsStarted() bool {
	return int(C.ganglion_supervisor_is_started(s.base)) != 0
}

//////////////////////////////////
//////////// Producer ////////////
//////////////////////////////////

// ProducerHandler docs here
type ProducerHandler interface {
	HandleReport(string, []byte)
}

// Producer docs here
type Producer struct {
	base *C.struct_ganglion_producer
}

func producerFinalizer(producer *Producer) {
	C.ganglion_producer_cleanup(producer.base)
}

//export producerReportCallbackWrapper
func producerReportCallbackWrapper(cContext unsafe.Pointer, cTopic *C.char, cMessage *C.char, cMessageSize C.int) {
	handler := *(*ProducerHandler)(cContext)
	message := C.GoBytes(unsafe.Pointer(cMessage), cMessageSize)
	topic := C.GoString(cTopic)
	handler.HandleReport(topic, message)
}

// CreateProducer docs here
func CreateProducer(brokers, id, compression string, queueSize, queueFlushRate int, handler ProducerHandler) *Producer {
	producer := &Producer{}
	if handler != nil {
		producer.base = C.ganglion_producer_new(C.CString(brokers), C.CString(id), C.CString(compression), C.int(queueSize), C.int(queueFlushRate), unsafe.Pointer(&handler), (*[0]byte)(C.cgo_producer_report_callback_wrapper))
	} else {
		producer.base = C.ganglion_producer_new(C.CString(brokers), C.CString(id), C.CString(compression), C.int(queueSize), C.int(queueFlushRate), nil, nil)
	}
	runtime.SetFinalizer(producer, producerFinalizer)
	return producer
}

// Publish docs here
func (p *Producer) Publish(topic string, message []byte) {
	cMessage := (*C.char)(unsafe.Pointer(&message[0]))
	cMessageLen := C.longlong(len(message))
	C.ganglion_producer_publish(p.base, C.CString(topic), cMessage, cMessageLen)
}

/////////////////////////////////
//////////// Helpers ////////////
/////////////////////////////////

// Shutdown docs here
func Shutdown() {
	runtime.GC() //force garbage collection to get rid of producer objects
	C.ganglion_shutdown()
}
