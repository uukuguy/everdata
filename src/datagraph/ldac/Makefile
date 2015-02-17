TARGET = libldac.a

OBJS = ldac.c.o \
	   ldac_blei.c.o \
	   ldac_gibbs.cc.o 

COMMON_CFLAGS= -g -fPIC -Wall -Wno-deprecated -Wno-unused-result 
FINAL_CFLAGS = ${COMMON_CFLAGS} -I../model -I../utils

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

FINAL_LDFLAGS = 

all: ${TARGET}

%.cc.o: %.cc
	${CC} -std=gnu++11 ${FINAL_CXXFLAGS} -o $*.cc.o -c  $*.cc

%.c.o: %.c
	${CC} -std=gnu11 ${FINAL_CFLAGS} -o $*.c.o -c  $*.c 

${TARGET}: ${OBJS}
	${AR} cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

