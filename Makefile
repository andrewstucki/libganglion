CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -I./build/libs -I./build -I./src -fPIC
LDFLAGS = -lpthread -lz -lc++
PREFIX = /usr/local

SOURCE_DIR = src
BUILD_DIR = build
TESTS_DIR = tests

SOURCES = ganglion_consumer.c ganglion_producer.c ganglion_supervisor.c
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SOURCES))

#currently actually build master due to rd_kafka_destroy deadlock
RDKAFKA_VERSION = 0.9.1
LZ4_VERSION = 131

OPENSSL_VERSION = 1_0_2h

SERDES_VERSION = 3.0.0
CURL_VERSION = 7.50.1
AVROC_VERSION = 1.8.1
JANSSON_VERSION = 2.7
SNAPPY_VERSION = 1.1.3
LZMA_VERSION = 5.2.2

PLATFORM := $(shell uname -s)
ifeq ($(PLATFORM),Linux)
		OPENSSL_CONFIGURE = "./config no-shared"
endif
ifeq ($(PLATFORM),Darwin)
		OPENSSL_CONFIGURE = "./Configure darwin64-x86_64-cc no-shared"
endif

ifdef USE_SASL
	SASL2_LIBS = $(shell pkg-config --libs sasl2)
	LDFLAGS += $(SASL2_LIBS)
	RDKAFKA_CONFIG_FLAGS = --enable-sasl
	RDKAFKA_SASL = "with"
else
	RDKAFKA_CONFIG_FLAGS = --disable-sasl
	RDKAFKA_SASL = "without"
endif

ifndef NO_SSL
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
rust-example: $(BUILD_DIR)/rust-example
cpp-example: $(BUILD_DIR)/cpp-example

examples: example go-example rust-example cpp-example

tests: CFLAGS += -DUNIT_TESTING=1
tests: clean $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/tests/libganglion_test_suite

deps:
	@echo "\033[1;4;32mDownloading dependant libraries\033[0m"
	@mkdir -p deps
	@echo "\033[36mDownloading librdkafka version: ${RDKAFKA_VERSION}\033[0m"
	@git clone --depth 1 https://github.com/edenhill/librdkafka.git deps/librdkafka-$(RDKAFKA_VERSION)
	@echo "\033[36mDownloading libserdes version: ${SERDES_VERSION}\033[0m"
	@curl -L https://github.com/confluentinc/libserdes/archive/v$(SERDES_VERSION).tar.gz -o deps/libserdes-$(SERDES_VERSION).tar.gz
	@echo "\033[36mExtracting libserdes\033[0m"
	@cd deps; \
	tar xzf libserdes-$(SERDES_VERSION).tar.gz
	@echo "\033[36mDone extracting libserdes\033[0m"
	@echo "\033[36mDownloading libavro version: ${AVROC_VERSION}\033[0m"
	@curl -L https://github.com/apache/avro/archive/release-$(AVROC_VERSION).tar.gz -o deps/libavro-$(AVROC_VERSION).tar.gz
	@echo "\033[36mExtracting libavro\033[0m"
	@cd deps; \
	mkdir libavro-$(AVROC_VERSION); \
	tar xzf libavro-$(AVROC_VERSION).tar.gz -C libavro-$(AVROC_VERSION) --strip-components 1 2>&1 > /dev/null
	@echo "\033[36mDone extracting libavro\033[0m"
	@echo "\033[36mDownloading libjansson version: ${JANSSON_VERSION}\033[0m"
	@curl -L https://github.com/akheron/jansson/archive/v$(JANSSON_VERSION).tar.gz -o deps/libjansson-$(JANSSON_VERSION).tar.gz
	@echo "\033[36mExtracting libjansson\033[0m"
	@cd deps; \
	mkdir libjansson-$(JANSSON_VERSION); \
	tar xzf libjansson-$(JANSSON_VERSION).tar.gz -C libjansson-$(JANSSON_VERSION) --strip-components 1
	@echo "\033[36mDone extracting libjansson\033[0m"
	@echo "\033[36mDownloading libcurl version: ${CURL_VERSION}\033[0m"
	@curl -L https://curl.haxx.se/download/curl-$(CURL_VERSION).tar.gz -o deps/libcurl-$(CURL_VERSION).tar.gz
	@echo "\033[36mExtracting libcurl\033[0m"
	@cd deps; \
	mkdir libcurl-$(CURL_VERSION); \
	tar xzf libcurl-$(CURL_VERSION).tar.gz -C libcurl-$(CURL_VERSION) --strip-components 1
	@echo "\033[36mDone extracting libcurl\033[0m"
	@echo "\033[36mDownloading liblzma version: ${LZMA_VERSION}\033[0m"
	@curl -L http://tukaani.org/xz/xz-$(LZMA_VERSION).tar.gz -o deps/liblzma-$(LZMA_VERSION).tar.gz
	@echo "\033[36mExtracting liblzma\033[0m"
	@cd deps; \
	mkdir liblzma-$(LZMA_VERSION); \
	tar xzf liblzma-$(LZMA_VERSION).tar.gz -C liblzma-$(LZMA_VERSION) --strip-components 1
	@echo "\033[36mDone extracting liblzma\033[0m"
	@echo "\033[36mDownloading libsnappy version: ${SNAPPY_VERSION}\033[0m"
	@curl -L https://github.com/google/snappy/releases/download/$(SNAPPY_VERSION)/snappy-$(SNAPPY_VERSION).tar.gz -o deps/libsnappy-$(SNAPPY_VERSION).tar.gz
	@echo "\033[36mExtracting libsnappy\033[0m"
	@cd deps; \
	mkdir libsnappy-$(SNAPPY_VERSION); \
	tar xzf libsnappy-$(SNAPPY_VERSION).tar.gz -C libsnappy-$(SNAPPY_VERSION) --strip-components 1
	@echo "\033[36mDone extracting libsnappy\033[0m"
	@echo "\033[36mDownloading liblz4 version: ${LZ4_VERSION}\033[0m"
	@curl -L https://github.com/Cyan4973/lz4/archive/r$(LZ4_VERSION).tar.gz -o deps/liblz4-$(LZ4_VERSION).tar.gz
	@echo "\033[36mExtracting liblz4\033[0m"
	@cd deps; \
	mkdir liblz4-$(LZ4_VERSION); \
	tar xzf liblz4-$(LZ4_VERSION).tar.gz -C liblz4-$(LZ4_VERSION) --strip-components 1
	@echo "\033[36mDone extracting liblz4\033[0m"
	@echo "\033[36mDownloading openssl version: ${OPENSSL_VERSION}\033[0m"
	@curl -L https://github.com/openssl/openssl/archive/OpenSSL_$(OPENSSL_VERSION).tar.gz -o deps/openssl-$(OPENSSL_VERSION).tar.gz
	@echo "\033[36mExtracting openssl\033[0m"
	@cd deps; \
	mkdir openssl-$(OPENSSL_VERSION); \
	tar xzf openssl-$(OPENSSL_VERSION).tar.gz -C openssl-$(OPENSSL_VERSION) --strip-components 1
	@echo "\033[36mDone extracting openssl\033[0m"

test-deps:
	@echo "\033[1;4;32mDownloading dependant test libraries\033[0m"
	@mkdir -p test-deps
	@echo "\033[36mDownloading cmocka version: ${CMOCKA_VERSION}\033[0m"
	@curl -L https://git.cryptomilk.org/projects/cmocka.git/snapshot/cmocka-$(CMOCKA_VERSION).tar.gz -o test-deps/cmocka-$(CMOCKA_VERSION).tar.gz
	@echo "\033[36mExtracting cmocka$<\033[0m"
	@cd test-deps; \
	tar xzf cmocka-$(CMOCKA_VERSION).tar.gz
	@echo "\033[36mDone extracting cmocka\033[0m"

$(BUILD_DIR)/libs/liblzma.a: deps
	@echo "\033[1;4;32mBuilding liblzma ${LZMA_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring liblzma\033[0m"
	@cd deps/liblzma-$(LZMA_VERSION); \
	./configure 2>&1 > /dev/null
	@echo "\033[36mCompiling liblzma\033[0m"
	@$(MAKE) -j4 -C deps/liblzma-$(LZMA_VERSION) CFLAGS="-w -fPIC" -s 2>&1 > /dev/null
	@mkdir -p $(BUILD_DIR)/libs/lzma
	@cp deps/liblzma-$(LZMA_VERSION)/src/liblzma/.libs/liblzma.a $(BUILD_DIR)/libs
	@cp deps/liblzma-$(LZMA_VERSION)/src/liblzma/api/lzma.h $(BUILD_DIR)/libs; \
	 cp -r deps/liblzma-$(LZMA_VERSION)/src/liblzma/api/lzma $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libsnappy.a: deps
	@echo "\033[1;4;32mBuilding libsnappy ${SNAPPY_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring libsnappy\033[0m"
	@cd deps/libsnappy-$(SNAPPY_VERSION); \
	./configure 2>&1 > /dev/null
	@echo "\033[36mCompiling libsnappy\033[0m"
	@$(MAKE) -j4 -C deps/libsnappy-$(SNAPPY_VERSION) CFLAGS="-w -fPIC" -s 2>&1 > /dev/null
	@cp deps/libsnappy-$(SNAPPY_VERSION)/.libs/libsnappy.a $(BUILD_DIR)/libs
	@cp deps/libsnappy-$(SNAPPY_VERSION)/snappy-c.h $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libjansson.a: deps
	@echo "\033[1;4;32mBuilding libjansson ${JANSSON_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring libjansson\033[0m"
	@cd deps/libjansson-$(JANSSON_VERSION); \
	mkdir build; \
	cd build; \
	cmake .. -Wno-dev 2>&1 > /dev/null
	@echo "\033[36mCompiling libjansson\033[0m"
	@$(MAKE) -j4 -C deps/libjansson-$(JANSSON_VERSION)/build CFLAGS="-w -fPIC" -s 2>&1 > /dev/null
	@cp deps/libjansson-$(JANSSON_VERSION)/build/lib/libjansson.a $(BUILD_DIR)/libs
	@cp deps/libjansson-$(JANSSON_VERSION)/build/include/*.h $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libavro.a: $(BUILD_DIR)/libs/liblzma.a $(BUILD_DIR)/libs/libsnappy.a $(BUILD_DIR)/libs/libjansson.a deps
	@echo "\033[1;4;32mBuilding libavro ${AVROC_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring libavro\033[0m"
	@cd deps/libavro-$(AVROC_VERSION)/lang/c/; \
	mkdir build; \
	cd build; \
	cmake .. -Wno-dev 2>&1 > /dev/null
	@echo "\033[36mCompiling libavro\033[0m"
	@$(MAKE) -j4 -C deps/libavro-$(AVROC_VERSION)/lang/c/build CFLAGS="-w -fPIC" -s 2>&1 > /dev/null
	@cp deps/libavro-$(AVROC_VERSION)/lang/c/build/src/libavro.a $(BUILD_DIR)/libs
	@cp deps/libavro-$(AVROC_VERSION)/lang/c/src/avro.h $(BUILD_DIR)/libs; \
	 cp -r deps/libavro-$(AVROC_VERSION)/lang/c/src/avro $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libserdes.a: $(BUILD_DIR)/libs/libavro.a $(BUILD_DIR)/libs/libcurl.a deps
	@echo "\033[1;4;32mBuilding libserdes ${SERDES_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring libserdes\033[0m"
	@cd deps/libserdes-$(SERDES_VERSION); \
	CFLAGS="-I../../build/libs -L../../build/libs" ./configure 2>&1 > /dev/null
	@echo "\033[36mCompiling libserdes\033[0m"
	@$(MAKE) -j4 libserdes.a -C deps/libserdes-$(SERDES_VERSION)/src CFLAGS="-w -fPIC -I`pwd`/$(BUILD_DIR)/libs" -s 2>&1 > /dev/null
	@cp deps/libserdes-$(SERDES_VERSION)/src/libserdes.a $(BUILD_DIR)/libs
	@mkdir -p $(BUILD_DIR)/libs/serdes
	@cp deps/libserdes-$(SERDES_VERSION)/src/serdes*.h $(BUILD_DIR)/libs/serdes

$(BUILD_DIR)/libs/librdkafka.a: $(BUILD_DIR)/libs/liblz4.a deps
	@echo "\033[1;4;32mBuilding librdkafka ${RDKAFKA_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring librdkafka ${RDKAFKA_SSL} ssl support and ${RDKAFKA_SASL} sasl support\033[0m"
	@cd deps/librdkafka-$(RDKAFKA_VERSION); \
	./configure $(RDKAFKA_CONFIG_FLAGS) 2>&1 > /dev/null
	@echo "\033[36mCompiling librdkafka\033[0m"
	@$(MAKE) librdkafka.a -j4 -C deps/librdkafka-$(RDKAFKA_VERSION)/src -s 2>&1 > /dev/null
	@cd deps/librdkafka-$(RDKAFKA_VERSION); \
	cp $(SOURCE_DIR)/rdkafka.h ../../$(BUILD_DIR)/libs; \
	cp $(SOURCE_DIR)/librdkafka.a ../../$(BUILD_DIR)/libs
	@echo "\033[36mDone compiling librdkafka\033[0m"

$(BUILD_DIR)/libs/libcurl.a: $(BUILD_DIR)/libs/libssl.a deps
	@echo "\033[1;4;32mBuilding libcurl ${CURL_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring libcurl\033[0m"
	@cd deps/libcurl-$(CURL_VERSION); \
	./configure CFLAGS="-I../../$(BUILD_DIR)/libs" --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-manual 2>&1 > /dev/null
	@echo "\033[36mCompiling libcurl\033[0m"
	@$(MAKE) -j4 -C deps/libcurl-$(CURL_VERSION) -s 2>&1 > /dev/null
	@mkdir -p $(BUILD_DIR)/libs/curl
	@cp deps/libcurl-$(CURL_VERSION)/lib/.libs/libcurl.a $(BUILD_DIR)/libs
	@cp deps/libcurl-$(CURL_VERSION)/include/curl/*.h $(BUILD_DIR)/libs/curl

$(BUILD_DIR)/libs/liblz4.a: deps
	@echo "\033[1;4;32mBuilding liblz4 ${LZ4_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mCompiling liblz4\033[0m"
	@$(MAKE) lib -j4 -C deps/liblz4-$(LZ4_VERSION) -s 2>&1 > /dev/null
	@cp deps/liblz4-$(LZ4_VERSION)/lib/liblz4.a $(BUILD_DIR)/libs
	@cp deps/liblz4-$(LZ4_VERSION)/lib/lz4.h $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libssl.a: deps
	@echo "\033[1;4;32mBuilding openssl ${OPENSSL_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring openssl\033[0m"
	@cd deps/openssl-$(OPENSSL_VERSION); \
	sh -c $(OPENSSL_CONFIGURE) 2>&1 > /dev/null
	@echo "\033[36mCompiling openssl\033[0m"
	@$(MAKE) -j4 -C deps/openssl-$(OPENSSL_VERSION) -s 2>&1 > /dev/null
	@cp deps/openssl-$(OPENSSL_VERSION)/*.a $(BUILD_DIR)/libs
	@cp -LR deps/openssl-$(OPENSSL_VERSION)/include/openssl $(BUILD_DIR)/libs

$(BUILD_DIR)/libs/libsasl2.a: deps
	@echo "\033[1;4;32mBuilding sasl ${OPENSSL_VERSION}\033[0m"
	@mkdir -p $(BUILD_DIR)/libs
	@echo "\033[36mConfiguring sasl\033[0m"
	@cd deps/sasl-$(OPENSSL_VERSION); \
	./config no-shared 2>&1 > /dev/null
	@echo "\033[36mCompiling sasl\033[0m"
	@$(MAKE) -j4 -C deps/sasl-$(OPENSSL_VERSION) -s 2>&1 > /dev/null
	@cp deps/sasl-$(OPENSSL_VERSION)/*.a $(BUILD_DIR)/libs
	@cp -LR deps/sasl-$(OPENSSL_VERSION)/include/sasl $(BUILD_DIR)/libs

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

$(BUILD_DIR)/libganglion.a: $(BUILD_DIR)/libs/librdkafka.a $(BUILD_DIR)/libs/libserdes.a $(OBJECTS)
	@echo "\033[1;4;32mCreating libganglion static library\033[0m"
	@cd $(BUILD_DIR)/libs; \
	mkdir -p librdkafka && cd librdkafka && $(AR) -x ../librdkafka.a && cd ..; \
	mkdir -p libserdes && cd libserdes && $(AR) -x ../libserdes.a && cd ..; \
	mkdir -p libcurl && cd libcurl && $(AR) -x ../libcurl.a && cd ..; \
	mkdir -p libavro && cd libavro && $(AR) -x ../libavro.a && cd ..; \
	mkdir -p libjansson && cd libjansson && $(AR) -x ../libjansson.a && cd ..; \
	mkdir -p liblzma && cd liblzma && $(AR) -x ../liblzma.a && cd ..; \
	mkdir -p libsnappy && cd libsnappy && $(AR) -x ../libsnappy.a && cd ..; \
	mkdir -p liblz4 && cd liblz4 && $(AR) -x ../liblz4.a && cd ..; \
	mkdir -p libssl && cd libssl && $(AR) -x ../libssl.a && cd ..; \
	mkdir -p libcrypto && cd libcrypto && $(AR) -x ../libcrypto.a && cd ..; \
	$(AR) -r ../libganglion.a lib*/*.o $(patsubst $(BUILD_DIR)/%,../%,$(OBJECTS))
	@echo "\033[36mDone creating libganglion.a\033[0m"

$(BUILD_DIR)/libganglion.dylib: $(OBJECTS)
	@echo "\033[1;4;32mCreating libganglion dynamic library\033[0m"
	@cd $(BUILD_DIR)/libs; \
	$(CC) -shared -o ../libganglion.dylib *.o $(patsubst $(BUILD_DIR)/%,../%,$(OBJECTS)) $(LDFLAGS)
	@echo "\033[36mDone creating libganglion.dylib\033[0m"

$(BUILD_DIR)/example: $(BUILD_DIR)/libganglion.a src/examples/basic.c
	@echo "\033[1;4;32mCompiling basic example\033[0m"
	@$(CC) -c $(CFLAGS) src/examples/basic.c -o $(BUILD_DIR)/example.o
	@$(CC) -O3 -o $@ $(BUILD_DIR)/example.o $(BUILD_DIR)/libganglion.a $(LDFLAGS) $(OPENSSL_LIBS)
	@strip $@
	@echo "\033[36mDone compiling $@\033[0m"

$(BUILD_DIR)/go-example: go/examples/basic.go
	@echo "\033[1;4;32mCompiling basic go example\033[0m"
	@go get github.com/andrewstucki/libganglion/go
	@go build -o $@ go/examples/basic.go
	@echo "\033[36mDone compiling $@\033[0m"

$(BUILD_DIR)/rust-example: $(BUILD_DIR)/libganglion.dylib rust/examples/basic.rs
	@echo "\033[1;4;32mCompiling basic rust example\033[0m"
	@cd rust; \
	LIBRARY_PATH=../$(BUILD_DIR) cargo build --example basic
	@cp rust/target/debug/examples/basic $@
	@echo "\033[36mDone compiling $@\033[0m"

$(BUILD_DIR)/cpp-example: $(BUILD_DIR)/libganglion.a src/examples/basic.cpp
	@echo "\033[1;4;32mCompiling basic c++ example\033[0m"
	@$(CXX) -c $(CFLAGS) src/examples/basic.cpp -o $(BUILD_DIR)/cpp-example.o
	@$(CXX) -O3 -o $@ $(BUILD_DIR)/cpp-example.o $(BUILD_DIR)/libganglion.a $(LDFLAGS) $(OPENSSL_LIBS)
	@strip $@
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
	rm -rf $(BUILD_DIR) deps test-deps rust/target rust/Cargo.lock

clean:
	rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/libganglion.dylib $(BUILD_DIR)/example $(BUILD_DIR)/go-example $(BUILD_DIR)/libs/*.o $(BUILD_DIR)/tests/*.o $(BUILD_DIR)/tests/libganglion*

install: $(BUILD_DIR)/libganglion.a $(BUILD_DIR)/libganglion.dylib $(SOURCE_DIR)/ganglion.h
	install -m 0644 $(SOURCE_DIR)/ganglion.h $(PREFIX)/include
	install -m 0644 $(BUILD_DIR)/libganglion.a $(PREFIX)/lib
	install -m 0644 $(BUILD_DIR)/libganglion.dylib $(PREFIX)/lib
	install -m 0644 $(SOURCE_DIR)/libganglion.pc $(PREFIX)/lib/pkgconfig/

uninstall:
	rm -f $(PREFIX)/include/ganglion.h $(PREFIX)/lib/libganglion.a
