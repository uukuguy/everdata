TARGET = libkvdb.a

KVDB_OBJS = kvdb.c.o 

KVDB_CFLAGS += -DHAS_LMDB 
LMDB_OBJS = kvdb_lmdb.c.o 
KVDB_OBJS += ${LMDB_OBJS}

KVDB_CFLAGS += -DHAS_LEVELDB
LEVELDB_OBJS = kvdb_leveldb.c.o 
KVDB_OBJS += ${LEVELDB_OBJS}

#KVDB_CFLAGS += -DHAS_ROCKSDB
#ROCKSDB_OBJS = kvdb_rocksdb.c.o 
#KVDB_OBJS += ${ROCKSDB_OBJS}

#KVDB_CFLAGS=-DHAS_LSM -I./deps/lsm
#LSM_OBJS = kvdb_lsm.c.o
#KVDB_OBJS += ${LSM_OBJS}

#KVDB_CFLAGS += -DHAS_EBLOB
#EBLOB_OBJS = kvdb_eblob.c.o
#KVDB_OBJS += ${EBLOB_OBJS}

all: ${TARGET}

include ../Makefile.common

FINAL_CFLAGS += ${KVDB_CFLAGS}

${TARGET}: ${KVDB_OBJS}
	ar -curv ${TARGET} ${KVDB_OBJS} 

clean:
	rm -f ${TARGET} ${KVDB_OBJS}


