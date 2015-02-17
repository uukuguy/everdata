TARGET = ../../bin/datagraph

OBJS = main.cc.o

.PHONY: model segmenter ldac svd
all: model segmenter ldac svd ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o ${TARGET} ${OBJS} ${FINAL_LDFLAGS}

include ../Makefile.common

LIBMMSEG_CXXFLAGS=-I./segmenter/libmmseg -I./segmenter/libmmseg/css -I./segmenter/libmmseg/utils
LIBMMSEG_LDFLAGS=-lmmseg

FINAL_CXXFLAGS += -I./ldac -I./model -I./segmenter ${LIBMMSEG_CXXFLAGS}
FINAL_LDFLAGS += ${LIBMMSEG_LDFLAGS}
FINAL_LDFLAGS += ./svd/libsvd.a ./ldac/libldac.a ./segmenter/libsegmenter.a ./model/libmodel.a 
FINAL_LDFLAGS += ../utils/libutils.a 
FINAL_LDFLAGS += ./svd/svdlibc/libsvdlib.a ./svd/redsvd/libredsvd.a 
FINAL_LDFLAGS += -lmlpack -larmadillo 
FINAL_LDFLAGS += -L/opt/local/lib/octave/3.8.2 -loctave 
FINAL_LDFLAGS += -lboost_filesystem-mt -lboost_system-mt -lstdc++

model:
	${MAKE} -C model

segmenter:
	${MAKE} -C segmenter

ldac:
	${MAKE} -C ldac

svd:
	${MAKE} -C svd

clean:
	${MAKE} -C model clean
	${MAKE} -C segmenter clean
	${MAKE} -C ldac clean
	${MAKE} -C svd clean
	rm -f ${TARGET} ${OBJS}

