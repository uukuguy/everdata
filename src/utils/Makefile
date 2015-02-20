TARGET = libutils.a

OBJS = logger.c.o \
	   filesystem.c.o \
	   dmat.cc.o \
	   smat.cc.o \
	   dlist.c.o \
	   binary_tree.c.o \
	   daemon.c.o \
	   sysinfo.c.o \
	   md5.c.o \
	   crc32.c.o \
	   zmalloc.c.o \
	   cboost.cc.o

include ../Makefile.common

all: ${TARGET}

${TARGET}: ${OBJS}
	${AR} -cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

