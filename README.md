#Ganglion

Libganglion is a high level supervised wrapper around librdkafka. It isn't very well documented right now and is only
partially complete, but the basic concept is there and the library is usable.

##Compiling

The makefile and the dependencies it builds require the following:

- `autoconf`
- gnu `make`
- `cmake`
- `curl`
- a working compiler toolchain

To run the tests run `make run-tests`.

To build the library run `make`.

To build an example application run `make example`.

To install the library run `make install`.

##Go Wrappers

The go wrappers assume you already have a built library available in your system (from previously running `make install`).
Once you have the library installed run `go get github.com/andrewstucki/libganglion/go` and you should have working go
bindings. To see the bindings in action run `go build examples/basic.go` after installing the bindings. As a shortcut to
all of this run `make go-example` and you should have a build binary in `build/go-example`.
