CC = gcc
CFLAGS = -Wall -Werror -I./build/libs -I./build
LDFLAGS = -lpthread -lz

SOURCE_DIR = src
BUILD_DIR = build

SOURCES = ganglion_consumer.c ganglion_producer.c ganglion_supervisor.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

RDKAFKA_VERSION = 0.9.0.99
OPENSSL_LIBS = $(shell pkg-config --libs openssl)

all: $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/example

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(SOURCE_DIR)/ganglion.h
	$(CC) -c $(CFLAGS) $< -o $@

deps:
	mkdir deps
	curl -L https://github.com/edenhill/librdkafka/archive/$(RDKAFKA_VERSION).tar.gz -o deps/librdkafka-$(RDKAFKA_VERSION).tar.gz
	cd deps; \
	tar xvzf librdkafka-$(RDKAFKA_VERSION).tar.gz

$(BUILD_DIR)/libs/librdkafka.a: deps
	mkdir -p $(BUILD_DIR)/libs
	cd deps/librdkafka-$(RDKAFKA_VERSION); \
	./configure --disable-sasl; \
	$(MAKE); \
	cp $(SOURCE_DIR)/rdkafka.h ../../$(BUILD_DIR)/libs; \
	cp $(SOURCE_DIR)/librdkafka.a ../../$(BUILD_DIR)/libs

$(BUILD_DIR)/libganglion.a: $(BUILD_DIR)/libs/librdkafka.a $(OBJECTS)
	cd $(BUILD_DIR)/libs; \
	ar -x librdkafka.a; \
	ar -r ../libganglion.a *.o $(patsubst $(BUILD_DIR)/%,../%,$(OBJECTS))

$(BUILD_DIR)/example: $(BUILD_DIR)/libganglion.a examples/basic.c
	$(CC) -c $(CFLAGS) examples/basic.c -o $(BUILD_DIR)/example.o
	$(CC) -o $(BUILD_DIR)/example $(BUILD_DIR)/example.o $(BUILD_DIR)/libganglion.a $(LDFLAGS) $(OPENSSL_LIBS)
	strip $(BUILD_DIR)/example

.PHONY: clean distclean

distclean:
	rm -rf $(BUILD_DIR) deps

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/example $(BUILD_DIR)/libs/*.o
