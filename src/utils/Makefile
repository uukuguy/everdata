TARGET = libutils.a

OBJS = logger.c.o \
	   filesystem.c.o \
	   dmat.cc.o \
	   smat.cc.o \
	   kvdb.cc.o \
	   dlist.c.o \
	   binary_tree.c.o 

include ../Makefile.common

all: ${TARGET}

${TARGET}: ${OBJS}
	${AR} -cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

