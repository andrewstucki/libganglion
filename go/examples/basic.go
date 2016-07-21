package main

import (
	"bufio"
	"fmt"
	"os"
	"strings"

	"github.com/andrewstucki/libganglion/go"
)

type testHandler struct{}

func (t *testHandler) HandleMessage(message []byte, partition int, offset int) {
	fmt.Printf("Got message %s from partition: %d with offset: %d\n", string(message), partition, offset)
}

func main() {
	defer ganglion.Shutdown()

	supervisor := ganglion.CreateSupervisor()
	consumerOne := ganglion.CreateConsumer("localhost:9092", 100, "analytics", "go_ganglion.analytics", &testHandler{})
	consumerTwo := ganglion.CreateConsumer("localhost:9092", 100, "stream", "go_ganglion.analytics", &testHandler{})

	supervisor.Register(consumerOne)
	supervisor.Register(consumerTwo)
	supervisor.Start()

	producer := ganglion.CreateProducer("localhost:9092", "go_ganglion", "none", 1000, 100, nil)

	var text string
	var err error
	reader := bufio.NewReader(os.Stdin)
	for {
		text, err = reader.ReadString('\n')
		text = strings.TrimSpace(text)
		if err != nil || text == "quit" {
			break
		}
		producer.Publish("analytics", []byte(text))
	}

	supervisor.Stop()
}
