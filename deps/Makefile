
JEMALLOC = jemalloc-3.6.0
ZEROMQ = zeromq-4.1.0
CZMQ = czmq-3.0.0

all: jemalloc zeromq czmq

CFLAGS_ZEROMQ=-I`pwd`/../zeromq/include
LDFLAGS_ZEROMQ="-L`pwd`/../zeromq/src/.libs -lzmq"
zeromq:
	tar zxvf ${ZEROMQ}.tar.gz && \
	ln -sf ${ZEROMQ} zeromq && \
	cd ${ZEROMQ} && \
	./autogen.sh && \
	./configure --with-libsodium && \
	make 

czmq:
	tar zxvf ${CZMQ}.tar.gz && \
	ln -sf ${CZMQ} czmq && \
	cd ${CZMQ} && \
	./autogen.sh && \
	./configure --with-libzmq=/usr/local && \ 
	make 

jemalloc:
	tar zxvf ${JEMALLOC}.tar.gz && \
	ln -sf ${JEMALLOC} jemalloc && \
	cd ${JEMALLOC} && \
	echo 3.6.0 > VERSION && \
	./autogen.sh && \
	./configure --with-jemalloc-prefix=je_ && \
	make build_lib

clean:
	rm -fr ${JEMALLOC} jemalloc
	rm -fr ${ZEROMQ} zeromq
	rm -fr ${CZMQ} czmq


