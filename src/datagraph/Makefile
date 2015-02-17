TARGET = ../bin/datagraph

OBJS = main.cc.o

COMMON_CFLAGS= -g -fPIC -Wall -Wno-deprecated -Wno-unused-result 

LIBMMSEG_CFLAGS=-I../include/mmseg -I../include/mmseg/css -I../include/mmseg/utils
LIBMMSEG_LDFLAGS=-lmmseg

FINAL_CFLAGS = ${COMMON_CFLAGS} -I./ldac -I./model -I./segmenter -I./utils -I../include 
FINAL_CFLAGS += ${LIBMMSEG_CFLAGS}

UNAME := $(shell uname)
ifeq (${UNAME}, Linux)
	FINAL_CFLAGS += -DOS_LINUX
endif
ifeq (${UNAME}, Darwin)
	FINAL_CFLAGS += -DOS_DARWIN
endif

FINAL_CXXFLAGS = ${FINAL_CFLAGS} 

FINAL_CFLAGS += ${CFLAGS}
FINAL_CXXFLAGS += ${CXXFLAGS} -Wno-unused-private-field

FINAL_LDFLAGS=-L../lib
FINAL_LDFLAGS += ${LIBMMSEG_LDFLAGS}
FINAL_LDFLAGS += ${LDFLAGS} ./svd/libsvd.a ./ldac/libldac.a ./segmenter/libsegmenter.a ./model/libmodel.a ./utils/libutils.a ../lib/libsvdlib.a ../lib/libredsvd.a -lmlpack -larmadillo -L/opt/local/lib/octave/3.8.2 -loctave -lboost_filesystem-mt -lboost_system-mt -lstdc++

%.cc.o: %.cc
	${CC} ${FINAL_CXXFLAGS} -o $*.cc.o -c  $*.cc

%.c.o: %.c
	${CC} ${FINAL_CFLAGS} -o $*.c.o -c  $*.c 

.PHONY: utils model segmenter ldac svd
all: utils model segmenter ldac svd ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${FINAL_LDFLAGS}

utils:
	${MAKE} -C utils

model:
	${MAKE} -C model

segmenter:
	${MAKE} -C segmenter

ldac:
	${MAKE} -C ldac

svd:
	${MAKE} -C svd

clean:
	${MAKE} -C utils clean
	${MAKE} -C model clean
	${MAKE} -C segmenter clean
	${MAKE} -C ldac clean
	${MAKE} -C svd clean
	rm -f ${TARGET} ${OBJS}

