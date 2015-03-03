EVERDATA_OBJS = edcluster.cc.o

EDBROKER = ../../bin/edbroker
EDBROKER_OBJS = edbroker_main.cc.o edbroker.cc.o

EDWORKER = ../../bin/edworker
EDWORKER_OBJS = edworker_main.cc.o edworker.cc.o datanode.cc.o bucket.cc.o channel.cc.o object.cc.o bucketdb.cc.o
EDCLIENT = ../../bin/edclient
EDCLIENT_OBJS = edclient_main.cc.o edclient.cc.o

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
FINAL_LDFLAGS += -lstdc++

ifeq (${UNAME}, Darwin)
	FINAL_LDFLAGS += -losxfuse -ldl
endif
ifeq (${UNAME}, Linux)
	FINAL_LDFLAGS += -lfuse -ldl
endif

FINAL_LDFLAGS += -lpthread -lssl -lcrypto
FINAL_LDFLAGS += -lstdc++ -lm -lz


${EDBROKER}: ${LIBBASE} ${EVERDATA_OBJS} ${EDBROKER_OBJS} 
	${CC} -o ${EDBROKER} ${EDBROKER_OBJS} ${EVERDATA_OBJS} ${FINAL_LDFLAGS}

${EDWORKER}: ${LIBBASE} ${EVERDATA_OBJS} ${EDWORKER_OBJS} 
	${CC} -o ${EDWORKER} ${EDWORKER_OBJS} ${EVERDATA_OBJS} ${FINAL_LDFLAGS}

${EDCLIENT}: ${LIBBASE} ${EVERDATA_OBJS} ${EDCLIENT_OBJS} 
	${CC} -o ${EDCLIENT} ${EDCLIENT_OBJS} ${EVERDATA_OBJS} ${FINAL_LDFLAGS}

${EDFS}: edclient.cc.o ${LIBBASE} ${EDFS_OBJS} 
	${CC} -o ${EDFS} ${EDFS_OBJS} edclient.cc.o ${FINAL_LDFLAGS}

clean:
	rm -f ${EDBROKER} ${EDBROKER_OBJS} ${EDWORKER} ${EDWORKER_OBJS} ${EDCLIENT} ${EDCLIENT_OBJS}

