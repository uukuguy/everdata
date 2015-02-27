
EDBROKER = ../../bin/edbroker
EDBROKER_OBJS = edbroker_main.c.o edbroker.c.o

EDWORKER = ../../bin/edworker
EDWORKER_OBJS = edworker_main.c.o edworker.c.o datanode.c.o bucket.c.o channel.c.o object.c.o bucketdb.c.o
#container.c.o 
EDCLIENT = ../../bin/edclient
EDCLIENT_OBJS = edclient_main.c.o edclient.c.o

EDFS = ../../bin/edfs
EDFS_OBJS = edfs.c.o

LIBUTILS = ../utils/libutils.a
LIBKVDB = ../kvdb/libkvdb.a
LIBCRUSH = ../crush/libcrush.a
CFLAGS_LIBCRUSH = -I../crush

all: ${EDBROKER} ${EDWORKER} ${EDCLIENT} ${EDFS}

include ../Makefile.common

FINAL_CFLAGS += ${CFLAGS_LIBCRUSH}
FINAL_CXXFLAGS += ${CFLAGS_LIBCRUSH}

ifeq (${UNAME}, Darwin)
	FINAL_CFLAGS += -I/opt/local/include/osxfuse
endif


FINAL_LDFLAGS += ${LIBCRUSH} ${LIBKVDB} ${LIBUTILS}
FINAL_LDFLAGS += -lczmq -lzmq -ljemalloc
FINAL_LDFLAGS += -llmdb -lleveldb
FINAL_LDFLAGS += -lmsgpack -lbz2

ifeq (${UNAME}, Darwin)
	FINAL_LDFLAGS += -losxfuse -ldl
endif
ifeq (${UNAME}, Linux)
	FINAL_LDFLAGS += -lfuse -ldl
endif

FINAL_LDFLAGS += -lpthread -lssl -lcrypto
FINAL_LDFLAGS += -lstdc++ -lm -lz


${EDBROKER}: ${LIBBASE} ${EDBROKER_OBJS} 
	${CC} -o ${EDBROKER} ${EDBROKER_OBJS} ${FINAL_LDFLAGS}

${EDWORKER}: ${LIBBASE} ${LIBSERVER_OBJS} ${EDWORKER_OBJS} 
	${CC} -o ${EDWORKER} ${EDWORKER_OBJS} ${LIBSERVER_OBJS} ${FINAL_LDFLAGS}

${EDCLIENT}: ${LIBBASE} ${EDCLIENT_OBJS} 
	${CC} -o ${EDCLIENT} ${EDCLIENT_OBJS} ${FINAL_LDFLAGS}

${EDFS}: edclient.c.o ${LIBBASE} ${EDFS_OBJS} 
	${CC} -o ${EDFS} ${EDFS_OBJS} edclient.c.o ${FINAL_LDFLAGS}

clean:
	rm -f ${EDBROKER} ${EDBROKER_OBJS} ${EDWORKER} ${EDWORKER_OBJS} ${EDCLIENT} ${EDCLIENT_OBJS}

