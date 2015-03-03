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
	   farmhash.cc.o \
	   zmalloc.c.o \
	   message.c.o \
	   zpipe.c.o \
	   cboost.cc.o

include ../Makefile.common

# first try -mavx -maes, if fails, retry with -mavx and/or -maes removed, 
# or with -mavx replaced by -msse4.1 or -msse4.2.
CXXFLAGS += -mavx -maes

all: ${TARGET}

${TARGET}: ${OBJS}
	${AR} -cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

