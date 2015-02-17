TARGET = libmodel.a

OBJS = corpus.cc.o \
	   docset.cc.o \
	   document.cc.o \
	   term.cc.o \
	   lexicon.cc.o 

include ../../Makefile.common

FINAL_CXXFLAGS += -I../segmenter

all: ${TARGET}

${TARGET}: ${OBJS}
	${AR} cruv ${TARGET} ${OBJS}

clean:
	rm -f ${TARGET} ${OBJS}

