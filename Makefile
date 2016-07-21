CC = gcc
CFLAGS = -Wall -Werror -I./build/libs -I./build -I./src -fPIC
LDFLAGS = -lpthread -lz
PREFIX = /usr/local

SOURCE_DIR = src
BUILD_DIR = build
TESTS_DIR = tests

SOURCES = ganglion_consumer.c ganglion_producer.c ganglion_supervisor.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

RDKAFKA_VERSION = 0.9.0.99

ifdef USE_SASL
	SASL2_LIBS = $(shell pkg-config --libs sasl2)
	LDFLAGS += $(SASL2_LIBS)
	RDKAFKA_CONFIG_FLAGS = --enable-sasl
	RDKAFKA_SASL = "with"
else
	RDKAFKA_CONFIG_FLAGS = --disable-sasl
	RDKAFKA_SASL = "without"
endif

ifdef USE_SSL
	OPENSSL_LIBS = $(shell pkg-config --libs openssl)
	LDFLAGS += $(OPENSSL_LIBS)
	RDKAFKA_CONFIG_FLAGS += --enable-ssl
	RDKAFKA_SSL = "with"
else
	RDKAFKA_CONFIG_FLAGS += --disable-ssl
	RDKAFKA_SSL = "without"
endif

CMOCKA_VERSION = 1.0.1
TEST_CFLAGS = -g -Wall -Werror -I./test-deps/cmocka-$(CMOCKA_VERSION)/include
TEST_LDFLAGS = -L./build/tests -lcmocka
TEST_SOURCES = ganglion_consumer_test.c ganglion_producer_test.c ganglion_supervisor_test.c ganglion_test_helpers.c ganglion_test_suite.c
TEST_OBJECTS = $(patsubst %.c,$(BUILD_DIR)/tests/%.o,$(TEST_SOURCES))

lib: clean $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/libganglion.dylib
example: clean $(BUILD_DIR)/example
go-example: $(BUILD_DIR)/go-example

tests: CFLAGS += -DUNIT_TESTING=1
tests: clean $(BUILD_DIR)/tests/libganglion_test_suite

deps:
	@echo "\033[1;4;32mDownloading dependant libraries\033[0m"
	@mkdir -p deps
	@echo "\033[36mDownloading librdkafka version: ${RDKAFKA_VERSION}\033[0m"
	@curl -L https://github.com/edenhill/librdkafka/archive/$(RDKAFKA_VERSION).tar.gz -o deps/librdkafka-$(RDKAFKA_VERSION).tar.gz
	@echo "\033[36mExtracting librdkafka\033[0m"
	@cd deps; \
	tar xzf librdkafka-$(RDKAFKA_VERSION).tar.gz
	@echo "\033[36mDone extracting librdkafka\033[0m"

test-deps:
	@echo "\033[1;4;32mDownloading dependant test libraries\033[0m"
	@mkdir -p test-deps
	@echo "\033[36mDownloading cmocka version: ${CMOCKA_VERSION}\033[0m"
	@curl -L https://git.cryptomilk.org/projects/cmocka.git/snapshot/cmocka-$(CMOCKA_VERSION).tar.gz -o test-deps/cmocka-$(CMOCKA_VERSION).tar.gz
	@echo "\033[36mExtracting cmocka$<\033[0m"
	@cd test-deps; \
	tar xzf cmocka-$(CMOCKA_VERSION).tar.gz
	@echo "\033[36mDone extracting cmocka\033[0m"

$(BUILD_DIR)/libs/librdkafka.a: deps
	@echo "\033[1;4;32mBuilding librdkafka ${RDKAFKA_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring librdkafka ${RDKAFKA_SSL} ssl support and ${RDKAFKA_SASL} sasl support\033[0m"
	@cd deps/librdkafka-$(RDKAFKA_VERSION); \
	./configure $(RDKAFKA_CONFIG_FLAGS) 2>&1 > /dev/null
	@echo "\033[36mCompiling librdkafka\033[0m"
	@$(MAKE) -C deps/librdkafka-$(RDKAFKA_VERSION) -s libs CFLAGS="-w -fPIC" 2>&1 > /dev/null
	@cd deps/librdkafka-$(RDKAFKA_VERSION); \
	cp $(SOURCE_DIR)/rdkafka.h ../../$(BUILD_DIR)/libs; \
	cp $(SOURCE_DIR)/librdkafka.a ../../$(BUILD_DIR)/libs
	@echo "\033[36mDone compiling librdkafka\033[0m"

$(BUILD_DIR)/tests/libcmocka.dylib: test-deps
	@echo "\033[1;4;32mBuilding libcmocka ${CMOCKA_VERSION}\033[0m"
	@mkdir -p test-deps/cmocka-$(CMOCKA_VERSION)/build
	@mkdir -p $(BUILD_DIR)/tests
	@echo "\033[36mConfiguring libcmocka\033[0m"
	@cd test-deps/cmocka-$(CMOCKA_VERSION)/build; \
	cmake .. -DCMAKE_C_FLAGS="-Wno-format-pedantic -w" -Wno-dev 2>&1 > /dev/null
	@echo "\033[36mCompiling libcmocka\033[0m"
	@$(MAKE) -C test-deps/cmocka-$(CMOCKA_VERSION)/build -s 2>&1 > /dev/null
	@cd test-deps/cmocka-$(CMOCKA_VERSION)/build; \
	cp src/libcmocka.dylib ../../../$(BUILD_DIR)/tests
	@echo "\033[36mDone compiling libcmocka\033[0m"

$(BUILD_DIR)/libganglion.a: $(BUILD_DIR)/libs/librdkafka.a $(OBJECTS)
	@echo "\033[1;4;32mCreating libganglion static library\033[0m"
	@cd $(BUILD_DIR)/libs; \
	$(AR) -x librdkafka.a; \
	$(AR) -r ../libganglion.a *.o $(patsubst $(BUILD_DIR)/%,../%,$(OBJECTS))
	@echo "\033[36mDone creating $<\033[0m"

$(BUILD_DIR)/libganglion.dylib: $(OBJECTS)
	@echo "\033[1;4;32mCreating libganglion dynamic library\033[0m"
	@cd $(BUILD_DIR)/libs; \
	$(CC) -shared -o ../libganglion.dylib *.o $(patsubst $(BUILD_DIR)/%,../%,$(OBJECTS)) $(LDFLAGS)
	@echo "\033[36mDone creating $<\033[0m"

$(BUILD_DIR)/example: $(BUILD_DIR)/libganglion.a examples/basic.c
	@echo "\033[1;4;32mCompiling basic example\033[0m"
	@$(CC) -c $(CFLAGS) examples/basic.c -o $(BUILD_DIR)/example.o
	@$(CC) -O3 -o $(BUILD_DIR)/example $(BUILD_DIR)/example.o $(BUILD_DIR)/libganglion.a $(LDFLAGS) $(OPENSSL_LIBS)
	#@strip $(BUILD_DIR)/example
	@echo "\033[36mDone compiling $@\033[0m"

$(BUILD_DIR)/go-example: examples/basic.go
	@echo "\033[1;4;32mCompiling basic go example\033[0m"
	@go get github.com/andrewstucki/libganglion/go
	@go build -o $@ examples/basic.go
	@echo "\033[36mDone compiling $@\033[0m"

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(SOURCE_DIR)/ganglion.h
	@echo "\033[1;4;32mCompiling $<\033[0m"
	@$(CC) -c $(CFLAGS) $< -o $@
	@echo "\033[36mDone compiling $<\033[0m"

$(BUILD_DIR)/tests/%.o: $(TESTS_DIR)/%.c $(BUILD_DIR)/tests/libcmocka.dylib
	@echo "\033[1;4;32mCompiling $<\033[0m"
	@$(CC) -c $(CFLAGS) $(TEST_CFLAGS) $< -o $@
	@echo "\033[36mDone compiling $<\033[0m"

$(BUILD_DIR)/tests/libganglion_test_suite: $(TEST_OBJECTS) $(BUILD_DIR)/libganglion.a
	@echo "\033[1;4;32mCompiling libganglion test suite\033[0m"
	@$(CC) $(TEST_OBJECTS) -o $(BUILD_DIR)/tests/libganglion_test_suite $(BUILD_DIR)/libganglion.a $(CFLAGS) $(TEST_CFLAGS) $(TEST_LDFLAGS) $(LDFLAGS)
	@echo "\033[36mDone compiling libganglion test suite\033[0m"

.PHONY: clean distclean install run-tests

run-tests: tests
	@echo "\033[1;4;32mRunning libganglion test suite\033[0m"
	@CMOCKA_TEST_ABORT='1' ./$(BUILD_DIR)/tests/libganglion_test_suite #needed for threading

distclean:
	rm -rf $(BUILD_DIR) deps test-deps

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/libganglion.dylib $(BUILD_DIR)/example $(BUILD_DIR)/go-example $(BUILD_DIR)/libs/*.o $(BUILD_DIR)/tests/*.o $(BUILD_DIR)/tests/libganglion*

install: $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/libganglion.dylib $(SOURCE_DIR)/ganglion.h
	install -m 0644 $(SOURCE_DIR)/ganglion.h $(PREFIX)/include
	install -m 0644 $(BUILD_DIR)/libganglion.a $(PREFIX)/lib
	install -m 0644 $(BUILD_DIR)/libganglion.dylib $(PREFIX)/lib
	install -m 0644 $(SOURCE_DIR)/libganglion.pc $(PREFIX)/lib/pkgconfig/

uninstall:
	rm -f $(PREFIX)/include/ganglion.h $(PREFIX)/lib/libganglion.a
