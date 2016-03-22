all: deps/librdkafka-0.9.0.99/src/librdkafka.a example

deps:
	mkdir deps
	curl -L https://github.com/edenhill/librdkafka/archive/0.9.0.99.tar.gz -o deps/librdkafka-0.9.0.99.tar.gz
	cd deps && tar xvzf librdkafka-0.9.0.99.tar.gz

deps/librdkafka-0.9.0.99/src/librdkafka.a: deps
	cd deps/librdkafka-0.9.0.99 && ./configure && make

build/ganglion_consumer.o:
	mkdir -p build
	cd build && gcc -c ../src/ganglion_consumer.c -I../deps/librdkafka-0.9.0.99/src

build/ganglion_supervisor.o:
	mkdir -p build
	cd build && gcc -c ../src/ganglion_supervisor.c -I../deps/librdkafka-0.9.0.99/src

build/libganglion.a: deps/librdkafka-0.9.0.99/src/librdkafka.a build/ganglion_consumer.o build/ganglion_supervisor.o
	mkdir -p build/libs
	cd build/libs && ar -x ../../deps/librdkafka-0.9.0.99/src/librdkafka.a
	cd build/libs && cp ../ganglion_consumer.o .
	cd build/libs && cp ../supervisor.o .
	cd build/libs && ar -r ../libganglion.a *.o

example: build/libganglion.a
	cd build && gcc -c ../examples/basic.c -I../deps/librdkafka-0.9.0.99/src
	cd build && gcc -o example basic.o libganglion.a -lpthread -lsasl2 -lssl -lcrypto -lz

.PHONY: clean

clean:
	rm -rf deps build
