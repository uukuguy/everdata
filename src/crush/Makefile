APP = ../../bin/crush_test
APP_OBJS = crush_test.cc.o

TARGET = libcrush.a
OBJS = builder.c.o \
	   mapper.c.o \
	   crush.c.o \
	   hash.c.o \
	   crush.cc.o

	   #CrushWrapper.cc.o \
	   #CrushCompiler.cc.o \
	   #CurshTester.cc.o 

FINAL_CXXFLAGS += -g
FINAL_CFLAGS += -g

all: ${TARGET} ${APP}

${TARGET}: ${OBJS}
	${AR} curv ${TARGET} ${OBJS}

${APP}: ${APP_OBJS} ${TARGET}
	${CC} -o ${APP} ${APP_OBJS} ${TARGET} -lstdc++

clean:
	rm -f ${TARGET} ${OBJS} ${APP_OBJS} ${APP}

%.cc.o: %.cc
	${CC} ${FINAL_CXXFLAGS} -o $*.cc.o -c  $*.cc

%.c.o: %.c
	${CC} ${FINAL_CFLAGS} -o $*.c.o -c  $*.c 



