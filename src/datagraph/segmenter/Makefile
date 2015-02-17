TARGET = libsegmenter.a

OBJS = segmenter.cc.o
LIBMMSEG = libmmseg-3.2.14

LIBMMSEG_CFLAGS=-I./libmmseg -I./libmmseg/css -I./libmmseg/utils
LIBMMSEG_LDFLAGS=-lmmseg

include ../../Makefile.common

FINAL_CXXFLAGS += ${LIBMMSEG_CFLAGS}
FINAL_LDFLAGS += ${LIBMMSEG_LDFLAGS}

all: ${TARGET}

${TARGET}: libmmseg ${OBJS}
	${AR} -cruv ${TARGET} ${OBJS}

clean:
	rm -fr ${TARGET} ${OBJS} libmmseg ${LIBMMSEG}

# ---------------- libmmseg ----------------

.PHONY: libmmseg

libmmseg: ${LIBMMSEG}
	${MAKE} -C ${LIBMMSEG}

${LIBMMSEG}:
	tar zxvf ${LIBMMSEG}.tar.gz && \
	ln -s ${LIBMMSEG}/src libmmseg

