EXAMPLE_FILES := Broadcast
override CXXFLAGS += -lpthread -Wpedantic -Wall -Wextra -Wsign-conversion -Wconversion -std=c++2a -Isrc -IuSockets/src
override LDFLAGS += uSockets/*.o -lz

PYFLAGS := $(shell python -m pybind11 --includes) -undefined dynamic_lookup
PYEXT := $(shell python3-config --extension-suffix)

DESTDIR ?=
prefix ?= /usr/local

# WITH_PROXY enables PROXY Protocol v2 support
ifeq ($(WITH_PROXY),1)
	override CXXFLAGS += -DUWS_WITH_PROXY
endif

# WITH_LIBDEFLATE=1 enables fast paths for SHARED_COMPRESSOR and inflation
ifeq ($(WITH_LIBDEFLATE),1)
	override CXXFLAGS += -I libdeflate -DUWS_USE_LIBDEFLATE
	override LDFLAGS += libdeflate/libdeflate.a
endif

# WITH_OPENSSL=1 enables OpenSSL 1.1+ support
ifeq ($(WITH_OPENSSL),1)
	# With problems on macOS, make sure to pass needed LDFLAGS required to find these
	override LDFLAGS += -lssl -lcrypto
else
	# WITH_WOLFSSL=1 enables WolfSSL 4.2.0 support (mutually exclusive with OpenSSL)
	ifeq ($(WITH_WOLFSSL),1)
		override LDFLAGS += -L/usr/local/lib -lwolfssl
	endif
endif

# WITH_LIBUV=1 builds with libuv as event-loop
ifeq ($(WITH_LIBUV),1)
	override LDFLAGS += -luv
endif

# WITH_ASIO=1 builds with ASIO as event-loop
ifeq ($(WITH_ASIO),1)
	override CXXFLAGS += -pthread
	override LDFLAGS += -lpthread
endif

# WITH_ASAN builds with sanitizers
ifeq ($(WITH_ASAN),1)
	override CXXFLAGS += -fsanitize=address -g
	override LDFLAGS += -lasan
endif

.PHONY: examples
examples:
	$(MAKE) -C uSockets; \
	for FILE in $(EXAMPLE_FILES); do $(CXX) -flto -O3 $(CXXFLAGS) examples/$$FILE.cpp -o $$FILE $(LDFLAGS) & done; \
	wait

.PHONY: pyserver
pyserver:
	$(MAKE) -C uSockets; \
	$(CXX) -flto -O3 $(CXXFLAGS) -Wall -shared  -fPIC $(PYFLAGS) examples/droidlet_webserver.cpp -o droidlet_webserver$(PYEXT) $(LDFLAGS);

.PHONY: capi
capi:
	$(MAKE) -C uSockets
	$(CXX) -shared -fPIC -flto -O3 $(CXXFLAGS) capi/App.cpp -o capi.so $(LDFLAGS)
	$(CXX) capi/example.c -O3 capi.so -o example

install:
	mkdir -p "$(DESTDIR)$(prefix)/include/uWebSockets"
	cp -r src/* "$(DESTDIR)$(prefix)/include/uWebSockets"

all:
	$(MAKE) examples
#	$(MAKE) -C fuzzing
#	$(MAKE) -C benchmarks
clean:
	rm -rf $(EXAMPLE_FILES) $(THREADED_EXAMPLE_FILES)
	rm -rf fuzzing/*.o benchmarks/*.o
