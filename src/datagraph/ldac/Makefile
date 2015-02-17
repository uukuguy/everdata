TARGET = libldac.a

OBJS = ldac.c.o \
	   ldac_blei.c.o \
	   ldac_gibbs.cc.o 

include ../../Makefile.common

FINAL_CFLAGS += -I../model
FINAL_CXXFLAGS += -I../model

all: ${TARGET}

${TARGET}: ${OBJS}
	${AR} cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

